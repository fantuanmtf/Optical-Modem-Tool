/*
 * inspect.c - metadata formatting for one path.
 */
#include "fileinfo.h"
#include "zxcommon.h"
#include "zxcompat.h"

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char mode_type_char(mode_t mode)
{
    if (S_ISREG(mode))
        return '-';
    if (S_ISDIR(mode))
        return 'd';
    if (S_ISLNK(mode))
        return 'l';
    if (S_ISCHR(mode))
        return 'c';
    if (S_ISBLK(mode))
        return 'b';
    if (S_ISFIFO(mode))
        return 'p';
    if (S_ISSOCK(mode))
        return 's';
    return '?';
}

static void format_mode(mode_t mode, char out[11])
{
    out[0] = mode_type_char(mode);
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID)
        out[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        out[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        out[9] = (mode & S_IXOTH) ? 't' : 'T';
    out[10] = '\0';
}

static char *read_link_target(const char *path, off_t hint)
{
    size_t cap = hint > 0 ? (size_t)hint + 1 : 256;
    int attempt;

    for (attempt = 0; attempt < 3; attempt++) {
        char *buf = malloc(cap);
        ssize_t got;

        if (buf == NULL)
            return NULL;
        got = readlink(path, buf, cap - 1);
        if (got < 0) {
            free(buf);
            return NULL;
        }
        if ((size_t)got < cap - 1) {
            buf[got] = '\0';
            return buf;
        }
        free(buf);
        cap *= 2;
    }
    return NULL;
}

static void owner_name(uid_t uid, char *buf, size_t buflen)
{
    struct passwd *pw = getpwuid(uid);

    if (pw != NULL)
        zx_strlcpy(buf, pw->pw_name, buflen);
    else
        snprintf(buf, buflen, "%lu", (unsigned long)uid);
}

static void group_name(gid_t gid, char *buf, size_t buflen)
{
    struct group *gr = getgrgid(gid);

    if (gr != NULL)
        zx_strlcpy(buf, gr->gr_name, buflen);
    else
        snprintf(buf, buflen, "%lu", (unsigned long)gid);
}

int fileinfo_inspect(const char *path)
{
    struct stat st;
    char mode[11];
    char owner[64];
    char group[64];
    char modified[64];
    char *target = NULL;
    const char *type = "unknown";
    const char *note = "";
    unsigned long long size;

    if (lstat(path, &st) != 0) {
        fprintf(stderr, "fileinfo: %s: %s\n", path, strerror(errno));
        return -1;
    }

    format_mode(st.st_mode, mode);
    owner_name(st.st_uid, owner, sizeof(owner));
    group_name(st.st_gid, group, sizeof(group));
    zx_format_time(st.st_mtime, modified, sizeof(modified));

    if (S_ISLNK(st.st_mode)) {
        target = read_link_target(path, st.st_size);
        type = "symbolic link";
        note = " (link)";
    } else if (S_ISDIR(st.st_mode)) {
        type = "directory";
        note = " (directory entry)";
    } else if (S_ISREG(st.st_mode)) {
        type = "regular file";
    } else if (S_ISCHR(st.st_mode)) {
        type = "character device";
        note = " (device)";
    } else if (S_ISBLK(st.st_mode)) {
        type = "block device";
        note = " (device)";
    } else if (S_ISFIFO(st.st_mode)) {
        type = "FIFO";
    } else if (S_ISSOCK(st.st_mode)) {
        type = "socket";
    }

    size = (unsigned long long)st.st_size;

    printf("Path:     %s\n", path);
    if (target != NULL)
        printf("Type:     %s -> %s\n", type, target);
    else if (S_ISLNK(st.st_mode))
        printf("Type:     %s -> (unreadable)\n", type);
    else
        printf("Type:     %s\n", type);
    printf("Size:     %llu bytes%s\n", size, note);
    printf("Mode:     %s\n", mode);
    printf("Links:    %lu\n", (unsigned long)st.st_nlink);
    printf("Owner:    %s\n", owner);
    printf("Group:    %s\n", group);
    printf("Modified: %s\n", modified);
    printf("\n");

    free(target);
    return 0;
}
