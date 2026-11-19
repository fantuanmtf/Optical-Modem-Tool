/*
 * menu.c - interactive TTY menu for zxtools.
 */
#include "zxtools.h"
#include "zxcommon.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void clear_screen(void)
{
    fputs("\033[H\033[2J", stdout);
    fflush(stdout);
}

static int read_line(char *buf, size_t buflen)
{
    if (fgets(buf, (int)buflen, stdin) == NULL)
        return -1;
    buf[strcspn(buf, "\r\n")] = '\0';
    return 0;
}

static int pause_menu(void)
{
    char line[32];

    fputs("\nPress Enter to return...", stdout);
    fflush(stdout);
    return read_line(line, sizeof(line));
}

void zxtools_menu(const char *self_dir)
{
    int quit = 0;

    while (!quit) {
        char choice[32];

        clear_screen();
        printf("========================================\n");
        printf("              ZXTOOLS %s\n", ZX_VERSION);
        printf("========================================\n\n");
        printf("  1) router-check    system / memory / disk / network\n");
        printf("  2) fileinfo        file details\n");
        printf("  3) zxnetmon        realtime traffic\n");
        printf("  4) router-check    save a report\n\n");
        printf("  q) quit\n\n");
        printf("========================================\n");
        printf("Select [1-4, q]: ");
        fflush(stdout);

        if (read_line(choice, sizeof(choice)) != 0)
            break;

        switch (choice[0]) {
        case '1':
            zxtools_run(self_dir, "router-check", 0, NULL);
            break;
        case '2': {
            char path[1024];
            char *args[1];

            printf("Path: ");
            fflush(stdout);
            if (read_line(path, sizeof(path)) != 0) {
                quit = 1;
                break;
            }
            if (path[0] == '\0') {
                printf("No path entered.\n");
            } else {
                args[0] = path;
                zxtools_run(self_dir, "fileinfo", 1, args);
            }
            break;
        }
        case '3':
            zxtools_run(self_dir, "zxnetmon", 0, NULL);
            break;
        case '4': {
            char *args[1] = { "--save" };
            zxtools_run(self_dir, "router-check", 1, args);
            break;
        }
        case 'q':
        case 'Q':
            quit = 1;
            break;
        default:
            printf("Unknown choice: %s\n", choice);
            break;
        }

        if (!quit && pause_menu() != 0)
            quit = 1;
    }
    clear_screen();
}
