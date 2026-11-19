/*
 * netdev.c - read interface counters from /proc/net/dev.
 */
#include "zxnetmon.h"
#include "zxcommon.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
