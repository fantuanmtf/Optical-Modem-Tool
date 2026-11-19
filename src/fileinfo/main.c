/*
 * fileinfo - show type, permissions, size, owner and mtime for files.
 */
#include "fileinfo.h"
#include "zxcommon.h"

#include <stdio.h>
#include <string.h>

void fileinfo_usage(FILE *fp)
{
    fprintf(fp,
            "fileinfo - show file metadata\n"
            "\n"
            "Usage:\n"
            "  fileinfo FILE [FILE...]\n"
            "\n"
            "Examples:\n"
            "  fileinfo /usr/data/hello\n"
            "  fileinfo /etc/passwd /tmp\n");
}

int main(int argc, char **argv)
{
    int i = 1;
    int failures = 0;
    int options = 1;
    int paths = 0;

    if (argc < 2) {
        fileinfo_usage(stderr);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {
        fileinfo_usage(stdout);
        return 0;
    }
    if (strcmp(argv[1], "-V") == 0 ||
        strcmp(argv[1], "--version") == 0) {
        printf("fileinfo %s\n", ZX_VERSION);
        return 0;
    }

    for (; i < argc; i++) {
        if (options && strcmp(argv[i], "--") == 0) {
            options = 0;
            continue;
        }
        if (options && argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "fileinfo: unknown option: %s\n", argv[i]);
            fileinfo_usage(stderr);
            return 2;
        }
        if (fileinfo_inspect(argv[i]) != 0)
            failures++;
        paths++;
    }

    if (paths == 0) {
        fileinfo_usage(stderr);
        return 1;
    }
    return failures == 0 ? 0 : 1;
}
