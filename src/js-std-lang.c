/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include "js-std-lang.h"

#define _throw_posix_error(__arg_vm) \
    js_throw(js_string_sz(&(__arg_vm->heap), (const char *)strerror(errno)))

struct js_result js_std_ceil(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    js_return(js_number(ceil(argv->number)));
}

struct js_result js_std_dump_vm(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_dump_vm(vm);
    js_return_null();
}

struct js_result js_std_endswith(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 1);
    js_assert(js_is_string(argv));
    char *lhs = js_get_string_base(argv);
    for (uint16_t i = 1; i < argc; i++) {
        js_assert(js_is_string(argv + i));
        if (ends_with_sz(lhs, js_get_string_base(argv + i))) {
            js_return(js_boolean(true));
        }
    }
    js_return(js_boolean(false));
}

struct js_result js_std_filter(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_function(argv + 1));
    struct js_value ret = js_array(&(vm->heap));
    buffer_for_each(argv->managed->array.base, argv->managed->array.length, argv->managed->array.capacity, i, v, {
        struct js_result result = js_call(vm, argv[1], 1, (struct js_value[]){*v});
        if (!result.success) {
            return result;
        }
        if (result.value.type != vt_boolean) {
            js_throw(js_scripture_sz("Filter function must return boolean"));
        }
        if (result.value.boolean) {
            js_push_array_element(&ret, *v);
        }
    });
    js_return(ret);
}

struct js_result js_std_floor(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    js_return(js_number(floor(argv->number)));
}

struct js_result js_std_format(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(js_is_string(argv));
    struct js_value ret = js_string(&(vm->heap), NULL, 0);
    enum { _searching,
        _expect_left_brace,
        _matching } state = _searching;
    char *vbase = NULL;
    for (char *p = js_get_string_base(argv); p < js_get_string_base(argv) + js_get_string_length(argv); p++) {
        // log_debug("%d %c", state, *p);
        switch (state) {
        case _searching:
            if (*p == '$') {
                state = _expect_left_brace;
            } else {
                string_buffer_append_ch(
                    ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity, *p);
            }
            break;
        case _expect_left_brace:
            js_assert(*p == '{');
            vbase = p + 1;
            state = _matching;
            break;
        case _matching:
            if (*p == '}') {
                uint16_t vlen = (uint16_t)(p - vbase);
                struct js_value val;
                if (isdigit(*vbase)) {
                    uint16_t idx = *vbase - '0';
                    for (char *p = vbase + 1; p < vbase + vlen; p++) {
                        js_assert(isdigit(*p));
                        idx = idx * 10 + (*p - '0');
                    }
                    val = (idx + 1) < argc ? argv[idx + 1] : js_null();
                } else {
                    struct js_result ret = js_get_variable(vm, vbase, vlen);
                    if (!ret.success) {
                        return ret;
                    }
                    val = ret.value;
                }
                struct print_stream out = {.type = string_stream};
                js_serialize_value(&out, tostring_style, &val, 0);
                string_buffer_append(
                    ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
                    out.base, out.length);
                free_stream(&out);
                state = _searching;
            }
            break;
        default:
            break;
        }
    }
    js_assert(state == _searching); // test if correctly ended with '}'
    js_return(ret);
}

struct js_result js_std_gc(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_gc(vm);
    js_return_null();
}

struct js_result js_std_join(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_string(argv + 1));
    struct js_value ret = js_string(&(vm->heap), NULL, 0);
    for (size_t i = 0; i < argv->managed->array.length; i++) {
        // be careful of vt_undefined
        struct js_value *elem = argv->managed->array.base + i;
        js_assert(js_is_string(elem));
        if (i > 0) {
            string_buffer_append(
                ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
                js_get_string_base(argv + 1), js_get_string_length(argv + 1));
        }
        string_buffer_append(
            ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
            js_get_string_base(elem), js_get_string_length(elem));
    }
    js_return(ret);
}

struct js_result js_std_length(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    if (argc != 1) {
        js_throw(js_scripture_sz("Require exactly one argument"));
    }
    if (argv->type == vt_array) {
        js_return(js_number((double)argv->managed->array.length));
    } else if (argv->type == vt_object) {
        js_return(js_number((double)argv->managed->object.length));
    } else if (js_is_string(argv)) {
        js_return(js_number((double)js_get_string_length(argv)));
    } else {
        js_throw(js_scripture_sz("Only string, array and object have length"));
    }
}

struct js_result js_std_map(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_function(argv + 1));
    struct js_value ret = js_array(&(vm->heap));
    buffer_for_each(argv->managed->array.base, argv->managed->array.length, argv->managed->array.capacity, i, v, {
        struct js_result result = js_call(vm, argv[1], 1, (struct js_value[]){*v});
        if (!result.success) {
            return result;
        }
        js_push_array_element(&ret, result.value);
    });
    js_return(ret);
}

static void _append_capture(struct js_vm *vm, struct js_value *arr, struct re_capture *cap) {
    if (!cap) {
        return;
    }
    if (cap->head && cap->tail) {
        js_push_array_element(arr, js_string(&(vm->heap), cap->head, cap->tail - cap->head));
    }
    buffer_for_each(cap->subs.base, cap->subs.length, cap->subs.capacity,
        i, c, _append_capture(vm, arr, c));
}

struct js_result js_std_match(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(js_is_string(argv));
    js_assert(js_is_string(argv + 1));
    struct re_capture cap = {0};
    struct js_value ret;
    if (re_match(js_get_string_base(argv), js_get_string_base(argv + 1), &cap)) {
        ret = js_array(&(vm->heap));
        _append_capture(vm, &ret, &cap);
    } else {
        ret = js_null();
    }
    re_free_capture(&cap);
    js_return(ret);
}

struct js_result js_std_modf(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    double inte = 0;
    double frac = modf(argv->number, &inte);
    struct js_value ret = js_array(&(vm->heap));
    js_push_array_element(&ret, js_number(inte));
    js_push_array_element(&ret, js_number(frac));
    js_return(ret);
}

struct js_result js_std_natural_compare(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(js_is_string(argv));
    js_assert(js_is_string(argv + 1));
    js_return(js_number(natural_compare_sz(js_get_string_base(argv), js_get_string_base(argv + 1))));
}

struct js_result js_std_pop(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_array);
    js_assert(argv->managed->array.length > 0);
    argv->managed->array.length--;
    js_return(argv->managed->array.base[argv->managed->array.length]);
}

struct js_result js_std_push(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_push_array_element(argv, argv[1]);
    js_return_null();
}

struct js_result js_std_reduce(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_function(argv + 1));
    struct js_value ret = js_null();
    buffer_for_each(argv->managed->array.base, argv->managed->array.length, argv->managed->array.capacity, i, v, {
        if (i == 0) {
            ret = *v;
        } else {
            struct js_result result = js_call(vm, argv[1], 2, (struct js_value[]){ret, *v});
            if (!result.success) {
                return result;
            }
            ret = result.value;
        }
    });
    js_return(ret);
}

struct js_result js_std_round(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    js_return(js_number(round(argv->number)));
}

struct _comparator_context {
    struct js_vm *vm;
    struct js_value *func;
};

// mingw is also windows style
#ifdef _WIN32
static int _comparator(void *ctx, const void *lhs, const void *rhs) {
#else
static int _comparator(const void *lhs, const void *rhs, void *ctx) {
#endif
    struct _comparator_context *comp_ctx = (struct _comparator_context *)ctx;
    struct js_result result = js_call(
        comp_ctx->vm, *(comp_ctx->func), 2,
        (struct js_value[]){*((struct js_value *)lhs), *((struct js_value *)rhs)});
    if (!result.success || result.value.type != vt_number) {
        return 0;
    } else {
        return (int)result.value.number;
    }
}

struct js_result js_std_sort(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_function(argv + 1));
    struct _comparator_context ctx = {.vm = vm, .func = argv + 1};
#ifdef _WIN32
    qsort_s(argv->managed->array.base, argv->managed->array.length,
        sizeof(struct js_value), _comparator, &ctx);
#else
    qsort_r(argv->managed->array.base, argv->managed->array.length,
        sizeof(struct js_value), _comparator, &ctx);
#endif
    js_return(*argv);
}

struct js_result js_std_split(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(js_is_string(argv));
    if (argc == 1) {
        struct js_value ret = js_array(&(vm->heap));
        js_push_array_element(&ret, argv[0]);
        js_return(ret);
    } else {
        js_assert(js_is_string(argv + 1));
        // DON'T use strtok, will modify source
        char *str = js_get_string_base(argv);
        size_t slen = js_get_string_length(argv);
        char *delim = js_get_string_base(argv + 1);
        size_t dlen = js_get_string_length(argv + 1);
        struct js_value ret = js_array(&(vm->heap));
        if (dlen == 0) {
            for (size_t i = 0; i < slen; i++) {
                js_push_array_element(&ret, js_string(&(vm->heap), str + i, 1));
            }
        } else {
            char *p, *q;
            for (p = str; p < str + slen;) {
                q = strstr(p, delim);
                // log_debug("%s %s", p, q);
                // see https://en.cppreference.com/w/c/string/byte/strstr.html for return value
                // TODO: optimize for scripture?
                // if delim is an empty string, acorrding to standard, cannot distinguish with match on first character, so empty string is forbidden here
                if (q == NULL) {
                    js_push_array_element(&ret, js_string(&(vm->heap), p, str + slen - p));
                    break;
                } else {
                    js_push_array_element(&ret, js_string(&(vm->heap), p, q - p));
                    p = q + dlen;
                }
            }
            if (q != NULL) {
                js_push_array_element(&ret, js_scripture_sz(""));
            }
        }
        js_return(ret);
    }
}

struct js_result js_std_startswith(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 1);
    js_assert(js_is_string(argv));
    char *lhs = js_get_string_base(argv);
    for (uint16_t i = 1; i < argc; i++) {
        js_assert(js_is_string(argv + i));
        if (starts_with_sz(lhs, js_get_string_base(argv + i))) {
            js_return(js_boolean(true));
        }
    }
    js_return(js_boolean(false));
}

// _serialize already inuse in msvc, "'_serialize': intrinsic function, cannot be defined"
static struct js_result __serialize(struct js_vm *vm, uint16_t argc, struct js_value *argv, enum serialized_style to) {
    js_assert(argc == 1);
    struct print_stream out = {.type = string_stream};
    js_serialize_value(&out, to, argv, 0);
    struct js_value ret = js_alloc_managed(&(vm->heap), vt_string);
    ret.managed->string.base = out.base;
    ret.managed->string.length = out.length;
    ret.managed->string.capacity = out.capacity;
    js_return(ret);
}

struct js_result js_std_todump(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    return __serialize(vm, argc, argv, todump_style);
}

struct js_result js_std_tojson(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    return __serialize(vm, argc, argv, tojson_style);
}

struct js_result js_std_tolower(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    struct js_value ret = js_string(&(vm->heap), js_get_string_base(argv), js_get_string_length(argv));
    char *base = js_get_string_base(&ret);
    for (char *p = base; p < base + js_get_string_length(&ret); p++) {
        *p = (char)tolower(*p);
    }
    js_return(ret);
}

struct js_result js_std_tonumber(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    char *str = js_get_string_base(argv);
    char *end;
    errno = 0; // fix windows bug: won't reset last error such as ERANGE, linux is ok
    double num = strtod(str, &end);
    if (errno) {
        _throw_posix_error(vm);
    }
    if (end == str || *end != '\0') {
        js_throw(js_scripture_sz("Not valid number"));
    }
    js_return(js_number(num));
}

struct js_result js_std_tostring(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    return __serialize(vm, argc, argv, tostring_style);
}

struct js_result js_std_toupper(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    struct js_value ret = js_string(&(vm->heap), js_get_string_base(argv), js_get_string_length(argv));
    char *base = js_get_string_base(&ret);
    for (char *p = base; p < base + js_get_string_length(&ret); p++) {
        *p = (char)toupper(*p);
    }
    js_return(ret);
}

struct js_result js_std_trunc(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    js_return(js_number(trunc(argv->number)));
}

void js_declare_std_lang_functions(struct js_vm *vm) {
    js_declare_std_function(ceil);
    js_declare_std_function(dump_vm);
    js_declare_std_function(endswith);
    js_declare_std_function(filter);
    js_declare_std_function(floor);
    js_declare_std_function(format);
    js_declare_std_function(gc);
    js_declare_std_function(join);
    js_declare_std_function(length);
    js_declare_std_function(map);
    js_declare_std_function(match);
    js_declare_std_function(modf);
    js_declare_std_function(natural_compare);
    js_declare_std_function(pop);
    js_declare_std_function(push);
    js_declare_std_function(reduce);
    js_declare_std_function(round);
    js_declare_std_function(sort);
    js_declare_std_function(split);
    js_declare_std_function(startswith);
    js_declare_std_function(todump);
    js_declare_std_function(tojson);
    js_declare_std_function(tolower);
    js_declare_std_function(tonumber);
    js_declare_std_function(tostring);
    js_declare_std_function(toupper);
    js_declare_std_function(trunc);
}