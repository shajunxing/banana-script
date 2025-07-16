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
#include <errno.h> // strerror(errno)
#include <stdarg.h> // string_buffer_append_fv
#include <stdbool.h>
#include <stdint.h> // uint64_t in buffer_dump
#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h>

#ifdef __GLIBC__
// unlike https://en.cppreference.com/w/c/algorithm/qsort said, even if defined __STDC_LIB_EXT1__ __STDC_WANT_LIB_EXT1__ is useless, error 'undefined reference to qsort_s'
// linux only has qsort_r, see https://www.man7.org/linux/man-pages/man3/qsort.3.html ,must define _GNU_SOURCE, or "warning: implicit declaration of function 'qsort_r'". I tried, but still got warning
// add definition here to suppress warning, god knows
void qsort_r(void *, size_t, size_t, int (*)(const void *, const void *, void *), void *);
#endif

// https://gcc.gnu.org/wiki/Visibility
// https://stackoverflow.com/questions/2810118/how-to-tell-the-mingw-linker-not-to-export-all-symbols
// https://www.linux.org/docs/man1/ld.html
// windows must add -Wl,--exclude-all-symbols, linux use -fvisibility=hidden -fvisibility-inlines-hidden
// Function declarations are implicitly extern, but global variables need to use extern
#ifdef DLL
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef EXPORT
            #define shared __declspec(dllexport)
        #else
            #define shared __declspec(dllimport)
        #endif
    #elif __GNUC__ >= 4
        #define shared extern __attribute__((visibility("default")))
    #else
        #define shared extern
    #endif
#else
    #define shared extern
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
// DON'T use 'warning', will confilct with '#pragma warning', lot's of 'warning C4068: unknown pragma ...'

#ifdef DEBUG
    #define log_debug(__arg_format, ...) \
        printf("DEBUG %s:%d:%s: " __arg_format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    #define log_expression(__arg_format, __arg_expr) \
        log_debug(#__arg_expr " = " __arg_format, __arg_expr);
#else
    #define log_debug(__arg_format, ...) (void)0
    #define log_expression(__arg_format, __arg_expr) (void)0
#endif

#define log_warning(__arg_format, ...) \
    printf("WARNING %s:%d:%s: " __arg_format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define log_error(__arg_format, ...) \
    printf("ERROR %s:%d:%s: " __arg_format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

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
        typeof(__arg_length) __new_length = (__arg_length) + 1; \
        buffer_alloc((__arg_base), (__arg_length), (__arg_capacity), __new_length); \
        (__arg_base)[(__arg_length)] = __arg_value; \
        (__arg_length) = __new_length; \
    } while (0)

#define buffer_put(__arg_base, __arg_length, __arg_capacity, __arg_index, __arg_value) \
    do { \
        typeof(__arg_index) __new_length = (__arg_index) + 1; \
        if (__new_length > (__arg_length)) { \
            buffer_alloc((__arg_base), (__arg_length), (__arg_capacity), __new_length); \
            (__arg_length) = __new_length; \
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
        typeof(__arg_length) __length = (__arg_length); \
        typeof(__arg_capacity) __capacity = (__arg_capacity); \
        printf(#__arg_base "=%p " #__arg_length "=%zu " #__arg_capacity "=%zu\n", \
            __base, (uint64_t)__length, (uint64_t)__capacity); \
        print_hex(__base, __length * sizeof(typeof(*(__arg_base)))); \
    } while (0)

#define buffer_for_each(__arg_base, __arg_length, __arg_capacity, __arg_i, __arg_v, __arg_block) \
    do { \
        typeof(__arg_base) __base = (__arg_base); \
        typeof(__arg_length) __length = (__arg_length); \
        for (typeof(__length) __arg_i = 0; __arg_i < __length; __arg_i++) { \
            typeof(__arg_base) __arg_v = __base + __arg_i; \
            __arg_block; \
        } \
    } while (0)

#define string_buffer_clear(__arg_base, __arg_length, __arg_capacity) \
    buffer_clear((__arg_base), (__arg_length), (__arg_capacity))

#define string_buffer_append(__arg_base, __arg_length, __arg_capacity, __arg_s, __arg_slen) \
    do { \
        /* DON'T use typeof(__arg_s) __s */ \
        /* 'error C2075: '__s': initialization requires a brace-enclosed initializer list' */ \
        const char *__s = (__arg_s); \
        typeof(__arg_slen) __slen = (__arg_slen); \
        /* reduce useless allocation on empty js_string() */ \
        /* and, js_string() then read_line, make sure enforce __s is NULL */ \
        if (__s && __slen) { \
            typeof(__arg_length) __length = (__arg_length); \
            typeof(__arg_length) __new_length = __length + (typeof(__arg_length))__slen; \
            /* It is necessary to add 1 zero byte in the end, to support no length functions such as 'puts' */ \
            /* Caution: __arg_base is for write */ \
            buffer_alloc((__arg_base), __length, (__arg_capacity), __new_length + 1); \
            memcpy((__arg_base) + __length, __s, __slen); \
            (__arg_length) = __new_length; \
        } \
    } while (0)

#define string_buffer_append_sz(__arg_base, __arg_length, __arg_capacity, __arg_s) \
    do { \
        /* DON'T use __s, 'warning C4700: uninitialized local variable '__s' used' */ \
        /* string_buffer_append_ch will be expanded '__s = __s' */ \
        typeof(__arg_s) _s = (__arg_s); \
        string_buffer_append((__arg_base), (__arg_length), (__arg_capacity), _s, strlen(_s)); \
    } while (0)

#define string_buffer_append_ch(__arg_base, __arg_length, __arg_capacity, __arg_ch) \
    do { \
        char __ch = (__arg_ch); \
        string_buffer_append((__arg_base), (__arg_length), (__arg_capacity), &__ch, 1); \
    } while (0)

#define string_buffer_append_f(__arg_base, __arg_length, __arg_capacity, __arg_format, ...) \
    do { \
        typeof(__arg_format) __format = __arg_format; \
        int __result = snprintf(NULL, 0, __format, ##__VA_ARGS__); \
        if (__result < 0) { \
            fatal("snprintf() failed: %s", strerror(errno)); \
        } \
        typeof(__arg_length) __length = (__arg_length); \
        typeof(__arg_length) __new_length = __length + (typeof(__arg_length))__result; \
        buffer_alloc((__arg_base), __length, (__arg_capacity), __new_length + 1); \
        __result = snprintf((__arg_base) + __length, (size_t)__result + 1, __format, ##__VA_ARGS__); \
        if (__result < 0) { \
            fatal("snprintf() failed: %s", strerror(errno)); \
        } \
        (__arg_length) = __new_length; \
    } while (0)

#define string_buffer_append_fv(__arg_base, __arg_length, __arg_capacity, __arg_format, __arg_va) \
    do { \
        typeof(__arg_format) __format = __arg_format; \
        va_list __va_copy; \
        va_copy(__va_copy, __arg_va); \
        int __result = vsnprintf(NULL, 0, __format, __arg_va); \
        if (__result < 0) { \
            fatal("vsnprintf() failed: %s", strerror(errno)); \
        } \
        typeof(__arg_length) __length = (__arg_length); \
        typeof(__arg_length) __new_length = __length + (typeof(__arg_length))__result; \
        buffer_alloc((__arg_base), __length, (__arg_capacity), __new_length + 1); \
        __result = vsnprintf((__arg_base) + __length, (size_t)__result + 1, __format, __va_copy); \
        va_end(__va_copy); \
        if (__result < 0) { \
            fatal("vsnprintf() failed: %s", strerror(errno)); \
        } \
        (__arg_length) = __new_length; \
    } while (0)

#define read_binary_file(__arg_fname, __arg_base, __arg_length, __arg_capacity) \
    do { \
        FILE *__fp = fopen(__arg_fname, "rb"); \
        if (__fp == NULL) { \
            fatal("Cannot open \"%s\"", __arg_fname); \
        } \
        fseek(__fp, 0, SEEK_END); \
        typeof(__arg_length) __fsize = (typeof(__arg_length))ftell(__fp); \
        enforce(__fsize >= 0); \
        enforce(__fsize % sizeof(*(__arg_base)) == 0); \
        typeof(__arg_length) __len = __fsize / sizeof(*(__arg_base)); \
        fseek(__fp, 0, SEEK_SET); \
        buffer_alloc(__arg_base, __arg_length, __arg_capacity, __arg_length + __len); \
        __arg_length += (typeof(__arg_length))fread( \
            __arg_base + __arg_length, sizeof(*(__arg_base)), __len, __fp); \
        fclose(__fp); \
    } while (0)

#define read_text_file(__arg_fname, __arg_base, __arg_length, __arg_capacity) \
    do { \
        enforce(sizeof(*(__arg_base)) == 1); \
        FILE *__fp = fopen(__arg_fname, "r"); \
        if (__fp == NULL) { \
            fatal("Cannot open \"%s\"", __arg_fname); \
        } \
        fseek(__fp, 0, SEEK_END); \
        typeof(__arg_length) __fsize = (typeof(__arg_length))ftell(__fp); \
        enforce(__fsize >= 0); \
        fseek(__fp, 0, SEEK_SET); \
        buffer_alloc(__arg_base, __arg_length, __arg_capacity, __arg_length + __fsize + 1); \
        __arg_length += (typeof(__arg_length))fread( \
            __arg_base + __arg_length, sizeof(*(__arg_base)), __fsize, __fp); \
        fclose(__fp); \
    } while (0)

#define write_file(__arg_fname, __arg_mode, __arg_base, __arg_length, __arg_capacity) \
    do { \
        FILE *__fp = fopen(__arg_fname, __arg_mode); \
        if (__fp == NULL) { \
            fatal("Cannot open \"%s\"", __arg_fname); \
        } \
        fwrite(__arg_base, sizeof(*(__arg_base)), __arg_length, __fp); \
        fclose(__fp); \
    } while (0)

#define read_line(__arg_fp, __arg_base, __arg_length, __arg_capacity) \
    do { \
        /* __arg_base may not be NULL, js_string is always not NULL */ \
        enforce(__arg_length == 0); \
        /* __arg_capacity may not be 0 too */ \
        for (;;) { \
            /* DON'T use __ch, 'warning C4700: uninitialized local variable '__ch' used' */ \
            /* string_buffer_append_ch will be expanded '__ch = __ch' */ \
            int __c = fgetc(__arg_fp); \
            if (__c == EOF || __c == '\n') { \
                break; \
            } \
            string_buffer_append_ch(__arg_base, __arg_length, __arg_capacity, __c); \
        } \
    } while (0)

#ifndef numargs
    // https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
    // in msvc, works fine even with old version
    // in gcc, only works with default or -std=gnu2x, -std=c?? will treat zero parameter as 1, maybe ## is only recognized by gnu extension
    #if defined(__GNUC__) && defined(__STRICT_ANSI__)
        #error numargs() only works with gnu extension enabled
    #endif
    #define _numargs_call(__arg_0, __arg_1) __arg_0 __arg_1
    #define _numargs_select(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, __arg_0, ...) __arg_0
    #define numargs(...) _numargs_call(_numargs_select, (_, ##__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif

shared void print_hex(void *, size_t);
shared char *string_dupe(const char *, size_t);
shared char *string_dupe_sz(const char *);
shared bool string_starts_with_sz(const char *, const char *);
shared bool string_ends_with_sz(const char *, const char *);
#define string_equals_sz(__arg_lhs, __arg_rhs) (strcmp(__arg_lhs, __arg_rhs) == 0)
shared char *string_join_internal_sz(const char *, size_t, ...);
#define string_join_sz(__arg_0, ...) string_join_internal_sz(__arg_0, numargs(__VA_ARGS__), ##__VA_ARGS__)
#define string_concat_sz(...) string_join_sz("", ##__VA_ARGS__)
shared int string_natural_compare_sz(const char *, const char *);

#ifdef DEBUG

shared char *random_sz_static(size_t *);
shared char *random_sz_dynamic();
shared void test_random_sz();
shared double random_double();
shared void test_random_double();
shared void test_buffer();
shared void test_string_buffer();
shared void test_string_buffer_loop();
shared void test_string_buffer_append_f();
shared void test_memmove();
shared void test_string_dupe();
shared void test_read_file();
shared void test_read_line();
shared void test_natural_compare();

#endif

#endif