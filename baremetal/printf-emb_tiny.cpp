/******************************************************************************

    (c) Copyright 2018 ee-quipment.com

    printf-emb_tiny.c - printf type functions for an embedded environment.

    int  snprintf(char *s, size_t n, const char *fmt, ...)
    int  vsnprintf(char *s, size_t n, const char *fmt, va_list ap)

    What IS supported:
        All integer conversion specifiers: d, i, o, u, x, and X.
        The unsigned char specifier: c
        The pointer specifier: p.
        The string specifier: s.
        The % escape specifier: %.
        All length modifers, although only ll has any effect.
        All flag modifiers.

    What is NOT supported:
        Floating point conversion specifiers: e, E, f, F, g, G, a, A.
        The number-of-characters specifier: n.
        Wide characters.
        Any other specifier not explicitly supported.

    No division operations (including modulo, etc) are performed.
    No c libraries are used.

    A NULL output string pointer s is allowed.

    The implementation is targeted at a 32-bit or more architectures that
    promote all variadic parameters to 32 bit ints except pointers which
    may be either 32 or 64 bits depending upon the architecture. ARM and
    IA-64 have been tested.

 *****************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>


#define MAX_OCTAL_STR_LEN         (22)                                  // 22 is the max 64 bit octal string length
#define ARG_STR_MAX_PREFIX_LEN    (2)                                   // "0", "0x", "0X", "+", "-", or " "
#define TOTAL_ARG_STR_LEN (MAX_OCTAL_STR_LEN+ARG_STR_MAX_PREFIX_LEN+1)  // longest string + ARG_STR_MAX_PREFIX_LEN + 1 for NUL

#define FLAG_MINUS        (1u << 1) // *
#define FLAG_PLUS         (1u << 2) // *
#define FLAG_SPACE        (1u << 3) // * format flags
#define FLAG_HASH         (1u << 4) // *
#define FLAG_ZERO         (1u << 5) // *
#define FLAG_PRECISION    (1u << 6) // precision defined in format string
#define FLAG_ARG_64       (1u << 7) // argument is 64 bits
#define FLAG_NEG          (1u << 8) // argument is < 0



/*
 * Typedef
 */
typedef struct safe_str_t {
    char   *front;  // pointer to front of string
    char   *s;      // pointer to current insertion point
    int     w;      // number of chars attemped to be written to s. May NOT be the length of s.
    size_t  n;      // unused chars left in string
} safe_str_t;

typedef union arg_t {
    struct { int32_t  lo, hi;
    }         as_lo_hi_32;
    struct { uint32_t lo, hi;
    }         as_lo_hi_32u;
    int64_t   as_64;
    uint64_t  as_64u;
    int32_t   as_32;
    uint32_t  as_32u;
    void    * as_ptr;
} arg_t;

static char const  flag_symbols[]       = "-+ #0";
static char const  length_symbols[]     = "hljztL";
static char const  pointer_specifier[]  = "sp";
static char const  decimal_specifier[]  = "diu";


/*
 * Private functions
 */
static void     _initSafeStr(safe_str_t *ss, char *s, size_t n);
static void     _appendC(safe_str_t *ss, const char c);
static void     _putC(safe_str_t *ss, const char c, bool front);
static void     _putS(safe_str_t *ss, const char *s, int n);
static void     _pad(safe_str_t *ss, const char c, int n);
static int      _prefix(safe_str_t *ss, uint32_t flag, char fmt);
static void     _binary_conv(uint64_t arg, safe_str_t *ss, char fmt);
static void     _udecimal_conv(uint64_t arg, safe_str_t *ss);
static int      _atoi(const char ** fmt);
static int      _char_in_str(char c, const char *s);
static void     _inc_fmt(const char ** fmt);



/******************************************************************************
 *
 * Write at most n bytes (including the terminating '0' to s. Return the
 * number of characters printed, excluding the terminating nul. The
 * return value will be the same regardless of the value of n.
 *
 * Values of 0 for n and NULL for s are permitted. No output will be generated
 * but a correct value will be returned.
 *
 *****************************************************************************/

extern int  vsnprintf(char *s, size_t n, const char *fmt, va_list ap);

int  emb_snprintf(char *s, size_t n, const char *fmt, ...) {
    int     n_chars;
    va_list ap;

    // input parameters are checked for validity in vsnprint

    va_start(ap, fmt);
    n_chars = vsnprintf(s, n, fmt, ap);
    va_end(ap);
    return (n_chars);
}



/******************************************************************************
 *
 * vsnprintf() is equivalent to snprintf(), except that it is called with
 * a va_list instead of a variable number of arguments.
 *
 * This function does not call the va_end macro. Because it invokes the
 * va_arg macro, the value of ap is undefined after the call.
 *
 *****************************************************************************/
int  vsnprintf(char *s, size_t n, const char *fmt, va_list ap) {
    safe_str_t  out_str;
    safe_str_t  arg_str;
    char        arg_string[TOTAL_ARG_STR_LEN];
    arg_t       arg;
    uint32_t    flag;
    int         width;
    int         precision;
    int         i;


    if (!fmt) { return (0); }   // ignore obvious badness

    _initSafeStr(&out_str, s, (s) ? n : 0);   // if string is null don't allow chars to be written

    while (*fmt) {
        if (*fmt != '%') {  _appendC(&out_str, *fmt); } // copy all non-specifer chars directly to output
        else {
            _inc_fmt(&fmt);  // step over format specifier flag
            _initSafeStr(&arg_str, arg_string, TOTAL_ARG_STR_LEN);  // reset arg_str

            // set flags
            flag = 0;
            while ((i = _char_in_str(*fmt, flag_symbols))) {
                flag |= (1u << i);
                _inc_fmt(&fmt);
            }

            // set width
            width = _atoi(&fmt);

            // set precision
            precision = 1;   // zero means don't print zeros
            if (*fmt == '.') {
                if (flag & FLAG_ZERO) { flag &= ~FLAG_ZERO; }  // the 0 flag is ignored if precision is specified
                _inc_fmt(&fmt);
                precision = _atoi(&fmt);
                flag |= FLAG_PRECISION;   // a precision has been defined
                if (precision > MAX_OCTAL_STR_LEN) { precision = MAX_OCTAL_STR_LEN; } // cap the length of the argument string
            }

            // set length. All parameters are promoted to int except ll
            while (_char_in_str(*fmt, length_symbols)) {
                if (*(fmt+1) == 'l') {    // "ll" is the only valid 2 char symbol
                    flag |= FLAG_ARG_64;
                }
                _inc_fmt(&fmt);
            }

            // determine specifier and create output string.
            if (*fmt == '%') { _appendC(&arg_str, '%'); }
            else {

                if (_char_in_str(*fmt, pointer_specifier)) {
                    if (sizeof(void *) == 8) { flag |= FLAG_ARG_64; }
                }

                if (flag & FLAG_ARG_64) { arg.as_64 = (int64_t) va_arg(ap, long long); }
                else                    { arg.as_64 = (int64_t) va_arg(ap, int); }

                if (*fmt == 'c') {
                    _appendC(&arg_str, (const char) arg.as_32);
                }
                else if (*fmt == 's') {
                    // handled below when the converted argument is copied to the output string.
                }
                else if (_char_in_str(*fmt, decimal_specifier)) {
                    if ((*fmt != 'u') && (arg.as_64 < 0)) {       // convert signed argument to unsigned + sign
                        arg.as_64u = (uint64_t) (-arg.as_64);
                        flag |= FLAG_NEG;
                    }
                    _udecimal_conv(arg.as_64u, &arg_str);
                }
                else {    // non-decimal specifier "oxXp"
                    if (arg.as_64 == 0) { flag &= ~FLAG_HASH; }   // don't prefix zero
                    _binary_conv(arg.as_64u, &arg_str, *fmt);
                }

            }

            /*
             * argument string created, apply padding and prefixes and signs
             */

            // if arg is zero and precision is zero don't write out any value
            if ((precision == 0) && (*(arg_str.front) == '0')) {
                _initSafeStr(&arg_str, arg_string, TOTAL_ARG_STR_LEN);  // reset arg_str
            }

            // left pad current argument length with 0 until precision reached
            while (arg_str.w < precision)  { _putC(&arg_str, '0', true); }

            // terminate the argument string
            *(arg_str.s) = '\0';

            // if string argument, alias the argument string to it
            if (*fmt == 's') {
                arg_str.front = static_cast<char*>(arg.as_ptr);   // arg is a string not a converted value
                while (arg_str.front[arg_str.w]) { ++arg_str.w; }         // compute strlen
                if ((flag & FLAG_PRECISION) && (precision < arg_str.w)) { // precision sets max string length
                    arg_str.w = precision;
                }
            }

            /*
             * Append formatted specifier to output string
             *
             * When right justifying, if '0' flag is set and precision
             * has NOT been specified then add prefixes and left pad
             * with '0'. Otherwise pad with spaces and then add prefixes.
             *
             * When left justifying, add prefixes then pad with spaces.
             *
             */

            // determine padding, if any
            width = width - arg_str.w;          // number of spaces to pad
            width -= _prefix(NULL, flag, *fmt); // adjust width for number of prefix characters, width < 0 okay

            if (flag & FLAG_MINUS) {      // left justify
                (void) _prefix(&out_str, flag, *fmt);
            }
            else {                        // right justify
                if (flag & FLAG_ZERO)  {  // if precision was specified then FLAG_ZERO has been cleared
                    (void) _prefix(&out_str, flag, *fmt);
                    _pad(&out_str, '0', width);
                }
                else {
                    _pad(&out_str, ' ', width);
                    (void) _prefix(&out_str, flag, *fmt);
                }
                width = 0;    // so there won't be any right padding below
            }

            // copy argument string to output
            _putS(&out_str, arg_str.front, arg_str.w);

            // pad on the right
            _pad(&out_str, ' ', width);
        }

        _inc_fmt(&fmt);   // search for next format specifier
    }

    if (s && n) {  *(out_str.s) = '\0'; }  // terminate output string if s non-NULL and is at least one char long
    return (out_str.w);                    // number of chars written to out_str excluding the nul, may exceed the string length
}


/******************************************************************************
 *
 * static void _initSafeStr(safe_str_t *ss, char *s, size_t n)
 *
 * Initialize the safe_str_t fields. n is the total number of bytes in string s.
 *
 *****************************************************************************/
static void _initSafeStr(safe_str_t *ss, char *s, size_t n) {
    ss->front = s;
    ss->s     = s;
    ss->w     = 0;
    ss->n     = (n == 0) ? 0 : n - 1;  // leave space for terminating NUL

}


/******************************************************************************
 *
 * void _putC(safe_str_t *ss, const char c, bool front)
 *
 * If front == false:
 * Insert c at *ss. Increment ss. Decrement string length remaining.
 * Do nothing if ss->s is full.
 *
 * If front == true:
 * Shift the string count characters to the right one position, adjusting
 * string length and put c in the front of the string. Do nothing if string is full.
 *
 *****************************************************************************/
static void _putC(safe_str_t *ss, const char c, bool front) {
    char * p;

    if (!ss) { return; }                // may be null when called from _prefix
    if (ss->n) {
        --(ss->n);                      // chars remaining now in string
        p = ss->s - 1;                  // last character in string
        *(ss->s++) = c;                 // speculatively append char, move next avail char pointer

        if (front) {
            while (p >= ss->front) {    // shift all the way to first character
                *(p + 1) = *(p);
                --p;
            }
            *(ss->front) = c;
        }
    }
    ++(ss->w);    // increment number of attempted chars written
}


/******************************************************************************
 *
 * void _appendC(safe_str_t *ss, const char c)
 *
 * Append c to ss. Increment ss. Decrement string length remaining.
 * Do nothing if ss->s is full.
 *
 *****************************************************************************/
static void _appendC(safe_str_t *ss, const char c) {
    _putC(ss, c, false);
}


/******************************************************************************
 *
 * void _putS(safe_str_t *ss, const char *s, int n)
 *
 * Append n characters from s to ss->s or until end of s is reached.
 * Adjust ss string length remaining. Truncate s if ss->s is full.
 *
 *****************************************************************************/
static void _putS(safe_str_t *ss, const char *s, int n) {
    while (*s && n) {
        _appendC(ss, *s);
        ++s;
        --n;
    }
}


/******************************************************************************
 *
 * void _pad(safe_str_t *ss, const char c, int n)
 *
 * Append n characters c to ss->s. Stop when string if string is full.
 * n may be less than 0, in which case no padding occurs.
 *
 *****************************************************************************/
static void _pad(safe_str_t *ss, const char c, int n) {
    while (n-- > 0) { _appendC(ss, c); }
}


/******************************************************************************
 *
 * int _prefix(safe_str_t *ss, uint32_t flag, char fmt)
 *
 * Output string prefix characters to ss->s. ss may be NULL.
 * Return number of characters written.
 *
 *****************************************************************************/
static int _prefix(safe_str_t *ss, uint32_t flag, char fmt) {
    int n = 0;

    // add octal and hex prefixes, including pointers even if FLAG_HASH not set
    if ((fmt == 'p') || (flag & FLAG_HASH)) {
        _appendC(ss, '0'); ++n;
        if (fmt == 'X')       { _appendC(ss, 'X'); ++n; }
        else if (fmt != 'o')  { _appendC(ss, 'x'); ++n; }
    }

    // add sign or space
    ++n;            // speculatively assume a character
    if (flag & FLAG_NEG)        { _appendC(ss, '-'); }
    else if (flag & FLAG_PLUS)  { _appendC(ss, '+'); }
    else if (flag & FLAG_SPACE) { _appendC(ss, ' '); }
    else { --n; }   // fix up wrong assumption

    return (n);
}



/******************************************************************************
 *
 * void _inc_fmt(const char ** fmt)
 *
 * Increment the format string pointer unless points to NUL.
 *
 *****************************************************************************/
static void _inc_fmt(const char ** fmt) {
    if (**fmt) { ++(*fmt); }
}


/******************************************************************************
 *
 * int _atoi(const char ** fmt)
 *
 * Convert a sequence of ascii numbers pointed to by *fmt to an unsigned integer.
 *
 *****************************************************************************/
static int _atoi(const char ** fmt) {
    int val = 0;

    while ((**fmt >= '0') && (**fmt <= '9')) {
        val = (val * 10) + (**fmt - '0');
        _inc_fmt(fmt);
    }
    return (val);
}


/******************************************************************************
 *
 * void _binary_conv(uint64_t arg, safe_str_t *ss, char fmt)
 *
 * Convert a unsigned uint64_t to a octal or hex string, or an 8 digit bcd
 * value to a decimal string. Leading zeros are suppressed unless fmt == 'z'
 * or unless the argument is zero in which case one zero is converted.
 *
 *****************************************************************************/
static void _binary_conv(uint64_t arg, safe_str_t *ss, char fmt) {
    bool      ff1;
    int       i, start, n_shift;
    uint32_t  digit, mask;

    if (fmt == 'o') {     // octal
        start   = 63;
        n_shift = 3;
        mask    = 0x07;
    }
    else {                // hex or decimal
        start   = 60;
        n_shift = 4;
        mask    = 0x0f;
    }

    ff1 = (fmt == 'z') ? true : false;  // find first one flag for leading zero suppression
    for (i=start; i>=0; i-=n_shift) {   // convert each nibble or octet to a hex ascii character
        digit = (arg >> i) & mask;
        if (digit) { ff1 = true; }      // at least one significant digit now
        if (digit > 9) {
            if (fmt == 'X') { digit += (('A' - '0') - 10); }
            else            { digit += (('a' - '0') - 10); }
        }
        if (ff1) { _appendC(ss, (const char) (digit + '0')); }
    }
    if (!ff1) { _appendC(ss, '0'); }       // arg was zero
}

/******************************************************************************
 *
 * void _udecimal_conv(uint64_t arg, safe_str_t *ss)
 *
 * Convert unsigned binary 64 bit arg to a string.
 *
 *****************************************************************************/
static void _udecimal_conv(uint64_t arg, safe_str_t *ss) {
    union {
        struct { uint64_t arg,            bcd_lo,     bcd_hi; };
        struct { uint32_t arg_lo, arg_hi, bcd0, bcd1, bcd2, bcd_pad; };
        uint32_t as_array[6];
    } bcd;
    uint32_t mask;

    for (int i=0; i<6; ++i) { // clear result registers
        bcd.as_array[i] = 0;
    }
    bcd.arg     = arg;


    /*
     * Double Dabble binary to bcd conversion algorithm
     * The dabble part of the algorithm of adding 3 if the nibble is greater than
     * four is implemented by speculatively adding 3 to all nibbles, then subtracting
     * 3 if the result is less than 8.
     */
    for (int bit=64; bit>0; --bit) {
        for (int i=4; i>=0; --i) {
            // dabble only the bcd registers
            if (i > 1) {
                bcd.as_array[i] += 0x33333333;                  // add 3 to each nibble
                mask = bcd.as_array[i] & 0x88888888;            // nibble msb set if result >= 8
                mask ^= 0x88888888;                             // nibble msb set is result < 8
                bcd.as_array[i] -= ((mask >> 2) + (mask >> 3)); // sub 3
            }
            // double everything except bcd_pad
            bcd.as_array[i+1] += ((uint32_t) (((int32_t) bcd.as_array[i]) < 0)); // test for msb set
            bcd.as_array[i] <<= 1;
        }
    }

    if (bcd.bcd_hi != 0) {
        _binary_conv(bcd.bcd_hi, ss, 'd');  // suppress leading zeros
        _binary_conv(bcd.bcd_lo, ss, 'z');  // don't suppress leading zeros
    }
    else {
        _binary_conv(bcd.bcd_lo, ss, 'd');  // suppress leading zeros
    }
}


/******************************************************************************
 *
 * int _char_in_str(char c, const char *s)
 *
 * Return location index plus one in s if c is in the string s, else return 0.
 *
 *****************************************************************************/
static int _char_in_str(char c, const char *s) {
    int n = 0;

    while (s[n]) {
        if (c == s[n]) { return (n+1); }
        ++n;
    }
    return (0);
}


#ifdef UNIT_TEST

#include <string.h>

#define TEST(cond)  failed |= !(cond)

int printf_UNIT_TEST (void) {
  char buf[128];
  volatile bool failed = false;   // keep from getting optimized out

  /* truncation and return value */
  TEST (snprintf (buf, 0, "abc") == 3);
  TEST (snprintf (NULL, 0, "abc") == 3);
  TEST (snprintf (buf, 5, "")    == 0);
  TEST (snprintf (buf, 5, "abc") == 3);
  TEST (snprintf (buf, 1, "abc") == 3 && buf[0] == '\0' && !strcmp (buf, ""));
  TEST (snprintf (buf, 2, "abc") == 3 && buf[1] == '\0' && !strcmp (buf, "a"));
  TEST (snprintf (buf, 3, "abc") == 3 && buf[2] == '\0' && !strcmp (buf, "ab"));
  TEST (snprintf (buf, 4, "abc") == 3 && buf[3] == '\0' && !strcmp (buf, "abc"));
  TEST (snprintf (buf, 5, "abc") == 3 && buf[3] == '\0' && !strcmp (buf, "abc"));

  /* %d, basic formatting */
  TEST (snprintf (buf, 128, "%d", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%d", 0) == 1 && !strcmp (buf, "0"));
  TEST (snprintf (buf, 128, "%.0d", 0) == 0 && !strcmp (buf, ""));
  TEST (snprintf (buf, 128, "%.0d", 1) == 1 && !strcmp (buf, "1"));
  TEST (snprintf (buf, 128, "%.d", 2) == 1 && !strcmp (buf, "2"));
  TEST (snprintf (buf, 128, "%d", -1) == 2 && !strcmp (buf, "-1"));
  TEST (snprintf (buf, 128, "%.3d", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%.3d", -5) == 4 && !strcmp (buf, "-005"));
  TEST (snprintf (buf, 128, "%5.0d", 0) == 5 && !strcmp (buf, "     "));
  TEST (snprintf (buf, 128, "%5.3d", 5) == 5 && !strcmp (buf, "  005"));
  TEST (snprintf (buf, 128, "%5.3d", -5) == 5 && !strcmp (buf, " -005"));
  TEST (snprintf (buf, 128, "%05.3d", -5) == 5 && !strcmp (buf, " -005"));
  TEST (snprintf (buf, 128, "%-5.3d", -5) == 5 && !strcmp (buf, "-005 "));
  //TEST (snprintf (buf, 128, "%-5.-3d", -5) == 5 && !strcmp (buf, "-5   ")); // negative precision, not a valid format

  /* %d, length modifiers */
  TEST (snprintf (buf, 128, "%hhd", (signed char)        -5) == 2 && !strcmp (buf, "-5"));
  TEST (snprintf (buf, 128, "%hhd", (unsigned char)       5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%hd",  (short)              -5) == 2 && !strcmp (buf, "-5"));
  TEST (snprintf (buf, 128, "%hd",  (unsigned short)      5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%ld",  (long)               -5) == 2 && !strcmp (buf, "-5"));
  TEST (snprintf (buf, 128, "%ld",  (unsigned long)       5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%zd",  (size_t)              5) == 1 && !strcmp (buf, "5"));

  /* %d, flags */
  TEST (snprintf (buf, 128, "%-d", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%-+d", 5) == 2 && !strcmp (buf, "+5"));
  TEST (snprintf (buf, 128, "%+-d", 5) == 2 && !strcmp (buf, "+5"));
  TEST (snprintf (buf, 128, "%+d", -5) == 2 && !strcmp (buf, "-5"));
  TEST (snprintf (buf, 128, "% d", 5) == 2 && !strcmp (buf, " 5"));
  TEST (snprintf (buf, 128, "% .0d", 0) == 1 && !strcmp (buf, " "));
  TEST (snprintf (buf, 128, "% +d", 5) == 2 && !strcmp (buf, "+5"));
  TEST (snprintf (buf, 128, "%03d", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%-03d", -5) == 3 && !strcmp (buf, "-5 "));
  TEST (snprintf (buf, 128, "%3d", -5) == 3 && !strcmp (buf, " -5"));
  TEST (snprintf (buf, 128, "%03d", -5) == 3 && !strcmp (buf, "-05"));

  /* %o, basic formatting */
  TEST (snprintf (buf, 128, "%o", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%o", 8) == 2 && !strcmp (buf, "10"));
  TEST (snprintf (buf, 128, "%o", 0) == 1 && !strcmp (buf, "0"));
  TEST (snprintf (buf, 128, "%#o", 5) == 2 && !strcmp (buf, "05"));
  TEST (snprintf (buf, 128, "%#o", 05) == 2 && !strcmp (buf, "05"));
  TEST (snprintf (buf, 128, "%.0o", 0) == 0 && !strcmp (buf, ""));
  TEST (snprintf (buf, 128, "%.0o", 1) == 1 && !strcmp (buf, "1"));
  TEST (snprintf (buf, 128, "%.3o", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%.3o", 8) == 3 && !strcmp (buf, "010"));
  TEST (snprintf (buf, 128, "%5.3o", 5) == 5 && !strcmp (buf, "  005"));

  /* %u, basic formatting */
  TEST (snprintf (buf, 128, "%u", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%u", 0) == 1 && !strcmp (buf, "0"));
  TEST (snprintf (buf, 128, "%.0u", 0) == 0 && !strcmp (buf, ""));
  TEST (snprintf (buf, 128, "%5.0u", 0) == 5 && !strcmp (buf, "     "));
  TEST (snprintf (buf, 128, "%.0u", 1) == 1 && !strcmp (buf, "1"));
  TEST (snprintf (buf, 128, "%.3u", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%5.3u", 5) == 5 && !strcmp (buf, "  005"));

  /* %x, basic formatting */
  TEST (snprintf (buf, 128, "%x", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%x", 31) == 2 && !strcmp (buf, "1f"));
  TEST (snprintf (buf, 128, "%x", 0) == 1 && !strcmp (buf, "0"));
  TEST (snprintf (buf, 128, "%.0x", 0) == 0 && !strcmp (buf, ""));
  TEST (snprintf (buf, 128, "%5.0x", 0) == 5 && !strcmp (buf, "     "));
  TEST (snprintf (buf, 128, "%.0x", 1) == 1 && !strcmp (buf, "1"));
  TEST (snprintf (buf, 128, "%.3x", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%.3x", 31) == 3 && !strcmp (buf, "01f"));
  TEST (snprintf (buf, 128, "%5.3x", 5) == 5 && !strcmp (buf, "  005"));

  /* %x, flags */
  TEST (snprintf (buf, 128, "%-x", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%-3x", 5) == 3 && !strcmp (buf, "5  "));
  TEST (snprintf (buf, 128, "%03x", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%#x", 31) == 4 && !strcmp (buf, "0x1f"));
  TEST (snprintf (buf, 128, "%#x", 0) == 1 && !strcmp (buf, "0"));

  /* %X, basic formatting */
  TEST (snprintf (buf, 128, "%X", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%X", 31) == 2 && !strcmp (buf, "1F"));
  TEST (snprintf (buf, 128, "%X", 0) == 1 && !strcmp (buf, "0"));
  TEST (snprintf (buf, 128, "%.0X", 0) == 0 && !strcmp (buf, ""));
  TEST (snprintf (buf, 128, "%5.0X", 0) == 5 && !strcmp (buf, "     "));
  TEST (snprintf (buf, 128, "%.0X", 1) == 1 && !strcmp (buf, "1"));
  TEST (snprintf (buf, 128, "%.3X", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%.3X", 31) == 3 && !strcmp (buf, "01F"));
  TEST (snprintf (buf, 128, "%5.3X", 5) == 5 && !strcmp (buf, "  005"));

  /* %X, flags */
  TEST (snprintf (buf, 128, "%-X", 5) == 1 && !strcmp (buf, "5"));
  TEST (snprintf (buf, 128, "%03X", 5) == 3 && !strcmp (buf, "005"));
  TEST (snprintf (buf, 128, "%#X", 31) == 4 && !strcmp (buf, "0X1F"));
  TEST (snprintf (buf, 128, "%#X", 0) == 1 && !strcmp (buf, "0"));

  /* %p, basic formatting */
  TEST (snprintf (buf, 128, "%p", 1234) == 5 && !strcmp (buf, "0x4d2"));
  TEST (snprintf (buf, 128, "%.8p", 1234) == 10 && !strcmp (buf, "0x000004d2"));
  TEST (snprintf (buf, 128, "%p", 0x20001ffc) == 10 && !strcmp (buf, "0x20001ffc"));
  TEST (snprintf (buf, 128, "%.8p", 0) == 10 && !strcmp (buf, "0x00000000"));
  TEST (snprintf (buf, 128, "%.6p", -1) == 10 && !strcmp (buf, "0xffffffff"));

  /* %c */
  TEST (snprintf (buf, 128, "%c", 'a') == 1 && !strcmp (buf, "a"));

  /* %s */
  TEST (snprintf (buf, 128, "%.2s", "abc") == 2 && !strcmp (buf, "ab"));
  TEST (snprintf (buf, 128, "%.6s", "abc") == 3 && !strcmp (buf, "abc"));
  TEST (snprintf (buf, 128, "%5s", "abc") == 5 && !strcmp (buf, "  abc"));
  TEST (snprintf (buf, 128, "%-5s", "abc") == 5 && !strcmp (buf, "abc  "));
  TEST (snprintf (buf, 128, "%5.2s", "abc") == 5 && !strcmp (buf, "   ab"));
  //TEST (snprintf (buf, 128, "%*s", 5, "abc") == 5 && !strcmp (buf, "  abc"));   // not supported
  //TEST (snprintf (buf, 128, "%*s", -5, "abc") == 5 && !strcmp (buf, "abc  "));  // not supported
  //TEST (snprintf (buf, 128, "%*.*s", 5, 2, "abc") == 5 && !strcmp (buf, "   ab"));  // not supported
  TEST (snprintf (buf, 128-5, "%s + cat_text", buf) == 16 && !strcmp (buf, "   ab + cat_text")); // this is allowed in contravention to the spec
  //TEST (snprintf (buf, 128, "%*.*s", 5, 2, "abc") == 5 && !strcmp (buf, "   ab"));  // not supported
  //TEST (snprintf (buf, 128-5, "cat_text + %s", buf) == 16 && !strcmp (buf, "cat_text +    ab")); // not supported

  /* %% and unrecognized conversion specifier*/
  TEST (snprintf (buf, 128, "%%") == 1 && !strcmp (buf, "%"));
 // TEST (snprintf (buf, 128, "%?") == 1 && !strcmp (buf, "%"));  // not supported

  return (failed);
}

#endif /* UNIT_TEST */

