/*
 * zxnetmon - realtime RX/TX monitor for /proc/net/dev.
 */
#include "zxnetmon.h"
#include "zxcommon.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *fp)
{
    fprintf(fp,
            "zxnetmon %s - realtime network traffic monitor\n"
            "\n"
            "Usage:\n"
            "  zxnetmon [-n SECONDS]\n"
            "  zxnetmon -i IFACE [-n SECONDS]\n"
            "\n"
            "Options:\n"
            "  -i, --interface IFACE  monitor one interface; omit to "
            "auto-detect\n"
            "                         virtual interfaces (use \"all\" for "
            "all non-loopback)\n"
            "  -n, --interval SECONDS sample interval (default: 1)\n"
            "  -h, --help             show this help\n"
            "\n"
            "Without -i, interfaces matching br*, vlan*, ppp*, wlan*, tun*,\n"
            "tap*, bond*, docker*, virbr*, vmnet*, or without a sysfs "
            "device\n"
            "entry are shown.  If none match, all non-loopback interfaces "
            "are used.\n"
            "\n"
            "Reads counters from /proc/net/dev. Press Ctrl+C to exit.\n"
            "On this modem's SSH server, use Telnet for a full interactive "
            "terminal.\n",
            ZX_VERSION);
}

int main(int argc, char **argv)
{
    const char *iface = NULL;
    unsigned interval = 1;
    unsigned long value;
    unsigned long long rx;
    unsigned long long tx;
    int i;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(stdout);
            return 0;
        }
        if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            printf("zxnetmon %s\n", ZX_VERSION);
            return 0;
        }
        if (strcmp(arg, "-i") == 0 || strcmp(arg, "--interface") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "zxnetmon: %s needs an interface\n", arg);
                return 2;
            }
            iface = argv[i];
            continue;
        }
        if (strncmp(arg, "--interface=", 12) == 0) {
            iface = arg + 12;
            continue;
        }
        if (strcmp(arg, "-n") == 0 || strcmp(arg, "--interval") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "zxnetmon: %s needs a number\n", arg);
                return 2;
            }
            arg = argv[i];
        } else if (strncmp(arg, "--interval=", 11) == 0) {
            arg += 11;
        } else {
            fprintf(stderr, "zxnetmon: unknown option: %s\n", arg);
            usage(stderr);
            return 2;
        }

        if (zx_parse_uint(arg, &value) != 0 || value == 0 ||
            value > 86400) {
            fprintf(stderr,
                    "zxnetmon: interval must be an integer between 1 and "
                    "86400\n");
            return 2;
        }
        interval = (unsigned)value;
    }

    if (iface != NULL && strcmp(iface, "all") != 0) {
        errno = 0;
        if (netdev_read(iface, &rx, &tx) != 0) {
            if (errno == ENOENT) {
                fprintf(stderr, "zxnetmon: interface '%s' not found.\n",
                        iface);
                fprintf(stderr, "Available interfaces:\n");
                netdev_list(stderr);
            } else {
                fprintf(stderr, "zxnetmon: cannot read /proc/net/dev\n");
            }
            return 1;
        }
        return monitor_run(iface, interval);
    }

    {
        zxmon_iflist list = { NULL, 0, 0 };
        const char *mode;
        int all = iface != NULL;
        int rc;

        rc = all ? netdev_all_nonloopback(&list) :
                   netdev_detect_virtual(&list);
        if (rc != 0) {
            fprintf(stderr, "zxnetmon: cannot enumerate /proc/net/dev\n");
            netdev_iflist_free(&list);
            return 1;
        }
        if (list.count == 0) {
            fprintf(stderr, "zxnetmon: no non-loopback interfaces found.\n");
            netdev_iflist_free(&list);
            return 1;
        }

        mode = all ? "all non-loopback" : "auto-detected";
        rc = monitor_run_many(list.names, list.count, interval, mode);
        netdev_iflist_free(&list);
        return rc;
    }
}
