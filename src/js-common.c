/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "js-common.h"

void print_hex(void *base, size_t length) {
#define _printable(__arg_ch) (__arg_ch >= 0x20 && __arg_ch <= 0x7e ? __arg_ch : ' ')
    static const size_t _segs_per_line = 2;
    static const size_t _bytes_per_seg = 8;
    size_t bytes_per_line = _segs_per_line * _bytes_per_seg;
    unsigned char *p = (unsigned char *)base;
    for (size_t loffs = 0; loffs <= length; loffs += bytes_per_line) { // <= make sure when length=0 also print a line
        printf("%8llu:  ", loffs);
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

#ifndef NOTEST

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

int test_random_sz(int argc, char *argv[]) {
    for (;;) {
        char *str = random_sz_dynamic();
        puts(str);
        free(str);
    }
    return EXIT_SUCCESS;
}

double random_double() {
    return pow(DBL_MAX, (double)rand() / RAND_MAX) * (rand() % 2 == 0 ? -1 : 1);
}

int test_random_double(int argc, char *argv[]) {
    for (;;) {
        double d = random_double();
        printf("%g\n", d);
    }
    return EXIT_SUCCESS;
}

int test_buffer(int argc, char *argv[]) {
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
    buffer_for_each(p, len, cap, i, v, printf("%4llu: %lld\n", i, *v));
    buffer_free(p, len, cap);
    return EXIT_SUCCESS;
}

int test_string_buffer(int argc, char *argv[]) {
    char *str = NULL;
    size_t len = 0;
    size_t cap = 0;
    string_buffer_append_sz(str, len, cap, "hello");
    string_buffer_append_ch(str, len, cap, ',');
    string_buffer_append_sz(str, len, cap, "world");
    string_buffer_append_ch(str, len, cap, '!');
    buffer_dump(str, len, cap);
    buffer_free(str, len, cap);
    return EXIT_SUCCESS;
}

int test_string_buffer_loop(int argc, char *argv[]) {
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
    return EXIT_SUCCESS;
}

int test_memmove(int argc, char *argv[]) {
    char mem[32] = {0};
    const char *str = "Hello,World!";
    print_hex(mem, countof(mem));
    strcpy(mem + 3, str);
    print_hex(mem, countof(mem));
    memmove(mem + 6, mem + 3, strlen(str));
    print_hex(mem, countof(mem)); //  memmove does not zero origin space
    return EXIT_SUCCESS;
}

#endif