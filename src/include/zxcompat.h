/*
 * zxcompat.h - portability shims for non-standard or optional libc helpers.
 *
 * configure probes the target libc and emits HAVE_* definitions in config.mk.
 * The zx_* wrappers always exist: they call the native function when it is
 * available, otherwise compat.c supplies a small POSIX implementation.
 */
#ifndef ZXCOMPAT_H
#define ZXCOMPAT_H

#include <stddef.h>

size_t zx_strlcpy(char *dst, const char *src, size_t size);
size_t zx_strlcat(char *dst, const char *src, size_t size);
char *zx_strcasestr(const char *haystack, const char *needle);
char *zx_strchrnul(const char *s, int c);

#endif /* ZXCOMPAT_H */
