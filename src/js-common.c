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
char *string_dupe(const char *src, size_t maxlen) {
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

char *string_dupe_sz(const char *src) {
    size_t len = strlen(src);
    // log_expression("%zu", len);
    char *dst = calloc(len + 1, 1);
    if (dst != NULL) {
        memcpy(dst, src, len);
    }
    return dst;
}

bool string_starts_with_sz(const char *str, const char *prefix) {
    size_t len = strlen(str);
    size_t prefixlen = strlen(prefix);
    return prefixlen <= len && strncmp(str, prefix, prefixlen) == 0;
}

bool string_ends_with_sz(const char *str, const char *suffix) {
    size_t len = strlen(str);
    size_t suffixlen = strlen(suffix);
    return suffixlen <= len && strncmp(str + len - suffixlen, suffix, suffixlen) == 0;
}

char *string_join_internal_sz(const char *sep, size_t nargs, ...) {
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

int string_natural_compare_sz(const char *x, const char *y) {
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

void _append_fv(char **base, size_t *length, size_t *capacity, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    string_buffer_append_fv(*base, *length, *capacity, fmt, args);
    va_end(args);
}

void test_string_buffer_append_f() {
    char *base = NULL;
    size_t length = 0;
    size_t capacity = 0;
    string_buffer_append_f(base, length, capacity, "/%g/%s", 3.14, "hello");
    buffer_dump(base, length, capacity);
    string_buffer_append_f(base, length, capacity, "/%g/%s", 2.72, "world");
    buffer_dump(base, length, capacity);
    _append_fv(&base, &length, &capacity, "/%g/%s", 3.14, "hello");
    buffer_dump(base, length, capacity);
    _append_fv(&base, &length, &capacity, "/%g/%s", 2.72, "world");
    buffer_dump(base, length, capacity);
}

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
    char *dst = string_dupe_sz(src);
    puts(dst);
    free(dst);
    dst = string_dupe(src, 3);
    puts(dst);
    free(dst);
    dst = string_dupe(src, 7);
    puts(dst);
    free(dst);
    dst = string_dupe(src, 100);
    puts(dst);
    free(dst);
}

void test_read_file() {
    const char *filename = "examples/power_of_2_length.txt";
    char *base = NULL;
    size_t length = 0;
    size_t capacity = 0;
    read_text_file(filename, base, length, capacity);
    puts(base);
    buffer_dump(base, length, capacity);
    read_text_file(filename, base, length, capacity);
    puts(base);
    buffer_dump(base, length, capacity);
    buffer_free(base, length, capacity);
    read_binary_file(filename, base, length, capacity);
    puts(base);
    buffer_dump(base, length, capacity);
    read_binary_file(filename, base, length, capacity);
    puts(base);
    buffer_dump(base, length, capacity);
    buffer_free(base, length, capacity);
}

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
    return string_natural_compare_sz(*((char **)lhs), *((char **)rhs));
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

#endif