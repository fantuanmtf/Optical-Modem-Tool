/*
 * storage.c - mounted filesystem usage via statvfs(2).
 */
#include "router-check.h"

#include <stdio.h>
#include <string.h>

#if defined(HAVE_STATVFS) && HAVE_STATVFS
#include <sys/statvfs.h>
#endif

#if defined(HAVE_STATVFS) && HAVE_STATVFS
static void unescape_mount(char *text)
{
    char *src = text;
    char *dst = text;

    while (*src != '\0') {
        if (src[0] == '\\' && src[1] >= '0' && src[1] <= '7' &&
            src[2] >= '0' && src[2] <= '7' &&
            src[3] >= '0' && src[3] <= '7') {
            *dst++ = (char)(((src[1] - '0') << 6) |
                            ((src[2] - '0') << 3) |
                            (src[3] - '0'));
            src += 4;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
#endif

static int storage_with_df(rc_report *rep)
{
    size_t bytes = report_popen(rep, "df -h 2>/dev/null");

    if (bytes == 0)
        bytes = report_popen(rep, "df -k 2>/dev/null");
    if (bytes == 0)
        report_line(rep, "df not available");
    return bytes == 0 ? -1 : 0;
}

void rc_storage(rc_report *rep)
{
#if defined(HAVE_STATVFS) && HAVE_STATVFS
    FILE *fp;
    char line[2048];
    int printed = 0;

    report_section(rep, "Storage");
    fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        storage_with_df(rep);
        return;
    }

    report_line(rep, "%-20s %9s %9s %9s %4s %s",
                "Filesystem", "Size", "Used", "Avail", "Use%", "Mounted on");

    while (fgets(line, sizeof(line), fp) != NULL) {
        char device[256];
        char mount[1024];
        char fstype[256];
        struct statvfs vfs;
        unsigned long long block_size;
        unsigned long long size;
        unsigned long long used;
        unsigned long long avail;
        unsigned long long percent = 0;
        char hsize[32];
        char hused[32];
        char havail[32];

        if (sscanf(line, "%255s %1023s %255s", device, mount, fstype) != 3)
            continue;
        unescape_mount(device);
        unescape_mount(mount);

        if (statvfs(mount, &vfs) != 0 || vfs.f_blocks == 0)
            continue;

        block_size = vfs.f_frsize != 0 ? vfs.f_frsize : vfs.f_bsize;
        size = (unsigned long long)vfs.f_blocks * block_size;
        used = (unsigned long long)(vfs.f_blocks - vfs.f_bfree) *
               block_size;
        avail = (unsigned long long)vfs.f_bavail * block_size;
        if (used + avail > 0)
            percent = used * 100 / (used + avail);

        zx_human_bytes(size, hsize, sizeof(hsize));
        zx_human_bytes(used, hused, sizeof(hused));
        zx_human_bytes(avail, havail, sizeof(havail));
        report_line(rep, "%-20s %9s %9s %9s %4llu%% %s",
                    device, hsize, hused, havail, percent, mount);
        printed++;
    }
    fclose(fp);

    if (printed == 0)
        storage_with_df(rep);
#else
    report_section(rep, "Storage");
    storage_with_df(rep);
#endif
}
