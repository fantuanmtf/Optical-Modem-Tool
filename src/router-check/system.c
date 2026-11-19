/*
 * system.c - system, CPU and process sections.
 */
#include "router-check.h"
#include "zxcompat.h"

#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static int cpu_key_wanted(const char *key)
{
    static const char *const keys[] = {
        "processor", "model name", "Processor", "Hardware",
        "Revision", "BogoMIPS"
    };
    size_t i;

    for (i = 0; i < ZX_ARRAY_LEN(keys); i++) {
        if (strcmp(key, keys[i]) == 0)
            return 1;
    }
    return 0;
}

void rc_system(rc_report *rep)
{
    struct utsname uts;
    char host[256] = "unknown";
    char when[64];
    char buf[512];
    time_t now = time(NULL);

    if (gethostname(host, sizeof(host) - 1) != 0)
        zx_strlcpy(host, "unknown", sizeof(host));
    host[sizeof(host) - 1] = '\0';
    zx_format_time(now, when, sizeof(when));

    report_line(rep, "Optical-Modem-Tool / router-check v%s", ZX_VERSION);
    report_line(rep, "Generated: %s", when);
    report_line(rep, "Host: %s", host);

    report_section(rep, "System");
    if (uname(&uts) == 0) {
        report_line(rep, "%s %s %s %s %s", uts.sysname, uts.nodename,
                    uts.release, uts.version, uts.machine);
    }

    if (zx_read_file("/proc/uptime", buf, sizeof(buf)) >= 0) {
        double seconds = 0.0;

        if (sscanf(buf, "%lf", &seconds) == 1) {
            unsigned long total = (unsigned long)seconds;
            unsigned long days = total / 86400;
            unsigned long hours = (total % 86400) / 3600;
            unsigned long mins = (total % 3600) / 60;
            unsigned long secs = total % 60;

            if (days == 1) {
                report_line(rep, "Uptime: 1 day, %02lu:%02lu:%02lu",
                            hours, mins, secs);
            } else if (days > 1) {
                report_line(rep, "Uptime: %lu days, %02lu:%02lu:%02lu",
                            days, hours, mins, secs);
            } else {
                report_line(rep, "Uptime: %02lu:%02lu:%02lu",
                            hours, mins, secs);
            }
        }
    }

    if (zx_read_file("/proc/loadavg", buf, sizeof(buf)) >= 0)
        report_line(rep, "Load: %s", zx_trim(buf));

    if (access("/etc/openwrt_release", R_OK) == 0) {
        report_line(rep, "");
        report_file(rep, "/etc/openwrt_release");
    }
}

void rc_cpu(rc_report *rep)
{
    FILE *fp;
    char line[1024];
    int printed = 0;

    report_section(rep, "CPU");
    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        report_line(rep, "no /proc/cpuinfo");
        return;
    }

    while (printed < 20 && fgets(line, sizeof(line), fp) != NULL) {
        char *colon = strchr(line, ':');
        char *key;
        char *value;

        if (colon == NULL)
            continue;
        *colon = '\0';
        key = zx_trim(line);
        value = zx_trim(colon + 1);
        if (!cpu_key_wanted(key))
            continue;
        report_line(rep, "%s: %s", key, value);
        printed++;
    }
    fclose(fp);

    if (printed == 0)
        report_line(rep, "no CPU details found");
}

struct proc_info {
    long pid;
    unsigned long uid;
    char name[64];
    char command[256];
};

static int read_proc_status(long pid, struct proc_info *info)
{
    char path[64];
    char line[512];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%ld/status", pid);
    fp = fopen(path, "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "Name:", 5) == 0) {
            zx_strlcpy(info->name, zx_trim(line + 5), sizeof(info->name));
        } else if (strncmp(line, "Uid:", 4) == 0) {
            unsigned long uid = 0;
            if (sscanf(line + 4, "%lu", &uid) == 1)
                info->uid = uid;
        }
    }
    fclose(fp);
    return 0;
}

static void read_proc_command(long pid, struct proc_info *info)
{
    char path[64];
    char buf[256];
    size_t got;
    size_t i;
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
    fp = fopen(path, "rb");
    if (fp == NULL) {
        snprintf(info->command, sizeof(info->command), "[%s]", info->name);
        return;
    }
    got = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[got] = '\0';
    for (i = 0; i < got; i++) {
        if (buf[i] == '\0')
            buf[i] = ' ';
    }
    zx_strlcpy(info->command, zx_trim(buf), sizeof(info->command));
    if (info->command[0] == '\0')
        snprintf(info->command, sizeof(info->command), "[%s]", info->name);
}

static int proc_cmp(const void *a, const void *b)
{
    const struct proc_info *pa = a;
    const struct proc_info *pb = b;

    if (pa->pid < pb->pid)
        return -1;
    if (pa->pid > pb->pid)
        return 1;
    return 0;
}

void rc_processes(rc_report *rep)
{
    DIR *dir;
    struct dirent *entry;
    struct proc_info *list = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t i;

    report_section(rep, "Processes");
    dir = opendir("/proc");
    if (dir == NULL) {
        report_line(rep, "cannot read /proc");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char *end = NULL;
        long pid = strtol(entry->d_name, &end, 10);

        if (end == entry->d_name || *end != '\0' || pid <= 0)
            continue;

        if (count == capacity) {
            size_t next = capacity == 0 ? 64 : capacity * 2;
            struct proc_info *grown = realloc(list, next * sizeof(*list));

            if (grown == NULL)
                break;
            list = grown;
            capacity = next;
        }

        memset(&list[count], 0, sizeof(list[count]));
        list[count].pid = pid;
        if (read_proc_status(pid, &list[count]) != 0)
            continue;
        read_proc_command(pid, &list[count]);
        count++;
    }
    closedir(dir);

    if (count == 0) {
        report_line(rep, "no processes visible");
        free(list);
        return;
    }

    qsort(list, count, sizeof(*list), proc_cmp);
    report_line(rep, "  PID USER      COMMAND");
    for (i = 0; i < count; i++) {
        struct passwd *pw = getpwuid((uid_t)list[i].uid);
        char user[64];

        if (pw != NULL)
            zx_strlcpy(user, pw->pw_name, sizeof(user));
        else
            snprintf(user, sizeof(user), "%lu", list[i].uid);
        report_line(rep, "%5ld %-9s %s", list[i].pid, user,
                    list[i].command);
    }
    free(list);
}
