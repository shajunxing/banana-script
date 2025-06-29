/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_COMMON_H
#define JS_COMMON_H

#ifdef _MSC_VER
    #pragma warning(disable : 4996) // such as "'strcpy' unsafe" or "'rmdir' deprecated"
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// https://gcc.gnu.org/wiki/Visibility
// https://stackoverflow.com/questions/2810118/how-to-tell-the-mingw-linker-not-to-export-all-symbols
// https://www.linux.org/docs/man1/ld.html
// windows must add -Wl,--exclude-all-symbols, linux use -fvisibility=hidden -fvisibility-inlines-hidden
#ifdef DLL
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef EXPORT
            #define shared __declspec(dllexport)
        #else
            #define shared __declspec(dllimport)
        #endif
    #elif __GNUC__ >= 4
        #define shared __attribute__((visibility("default")))
    #else
        #define shared
    #endif
#else
    #define shared
#endif

// c23 typeof
// https://learn.microsoft.com/en-us/cpp/c-language/typeof-c?view=msvc-170 use /std:clatest to enable it
// #ifdef _MSC_VER
//     #define typeof __typeof__
// #endif

#ifndef countof
    #define countof(__arg_array) (sizeof(__arg_array) / sizeof((__arg_array)[0]))
#endif

#ifndef offsetof
    #define offsetof(__arg_type, __arg_element) ((size_t)&(((__arg_type *)0)->__arg_element))
#endif

#ifndef max // unsafe, have side effect, be careful
    #define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// DON'T set macro 'log' because it is conflict with math function 'log'
// All these logs are for debug purpose, so 'log_info' is unnecessary

#ifdef NOLOGINFO
    #define log_info(__arg_format, ...) (void)0
#else
    #define log_info(__arg_format, ...) \
        printf("INFO %s:%d:%s: " __arg_format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

#ifdef NOLOGWARNING
    // DON'T use 'warning', will confilct with '#pragma warning', lot's of 'warning C4068: unknown pragma ...'
    #define log_warning(__arg_format, ...) (void)0
#else
    #define log_warning(__arg_format, ...) \
        printf("WARNING %s:%d:%s: " __arg_format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

#ifdef NOLOGERROR
    #define log_error(__arg_format, ...) (void)0
#else
    #define log_error(__arg_format, ...) \
        printf("ERROR %s:%d:%s: " __arg_format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

#define fatal(__arg_format, ...) \
    do { \
        log_error("Fatal error: " __arg_format, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while (0)

#define enforce(__arg_condition) \
    do { \
        if (!(__arg_condition)) { \
            fatal("Enforcement failed: %s", #__arg_condition); \
        } \
    } while (0)

#define alloc(__arg_type, __arg_length) ((__arg_type *)calloc((__arg_length), sizeof(__arg_type)))

// buffer's capacity is always pow of 2
#define buffer_alloc(__arg_base, __arg_length, __arg_capacity, __arg_required_capacity) \
    do { \
        typeof(__arg_required_capacity) __reqcap = (__arg_required_capacity); \
        if (__reqcap > (__arg_capacity)) { \
            typeof(__arg_capacity) __newcap = 1; \
            for (__newcap = (__arg_capacity) == 0 ? 1 : (__arg_capacity); __reqcap > __newcap; __newcap <<= 1) { \
                enforce(__newcap > 0); \
            } \
            if (__arg_base) { \
                (__arg_base) = (typeof(__arg_base))realloc((__arg_base), __newcap * sizeof(typeof(*(__arg_base)))); \
                enforce((__arg_base) != NULL); \
                memset((__arg_base) + (__arg_capacity), 0, (__newcap - (__arg_capacity)) * sizeof(typeof(*(__arg_base)))); \
            } else { \
                (__arg_base) = (typeof(__arg_base))alloc(typeof(*(__arg_base)), __newcap); \
                enforce((__arg_base) != NULL); \
            } \
            (__arg_capacity) = __newcap; \
        } \
    } while (0)

// NULL pointer can be safely passed to free()
// https://en.cppreference.com/w/c/memory/free
#define buffer_free(__arg_base, __arg_length, __arg_capacity) \
    do { \
        free(__arg_base); \
        (__arg_base) = NULL; \
        (__arg_length) = 0; \
        (__arg_capacity) = 0; \
    } while (0)

// DON'T surround '__arg_type' with parentheses, or will get "syntax error : ')'"
#define buffer_push(__arg_base, __arg_length, __arg_capacity, __arg_value) \
    do { \
        typeof(__arg_length) __newlen = (__arg_length) + 1; \
        buffer_alloc((__arg_base), (__arg_length), (__arg_capacity), __newlen); \
        (__arg_base)[(__arg_length)] = __arg_value; \
        (__arg_length) = __newlen; \
    } while (0)

#define buffer_put(__arg_base, __arg_length, __arg_capacity, __arg_index, __arg_value) \
    do { \
        typeof(__arg_index) __newlen = (__arg_index) + 1; \
        if (__newlen > (__arg_length)) { \
            buffer_alloc((__arg_base), (__arg_length), (__arg_capacity), __newlen); \
            (__arg_length) = __newlen; \
        } \
        (__arg_base)[(__arg_index)] = __arg_value; \
    } while (0)

// use "clear" instead of "empty", to avoid conflit meaning "is empty", refer C++ string
#define buffer_clear(__arg_base, __arg_length, __arg_capacity) \
    do { \
        memset((__arg_base), 0, (__arg_capacity) * sizeof(typeof(*(__arg_base)))); \
        (__arg_length) = 0; \
    } while (0)

// https://stackoverflow.com/questions/2524611/how-can-one-print-a-size-t-variable-portably-using-the-printf-family
#define buffer_dump(__arg_base, __arg_length, __arg_capacity) \
    do { \
        typeof(__arg_base) __base = (__arg_base); \
        typeof(__arg_length) __len = (__arg_length); \
        typeof(__arg_capacity) __cap = (__arg_capacity); \
        printf(#__arg_base "=%p " #__arg_length "=%zu " #__arg_capacity "=%zu\n", \
            __base, (uint64_t)__len, (uint64_t)__cap); \
        print_hex(__base, __len * sizeof(typeof(*(__arg_base)))); \
    } while (0)

#define buffer_for_each(__arg_base, __arg_length, __arg_capacity, __arg_i, __arg_v, __arg_block) \
    do { \
        typeof(__arg_base) __base = (__arg_base); \
        typeof(__arg_length) __len = (__arg_length); \
        for (typeof(__len) __arg_i = 0; __arg_i < __len; __arg_i++) { \
            typeof(__arg_base) __arg_v = __base + __arg_i; \
            __arg_block; \
        } \
    } while (0)

#define string_buffer_clear(__arg_base, __arg_length, __arg_capacity) buffer_clear((__arg_base), (__arg_length), (__arg_capacity))

#define string_buffer_append(__arg_base, __arg_length, __arg_capacity, __arg_str, __arg_len) \
    do { \
        typeof(__arg_length) __len = (__arg_length); \
        typeof(__arg_length) __newlen = __len + (typeof(__arg_length))(__arg_len); \
        /* TODO: Is it necessary to add 1 zero byte in the end? to support no length functions? */ \
        buffer_alloc((__arg_base), __len, (__arg_capacity), __newlen + 1); \
        memcpy((__arg_base) + __len, (__arg_str), (__arg_len)); \
        (__arg_length) = __newlen; \
    } while (0)

#define string_buffer_append_sz(__arg_base, __arg_length, __arg_capacity, __arg_str) \
    do { \
        const char *__str = (__arg_str); \
        string_buffer_append((__arg_base), (__arg_length), (__arg_capacity), __str, strlen(__str)); \
    } while (0)

#define string_buffer_append_ch(__arg_base, __arg_length, __arg_capacity, __arg_ch) \
    do { \
        char __ch = (__arg_ch); \
        string_buffer_append((__arg_base), (__arg_length), (__arg_capacity), &__ch, 1); \
    } while (0)

shared void print_hex(void *, size_t);

#ifndef NOTEST

    #define print_result(__arg_0, __arg_1) printf(#__arg_0 " = " __arg_1 "\n", __arg_0)

shared char *random_sz_static(size_t *);
shared char *random_sz_dynamic();
shared int test_random_sz(int, char *[]);
shared double random_double();
shared int test_random_double(int, char *[]);
shared int test_buffer(int, char *[]);
shared int test_string_buffer(int, char *[]);
shared int test_string_buffer_loop(int, char *[]);
shared int test_memmove(int, char *[]);

#endif

#endif