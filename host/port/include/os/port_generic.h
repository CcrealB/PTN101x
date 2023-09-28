/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#ifndef _PORT_GENERIC_H_
#define _PORT_GENERIC_H_

#include "uw_types.h"
#include "uw_errno.h"

int32_t os_printf(const char *fmt, ...);
int32_t os_null_printf(const char *fmt, ...);

int32_t os_memcmp(const void *s1, const void *s2, uint32_t n);
int32_t os_snprintf(char *buf, uint32_t size, const char *fmt, ...);
int32_t os_strcmp(const char *s1, const char *s2);
int32_t os_strncmp(const char *s1, const char *s2, const uint32_t n);
char *os_strcat(char *dst, const char *src);
char *os_strncat(char *dst, const char *src, j_size_t n);
void *os_memset(void *b, int c, size_t len);
void *os_memcpy(void *out, const void *in, size_t n);
uint32_t os_strlen(const char *str);

void os_exit(void);

result_t os_port_init(void);
void os_port_uninit(void);

/* Define trace levels */
#define LOG_LEVEL_NONE                  0     /* No trace messages to be generated    */
#define LOG_LEVEL_ERROR                 1     /* Error condition trace messages       */
#define LOG_LEVEL_WARNING               2     /* Warning condition trace messages     */
#define LOG_LEVEL_INFO                  3     /* Information traces                   */
#define LOG_LEVEL_DEBUG                 4     /* Debug messages for events            */

#define LOG_LEVEL                       LOG_LEVEL_INFO

/* The log color feature only available for Linux and Mac system, for Windows system please don't config it. */
#define LOG_COLOR_ENABLE                (0)
#if     LOG_COLOR_ENABLE
#define LOG_COLOR_BLACK                 "30"
#define LOG_COLOR_RED                   "31"
#define LOG_COLOR_GREEN                 "32"
#define LOG_COLOR_BROWN                 "33"
#define LOG_COLOR_BLUE                  "34"
#define LOG_COLOR_PURPLE                "35"
#define LOG_COLOR_CYAN                  "36"
#define LOG_COLOR(COLOR)                "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)                 "\033[1;" COLOR "m"
#define LOG_RESET_COLOR                 "\033[0m"
#define LOG_COLOR_E                     LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W                     LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I                     LOG_COLOR(LOG_COLOR_BLUE)
#define LOG_COLOR_D                     LOG_COLOR(LOG_COLOR_GREEN)
#else
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_RESET_COLOR
#endif

//#define LOG_FORMAT(tag, letter, format)  "["#tag"|"#letter"]: " format
#define LOG_FORMAT(tag, letter, format)  "["#letter"]: " format

#if (LOG_LEVEL > 0)

#define LOG_E(tag, format, ...)  { if(LOG_LEVEL >= LOG_LEVEL_ERROR)   os_printf(LOG_FORMAT(tag, E, format), ##__VA_ARGS__); }
#define LOG_W(tag, format, ...)  { if(LOG_LEVEL >= LOG_LEVEL_WARNING) os_printf(LOG_FORMAT(tag, W, format), ##__VA_ARGS__); }
#define LOG_I(tag, format, ...)  { if(LOG_LEVEL >= LOG_LEVEL_INFO)    os_printf(LOG_FORMAT(tag, I, format), ##__VA_ARGS__); }
#define LOG_D(tag, format, ...)  { if(LOG_LEVEL >= LOG_LEVEL_DEBUG)   os_printf(LOG_FORMAT(tag, D, format), ##__VA_ARGS__); }

#else

#define LOG_E(tag, format, ...)
#define LOG_W(tag, format, ...)
#define LOG_I(tag, format, ...)
#define LOG_D(tag, format, ...)

#endif

#endif//_PORT_GENERIC_H_
