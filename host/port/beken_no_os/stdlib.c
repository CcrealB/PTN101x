/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */

#include "includes.h"
#include <string.h>
#include <stdlib.h>

int32_t os_memcmp(const void *s1, const void *s2, uint32_t n)
{
    return memcmp(s1, s2, (unsigned int)n);
}

void *os_memcpy(void *out, const void *in, size_t n)
{
    return memcpy(out, in, n);
}

void *os_memset(void *b, int c, size_t len)
{
    return (void *)memset(b, c, len);
}

uint32_t os_strlen(const char *str)
{
    return strlen(str);
}

int32_t os_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

int32_t os_strncmp(const char *s1, const char *s2, const uint32_t n)
{
    return strncmp(s1, s2, (unsigned int)n);
}

int32_t os_snprintf(char *buf, uint32_t size, const char *fmt, ...)
{
    va_list args;
    int32_t rc;

    va_start(args, fmt);
    rc = vsnprintf(buf, size, fmt, args);
    va_end(args);

    /* if want to print more than the limitation */
    if (rc > size)
        rc = (int32_t)size - rc;

    return rc;
}

int32_t os_vsnprintf(char *buf, uint32_t size, const char *fmt, va_list ap)
{
    return vsnprintf(buf, size, fmt, ap);
}

