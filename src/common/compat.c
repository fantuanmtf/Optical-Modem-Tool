/*
 * compat.c - fallbacks for helpers that are not available in every libc.
 *
 * musl, glibc and uClibc differ on strlcpy/strlcat/strcasestr/strchrnul.
 * configure sets HAVE_* macros; the zx_* wrappers below keep the rest of the
 * code free of #ifdefs.
 */
#include "zxcompat.h"

#include <ctype.h>
#include <string.h>

#if defined(HAVE_STRLCPY) && HAVE_STRLCPY
size_t zx_strlcpy(char *dst, const char *src, size_t size)
{
    return strlcpy(dst, src, size);
}
#else
size_t zx_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);

    if (size > 0) {
        size_t copy = len < size - 1 ? len : size - 1;
        memcpy(dst, src, copy);
        dst[copy] = '\0';
    }
    return len;
}
#endif

#if defined(HAVE_STRLCAT) && HAVE_STRLCAT
size_t zx_strlcat(char *dst, const char *src, size_t size)
{
    return strlcat(dst, src, size);
}
#else
size_t zx_strlcat(char *dst, const char *src, size_t size)
{
    size_t dlen = 0;
    size_t slen = strlen(src);

    while (dlen < size && dst[dlen] != '\0')
        dlen++;
    if (dlen == size)
        return size + slen;
    if (slen >= size - dlen) {
        size_t copy = size - dlen - 1;
        memcpy(dst + dlen, src, copy);
        dst[dlen + copy] = '\0';
    } else {
        memcpy(dst + dlen, src, slen + 1);
    }
    return dlen + slen;
}
#endif

#if defined(HAVE_STRCASESTR) && HAVE_STRCASESTR
char *zx_strcasestr(const char *haystack, const char *needle)
{
    return strcasestr(haystack, needle);
}
#else
char *zx_strcasestr(const char *haystack, const char *needle)
{
    size_t nlen = strlen(needle);

    if (nlen == 0)
        return (char *)haystack;
    for (; *haystack; haystack++) {
        size_t i;
        for (i = 0; i < nlen; i++) {
            if (haystack[i] == '\0')
                return NULL;
            if (tolower((unsigned char)haystack[i]) !=
                tolower((unsigned char)needle[i]))
                break;
        }
        if (i == nlen)
            return (char *)haystack;
    }
    return NULL;
}
#endif

#if defined(HAVE_STRCHRNUL) && HAVE_STRCHRNUL
char *zx_strchrnul(const char *s, int c)
{
    return strchrnul(s, c);
}
#else
char *zx_strchrnul(const char *s, int c)
{
    char ch = (char)c;

    while (*s && *s != ch)
        s++;
    return (char *)s;
}
#endif
