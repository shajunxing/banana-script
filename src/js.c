/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

static struct js_value *_value_new(struct js *pjs) {
    struct js_value *ret = (struct js_value *)calloc(1, sizeof(struct js_value));
    link_push(pjs->values, ret);
    return ret;
}

static void _value_delete(struct js_value *val) {
    switch (val->type) {
    case vt_null:
    case vt_boolean:
        break;
    case vt_number:
        free(val);
        break;
    case vt_string:
        string_delete(val->value.string);
        free(val);
        break;
    case vt_array:
        list_delete(val->value.array);
        free(val);
        break;
    case vt_object:
        map_delete(val->value.object);
        free(val);
        break;
    case vt_function:
        map_delete(val->value.function.closure);
        free(val);
        break;
    case vt_cfunction:
        free(val);
        break;
    default:
        fatal("Unknown value type %d", val->type);
    }
}

const char *js_value_type_name(struct js_value *val) {
    static const char *const names[] = {"null", "boolean", "number", "string", "array", "object", "function", "function"};
    assert(val != NULL);
    return names[val->type];
}

struct js_value *js_null_new(struct js *pjs) {
    return pjs->value_null;
}

struct js_value *js_boolean_new(struct js *pjs, bool b) {
    return b ? pjs->value_true : pjs->value_false;
}

struct js_value *js_number_new(struct js *pjs, double n) {
    struct js_value *ret = _value_new(pjs);
    ret->type = vt_number;
    ret->value.number = n;
    return ret;
}

struct js_value *js_string_new(struct js *pjs, const char *str, size_t len) {
    struct js_value *ret = _value_new(pjs);
    ret->type = vt_string;
    ret->value.string = string_new(str, len);
    return ret;
}

struct js_value *js_array_new(struct js *pjs) {
    struct js_value *ret = _value_new(pjs);
    ret->type = vt_array;
    ret->value.array = list_new();
    return ret;
}

void js_array_push(struct js *pjs, struct js_value *arr, struct js_value *val) {
    assert((arr->type == vt_array));
    list_push(arr->value.array, val->type == vt_null ? NULL : val);
}

void js_array_put(struct js *pjs, struct js_value *arr, size_t idx, struct js_value *val) {
    assert((arr->type == vt_array));
    list_put(arr->value.array, idx, val->type == vt_null ? NULL : val); // prevent useless expand
}

struct js_value *js_array_get(struct js *pjs, struct js_value *arr, size_t idx) {
    struct js_value *ret;
    assert((arr->type == vt_array));
    ret = (struct js_value *)list_get(arr->value.array, idx);
    return ret ? ret : js_null_new(pjs); // replace NULL whth vt_null
}

struct js_value *js_object_new(struct js *pjs) {
    struct js_value *ret = _value_new(pjs);
    ret->type = vt_object;
    ret->value.object = map_new();
    return ret;
}

void js_object_put(struct js *pjs, struct js_value *obj, const char *key, size_t klen, struct js_value *val) {
    assert((obj->type == vt_object));
    map_put(obj->value.object, key, klen, val->type == vt_null ? NULL : val); // prevent useless expand
}

struct js_value *js_object_get(struct js *pjs, struct js_value *obj, const char *key, size_t klen) {
    struct js_value *ret;
    assert((obj->type == vt_object));
    ret = (struct js_value *)map_get(obj->value.object, key, klen);
    return ret ? ret : js_null_new(pjs); // replace NULL whth vt_null
}

struct js_value *js_function_new(struct js *pjs, size_t idx) {
    struct js_value *ret = _value_new(pjs);
    ret->type = vt_function;
    ret->value.function.index = idx;
    ret->value.function.closure = map_new();
    return ret;
}

struct js_value *js_cfunction_new(struct js *pjs, void (*cfunc)(struct js *)) {
    struct js_value *ret = _value_new(pjs);
    ret->type = vt_cfunction;
    ret->value.cfunction = cfunc;
    return ret;
}

void js_value_dump(struct js_value *val) {
    if (!val) {
        printf("NULL"); // shouldn't happen
        return;
    }
    switch (val->type) {
    case vt_null:
        printf("null");
        break;
    case vt_boolean:
        printf(val->value.boolean ? "true" : "false");
        break;
    case vt_number:
        printf("%lg", val->value.number);
        break;
    case vt_string:
        printf("\"%.*s\"", (int)val->value.string->len, val->value.string->p);
        break;
    case vt_array:
        printf("[");
        list_for_each(val->value.array, i, v, {
            printf("%llu:", i);
            js_value_dump((struct js_value *)v);
            printf(",");
        });
        printf("]");
        break;
    case vt_object:
        printf("{");
        map_for_each(val->value.object, k, v, {
            printf("%s:", k->p);
            js_value_dump((struct js_value *)v);
            printf(",");
        });
        printf("}");
        break;
    case vt_function:
        printf("<function{");
        map_for_each(val->value.function.closure, k, v, {
            printf("%s:", k->p);
            js_value_dump((struct js_value *)v);
            printf(",");
        });
        printf("}>");
        break;
    case vt_cfunction:
        printf("<c>");
        break;
    default:
        fatal("Unknown value type %d", val->type);
    }
}

struct js *js_new() {
    struct js *pjs = (struct js *)calloc(1, sizeof(struct js));
    pjs->tok.stat = ts_searching;
    pjs->tok.line = 1;
    buffer_alloc(struct js_token, pjs->tok_cache, pjs->tok_cache_cap, 0);
    // DONT use js_xx_new() because it will add value into pjs->values;
    pjs->value_null = (struct js_value *)calloc(1, sizeof(struct js_value));
    pjs->value_null->type = vt_null;
    pjs->value_true = (struct js_value *)calloc(1, sizeof(struct js_value));
    pjs->value_true->type = vt_boolean;
    pjs->value_true->value.boolean = true;
    pjs->value_false = (struct js_value *)calloc(1, sizeof(struct js_value));
    pjs->value_false->type = vt_boolean;
    pjs->value_false->value.boolean = false;
    pjs->values = link_new();
    pjs->stack = list_new();
    // stack root
    list_push(pjs->stack, js_stack_frame_new());
    // DONT setjmp() here because after this function exit, longjmp() behavior is undefined
    // https://en.cppreference.com/w/c/program/longjmp
    return pjs;
}

void js_delete(struct js *pjs) {
    free(pjs->src);
    free(pjs->tok_cache);
    free(pjs->value_null);
    free(pjs->value_true);
    free(pjs->value_false);
    link_for_each(pjs->values, node, _value_delete((struct js_value *)node->v));
    link_delete(pjs->values);
    list_for_each(pjs->stack, i, v, js_stack_frame_delete((struct js_stack_frame *)v));
    list_delete(pjs->stack);
    free(pjs);
}

void js_load_string(struct js *pjs, const char *p, size_t len) {
    size_t i;
    string_buffer_append(pjs->src, pjs->src_len, pjs->src_cap, p, len);
    for (;;) {
        js_lexer_next_token(pjs);
        if (pjs->tok.stat == ts_end_of_file) {
            break;
        } else if (pjs->tok.stat == ts_block_comment || pjs->tok.stat == ts_line_comment) {
            // remove comment here because in parser, token index always start from 0, can not invoke _next_token() at beginning, so if there are comments in beginning, no way to skip
            continue;
        } else {
            buffer_push(struct js_token, pjs->tok_cache, pjs->tok_cache_len, pjs->tok_cache_cap, pjs->tok);
        }
    }
    for (i = 0; i < pjs->tok_cache_len; i++) {
        log("%s", js_token_state_name(pjs->tok_cache[i].stat));
    }
}

void js_print_source(struct js *pjs) {
    printf("%.*s\n", (int)pjs->src_len, pjs->src);
}

void js_print_statistics(struct js *pjs) {
    printf("values len=%llu\n", pjs->values->len);
    printf("stack len=%llu\n", pjs->stack->len);
}

void js_dump_source(struct js *pjs) {
    printf("source ");
    buffer_dump(char, pjs->src, pjs->src_len, pjs->src_cap);
}

void js_dump_tokens(struct js *pjs) {
    if (pjs->tok_cache_len) {
        printf("token idx=%llu %s\n", pjs->tok_cache_idx,
               js_token_state_name(pjs->tok_cache[pjs->tok_cache_idx].stat));
    }
}

void js_dump_values(struct js *pjs) {
    printf("values len=%llu:\n", pjs->values->len);
    link_for_each(pjs->values, node, {
        printf("    ");
        js_value_dump((struct js_value *)node->v);
        printf("\n");
    });
}

void js_dump_stack(struct js *pjs) {
    printf("stack len=%llu cap=%llu:\n", pjs->stack->len, pjs->stack->cap);
    list_for_each(pjs->stack, level, frame, {
        printf("    %llu %.*s:\n", level, (int)js_stack_frame_descr_len(frame), js_stack_frame_descr_head(pjs, frame));
        printf("        parameters:\n");
        list_for_each(((struct js_stack_frame *)frame)->parameters, index, val, {
            printf("            %llu: ", index);
            js_value_dump(val);
            printf("\n");
        });
        printf("        variables:\n");
        map_for_each(((struct js_stack_frame *)frame)->variables, name, val, {
            printf("            %s = ", name->p);
            js_value_dump(val);
            printf("\n");
        });
    });
}

static void _mark_in_use(struct js_value *val) {
    if (!val->in_use) {
        val->in_use = true;
        if (val->type == vt_array) {
            list_for_each(val->value.array, i, v, _mark_in_use(v));
        } else if (val->type == vt_object) {
            map_for_each(val->value.object, k, v, _mark_in_use(v));
        } else if (val->type == vt_function) {
            map_for_each(val->value.function.closure, k, v, _mark_in_use(v));
        }
    }
}

void js_collect_garbage(struct js *pjs) {
    // mark
    list_for_each(pjs->stack, level, frame, {
        map_for_each(((struct js_stack_frame *)frame)->variables, name, val, _mark_in_use(val));
    });
    // sweep
    link_for_each(pjs->values, node, {
        struct js_value *val = (struct js_value *)(node->v);
        if (val->in_use) {
            val->in_use = false;
        } else {
            _value_delete(val);
            link_delete_node(pjs->values, node);
        }
    });
}

struct js_stack_frame *js_stack_frame_new() {
    struct js_stack_frame *frame = (struct js_stack_frame *)calloc(1, sizeof(struct js_stack_frame));
    frame->variables = map_new();
    frame->parameters = list_new();
    return frame;
}

void js_stack_frame_delete(struct js_stack_frame *frame) {
    map_delete(frame->variables);
    list_delete(frame->parameters);
    free(frame);
}

void js_stack_push(struct js *pjs, struct js_stack_frame *frame) {
    list_push(pjs->stack, frame);
}

struct js_stack_frame *js_stack_pop(struct js *pjs) {
    assert(pjs->stack->len > 1);
    return (struct js_stack_frame *)list_pop(pjs->stack);
}

struct js_stack_frame *js_stack_peek(struct js *pjs) {
    return (struct js_stack_frame *)pjs->stack->p[pjs->stack->len - 1];
}

void js_stack_forward(struct js *pjs, size_t tok_h, size_t tok_t) {
    struct js_stack_frame *frame = js_stack_frame_new();
    frame->descr_h = tok_h;
    frame->descr_t = tok_t;
    js_stack_push(pjs, frame);
    // js_dump_stack(pjs);
}

void js_stack_backward(struct js *pjs) {
    js_stack_frame_delete(js_stack_pop(pjs));
    // js_dump_stack(pjs);
}

void js_stack_backward_to(struct js *pjs, size_t level) {
    assert(level > 0);
    while (pjs->stack->len > level) {
        js_stack_backward(pjs);
    }
}

void js_variable_declare(struct js *pjs, char *name, size_t name_len, struct js_value *val) {
    struct map *variables = js_stack_peek(pjs)->variables;
    // log("%.*s", (int)name_len, name);
    // js_dump_stack(pjs);
    if (map_get(variables, name, name_len)) {
        js_throw(pjs, "Variable name already exists");
    }
    map_put(variables, name, name_len, val);
}

void js_variable_erase(struct js *pjs, char *name, size_t name_len) {
    struct map *variables = js_stack_peek(pjs)->variables;
    // log("%.*s", (int)name_len, name);
    // js_dump_stack(pjs);
    if (map_get(variables, name, name_len)) {
        map_put(variables, name, name_len, NULL);
        return;
    }
    js_throw(pjs, "Variable not found");
}

void js_variable_assign(struct js *pjs, char *name, size_t name_len, struct js_value *val) {
    int level; //  DONT use size_t because if level = 0,  level-- will become large positive number
    // log("%.*s", (int)name_len, name);
    // js_dump_stack(pjs);
    for (level = (int)pjs->stack->len - 1; level >= 0; level--) {
        struct js_stack_frame *frame = (struct js_stack_frame *)pjs->stack->p[level];
        if (map_get(frame->variables, name, name_len)) {
            map_put(frame->variables, name, name_len, val);
            return;
        }
    }
    js_throw(pjs, "Variable not found");
}

struct js_value *js_variable_fetch(struct js *pjs, char *name, size_t name_len) {
    int level;
    // log("%.*s", (int)name_len, name);
    // js_dump_stack(pjs);
    for (level = (int)pjs->stack->len - 1; level >= 0; level--) {
        struct js_stack_frame *frame = (struct js_stack_frame *)pjs->stack->p[level];
        struct js_value *ret = (struct js_value *)map_get(frame->variables, name, name_len);
        // log("level=%d, frame=%p, ret=%p", level, frame, ret);
        if (ret) {
            // log("Variable found");
            return ret;
        }
    }
    log("Variable \"%.*s\" not found", (int)name_len, name);
    js_throw(pjs, "Variable not found");
}

void js_parameter_push(struct js *pjs, struct js_value *param) {
    struct list *parameters = js_stack_peek(pjs)->parameters;
    list_push(parameters, param);
}

struct js_value *js_parameter_get(struct js *pjs, size_t idx) {
    struct js_value *ret;
    struct list *parameters = js_stack_peek(pjs)->parameters;
    ret = (struct js_value *)list_get(parameters, idx);
    return ret ? ret : js_null_new(pjs);
}

size_t js_parameter_length(struct js *pjs) {
    return js_stack_peek(pjs)->parameters->len;
}
