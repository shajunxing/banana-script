/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_DATA_H
#define JS_DATA_H

#include <stdbool.h>
#include <stdint.h>
#include "js-common.h"

// vt_undefined is used to distinguish between variable not exists and it's value is null, never return to user
#define js_value_type_list                                                                    \
    X(vt_undefined) /* 0 always means empty initial state  */                                 \
    X(vt_null)                                                                                \
    X(vt_boolean)                                                                             \
    X(vt_number)                                                                              \
    X(vt_scripture) /* constant string from c string literal, permently exists */             \
    X(vt_inscription) /* constant string from js source code, identifier or string literal */ \
    X(vt_string) /*managed  */                                                                \
    X(vt_array) /* managed */                                                                 \
    X(vt_object) /* managed */                                                                \
    X(vt_function) /* managed */                                                              \
    X(vt_c_function)

#define X(name) name,
enum js_value_type { js_value_type_list };
#undef X

struct js_managed_value;

struct js_value {
    uint8_t type;
    union {
        bool boolean;
        double number;
        struct {
            char *base;
            uint32_t length;
        } scripture;
        struct {
            uint8_t **base; // may be reallocated, so store pointer address
            uint32_t offset; // may be reallocated, so store offset
            uint32_t length;
        } inscription;
        struct js_managed_value *managed; // string, array, object, function
        void *c_function; // for module isolation, DON'T typedef here
    };
};

struct js_kv_pair {
    struct {
        char *base;
        uint16_t length; // different length with string, DON'T use one struct definition
        uint16_t capacity;
    } key;
    struct js_value value;
};

struct js_variable_map { // for globals, locals, parameters, closure, use uint16_t instead of size_t
    struct js_kv_pair *base;
    uint16_t length;
    uint16_t capacity;
};

struct js_managed_value {
    uint8_t type : 7;
    uint8_t in_use : 1;
    union {
        struct {
            char *base;
            size_t length;
            size_t capacity;
        } string;
        struct {
            struct js_value *base;
            size_t length;
            size_t capacity;
        } array;
        struct {
            struct js_kv_pair *base;
            size_t length;
            size_t capacity;
        } object; // TODO: Add "last visited"? or in higher level such as js_variable_get?
        struct {
            uint32_t ingress; // Caution: egress is NOT fixed
            struct js_variable_map closure;
        } function;
    };
};

struct js_heap {
    struct js_managed_value **base;
    size_t length;
    size_t capacity;
};

// if success, value is return data, or it is error description
struct js_result {
    bool success;
    struct js_value value;
};

// DON'T use conflict name such as 'k' 'v'
#define js_map_for_each(__arg_base, __arg_length, __arg_capacity, __arg_k, __arg_kl, __arg_v, __arg_block) \
    do {                                                                                                   \
        struct js_kv_pair *__base = (__arg_base);                                                          \
        typeof(__arg_capacity) __cap = (__arg_capacity);                                                   \
        for (typeof(__cap) __i = 0; __i < __cap; __i++) {                                                  \
            struct js_kv_pair *__kv = __base + __i;                                                        \
            char *__arg_k = __kv->key.base;                                                                \
            typeof(__kv->key.length) __arg_kl = __kv->key.length;                                          \
            struct js_value *__arg_v = &(__kv->value);                                                     \
            if (__arg_k != NULL && __arg_v->type != 0) {                                                   \
                __arg_block;                                                                               \
            }                                                                                              \
        }                                                                                                  \
    } while (0)

// DON'T use conflict name such as 'list'
#define js_list_for_each(__arg_base, __arg_length, __arg_capacity, __arg_i, __arg_v, __arg_block) \
    buffer_for_each(__arg_base, __arg_length, __arg_capacity, __arg_i, __arg_v, {                 \
        if (__arg_v->type != 0) {                                                                 \
            __arg_block;                                                                          \
        }                                                                                         \
    })

shared void js_map_put_internal(struct js_kv_pair **, size_t *, size_t *, const char *, uint16_t, struct js_value);
// remove '*' prefix, and fit for any type of 'length' 'capacity'
#define js_map_put(__arg_base, __arg_length, __arg_capacity, __arg_key, __arg_key_length, __arg_value) \
    do {                                                                                               \
        size_t __len = __arg_length;                                                                   \
        size_t __cap = __arg_capacity;                                                                 \
        js_map_put_internal(&(__arg_base), &__len, &__cap, __arg_key, __arg_key_length, __arg_value);  \
        __arg_length = (typeof(__arg_length))__len;                                                    \
        __arg_capacity = (typeof(__arg_capacity))__cap;                                                \
    } while (0)
#define js_map_put_sz(__arg_base, __arg_length, __arg_capacity, __arg_key, __arg_value) \
    js_map_put(__arg_base, __arg_length, __arg_capacity, __arg_key, (uint16_t)strlen(__arg_key), __arg_value)
shared struct js_value js_map_get(struct js_kv_pair *, size_t, size_t, const char *, uint16_t);
shared struct js_value js_map_get_sz(struct js_kv_pair *, size_t, size_t, const char *);
// same as js_map_put
#define js_map_free(__arg_base, __arg_length, __arg_capacity)                            \
    do {                                                                                 \
        for (typeof(__arg_capacity) __i = 0; __i < __arg_capacity; __i++) {              \
            struct js_kv_pair *__node = __arg_base + __i;                                \
            if (__node->key.base) {                                                      \
                buffer_free(__node->key.base, __node->key.length, __node->key.capacity); \
            }                                                                            \
        }                                                                                \
        buffer_free(__arg_base, __arg_length, __arg_capacity);                           \
    } while (0)
shared struct js_value js_null();
shared struct js_value js_boolean(bool);
shared struct js_value js_number(double);
shared struct js_value js_scripture(const char *, uint32_t);
shared struct js_value js_scripture_sz(const char *);
shared struct js_value js_inscription(uint8_t **, uint32_t, uint32_t);
shared struct js_value js_string(struct js_heap *, const char *, size_t);
shared struct js_value js_string_sz(struct js_heap *, const char *);
shared struct js_value js_array(struct js_heap *);
shared void js_array_push(struct js_value *, struct js_value);
shared void js_array_put(struct js_value *, size_t, struct js_value);
shared struct js_value js_array_get(struct js_value *, size_t);
shared struct js_value js_object(struct js_heap *);
shared void js_object_put(struct js_value *, const char *, uint16_t, struct js_value);
shared void js_object_put_sz(struct js_value *, const char *, struct js_value);
shared struct js_value js_object_get(struct js_value *, const char *, uint16_t);
shared struct js_value js_object_get_sz(struct js_value *, const char *);
shared struct js_value js_function(struct js_heap *, uint32_t);
shared struct js_value js_c_function(void *);
shared void js_mark(struct js_value *);
shared void js_sweep(struct js_heap *);
shared void js_managed_value_dump(struct js_managed_value *);
shared void js_value_dump(struct js_value *);
shared void js_value_print(struct js_value *);
shared bool js_is_string(struct js_value *);
shared char *js_string_base(struct js_value *);
shared size_t js_string_length(struct js_value *);
shared int js_string_compare(struct js_value *, struct js_value *);
shared struct js_result js_add(struct js_heap *, struct js_value *, struct js_value *);

#ifndef NOTEST

shared int test_data_structure_size(int, char *[]);
shared int test_js_map(int, char *[]);
shared int test_js_map_loop(int, char *[]);
shared int test_js_value(int, char *[]);
shared int test_js_value_loop(int, char *[]);
shared int test_js_value_bug(int, char *[]);
shared int test_js_string_family(int, char *[]);

#endif

#endif