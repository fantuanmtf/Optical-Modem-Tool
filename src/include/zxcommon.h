/*
 * zxcommon.h - small shared helpers for the Optical-Modem-Tool utilities.
 *
 * Only POSIX/Linux APIs are used.  Anything that may be missing from a libc
 * lives behind zxcompat.h instead.
 */
#ifndef ZXCOMMON_H
#define ZXCOMMON_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#ifndef ZX_VERSION
#define ZX_VERSION "1.3.0"
#endif

#define ZX_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

/*
 * A report sink.  console is normally stdout; log is an optional file that
 * receives exactly the same text (the old shell tools used tee for this).
 */
typedef struct {
    FILE *console;
    FILE *log;
} zx_output;

/* argv[0] without its directory component. */
const char *zx_basename(const char *path);

/* "512.00 B", "1.25 MB", ... (two decimals). */
void zx_human_bytes(unsigned long long bytes, char *buf, size_t buflen);

/* Same, with a "/s" suffix. */
void zx_human_rate(unsigned long long bytes_per_sec, char *buf, size_t buflen);

/* Local time formatted as YYYY-MM-DD HH:MM:SS. */
void zx_format_time(time_t when, char *buf, size_t buflen);

/* Strip leading/trailing whitespace in place; returns text. */
char *zx_trim(char *text);

/* Parse a non-negative decimal integer.  Returns 0 on success, -1 on error. */
int zx_parse_uint(const char *text, unsigned long *value);

/* mkdir -p.  Returns 0 on success, -1 on error. */
int zx_mkdir_p(const char *path, mode_t mode);

/* Read a whole file into buf (NUL terminated).  Returns byte count or -1. */
int zx_read_file(const char *path, char *buf, size_t buflen);

/* Open an optional log file.  path may be NULL (console only). */
int zx_output_open(zx_output *out, const char *path);
void zx_output_close(zx_output *out);

/* Print one formatted line to console and log. */
void zx_output_line(zx_output *out, const char *fmt, ...);

/* Write pre-formatted text verbatim to console and log. */
void zx_output_text(zx_output *out, const char *text);

/* Copy an open stream to console and log; returns bytes copied. */
size_t zx_output_stream(zx_output *out, FILE *fp);

#endif /* ZXCOMMON_H */
