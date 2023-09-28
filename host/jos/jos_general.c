/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#define INCLUDE_GENERAL
#include "jos_internal.h"

#ifdef J_DEBUG
int32_t time_diff(jtime_t *end_time, jtime_t *start_time)
{
    KASSERT0(end_time);
    KASSERT0(start_time);

    return (end_time->tv_usec - start_time->tv_usec) / 1000 +
        (end_time->tv_sec - start_time->tv_sec) * 1000;
}

#ifdef DEBUG_WITH_TIMERS
int32_t dbg_get_time_diff(void)
{
    static uint8_t      timer_initialized = 0;
    static os_time_t    timer;
    os_time_t           temp_timer;
    int32_t rc;

    if(!timer_initialized)
    {
        timer_initialized = 1;
        os_get_time(&timer);
    }

    os_get_time(&temp_timer);
    rc = time_diff(&temp_timer, &timer);
    os_memcpy(&timer, &temp_timer, sizeof(timer));
    return rc;
}
#endif

typedef struct {
    jtime_t t;
    char str[UW_LOG_MSG_LEN];
} log_msg_t;

static struct {
    log_msg_t *msgs;
    uint32_t num;
    uint32_t after;
    uint32_t done;
    BOOL started;

    uint32_t begin;
    uint32_t end;
} j_log_msg = {NULL};

result_t uw_log_init(uint32_t num)
{
    KASSERT(!j_log_msg.msgs, ("j_log_msg already initiliazed\n"));

    j_memset(&j_log_msg, 0, sizeof(j_log_msg));
    num++;

    j_log_msg.msgs = (log_msg_t *)jmalloc(num * sizeof(log_msg_t), M_ZERO);
    j_log_msg.num = num;

    return UWE_OK;
}

void uw_log_uninit(void)
{
    KASSERT(j_log_msg.msgs, ("j_log_msg used before init\n"));

    uw_log_stop();

    jfree(j_log_msg.msgs);
    j_log_msg.msgs = NULL;
}

BOOL uw_log_started(void)
{
    return j_log_msg.started;
}

void uw_log_start(void)
{
    KASSERT(j_log_msg.msgs, ("j_log_msg used before init\n"));
    j_log_msg.started = 1;
}

static void log_clear(void)
{
    j_log_msg.done = 0;
    j_log_msg.begin = j_log_msg.end = 0;
    j_log_msg.after = 0;
}

void uw_log_stop(void)
{
    j_log_msg.started = 0;
    uw_log_dump();
}

void uw_log_dump_after(uint32_t msg_num)
{
    msg_num = MIN(msg_num, j_log_msg.num-1);
    j_log_msg.after = msg_num;
    j_log_msg.done = 0;
}

#define OVF_INC(x, lim) ((((x)+1) != (lim)) * ((x)+1))
#define OVF_SUB(x, y, lim) ((x)>=(y) ? (x)-(y) : (x)+(lim)-(y))

static uint32_t log_next(void)
{
    j_log_msg.end = OVF_INC(j_log_msg.end, j_log_msg.num);

    /* If end reaches begin, we start losing messages */
    if(j_log_msg.begin == j_log_msg.end)
        j_log_msg.begin = OVF_INC(j_log_msg.begin, j_log_msg.num);

    if(j_log_msg.after)
        j_log_msg.done++;

    return j_log_msg.end;
}

BOOL uw_log_completed(void)
{
    return j_log_msg.after && j_log_msg.done >= j_log_msg.after;
}

char *uw_log_next_msg_buf(void)
{
    uint32_t pos;

    KASSERT(j_log_msg.msgs, ("j_log_msg used before init\n"));

    pos = log_next();

    jget_time(&j_log_msg.msgs[pos].t);

    return j_log_msg.msgs[pos].str;
}

void uw_log_dump(void)
{
    uint32_t i, all, before, first, prev, pos;

    KASSERT(j_log_msg.msgs, ("j_log_msg used before init\n"));

    jprintf("Dump j_log_msg:\n");

    if(j_log_msg.begin == j_log_msg.end)
    {
        jprintf("No messages in j_log_msg\n");
        return;
    }

    all = OVF_SUB(j_log_msg.end, j_log_msg.begin, j_log_msg.num);

    first = OVF_INC(j_log_msg.begin, j_log_msg.num);

    if(j_log_msg.after)
        before = all - j_log_msg.done;
    else
        before = 0;

    pos = prev = first;
    i = 0;

    while (1)
    {
        jprintf("%3ld: %3ld ms (%3ld ms): %s\n",
            (int32_t)i - (int32_t)before,
            time_diff(&j_log_msg.msgs[pos].t, &j_log_msg.msgs[first].t),
            time_diff(&j_log_msg.msgs[pos].t, &j_log_msg.msgs[prev].t),
            j_log_msg.msgs[pos].str);

        if(pos == j_log_msg.end)
            break;

        i++;
        prev = pos;
        pos = OVF_INC(pos, j_log_msg.num);
    }
    log_clear();
}

#endif

uint32_t j_arc4random(void)
{
    jtime_t x;
    jget_time(&x);
    return (x.tv_sec ^ x.tv_usec);
}

uint16_t __bswap16(uint16_t value)
{
    return (
        (value  >> 8) |
        (value) << 8);
}

uint32_t __bswap32(uint32_t value)
{
    return (
        (value  >> 24) |
        (value  >>  8 & 0x0000ff00) |
        (value  <<  8 & 0x00ff0000) |
        (value) << 24);
}

/*
 *   JOS String Manipulation Functions
 */

char *j_strdup(const char *str)
{
    uint32_t len;
    char *dup;

    len = j_strlen(str)+1;
    dup = (char *)jmalloc(len, M_ZERO);
    j_memcpy(dup, str, len);

    return dup;
}

uint32_t j_strnlen(char *str, uint32_t maxlen)
{
    uint32_t len;

    for (len = 0; *(str++) && maxlen--; len++)
        ;

    return len;
}

/**
 * Function name:  j_strcasecmp
 * Description:    Compares two strings ignoring the case of the characters
 * Parameters:
 *     @p1:        (IN) The first string to compare
 *     @p2:        (IN) The second string to compare
 *
 * Return value:   An integer less than, equal to, or greater than zero if
 *                 s1  is  found,  respectively,  to  be  less than, to match,
 *                 or be greater than s2.
 * Scope:          Global
 **/
int32_t j_strcasecmp(const char *p1, const char *p2)
{
    for (; j_tolower(*p1) == j_tolower(*p2) && *p1; ++p1, ++p2)
        ;
    return j_tolower(*p1) - j_tolower(*p2);
}

/**
 * Function name:  j_strstr
 * Description:    Searches for the first occurrence of a given sub-string
 *                 within a given input string.
 * Parameters:
 *     @instr:     (IN) The input string in which to search for the @substr
 *                      sub-string.
 *     @substr:    (IN) The sub-string to find.
 *
 * Return value:   Returns a pointer to the first occurence of the sub-string
 *                 @substr within the input string @instr, or NULL if the
 *                 input string does not contain the sub-string.
 * Scope:          Global
 **/
char *j_strstr(const char *instr, const char *substr)
{
    uint32_t len = j_strlen(substr);

    for (;*instr; instr++)
    {
        if(!j_strncmp(instr, substr, len))
            return (char *)instr;
    }

    return NULL;
}

/**
 * Function name:  j_numtostr
 * Description:    Converts a 32-bit unsigned integer into a "numeric string".
 * Parameters:
 *     @outstr:     (OUT) Pointer to the beginning of a pre-allocated characters
 *                        array, to be filled with the requested string.
 *                        The function returns a null-terminated string that
 *                        contains a text representation of the given number
 *                        (@num), according to the specified @base, @min_digits,
 *                        and @max_digits values.
 *     @base:       (IN)  The number's (@num) base.
 *     @min_digits: (IN)  Minimal amount of digits to include in the returned
 *                        string. The number in the output string will be padded
 *                        with the amount of zeros ('0') required to meet the
 *                        minimal digits requirement.
 *     @max_digits: (IN)  The maximum amount of digits to include in the
 *                        returned string. This field should be used to ensure
 *                        that the returned string does not exceed the size of
 *                        the output string parameter - @outstr.
 *     @num:        (IN)  A 32-bit unsigned integer.
 *
 * Return value:   Returns the output string (@outstr) on success, or NULL on
 *                 failure.
 * Scope:          Global
 **/
char *j_numtostr(char *outstr, uint8_t base, uint32_t min_digits,
    uint32_t max_digits, uint32_t num)
{
    uint32_t i = 0, j = 0;
    uint8_t val;
    char str[32];

    KASSERT(outstr, ("Passed NULL pointer to j_numtostr\n"));

    if(base < 2)
    {
        DBG_E(DJOS_GENERAL, ("Cannot convert a numeric value with base = %d\n",
            base));
        return NULL;
    }

    do
    {
        val = (uint8_t)(num % base);
        num /= base;

        if(val > 9)
            str[j] = val+'A'-10;
        else
            str[j] = val+'0';

        j++;
    } while (num);

    /* Pad for minimal amount of digits */
    while (j < min_digits--)
        outstr[i++] = '0';

    /* Reverse string up to max digits requested */
    while (j && max_digits--)
    {
        outstr[i] = str[j-1];
        i++; j--;
    }

    outstr[i] = 0;
    return outstr;
}

/**
 * Function name:  j_strlcpy
 * Description:    Copies a string to a given destination buffer.
 *                 Note:
 *                 The function
 *                 - Guarantees null-termination of the destination string,
 *                   unless the size of the destination buffer (@dst_size) is
 *                   zero.
 *                 - Copies up to @dst_size - 1 bytes to the destination buffer
 *                   (@dst); the last byte is reserved for the null-terminator
 *                   character.
 *                 - Does not zero-fill any extra characters in the destination
 *                   buffer (@dst), after the null terminator.
 * Parameters:
 *     @dst:       (OUT) Destination buffer.
 *     @src:       (IN)  Null-terminated source string.
 *     @dst_size:  (IN)  The size of the destination buffer (@dst), in bytes.
 * Return value:   Returns the length of the source string (@src).
 *                 Note: A return value greater than @dst_size - 1 indicates
 *                 that the copied string (including the null-terminator
 *                 character) was truncated to the size of the destination
 *                 buffer (@dst_size).
 * Scope:          Global
 **/
uint32_t j_strlcpy(char *dst, const char *src, uint32_t dst_size)
{
    uint32_t i;

    for (i = 0; i < dst_size; i++)
    {
        if((*dst++ = *src++) == '\0')
            return i;
    }

    if(dst_size)
        *(dst - 1) = '\0';

    return dst_size + j_strlen(src);
}

/* Internal function used by j_strtonum */
static uint32_t numeric_value(char c, uint32_t base)
{
    uint32_t val = NUMERIC_VALUE_INVALID;

    if(c >= '0' && c <= '9')
        val = c - '0';
    else if(c >= 'A' && c <= 'F')
        val = c - 'A' + 10;
    else if(c >= 'a' && c <= 'f')
        val = c - 'a' + 10;

    if(val >= base)
        return NUMERIC_VALUE_INVALID;

    return val;
}

/**
 * Function name:  j_strtonum
 * Description:    Converts a "numeric string" into a 32-bit unsigned integer.
 * Parameters:
 *     @instr:    (IN)  A null-terminated string containing the numeric text
 *                      representation to convert.
 *     @base:     (IN)  The number's base.
 *     @last_ptr: (OUT) Optional parameter. If not NULL, will be updated by the
 *                      function to point to the location in the input string
 *                      (@instr) at which the conversion ended (i.e. one
 *                      character past the last recognized numeric character in
 *                      the string).
 *
 * Return value:   On success, returns the 32-bit integer represented by the
 *                 input string (@instr), according to the given @base;
 *                 on failure returns NUMERIC_VALUE_INVALID, indicating that the
 *                 input string does not represent a number.
 * Scope:          Global
 **/
uint32_t j_strtonum(const char *instr, uint8_t base, char **last_ptr)
{
    const char *ptr = instr;
    uint32_t value = 0;
    uint32_t cvalue;

    cvalue = numeric_value(*ptr, base);
    while (*ptr && cvalue != NUMERIC_VALUE_INVALID)
    {
        value *= base;
        value += cvalue;
        ptr++;
        cvalue = numeric_value(*ptr, base);
    }

    if(last_ptr)
        *last_ptr = (char *)ptr;

    if(ptr == instr)
        return NUMERIC_VALUE_INVALID;

    return value;
}

/**
 * Function name:  j_is_space
 * Description:    Checks whether a given character is a white space.
 * Parameters:
 *     @c: (IN) The character to check.
 *
 * Return value:   Returns TRUE if the character (@c) is a white space;
 *                 otherwise returns FALSE.
 * Scope:          Global
 **/
BOOL j_is_space(char c)
{
    if(c == ' ' ||
        c == '\t' ||
        c == '\r' ||
        c == '\n')
    {
        return 1;
    }

    return 0;
}

/**
 * Function name:  j_strrchr
 * Description:    Find the last occurrence of @c in @cs.
 * Parameters:
 *     @cs:        (IN) A NULL terminated string
 *     @c:         (IN) Character to look for
 *
 * Return value:   A pointer to the last occurrence of @c in @cs, or NULL if
 *                 not found.
 * Scope:          Global
 **/
char *j_strrchr(char *cs, char c)
{
    char *pos = NULL;

    for (; *cs; cs++)
    {
        if(*cs == c)
            pos = cs;
    }
    return pos;
}

/**
 * Function name:  j_strlchr
 * Description:    Find the first occurrence of @c in @cs.
 * Parameters:
 *     @cs:        (IN) A NULL terminated string
 *     @len:       (IN) String length
 *     @c:         (IN) Character to look for
 *
 * Return value:   A pointer to the first occurrence of @c in @cs, or NULL if
 *                 not found.
 * Scope:          Global
 **/
char *j_strlchr(char *cs, uint32_t len, char c)
{
    for (; len && *cs; len--, cs++)
    {
        if(*cs == c)
            return cs;
    }

    return NULL;
}

/**
 * Function name:  j_strchr
 * Description:    Find the first occurrence of @c in @cs.
 * Parameters:
 *     @cs:        (IN) A NULL terminated string
 *     @c:         (IN) Character to look for
 *
 * Return value:   A pointer to the first occurrence of @c in @cs, or NULL if
 *                 not found.
 * Scope:          Global
 **/
char *j_strchr(char *cs, char c)
{
    while (*cs)
    {
        if(*cs == c)
            return cs;

        cs++;
    }

    return NULL;
}

int32_t j_tolower(int32_t c)
{
    if(c >= 'A' && c < 'Z')
        return c + 'a' - 'A';

    return c;
}

/**
 * Function name:  j_strltrim
 * Description:    Trim leading white spaces.
 * Parameters:
 *     @str:       (IN) String, doesn't have to be NULL terminated.
 *     @len:       (IN/OUT) On input, length of the input string.
 *                          On output, length of the returned string.
 *
 * Return value:   Pointer to the first non-white character
 * Scope:          Global
 **/
char *j_strltrim(char *str, uint32_t *len)
{
    uint32_t off;

    for (off = 0; off < *len && j_is_space(*str); off++, str++)
        ;

    *len -= off;
    return str;
}

/**
 * Function name:  j_strtrim
 * Description:    Trim leading and trailing white spaces.
 * Parameters:
 *     @str:       (IN) String, doesn't have to be NULL terminated but NULL char
 *                      will indicate termination regardless of len.
 *     @len:       (IN/OUT) On input, length of the input string.
 *                          On output, length of the returned string.
 *                          NULL or 0 indicates NULL terminated string.
 *     @move_str   (IN) TRUE to move string to the beginning of the buffer so
 *                      return value will always be 'str'. FALSE to leave chars
 *                      where they are.
 *
 * Return value:   Pointer to the first non-white character
 * Scope:          Global
 **/
char *j_strtrim(char *str, uint32_t *len, BOOL move_str)
{
    uint32_t i, off, end, length;

    if(!str)
        return NULL;

    length = len && *len > 0 ? *len : UW_MAX_UINT32;

    /* Skip white space at the beginning, set 'off' to the 1st non-white space
     * char */
    for (off = 0; off < length && j_is_space(str[off]); off++)
        ;

    /* Traverse string, set 'end' after the last non-white space char */
    for (end = off, i = off; i < length && str[i]; i++)
    {
        if(!j_is_space(str[i]))
            end = i + 1;
    }

    length = end - off;
    if(move_str && off > 0)
    {
        for (end = 0; end < length; end++)
            str[end] = str[off + end];

        off = 0;
    }

    if(len)
        *len = length;

    /* If the input was NULL terminated make the output NULL terminated */
    if(!str[i])
        str[end] = '\0';

    return str + off;
}

static void str_to_array_helper(char *p, uint32_t len, const char *delimiters,
    uint32_t delimiters_n, uint32_t *array_n, char **array)
{
    uint32_t off, i;
    BOOL non_delimiter;

    for (off = 0, i = 0, non_delimiter = 0; off < len; p++, off++)
    {
        if(*p == '\0' || j_strlchr((char *)delimiters, delimiters_n, *p))
        {
            *p = '\0';
            non_delimiter = 0;
        }
        else if(!non_delimiter)
        {
            /* first non-delimiter character */
            non_delimiter = 1;
            if(array)
                array[i] = p;
            i++;
        }
    }

    *array_n = i;
 }

/**
 * Function name:  j_str_to_array
 * Description:    Split a buffer to an array of strings using a list of
 *                 delimiters. NULL characters ('\0') contained in the buffer
 *                 are automatically treated as delimiters.
 * Parameters:
 *      @buf:          (IN)  Characters array (may contain multiple NULL
 *                           characters)
 *      @len:          (IN)  Size of characters array
 *      @delimiters:   (IN)  Array of delimiters
 *      @delimiters_n: (IN)  Size of array of delimiters
 *      @array_n:      (OUT) Size of output array
 *
 * Return value:   Array of NULL terminated strings
 * Scope:          Global
 *
 *
 * Example 1:
 *     Input:
 *         buf "(\"service\",(0-1)),(\"callheld\",(0-2))"
 *         delimiters "()\"-"
 *     Output array:
 *         {"service", "0", "1", "callheld", "0", "2"}
 *
 * Example 2:
 *     Input:
 *         buf "0,1,2,3,4"
 *         delimiters ","
 *     Output array:
 *         {"0", "1", "2", "3", "4"}
 **/
char **j_str_to_array(char *buf, uint32_t len, const char *delimiters,
    uint32_t delimiters_n, uint32_t *array_n)
{
    char **array;

    /* Pass 1: count strings and replace delimiters with '\0' */
    str_to_array_helper(buf, len, delimiters, delimiters_n, array_n, NULL);

    if(*array_n == 0)
        return NULL;

    array = (char **)jmalloc((*array_n) * sizeof(char *), M_ZERO);

    /* Pass 2: set pointers in the array. Each pointer points to a NULL
     * terminated string.
     */
    str_to_array_helper(buf, len, delimiters, delimiters_n, array_n, array);

    return array;
}

const char *uwe_str(result_t rc)
{
    switch (rc)
    {
    case UWE_OK:
        return "OK";
    case UWE_NOTSTARTED:
        return "OPERATION NOT STARTED";
    case UWE_INPROGRESS:
        return "OPERATION IN PROGRESS";
    case UWE_PERM:
        return "OPERATION NOT PERMITTED";
    case UWE_NOENT:
        return "NO SUCH ENTRY";
    case UWE_IO:
        return "INPUT/OUTPUT ERROR";
    case UWE_NXIO:
        return "DEVICE NOT CONFIGURED";
    case UWE_NOMEM:
        return "FAILED ALLOCATING MEMORY";
    case UWE_BUSY:
        return "RESOURCE IS BUSY";
    case UWE_NODEV:
        return "NO SUCH DEVICE";
    case UWE_INVAL:
        return "INVALID ARGUMENT";
    case UWE_NOTSUP:
        return "OPERATION NOT SUPPORTED";
    case UWE_TIMEDOUT:
        return "OPERATION TIMED OUT";
    case UWE_SUSPENDED:
        return "DEVICE IS SUSPENDED";
    case UWE_UNKNOWN:
        return "GENERAL-PURPOSE ERROR";
    case UWE_TEST_FAILED:
        return "LOGICAL TEST FAILURE";
    case UWE_STATE:
        return "INCORRECT STATE";
    case UWE_STALLED:
        return "PIPE IS STALLED";
    case UWE_PARAM:
        return "INVALID PARAMETER";
    case UWE_ABORTED:
        return "OPERATION ABORTED";
    case UWE_SHORT:
        return "SHORT TRANSFER";
    case UWE_WOULDBLOCK:
        return "WOULD BLOCK";
    case UWE_ALREADY:
        return "ALREADY";
    case UWE_EXPIRED:
        return "EVALUATION TIME EXPIRED";
    case UWE_FULL:
        return "RESOURCE IS FULL";
    case UWE_NETDOWN:
        return "NET DOWN";
    case UWE_NETUNREACH:
        return "NET UNREACHABLE";
    case UWE_NETRESET:
        return "NET RESET";
    case UWE_CONNABORTED:
        return "CONNECTION ABORTED";
    case UWE_CONNRESET:
        return "CONNECTION RESET";
    case UWE_ISCONN:
        return "ALREADY CONNECTED";
    case UWE_NOTCONN:
        return "NOT CONNECTED";
    case UWE_CONNREFUSED:
        return "CONNECTION REFUSED";
    case UWE_HOSTDOWN:
        return "HOST DOWN";
    case UWE_HOSTUNREACH:
        return "HOST UNREACHABLE";
    case UWE_NOLINK:
        return "NO LINK";
    case UWE_PROTO:
        return "PROTOCOL";
    case UWE_NOPROTOOPT:
        return "NO PROTOCOL OPTION";
    case UWE_DESTADDRREQ:
        return "DEST ADDR REQUIRED";
    case UWE_ADDRNOTAVAIL:
        return "CAN'T ASSIGN REQUESTED ADDRESS";
    case UWE_MSGSIZE:
        return "MESSAGE TOO LONG";
    case UWE_INTR:
        return "OPERATION INTERRUPTED";
    case UWE_PROTOTYPE:
        return "PROTOCOL TYPE MISMATCH";
    }
    return "INVALID RESULT_T VALUE";
}

void jvirt_to_sg(void *_src, sg_list_item_t *sg_list, uint32_t size)
{
    uint32_t total;
    uint16_t page;
    uint8_t *src = (uint8_t *)_src;

    KASSERT(sg_list, ("JOS: No sg list passed to jvirt_to_sg\n"));
    KASSERT(_src, ("JOS: No virtual address passed to jvirt_to_sg\n"));

    for (total = 0, page = 0; total < size; page++)
    {
        KASSERT(sg_list[page].vaddr, ("JOS: No virtual address at page %d "
            "of jvirt_to_sg\n", page));

        j_memcpy(sg_list[page].vaddr, src + total,
            MIN(sg_list[page].size, size - total));

        total += sg_list[page].size;
    }
}

void jsg_to_virt(sg_list_item_t *sg_list, void *_dst, uint32_t size)
{
    uint32_t total;
    uint16_t page;
    uint8_t *dst = (uint8_t *)_dst;

    KASSERT(sg_list, ("JOS: No sg list passed to jsg_to_virt\n"));
    KASSERT(_dst, ("JOS: No virtual address passed to jsg_to_virt\n"));

    for (total = 0, page = 0; total < size; page++)
    {
        KASSERT(sg_list[page].vaddr, ("JOS: No virtual address at page %d "
            "of jsg_to_virt\n", page));

        j_memcpy(dst + total, sg_list[page].vaddr,
            MIN(sg_list[page].size, size - total));

        total += sg_list[page].size;
    }
}

#if defined(CONFIG_TEST) || defined(CONFIG_FD_TEST) || defined(J_DEBUG)

result_t jdbg_pattern_start_update(uint32_t *start, uint32_t size)
{
    KASSERT0(start);

    if(size & 0x3)
    {
        DBG_E(DJOS_GENERAL, ("JOS: Error: size is %lu. "
            "It must be a multiplicative of 4!\n", size));

        return UWE_STATE;
    }

    /* Update value to be the next 4 bytes in the verification pattern */
    *start += (size >> 2);

    return UWE_OK;
}

static result_t pattern_util_buf(uint8_t *buf, uint32_t size, uint32_t *pos,
    uint32_t start, BOOL is_write)
{
    DBG_X(DJOS_GENERAL, ("JOS: %s buffer. buffer %p, pos %lu, size %lu, "
        "start %lu (0x%lx)\n", (is_write ? "Writing" : "Validating"), buf, *pos,
        size, start, start));

    for (; size; (*pos)++)
    {
        uint32_t i;
        uint32_t val32 = ((*pos) + start);

        for (i = 0; i < 4 && size; size--, i++, buf++)
        {
            uint8_t val = (uint8_t)((val32 >> (i << 3)) & 0xff);

            if(is_write)
            {
                *buf = val;
            }
            else if(*buf != val)
            {
                DBG_E(DJOS_GENERAL, ("JOS: Error in validation buffer %p, "
                    "pos %lu, found 0x%02x, expected 0x%02x (0x%lx (%lu) "
                    "octet #%u)\n", buf, *pos, *buf, val, val32, val32, i));

                return UWE_INVAL;
            }
        }
    }

    return UWE_OK;
}

static result_t pattern_util_mem_desc(mem_desc_h membuf, uint32_t size,
    uint32_t start, BOOL is_write)
{
    uint32_t pos = 0;
    result_t rc = UWE_OK;

    DBG_X(DJOS_GENERAL, ("JOS: %s pattern %p, size %lu, "
        "start %lu, is_write %d\n", (is_write ? "Writing" : "Validating"),
        membuf, size, start, is_write));

    KASSERT0(membuf);

    if(membuf->vaddr)
    {
        rc = pattern_util_buf((uint8_t *)membuf->vaddr, size, &pos, start,
            is_write);
    }
    else if(membuf->sg_list)
    {
        sg_list_item_t *sg;
        uint32_t _size = size;

        /* XXX Unsafe: How long is sg_list? */
        /* Iterate over scatter-gather chunks */
        for (sg = membuf->sg_list; ; sg++)
        {
            uint32_t s = MIN(_size, sg->size);

            KASSERT(sg->size, ("sg->size == 0\n"));
            KASSERT0(sg->vaddr);

            _size -= s;
            rc = pattern_util_buf((uint8_t *)sg->vaddr, s, &pos, start,
                is_write);

            if(rc || !_size)
                break;
        }
    }
    else
    {
        DBG_E(DJOS_GENERAL, ("JOS: Error: Only Virutal address and "
            "scatter-gather are supported by validation util\n"));

        return UWE_NOTSUP;
    }

    if(rc)
    {
        DBG_E(DJOS_GENERAL, ("JOS: Error in validation buffer. size %lu "
            "pos %lu (0x%lx), start %lu (0x%lx)\n", size, pos, pos, start,
            start));

        if(!membuf->vaddr)
            return rc;

        DBG_HEX_DUMP(DJOS_GENERAL, DL_ERROR, membuf->vaddr, MIN(size, 1024), 0);
    }

    return rc;
}

result_t jdbg_pattern_set(mem_desc_h membuf, uint32_t size, uint32_t start)
{
    KASSERT0(membuf);

    DBG_X(DJOS_GENERAL, ("JOS: Writing pattern to %p, size %lu, start %lu\n",
        membuf->vaddr, size, start));

    return pattern_util_mem_desc(membuf, size, start, 1);
}

result_t jdbg_pattern_check(mem_desc_h membuf, uint32_t size, uint32_t start)
{
    KASSERT0(membuf);

    DBG_X(DJOS_GENERAL, ("JOS: - Checking pattern of %p, size %lu, start %lu\n",
        membuf->vaddr, size, start));

    return pattern_util_mem_desc(membuf, size, start, 0);
}

void jdbg_hex_dump(const char *file, int32_t line_no, uint8_t *buf, uint32_t size,
    uint8_t per_row)
{
    uint32_t i, row;
    uint8_t line[140];
    uint32_t left = size;

    if(!buf)
        return;

    per_row = MIN(per_row, 32);
    per_row = !per_row ? 16 : per_row;

    DBG_E(DJOS_GENERAL, ("Dump buffer %p size %lu:\n", buf, size));

#define TO_CHAR(a)      ((a) > 9 ? ((a) - 10 + 'A') : (a) + '0')
#define IS_PRINTABLE(a) ((a) > 31 && (a) < 127)

    for (row = 0; left; row++)
    {
        j_memset(line, ' ', sizeof(line));

        for (i = 0; (i < per_row) && left; i++, left--, buf++)
        {
            uint8_t val = *buf;

            /* The HEX value */
            line[(i * 3)] = TO_CHAR(val >> 4);
            line[(i * 3) + 1] = TO_CHAR(val & 0x0F);

            /* The print char */
            line[(per_row * 3) + 2 + i] = IS_PRINTABLE(val) ? val : '.';
        }

        line[(per_row * 3) + 2 + per_row] = '\0';

        DBG_E(DJOS_GENERAL, ("[%4u]: %s\n", row * per_row, line));
    }

#undef TO_CHAR
#undef IS_PRINTABLE
}

#endif

const char *j_code2str(int32_t code, j_code2str_t codes_list[])
{
    j_code2str_t *cs;

    for (cs = &codes_list[0];
        cs->code != J_CODE2STR_LAST && cs->code != code;
        cs++)
        ;

    return cs->str;
}

void j_bit2str_dump(uint32_t bitmap, j_bit2str_t bit_list[])
{
    j_bit2str_t *curr_bit;
    BOOL printed = 0;

    jprintf("(");

    for (curr_bit = bit_list;  curr_bit->bit != J_UNDEFINED_MASK; curr_bit++)
    {
        if  (bitmap & curr_bit->bit)
        {
            if(printed)
                jprintf(" | ");

            jprintf("%s", curr_bit->string);
            printed = 1;
        }
    }
    jprintf(")");
}

/* TODO: consider switching this and other bit manipulation functions with
 * compiler builtins where available */
uint32_t uw_count_set_bits(uint32_t value)
{
    static CONST uint8_t bits_set_table[] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    };

    uint32_t count = bits_set_table[value & 0xff] +
        bits_set_table[(value >> 8) & 0xff] +
        bits_set_table[(value >> 16) & 0xff] +
        bits_set_table[value >> 24];

    return count;
}
