/*
 * router-check - read-only health report for BusyBox optical modems.
 */
#include "router-check.h"
#include "zxcompat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: router-check [-o FILE | --save]\n"
            "\n"
            "Read-only system report: kernel, CPU, memory, storage, "
            "interfaces,\n"
            "routes, listening sockets, ARP table and processes.\n"
            "\n"
            "Options:\n"
            "  -o, --output FILE   Write the report to FILE as well as stdout.\n"
            "      --save          Save under /usr/data/zxtools/reports/.\n"
            "  -h, --help          Show this help.\n");
}

static void make_parent(const char *path)
{
    char *copy = strdup(path);
    char *slash;

    if (copy == NULL)
        return;
    slash = strrchr(copy, '/');
    if (slash != NULL) {
        if (slash == copy)
            slash[1] = '\0';
        else
            *slash = '\0';
        zx_mkdir_p(copy, 0755);
    }
    free(copy);
}

int main(int argc, char **argv)
{
    const char *output = NULL;
    char save_path[1024];
    int save = 0;
    int i;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(stdout);
            return 0;
        }
        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "router-check: missing argument for %s\n",
                        arg);
                return 2;
            }
            output = argv[i];
            continue;
        }
        if (strncmp(arg, "--output=", 9) == 0) {
            output = arg + 9;
            continue;
        }
        if (strcmp(arg, "--save") == 0) {
            save = 1;
            continue;
        }
        fprintf(stderr, "router-check: unknown option: %s\n", arg);
        usage(stderr);
        return 2;
    }

    if (save && output == NULL) {
        const char *home = getenv("ZXTOOLS_HOME");
        char stamp[32];
        time_t now = time(NULL);
        struct tm tm;

        if (home == NULL || *home == '\0')
            home = "/usr/data/zxtools";
        if (localtime_r(&now, &tm) == NULL ||
            strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &tm) == 0)
            zx_strlcpy(stamp, "report", sizeof(stamp));
        snprintf(save_path, sizeof(save_path),
                 "%s/reports/router-check-%s.txt", home, stamp);
        output = save_path;
    }

    if (output != NULL)
        make_parent(output);

    {
        rc_report rep;

        if (report_open(&rep, output) != 0) {
            fprintf(stderr, "router-check: cannot write %s: %s\n", output,
                    strerror(errno));
            return 1;
        }

        rc_system(&rep);
        rc_cpu(&rep);
        rc_memory(&rep);
        rc_storage(&rep);
        rc_network(&rep);
        rc_routes(&rep);
        rc_listening(&rep);
        rc_bridge(&rep);
        rc_arp(&rep);
        rc_processes(&rep);
        rc_wireless(&rep);

        report_line(&rep, "");
        report_line(&rep, "Report complete.");
        if (output != NULL)
            report_line(&rep, "Saved: %s", output);
        report_close(&rep);
    }

    return 0;
}
