/*
 * netdev.c - read interface counters from /proc/net/dev.
 */
#include "zxnetmon.h"
#include "zxcommon.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Parse the interface name before ':' into name. */
static int parse_name(const char *line, char *name, size_t namelen)
{
    const char *start = line;
    const char *colon;
    size_t len;

    while (*start == ' ' || *start == '\t')
        start++;
    colon = strchr(start, ':');
    if (colon == NULL || colon == start)
        return -1;

    len = (size_t)(colon - start);
    while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t'))
        len--;
    if (len == 0 || len >= namelen)
        return -1;
    memcpy(name, start, len);
    name[len] = '\0';
    return 0;
}

/* Read the 16 numeric counters after the colon into values[]. */
static int parse_counters(const char *rest, unsigned long long *values,
                          size_t count)
{
    const char *p = rest;
    size_t i;

    for (i = 0; i < count; i++) {
        char *end = NULL;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == '\n')
            break;
        errno = 0;
        values[i] = strtoull(p, &end, 10);
        if (end == p || errno != 0)
            break;
        p = end;
    }
    return i == count ? 0 : -1;
}

int netdev_read(const char *iface, unsigned long long *rx,
                unsigned long long *tx)
{
    FILE *fp;
    char line[512];
    int found = 0;

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char name[64];
        unsigned long long values[16];

        if (parse_name(line, name, sizeof(name)) != 0)
            continue;
        if (strcmp(name, iface) != 0)
            continue;
        if (parse_counters(strchr(line, ':') + 1, values,
                           ZX_ARRAY_LEN(values)) != 0)
            break;
        *rx = values[0];
        *tx = values[8];
        found = 1;
        break;
    }
    fclose(fp);

    if (!found)
        errno = ENOENT;
    return found ? 0 : -1;
}

void netdev_list(FILE *fp)
{
    FILE *in;
    char line[512];

    in = fopen("/proc/net/dev", "r");
    if (in == NULL)
        return;
    while (fgets(line, sizeof(line), in) != NULL) {
        char name[64];

        if (parse_name(line, name, sizeof(name)) == 0)
            fprintf(fp, "  %s\n", name);
    }
    fclose(in);
}

void netdev_iflist_free(zxmon_iflist *list)
{
    size_t i;

    if (list == NULL)
        return;
    for (i = 0; i < list->count; i++)
        free(list->names[i]);
    free(list->names);
    list->names = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int iflist_add(zxmon_iflist *list, const char *name)
{
    char *copy;

    if (list->count == list->capacity) {
        size_t next = list->capacity == 0 ? 8 : list->capacity * 2;
        char **grown = realloc(list->names, next * sizeof(*grown));

        if (grown == NULL)
            return -1;
        list->names = grown;
        list->capacity = next;
    }

    copy = strdup(name);
    if (copy == NULL)
        return -1;
    list->names[list->count++] = copy;
    return 0;
}

static int enumerate_interfaces(zxmon_iflist *list)
{
    FILE *fp;
    char line[512];

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char name[64];

        if (parse_name(line, name, sizeof(name)) != 0)
            continue;
        if (iflist_add(list, name) != 0) {
            fclose(fp);
            netdev_iflist_free(list);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

static int name_has_virtual_prefix(const char *name)
{
    static const char *const prefixes[] = {
        "br", "vlan", "ppp", "wlan", "tun", "tap",
        "bond", "docker", "virbr", "vmnet"
    };
    size_t i;

    for (i = 0; i < ZX_ARRAY_LEN(prefixes); i++) {
        size_t len = strlen(prefixes[i]);

        if (strncmp(name, prefixes[i], len) == 0)
            return 1;
    }
    return 0;
}

/* A missing /sys/class/net/<if>/device usually means a virtual device. */
static int sysfs_device_absent(const char *name)
{
    char dir[64];
    char path[256];
    struct stat st;

    if (stat("/sys/class/net", &st) != 0 || !S_ISDIR(st.st_mode))
        return 0;
    snprintf(dir, sizeof(dir), "/sys/class/net/%s", name);
    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode))
        return 0;
    snprintf(path, sizeof(path), "%s/device", dir);
    if (stat(path, &st) == 0)
        return 0;
    return errno == ENOENT || errno == ENOTDIR;
}

static int is_virtual_interface(const char *name)
{
    return name_has_virtual_prefix(name) || sysfs_device_absent(name);
}

static int copy_nonloopback(zxmon_iflist *dst, const zxmon_iflist *src,
                           int virtual_only)
{
    size_t i;

    for (i = 0; i < src->count; i++) {
        const char *name = src->names[i];

        if (strcmp(name, "lo") == 0)
            continue;
        if (virtual_only && !is_virtual_interface(name))
            continue;
        if (iflist_add(dst, name) != 0)
            return -1;
    }
    return 0;
}

int netdev_detect_virtual(zxmon_iflist *list)
{
    zxmon_iflist all = { NULL, 0, 0 };

    if (enumerate_interfaces(&all) != 0)
        return -1;
    if (copy_nonloopback(list, &all, 1) != 0) {
        netdev_iflist_free(&all);
        return -1;
    }
    if (list->count == 0 && copy_nonloopback(list, &all, 0) != 0) {
        netdev_iflist_free(&all);
        return -1;
    }
    netdev_iflist_free(&all);
    return 0;
}

int netdev_all_nonloopback(zxmon_iflist *list)
{
    zxmon_iflist all = { NULL, 0, 0 };

    if (enumerate_interfaces(&all) != 0)
        return -1;
    if (copy_nonloopback(list, &all, 0) != 0) {
        netdev_iflist_free(&all);
        return -1;
    }
    netdev_iflist_free(&all);
    return 0;
}
