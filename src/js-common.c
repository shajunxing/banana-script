/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <float.h>
#include <math.h>
#include "js-common.h"

void print_hex(void *base, size_t length) {
#define _printable(__arg_ch) (__arg_ch >= 0x20 && __arg_ch <= 0x7e ? __arg_ch : ' ')
    static const size_t _segs_per_line = 2;
    static const size_t _bytes_per_seg = 8;
    size_t bytes_per_line = _segs_per_line * _bytes_per_seg;
    unsigned char *p = (unsigned char *)base;
    for (size_t loffs = 0; loffs <= length; loffs += bytes_per_line) { // <= make sure when length=0 also print a line
        printf("%8zu:  ", loffs);
        for (int what = 0; what <= 1; what++) { // 0 hex, 1 ascii
            for (size_t seg = 0; seg < _segs_per_line; seg++) {
                for (size_t boffs = 0; boffs < _bytes_per_seg; boffs++) {
                    size_t offs = loffs + seg * _bytes_per_seg + boffs;
                    if (offs < length) {
                        // const unsigned char *fmt = what == 0 ? "%02x " : "%c";
                        // unsigned char ch = what == 0 ? p[offs] : _printable(p[offs]);
                        // printf(fmt, ch);
                        if (what == 0) {
                            unsigned char ch = p[offs];
                            if (ch == 0) {
                                printf(".. ");
                            } else {
                                printf("%02x ", ch);
                            }
                        } else {
                            printf("%c", _printable(p[offs]));
                        }
                    } else {
                        printf(what == 0 ? "   " : " ");
                    }
                }
                printf(what == 0 ? " " : "");
            }
            printf(" ");
        }
        printf("\n");
    }
#undef _printable
}

// strncpy will cause gcc warning: 'strncpy' output truncated before terminating nul copying as many bytes from a string as its length
// use https://stackoverflow.com/questions/46013382/c-strndup-implicit-declaration instead
char *dupe_s(const char *src, size_t maxlen) {
    size_t len;
    for (len = 0; len < maxlen && src[len] != '\0'; len++)
        ;
    // log_expression("%zu", len);
    char *dst = calloc(len + 1, 1);
    if (dst != NULL) {
        memcpy(dst, src, len);
    }
    return dst;
}

char *dupe_sz(const char *src) {
    size_t len = strlen(src);
    // log_expression("%zu", len);
    char *dst = calloc(len + 1, 1);
    if (dst != NULL) {
        memcpy(dst, src, len);
    }
    return dst;
}

bool starts_with_sz(const char *str, const char *prefix) {
    size_t len = strlen(str);
    size_t prefixlen = strlen(prefix);
    return prefixlen <= len && strncmp(str, prefix, prefixlen) == 0;
}

bool ends_with_sz(const char *str, const char *suffix) {
    size_t len = strlen(str);
    size_t suffixlen = strlen(suffix);
    return suffixlen <= len && strncmp(str + len - suffixlen, suffix, suffixlen) == 0;
}

char *join_sz_internal(const char *sep, size_t nargs, ...) {
    size_t i, len;
    char *ret;
    va_list args;
    va_start(args, nargs);
    for (len = 0, i = 0; i < nargs; i++) {
        len += strlen(va_arg(args, char *));
    }
    va_end(args);
    len += (strlen(sep) * (nargs - 1));
    ret = (char *)calloc(len + 1, 1);
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        if (i > 0) {
            strcat(ret, sep);
        }
        strcat(ret, va_arg(args, char *));
    }
    va_end(args);
    return ret;
}

int natural_compare_sz(const char *x, const char *y) {
    // string natural comparation "alphanum" algorithm
    // https://stackoverflow.com/questions/1601834/c-implementation-of-or-alternative-to-strcmplogicalw-in-shlwapi-dll
    // https://docs.microsoft.com/en-us/previous-versions/technet-magazine/hh475812(v=msdn.10)?redirectedfrom=MSDN
    // https://web.archive.org/web/20210207124255/davekoelle.com/alphanum.html
    if (x == NULL && y == NULL)
        return 0;
    if (x == NULL)
        return -1;
    if (y == NULL)
        return 1;
    size_t lx = strlen(x), ly = strlen(y);
    for (size_t mx = 0, my = 0; mx < lx && my < ly; mx++, my++) {
        if (isdigit(x[mx]) && isdigit(y[my])) {
            long vx = 0, vy = 0;
            for (; mx < lx && isdigit(x[mx]); mx++)
                vx = vx * 10 + x[mx] - '0';
            for (; my < ly && isdigit(y[my]); my++)
                vy = vy * 10 + y[my] - '0';
            if (vx != vy)
                return vx > vy ? 1 : -1;
        }
        if (mx < lx && my < ly && x[mx] != y[my])
            return x[mx] > y[my] ? 1 : -1;
    }
    return lx > ly ? 1 : -1;
}

// "error C2099: initializer is not a constant" because stdout cannot be determined at compile time
// struct print_stream stdout_stream = {.type = file_stream, .fp = stdout};

void free_stream(struct print_stream *out) {
    if (out->type == string_stream) {
        buffer_free(out->base, out->length, out->capacity);
    }
}

// printf family hardly returns negative, and no errno generated, so if negative, fatal
// even if wrong fp or NULL, just no output or segment fault
void vprintf_to_stream(struct print_stream *out, const char *format, va_list args) {
    va_list args_copy;
    int result;
    if (out->type == file_stream) {
        result = vfprintf(out->fp, format, args);
        if (result < 0) {
            fatal("vfprintf failed");
        }
    } else {
        va_copy(args_copy, args);
        result = vsnprintf(NULL, 0, format, args_copy);
        va_end(args_copy);
        if (result < 0) {
            fatal("vsnprintf failed");
        }
        size_t new_length = out->length + (size_t)result;
        buffer_alloc(out->base, out->length, out->capacity, new_length + 1);
        result = vsnprintf(out->base + out->length, (size_t)result + 1, format, args);
        if (result < 0) {
            fatal("vsnprintf failed");
        }
        out->length = new_length;
    }
}

void printf_to_stream(struct print_stream *out, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf_to_stream(out, format, args);
    va_end(args);
}

void putc_to_stream(struct print_stream *out, char c) {
    if (out->type == file_stream) {
        fputc(c, out->fp);
    } else {
        string_buffer_append_ch(out->base, out->length, out->capacity, c);
    }
}

void puts_to_stream(struct print_stream *out, const char *s, size_t len) {
    if (out->type == file_stream) {
        fprintf(out->fp, "%.*s", (int)len, s);
    } else {
        string_buffer_append(out->base, out->length, out->capacity, s, len);
    }
}

void putsz_to_stream(struct print_stream *out, const char *s) {
    if (out->type == file_stream) {
        fputs(s, out->fp);
    } else {
        string_buffer_append_sz(out->base, out->length, out->capacity, s);
    }
}

void escape_to_stream(struct print_stream *out, const char *base, size_t length) {
    for (size_t i = 0; i < length; i++) {
        char c = base[i];
        char *s = NULL;
        switch (c) { // https://en.cppreference.com/w/c/language/escape.html
        case '\'':
            s = "\\'";
            break;
        case '\"':
            s = "\\\"";
            break;
        case '\?':
            s = "\\?";
            break;
        case '\\':
            s = "\\\\";
            break;
        case '\a':
            s = "\\a";
            break;
        case '\b':
            s = "\\b";
            break;
        case '\f':
            s = "\\f";
            break;
        case '\n':
            s = "\\n";
            break;
        case '\r':
            s = "\\r";
            break;
        case '\t':
            s = "\\t";
            break;
        case '\v':
            s = "\\v";
            break;
        default:
            break;
        }
        s ? putsz_to_stream(out, s) : putc_to_stream(out, c);
    }
}

/*
Less than 200 lines of C code implements regular expression matching with capture, escape, character class, and quantifier functions

I like short, lean, clear and easy-to-read code, and I learned from classic example <https://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html> of master Rob Pike. Master's code looks elegant, but it has a huge problem: recursive layers are too deep, and risk of stack burst is high. And my implementation idea is:

1. First of all, pay tribute to master's code, construct a basic `match` and `match_here` two-layer structure, which can correctly parse `^` `$`;
2. Then use tree structure to achieve arbitrary depth nested `()`;
3. Then implement pattern matching of a single character, abstract `match_char` function, in which `\d` `\s` `\w` `[]`;
4. Finally, add quantifier `*` `+` `?`.

Of course, traditional regular libraries are generally precompiled to AST because they support many functions, such as need for backscanning, etc., and my simple recursive descending code framework here can be regarded as implementing most common functions, which can be expanded with more escapes, character classes, and quantifiers.
*/

static void _start_capture(char *txt, struct re_capture **cap) {
#define __last_sub() ((*cap)->subs.base + (*cap)->subs.length - 1)
    // printf("_start_capture\n");
    if (cap && *cap) {
        // un-successfully ended captures can be re-used
        if (!(*cap)->subs.base || ((*cap)->subs.base && __last_sub()->head && __last_sub()->tail)) {
            buffer_push((*cap)->subs.base, (*cap)->subs.length, (*cap)->subs.capacity,
                (struct re_capture){.super = (*cap)});
        }
        (*cap) = __last_sub();
        (*cap)->head = txt;
    }
#undef __last_sub
}

static void _end_capture(char *txt, struct re_capture **cap) {
    // printf("_end_capture\n");
    if (cap && *cap) {
        (*cap)->tail = txt;
        (*cap) = (*cap)->super;
    }
}

void re_free_capture(struct re_capture *cap) {
    buffer_for_each(cap->subs.base, cap->subs.length, cap->subs.capacity, i, c, re_free_capture(c));
    buffer_free(cap->subs.base, cap->subs.length, cap->subs.capacity);
}

// match single character, including escape pattern and class pattern
static bool _match_char(char c, char *ph, char *pt) {
    // log_debug("_match_char('%c\', '%c', '%c')", c, *ph, *pt);
    bool inv = false;
#define __check_return(__arg_expr) \
    do { \
        /* Cautions: must cast to bool or will cause error, for example: 1 ^ 8 = 9 */ \
        if (inv ^ (bool)(__arg_expr)) { \
            /* log_debug("inv=%d, expr=%d, return true", inv, (__arg_expr)); */ \
            return true; \
        } \
    } while (0)
    while (ph < pt) {
        // log_debug("while ('%c' < '%c')", *ph, *pt);
        if (*ph == '^') {
            // log_debug("if (*ph == '^')");
            inv = true;
            ph++;
        } else if (*ph == '\\') {
            // log_debug("else if (*ph == '\\')");
            ph++;
            if (*ph == 'd') {
                __check_return(isdigit(c));
            } else if (*ph == 's') {
                __check_return(isspace(c));
            } else if (*ph == 'w') {
                __check_return(isalnum(c) || c == '_');
            } else {
                // log_debug("__check_return(c == *ph)");
                __check_return(c == *ph);
            }
            ph++;
        } else if (*ph == '.') {
            // including '\r' '\n'
            __check_return(true);
            ph++;
        } else if (*(ph + 1) == '-') {
            __check_return(c >= *ph && c <= *(ph + 2));
            ph += 3;
        } else {
            __check_return(c == *ph);
            ph++;
        }
    }
    return false;
#undef __check_return
}

static bool _match_here(char *txt, char *pat, struct re_capture *cap) {
    // log_debug("_match_here(\"%s\", \"%s\")", txt, pat);
#define __set_capture_head() (cap ? (cap->head = txt) : (void)0)
#define __set_capture_tail() (cap ? (cap->tail = txt) : (void)0)
#define __start_capture() (cap ? _start_capture(txt, &cap) : (void)0)
#define __end_capture() (cap ? _end_capture(txt, &cap) : (void)0)
    __set_capture_head();
    for (;;) {
        // DON'T use "if (*pat == '(' && cap)", *pat must always be consumed
        if (*pat == '(') {
            __start_capture();
            pat++;
        } else if (*pat == ')') {
            __end_capture();
            pat++;
        } else if (*pat == '\0') {
            __set_capture_tail();
            return true;
        } else if (*pat == '$') {
            bool ret = *txt == '\0';
            ret ? __set_capture_tail() : (void)0;
            return ret;
        } else if (*txt == '\0') {
            return false;
        } else {
            char *ph, *pt;
            if (*pat == '\\') {
                ph = pat;
                pat++;
                pt = pat + 1;
            } else if (*pat == '[') {
                ph = pat + 1;
                do {
                    pat++;
                    if (*pat == '\0') {
                        return false;
                    }
                } while (*pat != ']');
                pt = pat;
            } else {
                ph = pat;
                pt = pat + 1;
            }
            pat++;
            // quantifiers
            if (*pat == '?') {
                // zero or one
                if (_match_char(*txt, ph, pt)) {
                    txt++;
                }
                pat++;
            } else {
                // '+' is combination of 'one' and 'zero or more'
                if (*pat != '*') {
                    // 'one': '+' and others
                    char c = *txt;
                    txt++;
                    if (!_match_char(c, ph, pt)) {
                        return false;
                    }
                }
                if (*pat == '*' || *pat == '+') {
                    // 'zero or more': '*' and '+'
                    while (*txt != '\0' && _match_char(*txt, ph, pt)) {
                        // log_debug("'zero or more': '*' and '+'");
                        txt++;
                    }
                    pat++;
                }
            }
        }
    }
#undef __end_capture
#undef __start_capture
#undef __set_capture_tail
#undef __set_capture_head
}

// text and pattern's current position cannot be put into context, because they will be re-entered in namy functions
bool re_match(char *text, char *pattern, struct re_capture *capture) {
    // log_debug("re_match(\"%s\", \"%s\")", text, pattern);
    if (*pattern == '^') {
        return _match_here(text, pattern + 1, capture);
    }
    for (;; text++) {
        if (_match_here(text, pattern, capture)) {
            return true;
        }
        if (*text == '\0') {
            return false;
        }
    }
}

#ifdef DEBUG

char *random_sz_static(size_t *plen) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static char ret[80];
    size_t len = (1 + rand() % (countof(ret) - 1));
    int i;
    memset(ret, 0, sizeof(ret));
    for (i = 0; i < len; i++) {
        ret[i] = charset[rand() % (countof(charset) - 1)]; // -1 means get rid of tailing '\0'
    }
    if (plen) {
        *plen = len;
    }
    return ret;
}

char *random_sz_dynamic() {
    size_t len;
    char *str = random_sz_static(&len);
    char *ret = (char *)alloc(char, len + 1);
    memcpy(ret, str, len);
    return ret;
}

void test_random_sz() {
    for (;;) {
        char *str = random_sz_dynamic();
        puts(str);
        free(str);
    }
}

double random_double() {
    return pow(DBL_MAX, (double)rand() / RAND_MAX) * (rand() % 2 == 0 ? -1 : 1);
}

void test_random_double() {
    for (;;) {
        double d = random_double();
        printf("%g\n", d);
    }
}

void test_buffer() {
    long long *p = NULL;
    size_t len = 0;
    size_t cap = 0;
    buffer_alloc(p, len, cap, 0);
    buffer_push(p, len, cap, 1);
    buffer_dump(p, len, cap);
    buffer_alloc(p, len, cap, 4);
    buffer_dump(p, len, cap);
    buffer_push(p, len, cap, 2);
    buffer_push(p, len, cap, 3);
    buffer_push(p, len, cap, 4);
    buffer_push(p, len, cap, 5);
    buffer_push(p, len, cap, 6);
    buffer_put(p, len, cap, 10, 10);
    buffer_dump(p, len, cap);
    buffer_for_each(p, len, cap, i, v, printf("%4zu: %lld\n", i, *v));
    buffer_free(p, len, cap);
}

void test_string_buffer() {
    char *str = NULL;
    size_t len = 0;
    size_t cap = 0;
    string_buffer_append(str, len, cap, NULL, 0);
    string_buffer_append_sz(str, len, cap, "hello");
    string_buffer_append(str, len, cap, NULL, 0);
    string_buffer_append_ch(str, len, cap, ',');
    string_buffer_append(str, len, cap, NULL, 0);
    string_buffer_append_sz(str, len, cap, "world");
    string_buffer_append(str, len, cap, NULL, 0);
    string_buffer_append_ch(str, len, cap, '!');
    string_buffer_append(str, len, cap, NULL, 0);
    buffer_dump(str, len, cap);
    buffer_free(str, len, cap);
}

void test_string_buffer_loop() {
    for (;;) {
        char *str = NULL;
        size_t len = 0;
        size_t cap = 0;
        int i, j;
        for (i = 0; i < rand() % 100; i++) {
            for (j = 0; j < rand() % 100; j++) {
                char ch = 33 + (rand() % 94);
                // getchar();
                string_buffer_append_ch(str, len, cap, ch);
                putchar('+');
                // js_info("string_buffer_append_ch %d %d", len, cap);
                // print_hex(str, cap);
            }
            string_buffer_clear(str, len, cap);
            putchar('-');
            // js_info("string_buffer_clear %d %d", len, cap);
            // print_hex(str, cap);
        }
        buffer_free(str, len, cap);
        putchar('x');
    }
}

// void _append_fv(char **base, size_t *length, size_t *capacity, const char *fmt, ...) {
//     va_list args;
//     va_start(args, fmt);
//     string_buffer_append_fv(*base, *length, *capacity, fmt, args);
//     va_end(args);
// }

// void test_string_buffer_append_f() {
//     char *base = NULL;
//     size_t length = 0;
//     size_t capacity = 0;
//     string_buffer_append_f(base, length, capacity, "/%g/%s", 3.14, "hello");
//     buffer_dump(base, length, capacity);
//     string_buffer_append_f(base, length, capacity, "/%g/%s", 2.72, "world");
//     buffer_dump(base, length, capacity);
//     _append_fv(&base, &length, &capacity, "/%g/%s", 3.14, "hello");
//     buffer_dump(base, length, capacity);
//     _append_fv(&base, &length, &capacity, "/%g/%s", 2.72, "world");
//     buffer_dump(base, length, capacity);
// }

void test_memmove() {
    char mem[32] = {0};
    const char *str = "Hello,World!";
    print_hex(mem, countof(mem));
    strcpy(mem + 3, str);
    print_hex(mem, countof(mem));
    memmove(mem + 6, mem + 3, strlen(str));
    print_hex(mem, countof(mem)); //  memmove does not zero origin space
}

void test_string_dupe() {
    const char *src = (const char *)test_string_dupe; // no end with 0
    char *dst = dupe_sz(src);
    puts(dst);
    free(dst);
    dst = dupe_s(src, 3);
    puts(dst);
    free(dst);
    dst = dupe_s(src, 7);
    puts(dst);
    free(dst);
    dst = dupe_s(src, 100);
    puts(dst);
    free(dst);
}

// void test_read_file() {
//     const char *filename = "examples/power_of_2_length.txt";
//     char *base = NULL;
//     size_t length = 0;
//     size_t capacity = 0;
//     read_text_file(filename, base, length, capacity);
//     puts(base);
//     buffer_dump(base, length, capacity);
//     read_text_file(filename, base, length, capacity);
//     puts(base);
//     buffer_dump(base, length, capacity);
//     buffer_free(base, length, capacity);
//     read_binary_file(filename, base, length, capacity);
//     puts(base);
//     buffer_dump(base, length, capacity);
//     read_binary_file(filename, base, length, capacity);
//     puts(base);
//     buffer_dump(base, length, capacity);
//     buffer_free(base, length, capacity);
// }

void test_read_line() {
    {
        char *base = NULL;
        size_t length = 0;
        size_t capacity = 0;
        printf(": ");
        read_line(stdin, base, length, capacity);
        buffer_dump(base, length, capacity);
    }
    {
        char *base = alloc(char, 1);
        size_t length = 0;
        size_t capacity = 1;
        printf(": ");
        read_line(stdin, base, length, capacity);
        buffer_dump(base, length, capacity);
    }
}

static int _comp(const void *lhs, const void *rhs) {
    return natural_compare_sz(*((char **)lhs), *((char **)rhs));
}

static int _rev_comp(const void *lhs, const void *rhs) {
    return _comp(rhs, lhs);
}

void test_natural_compare() {
    char *list[] = {
        "1000X Radonius Maximus",
        "10X Radonius",
        "200X Radonius",
        "20X Radonius",
        "20X Radonius Prime",
        "30X Radonius",
        "40X Radonius",
        "Allegia 50 Clasteron",
        "Allegia 500 Clasteron",
        "Allegia 50B Clasteron",
        "Allegia 51 Clasteron",
        "Allegia 6R Clasteron",
        "Alpha 100",
        "Alpha 2",
        "Alpha 200",
        "Alpha 2A",
        "Alpha 2A-8000",
        "Alpha 2A-900",
        "Callisto Morphamax",
        "Callisto Morphamax 500",
        "Callisto Morphamax 5000",
        "Callisto Morphamax 600",
        "Callisto Morphamax 6000 SE",
        "Callisto Morphamax 6000 SE2",
        "Callisto Morphamax 700",
        "Callisto Morphamax 7000",
        "Xiph Xlater 10000",
        "Xiph Xlater 2000",
        "Xiph Xlater 300",
        "Xiph Xlater 40",
        "Xiph Xlater 5",
        "Xiph Xlater 50",
        "Xiph Xlater 500",
        "Xiph Xlater 5000",
        "Xiph Xlater 58",
    };
    qsort(list, countof(list), sizeof(char *), _comp);
    for (size_t i = 0; i < countof(list); i++) {
        puts(list[i]);
    }
    puts("");
    qsort(list, countof(list), sizeof(char *), _rev_comp);
    for (size_t i = 0; i < countof(list); i++) {
        puts(list[i]);
    }
}

void test_print_stream() {
    {
        struct print_stream out = {.type = file_stream, .fp = stdout};
        printf_to_stream(&out, "-%c-%d-%g-%s-", 'A', 666, 3.14, "Hello");
        printf_to_stream(&out, "-%c-%d-%g-%s-", 'A', 666, 3.14, "Hello");
        char *esc_seqs = "\'\"\?\\\a\b\f\n\r\t\v";
        escape_to_stream(&out, esc_seqs, strlen(esc_seqs));
        out = (struct print_stream){.type = string_stream};
        printf_to_stream(&out, "-%c-%d-%g-%s-", 'A', 666, 3.14, "Hello");
        printf_to_stream(&out, "-%c-%d-%g-%s-", 'A', 666, 3.14, "Hello");
        puts(out.base);
        for (;;) {
            printf_to_stream(&out, "-%c-%d-%g-%s-", 'A', 666, 3.14, "Hello");
            printf_to_stream(&out, "-%c-%d-%g-%s-", 'A', 666, 3.14, "Hello");
            buffer_free(out.base, out.length, out.capacity);
        }
    }
    // for (;;) {
    //     struct js_heap heap = {0};
    //     struct js_value val = _random_js_value(&heap, vt_object, 2);
    //     struct print_stream out = {.type = string_stream};
    //     js_serialize_value(&out, todump_style, &val, 0);
    //     putsz_to_stream(&out, "\n\n");
    //     js_serialize_value(&out, tojson_style, &val, 0);
    //     putsz_to_stream(&out, "\n\n");
    //     js_serialize_value(&out, tostring_style, &val, 0);
    //     putsz_to_stream(&out, "\n\n\n\n");
    //     // puts(out.base);
    //     free_stream(&out);
    //     js_sweep(&heap);
    //     buffer_free(heap.base, heap.length, heap.capacity);
    // }
}

static void _print_capture_recursively(struct re_capture *cap, int depth) {
    if (!cap) {
        printf("%*snull\n", depth * 4, "");
        return;
    }
    if (cap->head && cap->tail) {
        printf("%*s(%zu) \"%.*s\"\n", depth * 4, "", cap->subs.length, (int)(cap->tail - cap->head), cap->head);
    } else {
        printf("%*s(%zu) empty\n", depth * 4, "", cap->subs.length);
    }
    buffer_for_each(cap->subs.base, cap->subs.length, cap->subs.capacity,
        i, c, _print_capture_recursively(c, depth + 1));
}

static void _print_capture(struct re_capture *cap) {
    _print_capture_recursively(cap, 1);
}

void test_regex() {
    char *samples[][2] = {
        {"", ""},
        {"abcdefghijklmnopqrstuvwxyz", ""},
        {"", "abcdefghijklmnopqrstuvwxyz"},
        {"abc", "abcdefghijklmnopqrstuvwxyz"},
        {"opq", "abcdefghijklmnopqrstuvwxyz"},
        {"xyz", "abcdefghijklmnopqrstuvwxyz"},
        {"abcdefghijklmnopqrstuvwxyz", "^abc"},
        {"abcdefghijklmnopqrstuvwxyz", "abc"},
        {"abcdefghijklmnopqrstuvwxyz", "abc$"},
        {"abcdefghijklmnopqrstuvwxyz", "^opq"},
        {"abcdefghijklmnopqrstuvwxyz", "opq"},
        {"abcdefghijklmnopqrstuvwxyz", "opq$"},
        {"abcdefghijklmnopqrstuvwxyz", "^xyz"},
        {"abcdefghijklmnopqrstuvwxyz", "xyz"},
        {"abcdefghijklmnopqrstuvwxyz", "xyz$"},
        {"abcdefghijklmnopqrstuvwxyz", "(d(e(fg)h)i(jk)l)mn(opq)"},
        {"abcdefghijklmnopqrstuvwxyz", "mn(\\o\\p\\q)rst"},
        {"abc123def456ghi789", "[^\\d\\o\\p\\q"},
        {"abc123def456ghi789", "[3-5]"},
        {"aaabbbccc", "b"},
        {"aaabbbccc", "a*"}, // matched at beginning following greeding rule, result is "aaa"
        {"aaabbbccc", "b*"}, // also matched at beginning, result is ""
        {"aaabbbccc", "a+"},
        {"aaabbbccc", "b+"},
        {"bbbccc", "a?b"},
        {"abbbccc", "a?b"},
        {"aabbbccc", "a?b"},
        {"Unknown-14886@noemail.invalid", "^([\\w\\.-]+)\\@([\\w-]+)\\.([a-zA-Z\\w]+)$"},
        {"asdf", "a(sd)z"},
        {"filename.ext", "[^\\.]+$"},
        {"filename.ext", "[^.]+$"},
        {"lK98hBgmK*tNNkYt5E3fv", "[0-9a-zA-Z]+"},
        {"lK98hBgmK*tNNkYt5E3fv", "^[0-9a-zA-Z]+"},
        {"lK98hBgmK*tNNkYt5E3fv", "[0-9a-zA-Z]+$"},
    };
    // for (;;) { // test memory leak
    for (size_t i = 0; i < countof(samples); i++) {
        struct re_capture capture = {0};
        printf("\"%s\", \"%s\": %s\n", samples[i][0], samples[i][1],
            re_match(samples[i][0], samples[i][1], &capture) ? "true" : "false");
        _print_capture(&capture);
        re_free_capture(&capture);
    }
    // }
}

#endif