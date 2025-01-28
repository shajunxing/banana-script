/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_H
#define JS_H

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://gcc.gnu.org/wiki/Visibility
// https://stackoverflow.com/questions/2810118/how-to-tell-the-mingw-linker-not-to-export-all-symbols
// https://www.linux.org/docs/man1/ld.html
// windows must add -Wl,--exclude-all-symbols, linux use -fvisibility=hidden -fvisibility-inlines-hidden
#if defined DLL
    #if defined _WIN32 || defined __CYGWIN__
        #if defined EXPORT
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

// https://stackoverflow.com/questions/24736304/unable-to-use-inline-in-declaration-get-error-c2054
#if defined(_MSC_VER) && _MSC_VER < 1900
    #define inline __inline
#endif

#if !defined(NDEBUG)
    // https://stackoverflow.com/questions/4384765/whats-the-difference-between-pretty-function-function-func
    #if __STDC_VERSION__ >= 199901L // c99
        #define log(fmt, ...) printf("%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    #elif defined(__PRETTY_FUNCTION__)
        #define log(fmt, ...) printf("%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
    #elif defined(__FUNCTION__)
        #define log(fmt, ...) printf("%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #else
        #define log(fmt, ...) printf("%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #endif
#else
    // https://stackoverflow.com/questions/25021081/what-does-the-c-precompiler-do-with-macros-defined-as-void0
    #define log(fmt, ...) (void)0
#endif

// https://en.cppreference.com/w/c/preprocessor/replace
#if __STDC_VERSION__ >= 199901L // c99
    #include <stdbool.h>
#else
// https://stackoverflow.com/questions/1608318/is-bool-a-native-c-type/1608350
typedef enum {
    false = (1 == 0),
    true = (!false)
} bool;
#endif

#define countof(a) (sizeof(a) / sizeof((a)[0]))
#if !defined(max) // unsafe, have side effect, be careful
    #define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// https://stackoverflow.com/questions/2611764/can-i-use-a-binary-literal-in-c-or-c
#define _hex(n) 0x##n##LU
#define _b8(x) ((x & 0x0000000FLU) ? 1 : 0) + ((x & 0x000000F0LU) ? 2 : 0) + ((x & 0x00000F00LU) ? 4 : 0) + ((x & 0x0000F000LU) ? 8 : 0) + ((x & 0x000F0000LU) ? 16 : 0) + ((x & 0x00F00000LU) ? 32 : 0) + ((x & 0x0F000000LU) ? 64 : 0) + ((x & 0xF0000000LU) ? 128 : 0)
#define b8(d) ((unsigned char)_b8(_hex(d)))
#define b16(dmsb, dlsb) (((unsigned short)b8(dmsb) << 8) + b8(dlsb))
#define b32(dmsb, db2, db3, dlsb) (((unsigned long)b8(dmsb) << 24) + ((unsigned long)b8(db2) << 16) + ((unsigned long)b8(db3) << 8) + b8(dlsb))

// https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments compatible with msvc
// _1 ... _63 are param name placeholders, finally returns N
// zero parameter will cause problem, see https://gist.github.com/aprell/3722962 also only support gcc/clang
#define _expand(x) x
#define _numargs(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...) N
#define numargs(...) _expand(_numargs(_, ##__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define fatal(fmt, ...)                          \
    do {                                         \
        log("Fatal error: " fmt, ##__VA_ARGS__); \
        exit(EXIT_FAILURE);                      \
    } while (0)

#if defined assert
    #undef assert
#endif

#define assert(cond)                            \
    do {                                        \
        if (!(cond)) {                          \
            log("Assertion failed: %s", #cond); \
            exit(EXIT_FAILURE);                 \
        }                                       \
    } while (0)

// js-buffer.c

shared char *read_file(const char *filename, size_t *plen);
shared char *read_line(FILE *fp, size_t *plen);
shared const char *ascii_abbreviation(int ch);
shared void print_hex(void *p, size_t len);

// buffer's capacity is always pow of 2
#define buffer_alloc(type, p, cap, reqcap)                    \
    do {                                                      \
        size_t _reqcap = (reqcap);                            \
        size_t _newcap = 1;                                   \
        for (_newcap = 1; _newcap < _reqcap; _newcap <<= 1) { \
            assert(_newcap > 0);                              \
        }                                                     \
        (p) = (type *)calloc(_newcap, sizeof(type));          \
        assert((p) != NULL);                                  \
        (cap) = _newcap;                                      \
    } while (0)

#define buffer_realloc(type, p, cap, reqcap)                          \
    do {                                                              \
        size_t _reqcap = (reqcap);                                    \
        if ((cap) < _reqcap) {                                        \
            size_t _newcap;                                           \
            for (_newcap = (cap); _newcap < _reqcap; _newcap <<= 1) { \
                assert(_newcap > 0);                                  \
            }                                                         \
            (p) = (type *)realloc((p), _newcap * sizeof(type));       \
            assert((p) != NULL);                                      \
            memset((p) + (cap), 0, (_newcap - (cap)) * sizeof(type)); \
            (cap) = _newcap;                                          \
        }                                                             \
    } while (0)

// DONT surround 'type' with parentheses, or will get "syntax error : ')'"
#define buffer_push(type, p, len, cap, v)          \
    do {                                           \
        size_t _newlen = (len) + 1;                \
        buffer_realloc(type, (p), (cap), _newlen); \
        (p)[(len)] = v;                            \
        (len) = _newlen;                           \
    } while (0)

// use "clear" instead of "empty", to avoid conflit meaning "is empty", refer C++ string
#define buffer_clear(type, p, num, cap)       \
    do {                                      \
        memset((p), 0, (cap) * sizeof(type)); \
        (num) = 0;                            \
    } while (0)

// https://stackoverflow.com/questions/2524611/how-can-one-print-a-size-t-variable-portably-using-the-printf-family
#define buffer_dump(type, p, num, cap)             \
    do {                                           \
        type *_p = (p);                            \
        size_t _num = (num);                       \
        size_t _cap = (cap);                       \
        printf("num=%llu cap=%llu\n", _num, _cap); \
        print_hex(_p, _cap * sizeof(type));        \
    } while (0)

#define string_buffer_new(str, len, cap) buffer_alloc(char, (str), (cap), 1)
#define string_buffer_clear(str, len, cap) buffer_clear(char, (str), (len), (cap))
#define string_buffer_append(str, len, cap, s, l)        \
    do {                                                 \
        size_t _newlen = (len) + (l);                    \
        buffer_realloc(char, (str), (cap), _newlen + 1); \
        memcpy((str) + (len), (s), (l));                 \
        (len) = _newlen;                                 \
    } while (0)
#define string_buffer_append_sz(str, len, cap, s)                  \
    do {                                                           \
        const char *_s = (s);                                      \
        string_buffer_append((str), (len), (cap), _s, strlen(_s)); \
    } while (0)
#define string_buffer_append_ch(str, len, cap, c)          \
    do {                                                   \
        char _c = (c);                                     \
        string_buffer_append((str), (len), (cap), &_c, 1); \
    } while (0)

struct string {
    char *p;
    size_t len;
    size_t cap;
};

#define string_dump(s)                              \
    do {                                            \
        struct string *_s = (s);                    \
        buffer_dump(char, _s->p, _s->len, _s->cap); \
    } while (0)
shared struct string *string_new(const char *p, size_t len);
shared void string_delete(struct string *str);
shared void string_clear(struct string *str);
shared void string_append(struct string *str, const char *p, size_t len);
// _sz ended macros, if str is a function, be careful to prevent side effect
#define string_append_sz(str, p)              \
    do {                                      \
        char *_p = (p);                       \
        string_append((str), _p, strlen(_p)); \
    } while (0)
#define string_append_ch(str, ch)      \
    do {                               \
        char _ch = (ch);               \
        string_append((str), &_ch, 1); \
    } while (0)
shared int string_compare(struct string *str_l, struct string *str_r);

struct list {
    void **p;
    size_t len;
    size_t cap;
};

#define list_dump(l)                                  \
    do {                                              \
        struct list *_l = (l);                        \
        buffer_dump(void *, _l->p, _l->len, _l->cap); \
    } while (0)
shared struct list *list_new(size_t reqcap);
shared void list_delete(struct list *list);
shared void list_clear(struct list *list);
shared void list_push(struct list *list, void *val);
shared void *list_pop(struct list *list);
shared void list_put(struct list *list, size_t idx, void *val);
shared void *list_get(struct list *list, size_t idx);
// DONT use conflict name such as 'list'
// since list_put will expand, here skip NULL element to prevent NULL pointer crash, for example in _mark_in_use, be careful
#define list_for_each(l, i, v, block)   \
    do {                                \
        struct list *_l = (l);          \
        size_t i;                       \
        for (i = 0; i < _l->len; i++) { \
            void *v = _l->p[i];         \
            if (v) {                    \
                block;                  \
            }                           \
        }                               \
    } while (0)

struct mapnode {
    struct string *k;
    void *v;
};

struct map {
    struct mapnode *p;
    size_t len;
    size_t cap;
};

#define map_dump(m)                                           \
    do {                                                      \
        struct map *_m = (m);                                 \
        buffer_dump(struct mapnode, _m->p, _m->len, _m->cap); \
    } while (0)

shared struct map *map_new(size_t reqcap);
shared void map_delete(struct map *map);
shared void map_clear(struct map *map);
shared void map_put(struct map *map, const char *key, size_t klen, void *val);
#define map_put_sz(m, k, v)              \
    do {                                 \
        const char *_k = (k);            \
        map_put((m), _k, strlen(_k), v); \
    } while (0)
shared void *map_get(struct map *map, const char *key, size_t klen);
shared void *map_get_sz(struct map *map, const char *key);
// DONT use conflict name such as 'k' 'v'
#define map_for_each(m, _k, _v, block)        \
    do {                                      \
        struct map *_m = (m);                 \
        size_t _i;                            \
        for (_i = 0; _i < _m->cap; _i++) {    \
            struct mapnode *_kv = _m->p + _i; \
            struct string *_k = _kv->k;       \
            void *_v = _kv->v;                \
            if (_k && _v) {                   \
                block;                        \
            }                                 \
        }                                     \
    } while (0)

struct link;

struct linknode {
    void *v;
    struct link *owner;
    struct linknode *next;
    struct linknode *prev;
};

struct link {
    struct linknode *p;
    size_t len;
};

shared struct link *link_new();
shared void link_push(struct link *link, void *val);
#define link_for_each(l, n, block)   \
    do {                             \
        struct link *_l = (l);       \
        struct linknode *_n = _l->p; \
        while (_n) {                 \
            struct linknode *n = _n; \
            _n = _n->next;           \
            block;                   \
        }                            \
    } while (0)
shared void link_delete_node(struct link *link, struct linknode *node);
shared void link_dump(struct link *link);
shared void link_delete(struct link *link);

// https://codelucky.com/c-macros/
// https://en.wikipedia.org/wiki/X_Macro
// must use X, others will cause clang-format wrong indent, don't know why
#define js_token_state_list                     \
    X(ts_searching)                             \
    X(ts_end_of_file)                           \
    X(ts_identifier_matching)                   \
    X(ts_identifier)                            \
    X(ts_division_matching)                     \
    X(ts_division)                              \
    X(ts_line_comment_matching_new_line)        \
    X(ts_line_comment)                          \
    X(ts_block_comment_matching_end_star)       \
    X(ts_block_comment_matching_end_slash)      \
    X(ts_block_comment)                         \
    X(ts_minus)                                 \
    X(ts_number_matching)                       \
    X(ts_number_matching_fraction_first)        \
    X(ts_number_matching_fraction_rest)         \
    X(ts_number_matching_exponent_first)        \
    X(ts_number_matching_exponent_number_first) \
    X(ts_number_matching_exponent_rest)         \
    X(ts_number)                                \
    X(ts_plus)                                  \
    X(ts_multiplication)                        \
    X(ts_left_parenthesis)                      \
    X(ts_right_parenthesis)                     \
    X(ts_left_bracket)                          \
    X(ts_right_bracket)                         \
    X(ts_left_brace)                            \
    X(ts_right_brace)                           \
    X(ts_string_matching)                       \
    X(ts_string_matching_control)               \
    X(ts_string)                                \
    X(ts_null)                                  \
    X(ts_true)                                  \
    X(ts_false)                                 \
    X(ts_colon)                                 \
    X(ts_comma)                                 \
    X(ts_assignment_matching)                   \
    X(ts_equal_to)                              \
    X(ts_assignment)                            \
    X(ts_less_than_matching)                    \
    X(ts_less_than_or_equal_to)                 \
    X(ts_less_than)                             \
    X(ts_greater_than_matching)                 \
    X(ts_greater_than_or_equal_to)              \
    X(ts_greater_than)                          \
    X(ts_not_matching)                          \
    X(ts_not_equal_to)                          \
    X(ts_not)                                   \
    X(ts_and_matching)                          \
    X(ts_and)                                   \
    X(ts_or_matching)                           \
    X(ts_or)                                    \
    X(ts_mod)                                   \
    X(ts_let)                                   \
    X(ts_if)                                    \
    X(ts_else)                                  \
    X(ts_while)                                 \
    X(ts_semicolon)                             \
    X(ts_do)                                    \
    X(ts_for)                                   \
    X(ts_one_dot_matching)                      \
    X(ts_member_access)                         \
    X(ts_two_dot_matching)                      \
    X(ts_spread)                                \
    X(ts_question_matching)                     \
    X(ts_optional_chaining)                     \
    X(ts_question)                              \
    X(ts_division_assignment)                   \
    X(ts_minus_matching)                        \
    X(ts_minus_assignment)                      \
    X(ts_minus_minus)                           \
    X(ts_plus_matching)                         \
    X(ts_plus_assignment)                       \
    X(ts_plus_plus)                             \
    X(ts_multiplication_matching)               \
    X(ts_multiplication_assignment)             \
    X(ts_mod_matching)                          \
    X(ts_mod_assignment)                        \
    X(ts_break)                                 \
    X(ts_continue)                              \
    X(ts_function)                              \
    X(ts_return)                                \
    X(ts_in)                                    \
    X(ts_of)                                    \
    X(ts_typeof)                                \
    X(ts_delete)

#define X(name) name,
enum js_token_state { js_token_state_list };
#undef X

// move outside to be a struct, for backup restore token position
struct js_token {
    enum js_token_state stat;
    // src support increasement at run time, may be reallocated, so token position use offset instead of pointer
    size_t h; // source may be reallocated, record offset
    size_t t;
    long line;
    double num;
    // string
    // DONT parse escape here, keep it simple, prevent memory copy and possible memory leak in syntax pasing stage
    // string head is always token head + 1, and length is token length - 2
};

enum js_value_type {
    vt_null,
    vt_boolean,
    vt_number,
    vt_string,
    vt_array,
    vt_object,
    vt_function,
    vt_cfunction
};

struct js;

struct js_value {
    enum js_value_type type;
    union {
        bool boolean;
        double number;
        struct string *string;
        struct list *array;
        struct map *object;
        // forward declaraction can only be pointer, so put js_value behind js_token
        // https://stackoverflow.com/questions/9776391/how-to-solve-the-compiling-error-c2079-on-windows
        struct {
            size_t index; // cached token start location
            struct map *closure;
        } function;
        void (*cfunction)(struct js *);
    } value;
    bool in_use;
};

shared const char *js_value_type_name(struct js_value *val);
shared struct js_value *js_null_new(struct js *pjs);
shared struct js_value *js_boolean_new(struct js *pjs, bool b);
shared struct js_value *js_number_new(struct js *pjs, double n);
shared struct js_value *js_string_new(struct js *pjs, const char *str, size_t len);
static inline struct js_value *js_string_new_sz(struct js *pjs, const char *str) {
    return js_string_new(pjs, str, strlen(str));
}
shared struct js_value *js_array_new(struct js *pjs);
shared void js_array_push(struct js *pjs, struct js_value *arr, struct js_value *val);
shared void js_array_put(struct js *pjs, struct js_value *arr, size_t idx, struct js_value *val);
shared struct js_value *js_array_get(struct js *pjs, struct js_value *arr, size_t idx);
shared struct js_value *js_object_new(struct js *pjs);
shared void js_object_put(struct js *pjs, struct js_value *obj, const char *key, size_t klen, struct js_value *val);
static inline void js_object_put_sz(struct js *pjs, struct js_value *obj, const char *key, struct js_value *val) {
    js_object_put(pjs, obj, key, strlen(key), val);
}
shared struct js_value *js_object_get(struct js *pjs, struct js_value *obj, const char *key, size_t klen);
static inline struct js_value *js_object_get_sz(struct js *pjs, struct js_value *obj, const char *key) {
    return js_object_get(pjs, obj, key, strlen(key));
}
shared struct js_value *js_function_new(struct js *pjs, size_t idx);
shared struct js_value *js_cfunction_new(struct js *pjs, void (*cfunc)(struct js *));
shared void js_value_dump(struct js_value *val);

struct js {
    // source
    char *src;
    size_t src_len;
    size_t src_cap;
    // error
    jmp_buf err_env;
    const char *err_msg;
    const char *err_file;
    long err_line;
    // token
    struct js_token tok;
    int tok_num_sign; // temp data for calculating number
    double tok_num_inte;
    double tok_num_frac;
    double tok_num_frac_depth;
    int tok_num_exp_sign;
    double tok_num_exp;
    // token cache
    struct js_token *tok_cache;
    size_t tok_cache_len;
    size_t tok_cache_cap;
    size_t tok_cache_idx;
    // All parser functions parse only and return NULL when it is false
    bool parse_exec;
    bool mark_break;
    bool mark_continue;
    bool mark_return;
    struct js_value *value_null; // reduce duplicated values
    struct js_value *value_true;
    struct js_value *value_false;
    struct link *values;
    struct list *stack;
    struct js_value *result; // function result CANNOT be put in stack, because 'return' could be inside more stack level inside function
};

struct js_stack_frame {
    size_t descr_h;
    size_t descr_t;
    struct map *variables;
    struct list *parameters;
};
static inline char *js_stack_frame_descr_head(struct js *pjs, struct js_stack_frame *frame) {
    return pjs->src + frame->descr_h;
}
static inline size_t js_stack_frame_descr_len(struct js_stack_frame *frame) {
    return frame->descr_t - frame->descr_h;
}

#define js_try(pjs) (setjmp((pjs)->err_env) == 0)

// DONT use name "message", will conflict with inside macro
#define js_throw(pjs, msg)          \
    do {                            \
        struct js *_pjs = (pjs);    \
        _pjs->err_file = __FILE__;  \
        _pjs->err_line = __LINE__;  \
        _pjs->err_msg = (msg);      \
        log("%s", (msg));           \
        longjmp(_pjs->err_env, -1); \
    } while (0)

shared struct js *js_new();
shared void js_delete(struct js *pjs);
shared void js_load_string(struct js *pjs, const char *p, size_t len);
#define js_load_string_sz(pjs, p)              \
    do {                                       \
        const char *_p = (p);                  \
        js_load_string((pjs), _p, strlen(_p)); \
    } while (0)
shared void js_print_source(struct js *pjs);
shared void js_dump_source(struct js *pjs);
shared void js_dump_tokens(struct js *pjs);
shared void js_dump_values(struct js *pjs);
shared void js_dump_stack(struct js *pjs);
shared void js_collect_garbage(struct js *pjs);
shared struct js_stack_frame *js_stack_frame_new();
shared void js_stack_frame_delete(struct js_stack_frame *frame);
shared void js_stack_push(struct js *pjs, struct js_stack_frame *frame);
shared struct js_stack_frame *js_stack_pop(struct js *pjs);
shared struct js_stack_frame *js_stack_peek(struct js *pjs);
shared void js_stack_forward(struct js *pjs, size_t tok_h, size_t tok_t);
shared void js_stack_backward(struct js *pjs);
shared void js_stack_backward_to(struct js *pjs, size_t level);
shared void js_variable_declare(struct js *pjs, char *name, size_t name_len, struct js_value *val);
#define js_variable_declare_sz(pjs, name, val)                 \
    do {                                                       \
        char *_name = (name);                                  \
        js_variable_declare((pjs), _name, strlen(_name), val); \
    } while (0)
shared void js_variable_erase(struct js *pjs, char *name, size_t name_len);
shared void js_variable_assign(struct js *pjs, char *name, size_t name_len, struct js_value *val);
#define js_variable_assign_sz(pjs, name, val)                    \
    do {                                                         \
        char *_name = (name);                                    \
        js_variable_assign_sz((pjs), _name, strlen(_name), val); \
    } while (0)
shared struct js_value *js_variable_fetch(struct js *pjs, char *name, size_t name_len);
static inline struct js_value *js_variable_fetch_sz(struct js *pjs, char *name) {
    return js_variable_fetch(pjs, name, strlen(name));
}
shared void js_parameter_push(struct js *pjs, struct js_value *param);
shared struct js_value *js_parameter_get(struct js *pjs, size_t idx);
shared size_t js_parameter_length(struct js *pjs);

// js-lexer.c

shared const char *js_token_state_name(enum js_token_state stat);
shared void js_lexer_next_token(struct js *pjs);
shared void js_lexer_print_error(struct js *pjs);

// js-parser.c

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Property_accessors
enum js_value_accessor_type {
    at_value, // only value, no key or index information, can only get, no put
    at_identifier,
    at_array_member,
    at_object_member,
    at_optional_member
};

struct js_value_accessor {
    enum js_value_accessor_type type;
    struct js_value *v; // value or array or object
    char *p; // identifier head, or object key head
    size_t n; // identifier len, or array index, or object key len
};

shared struct js_value *js_parse_value(struct js *pjs);
shared struct js_value *js_parse_array(struct js *pjs);
shared struct js_value *js_parse_object(struct js *pjs);
shared struct js_value *js_parse_expression(struct js *pjs);
shared struct js_value *js_parse_logical_expression(struct js *pjs);
shared struct js_value *js_parse_relational_expression(struct js *pjs);
shared struct js_value *js_parse_additive_expression(struct js *pjs);
shared struct js_value *js_parse_multiplicative_expression(struct js *pjs);
shared struct js_value *js_parse_prefix_expression(struct js *pjs);
shared struct js_value *js_parse_access_call_expression(struct js *pjs);
shared struct js_value_accessor js_parse_accessor(struct js *pjs);
shared void js_parse_assignment_expression(struct js *pjs);
shared void js_parse_declaration_expression(struct js *pjs);
shared void js_parse_script(struct js *pjs);
shared void js_parse_statement(struct js *pjs);
shared void js_parse_function(struct js *pjs);
shared void js_parser_print_error(struct js *pjs);

// js-functions.c

shared void js_print_values(struct js *pjs);

#endif