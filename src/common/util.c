/*
 * util.c - shared output, formatting and filesystem helpers.
 */
#include "zxcommon.h"
#include "zxcompat.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const char *zx_basename(const char *path)
{
    const char *slash;

    if (path == NULL)
        return "";
    slash = strrchr(path, '/');
    return slash != NULL ? slash + 1 : path;
}

void zx_human_bytes(unsigned long long bytes, char *buf, size_t buflen)
{
    static const char *const units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = (double)bytes;
    size_t unit = 0;

    while (value >= 1024.0 && unit + 1 < ZX_ARRAY_LEN(units)) {
        value /= 1024.0;
        unit++;
    }
    snprintf(buf, buflen, "%.2f %s", value, units[unit]);
}

void zx_human_rate(unsigned long long bytes_per_sec, char *buf, size_t buflen)
{
    if (buflen == 0)
        return;
    zx_human_bytes(bytes_per_sec, buf, buflen);
    zx_strlcat(buf, "/s", buflen);
}

void zx_format_time(time_t when, char *buf, size_t buflen)
{
    struct tm tm;

    if (localtime_r(&when, &tm) == NULL) {
        if (buflen > 0)
            buf[0] = '\0';
        return;
    }
    if (strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &tm) == 0 && buflen > 0)
        buf[0] = '\0';
}

char *zx_trim(char *text)
{
    char *end;

    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
        text++;
    end = text + strlen(text);
    while (end > text) {
        char c = end[-1];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            break;
        end--;
    }
    *end = '\0';
    return text;
}

int zx_parse_uint(const char *text, unsigned long *value)
{
    char *end = NULL;
    unsigned long parsed;

    if (text == NULL || *text == '\0' || *text == '-')
        return -1;
    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0')
        return -1;
    *value = parsed;
    return 0;
}

static int ensure_dir(const char *path, mode_t mode)
{
    struct stat st;

    if (mkdir(path, mode) == 0)
        return 0;
    if (errno != EEXIST)
        return -1;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    return 0;
}

int zx_mkdir_p(const char *path, mode_t mode)
{
    char *copy;
    char *p;
    size_t len;
    int rc = 0;

    if (path == NULL || *path == '\0') {
        errno = EINVAL;
        return -1;
    }
    copy = strdup(path);
    if (copy == NULL)
        return -1;

    len = strlen(copy);
    while (len > 1 && copy[len - 1] == '/')
        copy[--len] = '\0';

    for (p = copy + 1; *p != '\0'; p++) {
        if (*p != '/')
            continue;
        *p = '\0';
        if (ensure_dir(copy, mode) != 0) {
            rc = -1;
            goto out;
        }
        *p = '/';
    }
    if (ensure_dir(copy, mode) != 0)
        rc = -1;
out:
    free(copy);
    return rc;
}

int zx_read_file(const char *path, char *buf, size_t buflen)
{
    FILE *fp;
    size_t got;

    if (buflen == 0)
        return -1;
    fp = fopen(path, "r");
    if (fp == NULL)
        return -1;
    got = fread(buf, 1, buflen - 1, fp);
    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    buf[got] = '\0';
    fclose(fp);
    return (int)got;
}

int zx_output_open(zx_output *out, const char *path)
{
    out->console = stdout;
    out->log = NULL;
    if (path != NULL) {
        out->log = fopen(path, "w");
        if (out->log == NULL)
            return -1;
    }
    return 0;
}

void zx_output_close(zx_output *out)
{
    if (out->log != NULL && out->log != out->console)
        fclose(out->log);
    out->log = NULL;
}

void zx_output_line(zx_output *out, const char *fmt, ...)
{
    char line[4096];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    if (out->console != NULL) {
        fputs(line, out->console);
        fputc('\n', out->console);
    }
    if (out->log != NULL) {
        fputs(line, out->log);
        fputc('\n', out->log);
    }
}

void zx_output_text(zx_output *out, const char *text)
{
    if (text == NULL)
        return;
    if (out->console != NULL)
        fputs(text, out->console);
    if (out->log != NULL)
        fputs(text, out->log);
}

size_t zx_output_stream(zx_output *out, FILE *fp)
{
    char buf[4096];
    size_t total = 0;
    size_t got;

    while ((got = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (out->console != NULL)
            fwrite(buf, 1, got, out->console);
        if (out->log != NULL)
            fwrite(buf, 1, got, out->log);
        total += got;
    }
    return total;
}
