/*
 * zxtools - small toolbox menu and dispatcher.
 */
#include "zxtools.h"
#include "zxcommon.h"
#include "zxcompat.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void zxtools_usage(FILE *fp)
{
    fprintf(fp,
            "zxtools %s - optical modem toolbox\n"
            "\n"
            "Usage:\n"
            "  zxtools                         interactive menu\n"
            "  zxtools router-check [options]  system / memory / storage / "
            "network report\n"
            "  zxtools fileinfo PATH...        file type, size, mode, owner, "
            "mtime\n"
            "  zxtools netmon [options]        realtime RX/TX monitor "
            "(alias: zxnetmon)\n"
            "  zxtools help                    this help\n"
            "\n"
            "Examples:\n"
            "  zxtools\n"
            "  zxtools router-check\n"
            "  zxtools router-check --save\n"
            "  zxtools fileinfo /usr/data/hello\n"
            "  zxtools netmon -i br0 -n 1\n"
            "\n"
            "The interactive menu needs a terminal. Over non-PTY SSH use a "
            "subcommand:\n"
            "  ssh -T root@192.168.1.1 'zxtools router-check'\n"
            "For a full terminal use Telnet.\n",
            ZX_VERSION);
}

static void find_self_dir(const char *argv0, char *dir, size_t dirlen)
{
    char exe[PATH_MAX];
    ssize_t got;
    char *slash;

    got = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (got > 0) {
        exe[got] = '\0';
        slash = strrchr(exe, '/');
        if (slash != NULL) {
            if (slash == exe)
                slash[1] = '\0';
            else
                *slash = '\0';
            zx_strlcpy(dir, exe, dirlen);
            return;
        }
    }

    zx_strlcpy(dir, argv0 != NULL ? argv0 : ".", dirlen);
    slash = strrchr(dir, '/');
    if (slash != NULL) {
        if (slash == dir)
            slash[1] = '\0';
        else
            *slash = '\0';
    } else {
        zx_strlcpy(dir, ".", dirlen);
    }
}

int main(int argc, char **argv)
{
    char self_dir[PATH_MAX];
    const char *command;

    find_self_dir(argc > 0 ? argv[0] : NULL, self_dir, sizeof(self_dir));

    if (argc < 2) {
        if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
            zxtools_menu(self_dir);
        else
            zxtools_usage(stdout);
        return 0;
    }

    command = argv[1];
    if (strcmp(command, "help") == 0 || strcmp(command, "-h") == 0 ||
        strcmp(command, "--help") == 0) {
        zxtools_usage(stdout);
        return 0;
    }
    if (strcmp(command, "-V") == 0 || strcmp(command, "--version") == 0) {
        printf("zxtools %s\n", ZX_VERSION);
        return 0;
    }
    if (strcmp(command, "router-check") == 0 ||
        strcmp(command, "check") == 0) {
        return zxtools_run(self_dir, "router-check", argc - 2, argv + 2);
    }
    if (strcmp(command, "fileinfo") == 0 ||
        strcmp(command, "info") == 0) {
        return zxtools_run(self_dir, "fileinfo", argc - 2, argv + 2);
    }
    if (strcmp(command, "netmon") == 0 ||
        strcmp(command, "zxnetmon") == 0 ||
        strcmp(command, "monitor") == 0) {
        return zxtools_run(self_dir, "zxnetmon", argc - 2, argv + 2);
    }

    fprintf(stderr, "zxtools: unknown command: %s\n\n", command);
    zxtools_usage(stderr);
    return 2;
}
