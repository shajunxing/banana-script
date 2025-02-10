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

// https://www.geeksforgeeks.org/the-offsetof-macro/
#if !defined(offsetof)
    #define offsetof(type, element) ((size_t) & (((type *)0)->element))
#endif

#define fatal(fmt, ...)                                                             \
    do {                                                                            \
        printf("%s:%d: Fatal error: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        exit(EXIT_FAILURE);                                                         \
    } while (0)

#if defined assert
    #undef assert
#endif

#define assert(cond)                                                            \
    do {                                                                        \
        if (!(cond)) {                                                          \
            printf("%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #cond); \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

// js-buffer.c

shared char *read_file(const char *filename, size_t *plen);
shared char *read_line(FILE *fp, size_t *plen);
shared const char *ascii_abbreviation(int ch);
shared void print_hex(void *p, size_t len);

#define alloc(type, len) ((type *)calloc((len), sizeof(type)))

// buffer's capacity is always pow of 2
#define buffer_alloc(type, p, len, cap, reqcap)                                        \
    do {                                                                               \
        size_t _reqcap = (reqcap);                                                     \
        if (_reqcap > (cap)) {                                                         \
            size_t _newcap = 1;                                                        \
            for (_newcap = (cap) == 0 ? 1 : (cap); _reqcap > _newcap; _newcap <<= 1) { \
                assert(_newcap > 0);                                                   \
            }                                                                          \
            if (p) {                                                                   \
                (p) = (type *)realloc((p), _newcap * sizeof(type));                    \
                assert((p) != NULL);                                                   \
                memset((p) + (cap), 0, (_newcap - (cap)) * sizeof(type));              \
            } else {                                                                   \
                (p) = (type *)calloc(_newcap, sizeof(type));                           \
                assert((p) != NULL);                                                   \
            }                                                                          \
            (cap) = _newcap;                                                           \
        }                                                                              \
    } while (0)

// NULL pointer can be safely passed to free()
// https://en.cppreference.com/w/c/memory/free
#define buffer_free(type, p, len, cap) \
    do {                               \
        free(p);                       \
        (p) = NULL;                    \
        (len) = 0;                     \
        (cap) = 0;                     \
    } while (0)

// DONT surround 'type' with parentheses, or will get "syntax error : ')'"
#define buffer_push(type, p, len, cap, v)               \
    do {                                                \
        size_t _newlen = (len) + 1;                     \
        buffer_alloc(type, (p), (len), (cap), _newlen); \
        (p)[(len)] = v;                                 \
        (len) = _newlen;                                \
    } while (0)

#define buffer_put(type, p, len, cap, i, v)                 \
    do {                                                    \
        size_t _newlen = (i) + 1;                           \
        if (_newlen > (len)) {                              \
            buffer_alloc(type, (p), (len), (cap), _newlen); \
            (len) = _newlen;                                \
        }                                                   \
        (p)[(i)] = v;                                       \
    } while (0)

// use "clear" instead of "empty", to avoid conflit meaning "is empty", refer C++ string
#define buffer_clear(type, p, len, cap)       \
    do {                                      \
        memset((p), 0, (cap) * sizeof(type)); \
        (len) = 0;                            \
    } while (0)

// https://stackoverflow.com/questions/2524611/how-can-one-print-a-size-t-variable-portably-using-the-printf-family
#define buffer_dump(type, p, len, cap)                      \
    do {                                                    \
        type *_p = (p);                                     \
        size_t _len = (len);                                \
        size_t _cap = (cap);                                \
        printf("p=%p len=%llu cap=%llu\n", _p, _len, _cap); \
        print_hex(_p, _cap * sizeof(type));                 \
    } while (0)

#define string_buffer_clear(str, len, cap) buffer_clear(char, (str), (len), (cap))

#define string_buffer_append(str, len, cap, s, l)            \
    do {                                                     \
        size_t _len = (len);                                 \
        size_t _newlen = _len + (l);                         \
        buffer_alloc(char, (str), _len, (cap), _newlen + 1); \
        memcpy((str) + _len, (s), (l));                      \
        (len) = _newlen;                                     \
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

struct link_head {
    struct link_head *owner;
    struct link_head *next;
    struct link_head *prev;
};

shared void link_push(struct link_head *ptr, size_t *len, struct link_head *val);
#define link_for_each(p, v, block)                              \
    do {                                                        \
        struct link_head *_v = (struct link_head *)((p)->next); \
        while (_v) {                                            \
            struct link_head *v = _v;                           \
            _v = _v->next;                                      \
            block;                                              \
        }                                                       \
    } while (0)
shared void link_remove(struct link_head *ptr, size_t *len, struct link_head *val);
shared void link_dump(struct link_head *ptr, size_t *len);

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
    X(ts_exponentiation_matching)               \
    X(ts_exponentiation_assignment)             \
    X(ts_exponentiation)                        \
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

/*
chatgpt's answer:

does "scripture" more unchangable than "inscription"?
ChatGPT said:
ChatGPT

Yes, "scripture" generally implies something more unchangeable than "inscription."

    Scripture refers to sacred writings, often seen as divinely inspired and permanent. It is typically considered unalterable over time, especially in religious contexts where it holds authoritative and eternal truth.

    Inscription refers to text that is carved or written onto something, but it doesn't necessarily carry the same weight of unchangeable truth. While an inscription can be permanent in a physical sense, it doesn't inherently imply that the content is as sacred or immutable as scripture.

In short, "scripture" carries a stronger connotation of unchanging and sacred text than "inscription." Would you like to go with scripture for your variable name?
You said:
maybe, it looks nice
ChatGPT said:
ChatGPT

Great choice! scripture has a strong, meaningful vibe for a text string that's unchanging. If you need help with anything else, feel free to ask!
*/

enum js_value_type {
    vt_undefined = 0, // only means not exists, because 0 always means empty state, won't return to user
    vt_null,
    vt_boolean,
    vt_number,
    vt_scripture, // constant string from c string literal, permently exists
    vt_inscription, // constant string from js source code, identifier or string literal
    vt_string, // managed string
    vt_array,
    vt_object,
    vt_function,
    vt_c_function
};

struct js;
struct js_managed_string;
struct js_managed_array;
struct js_managed_object;
struct js_managed_function;

struct js_value {
    enum js_value_type type;
    union {
        bool boolean;
        double number;
        struct {
            const char *p;
            size_t len;
        } scripture;
        struct {
            size_t h; // may be reallocated, so store offset
            size_t len;
        } inscription;
        struct js_managed_string *string;
        struct js_managed_array *array;
        struct js_managed_object *object;
        struct js_managed_function *function;
        void (*c_function)(struct js *);
    } value;
};

struct js_managed_head {
    struct link_head h;
    bool in_use;
    enum js_value_type type;
};

struct js_managed_string {
    struct js_managed_head h;
    char *p;
    size_t len;
    size_t cap;
};

struct js_managed_array {
    struct js_managed_head h;
    struct js_value *p;
    size_t len;
    size_t cap;
};

struct js_key_value {
    struct {
        char *p;
        size_t len;
        size_t cap;
    } k;
    struct js_value v;
};

struct js_managed_object {
    struct js_managed_head h;
    struct js_key_value *p;
    size_t len;
    size_t cap;
};

struct js_managed_function {
    struct js_managed_head h;
    size_t index; // start location
    struct {
        struct js_key_value *p;
        size_t len;
        size_t cap;
    } closure; // closure
};

// js-value.c

shared void js_value_map_put(struct js_key_value **p, size_t *len, size_t *cap, const char *key, size_t klen, struct js_value val);
shared void js_value_map_put_sz(struct js_key_value **p, size_t *len, size_t *cap, const char *key, struct js_value val);
shared struct js_value js_value_map_get(struct js_key_value *p, size_t cap, const char *key, size_t klen);
shared struct js_value js_value_map_get_sz(struct js_key_value *p, size_t cap, const char *key);
// DONT use conflict name such as 'k' 'v'
#define js_value_map_for_each(_p, cap, _k, _kl, _v, block) \
    do {                                                   \
        struct js_key_value *__p = (_p);                   \
        size_t _cap = (cap);                               \
        size_t _i;                                         \
        for (_i = 0; _i < _cap; _i++) {                    \
            struct js_key_value *_kv = __p + _i;           \
            char *_k = _kv->k.p;                           \
            size_t _kl = _kv->k.len;                       \
            struct js_value _v = _kv->v;                   \
            if (_k != NULL && _v.type != 0) {              \
                block;                                     \
            }                                              \
        }                                                  \
    } while (0)
shared void js_value_map_free(struct js_key_value **p, size_t *len, size_t *cap);
shared void js_value_map_dump(struct js_key_value *p, size_t len, size_t cap);
// DONT use conflict name such as 'list'
#define js_value_list_for_each(p, len, i, v, block) \
    do {                                            \
        struct js_value *_p = (p);                  \
        size_t _len = (len);                        \
        size_t i;                                   \
        for (i = 0; i < _len; i++) {                \
            struct js_value v = _p[i];              \
            if (v.type != 0) {                      \
                block;                              \
            }                                       \
        }                                           \
    } while (0)
shared struct js_value js_undefined();
shared struct js_value js_null();
shared struct js_value js_boolean(bool b);
shared struct js_value js_number(double d);
shared struct js_value js_scripture(const char *p, size_t len);
shared struct js_value js_scripture_sz(const char *sz);
shared struct js_value js_inscription(size_t h, size_t len);
shared struct js_value js_string(struct js *pjs, const char *str, size_t len);
shared struct js_value js_string_sz(struct js *pjs, const char *str);
shared struct js_value js_array(struct js *pjs);
shared void js_array_push(struct js *pjs, struct js_value *arr, struct js_value val);
shared void js_array_put(struct js *pjs, struct js_value *arr, size_t idx, struct js_value val);
shared struct js_value js_array_get(struct js *pjs, struct js_value *arr, size_t idx);
shared struct js_value js_object(struct js *pjs);
shared void js_object_put(struct js *pjs, struct js_value *obj, const char *key, size_t klen, struct js_value val);
shared void js_object_put_sz(struct js *pjs, struct js_value *obj, const char *key, struct js_value val);
shared struct js_value js_object_get(struct js *pjs, struct js_value *obj, const char *key, size_t klen);
shared struct js_value js_object_get_sz(struct js *pjs, struct js_value *obj, const char *key);
shared struct js_value js_function(struct js *pjs, size_t idx);
shared struct js_value js_c_function(struct js *pjs, void (*cfunc)(struct js *));
shared const char *js_value_type_name(enum js_value_type type);
shared void js_value_dump(struct js *pjs, struct js_value val);

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
    struct link_head heap; // managed values
    size_t heap_len;
    struct js_call_stack_frame *call_stack;
    size_t call_stack_len;
    size_t call_stack_cap;
    size_t *call_stack_depths; // backup and restore position for 'break' 'continue' 'try-catch'
    size_t call_stack_depths_len;
    size_t call_stack_depths_cap;
    struct js_value result; // function result CANNOT be put in call_stack, because 'return' could be inside more call_stack depth inside function
    struct js_value *eval_stack;
    size_t eval_stack_len;
    size_t eval_stack_cap;
    struct js_bytecode *bytecodes;
    size_t bytecodes_len;
    size_t bytecodes_cap;
    size_t bytecodes_idx;
    char *tablet; // store constant strings;
    size_t tablet_len;
    size_t tablet_cap;
};

#define js_call_stack_frame_type_list \
    X(cs_root)                        \
    X(cs_block)                       \
    X(cs_for)                         \
    X(cs_function)

#define X(name) name,
enum js_call_stack_frame_type { js_call_stack_frame_type_list };
#undef X

struct js_call_stack_frame {
    enum js_call_stack_frame_type type;
    struct js_key_value *vars; // variables map
    size_t vars_len;
    size_t vars_cap;
    struct js_value *params; // parameters list
    size_t params_len;
    size_t params_cap;
    size_t ret_addr; // function stack return address
};

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
shared void js_collect_garbage(struct js *pjs);
shared void js_load_string(struct js *pjs, const char *p, size_t len);
shared void js_load_string_sz(struct js *pjs, const char *p);
shared void js_print_source(struct js *pjs);
shared void js_print_statistics(struct js *pjs);
shared void js_dump_source(struct js *pjs);
shared void js_dump_heap(struct js *pjs);
shared void js_dump_call_stack(struct js *pjs);
shared void js_call_stack_frame_clear(struct js_call_stack_frame *frame);
shared struct js_call_stack_frame *js_call_stack_peek(struct js *pjs);
shared void js_call_stack_push(struct js *pjs, enum js_call_stack_frame_type type, size_t ret_addr);
shared void js_call_stack_pop(struct js *pjs);
shared void js_call_stack_backup(struct js *pjs);
shared void js_call_stack_restore(struct js *pjs);
shared void js_variable_declare(struct js *pjs, char *name, size_t name_len, struct js_value val);
shared void js_variable_declare_sz(struct js *pjs, char *name, struct js_value val);
shared void js_variable_delete(struct js *pjs, char *name, size_t name_len);
shared void js_variable_delete_sz(struct js *pjs, char *name);
shared void js_variable_put(struct js *pjs, char *name, size_t name_len, struct js_value val);
shared void js_variable_put_sz(struct js *pjs, char *name, struct js_value val);
shared struct js_value js_variable_get(struct js *pjs, char *name, size_t name_len);
shared struct js_value js_variable_get_sz(struct js *pjs, char *name);
shared void js_parameter_push(struct js *pjs, struct js_value param);
shared struct js_value js_parameter_get(struct js *pjs, size_t idx);
shared size_t js_parameter_length(struct js *pjs);

// after op_member_put, will leave final array/object on eval stack, because it may be r-value, but if it is l-value, it is useless any more, so after each statement, eval stack should be cleared.

#define js_opcode_list             \
    X(op_value)                    \
    X(op_array)                    \
    X(op_object)                   \
    X(op_variable_declare)         \
    X(op_variable_delete)          \
    X(op_variable_put)             \
    X(op_variable_get)             \
    X(op_member_put)               \
    X(op_member_get)               \
    X(op_array_push)               \
    X(op_array_spread)             \
    X(op_object_optional)          \
    X(op_eval_stack_duplicate)     \
    X(op_eval_stack_pop)           \
    X(op_eval_stack_clear)         \
    X(op_add)                      \
    X(op_sub)                      \
    X(op_mul)                      \
    X(op_pow)                      \
    X(op_div)                      \
    X(op_mod)                      \
    X(op_eq)                       \
    X(op_ne)                       \
    X(op_lt)                       \
    X(op_le)                       \
    X(op_gt)                       \
    X(op_ge)                       \
    X(op_and)                      \
    X(op_or)                       \
    X(op_not)                      \
    X(op_ternary)                  \
    X(op_typeof)                   \
    X(op_jump)                     \
    X(op_jump_if_false)            \
    X(op_call_stack_push_block)    \
    X(op_call_stack_push_for)      \
    X(op_call_stack_push_function) \
    X(op_call_stack_pop)           \
    X(op_call_stack_backup)        \
    X(op_call_stack_restore)       \
    X(op_for_in)                   \
    X(op_for_of)                   \
    X(op_nop)                      \
    X(op_function)                 \
    X(op_parameter_get)            \
    X(op_parameter_push)           \
    X(op_parameter_spread)         \
    X(op_call)                     \
    X(op_return)                   \
    X(op_get_result)

#define X(name) name,
enum js_opcode { js_opcode_list };
#undef X

// DONT put array/object into op_value, because they will be garbage collected!!!
struct js_bytecode {
    enum js_opcode opcode;
    struct js_value operand;
};

shared void js_evaluation_stack_push(struct js *pjs, struct js_value item);
shared void js_evaluation_stack_pop(struct js *pjs, size_t count);
shared void js_evaluation_stack_clear(struct js *pjs);
shared struct js_value *js_evaluation_stack_peek(struct js *pjs, intptr_t inv);
shared struct js_bytecode js_bytecode(enum js_opcode op, ...);
shared size_t js_add_bytecode(struct js *pjs, struct js_bytecode bytecode);
shared void js_dump_bytecode(struct js *pjs, struct js_bytecode *pcode);
shared void js_dump_bytecodes(struct js *pjs);
shared void js_dump_evaluation_stack(struct js *pjs);
shared void js_dump_tablet(struct js *pjs);
shared size_t js_inscribe_tablet(struct js *pjs, char *p, size_t len);

// js-lexer.c

shared const char *js_token_state_name(enum js_token_state stat);
shared void js_next_token(struct js *pjs);
shared void js_token_dump(struct js *pjs);
shared void js_print_error(struct js *pjs);

// js-parser.c

shared void js_parse_expression(struct js *pjs);
shared void js_parse_script(struct js *pjs);

// js-interpreter.c

shared void js_interpret(struct js *pjs);

// js-functions.c

shared void js_c_print(struct js *pjs);
shared void js_c_clock(struct js *pjs);

#endif