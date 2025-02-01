/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

struct js *js_new() {
    struct js *pjs = (struct js *)calloc(1, sizeof(struct js));
    pjs->tok.stat = ts_searching;
    pjs->tok.line = 1;
    // stack root
    js_stack_forward(pjs, 0, 0);
    // DONT setjmp() here because after this function exit, longjmp() behavior is undefined
    // https://en.cppreference.com/w/c/program/longjmp
    return pjs;
}

void _free_managed(struct js_managed_head *mh) {
    switch (mh->type) {
    case vt_string:
        free(((struct js_managed_string *)mh)->p);
        free(mh);
        break;
    case vt_array:
        free(((struct js_managed_array *)mh)->p);
        free(mh);
        break;
    case vt_object: {
        struct js_managed_object *mo = (struct js_managed_object *)mh;
        js_value_map_free(&(mo->p), &(mo->len), &(mo->cap));
        free(mh);
        break;
    }
    case vt_function: {
        struct js_managed_function *mf = (struct js_managed_function *)mh;
        js_value_map_free(&(mf->closure.p), &(mf->closure.len), &(mf->closure.cap));
        free(mh);
        break;
    }
    default:
        fatal("Illegal managed type \"%s\"", js_value_type_name(mh->type));
        break;
    }
}

void js_delete(struct js *pjs) {
    int level;
    free(pjs->src);
    free(pjs->tok_cache);
    link_for_each(&(pjs->heap), v, {
        link_remove(&(pjs->heap), &(pjs->heap_len), v);
        _free_managed((struct js_managed_head *)v);
    });
    for (level = 0; level < pjs->stack_len; level++) {
        js_stack_frame_clear(pjs->stack + level);
    }
    free(pjs->stack);
    free(pjs);
}

static void _mark_in_use(struct js_value val) {
    switch (val.type) {
    case vt_string:
        if (!val.value.string->h.in_use) {
            val.value.string->h.in_use = true;
        }
        break;
    case vt_array:
        if (!val.value.array->h.in_use) {
            val.value.array->h.in_use = true;
            js_value_list_for_each(val.value.array->p, val.value.array->len, i, v, {
                _mark_in_use(v);
            });
        }
        break;
    case vt_object:
        if (!val.value.object->h.in_use) {
            val.value.object->h.in_use = true;
            js_value_map_for_each(val.value.object->p, val.value.object->cap, k, kl, v, {
                // https://stackoverflow.com/questions/1486904/how-do-i-best-silence-a-warning-about-unused-variables
                (void)k;
                (void)kl;
                _mark_in_use(v);
            });
        }
        break;
    case vt_function:
        if (!val.value.function->h.in_use) {
            val.value.function->h.in_use = true;
            js_value_map_for_each(val.value.function->closure.p, val.value.function->closure.cap, k, kl, v, {
                (void)k;
                (void)kl;
                _mark_in_use(v);
            });
        }
        break;
    default:
        break;
    }
}

void js_collect_garbage(struct js *pjs) {
    size_t level;
    // mark
    for (level = 0; level < pjs->stack_len; level++) {
        struct js_stack_frame *frame = pjs->stack + level;
        js_value_map_for_each(frame->vars, frame->vars_cap, k, kl, v, {
            (void)k;
            (void)kl;
            _mark_in_use(v);
        });
    }
    // sweep
    link_for_each(&pjs->heap, v, {
        struct js_managed_head *mh = (struct js_managed_head *)v;
        if (mh->in_use) {
            mh->in_use = false;
        } else {
            link_remove(&(pjs->heap), &(pjs->heap_len), v);
            _free_managed(mh);
        }
    });
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

void js_load_string_sz(struct js *pjs, const char *p) {
    js_load_string(pjs, p, strlen(p));
}

void js_print_source(struct js *pjs) {
    printf("%.*s\n", (int)pjs->src_len, pjs->src);
}

void js_print_statistics(struct js *pjs) {
    printf("heap len=%llu\n", pjs->heap_len);
    printf("stack len=%llu\n", pjs->stack_len);
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

void js_dump_heap(struct js *pjs) {
    printf("heap len=%llu:\n", pjs->heap_len);
    link_for_each(&(pjs->heap), v, {
        enum js_value_type type = ((struct js_managed_head *)v)->type;
        printf("    ");
        switch (type) {
        case vt_string: {
            struct js_managed_string *ms = (struct js_managed_string *)v;
            printf("\"%.*s\"", (int)ms->len, ms->p);
            break;
        }
        case vt_array: {
            struct js_managed_array *ma = (struct js_managed_array *)v;
            printf("[");
            js_value_list_for_each(ma->p, ma->len, i, v, {
                printf("%llu:", i);
                js_value_dump(v);
                printf(",");
            });
            printf("]");
            break;
        }
        case vt_object: {
            struct js_managed_object *mo = (struct js_managed_object *)v;
            printf("{");
            js_value_map_for_each(mo->p, mo->cap, k, kl, v, {
                printf("%.*s:", (int)kl, k);
                js_value_dump(v);
                printf(",");
            });
            printf("}");
            break;
        }
        case vt_function: {
            struct js_managed_function *mf = (struct js_managed_function *)v;
            printf("<function{");
            js_value_map_for_each(mf->closure.p, mf->closure.cap, k, kl, v, {
                printf("%.*s:", (int)kl, k);
                js_value_dump(v);
                printf(",");
            });
            printf("}>");
            break;
        }
        default:
            fatal("Unknown managed type \"%s\"", js_value_type_name(type));
            break;
        }
        printf("\n");
    });
}

static char *_frame_descr_head(struct js *pjs, struct js_stack_frame *frame) {
    return pjs->src + frame->descr_h;
}

static size_t _frame_descr_len(struct js_stack_frame *frame) {
    return frame->descr_t - frame->descr_h;
}

void js_dump_stack(struct js *pjs) {
    size_t level;
    printf("stack len=%llu cap=%llu:\n", pjs->stack_len, pjs->stack_cap);
    for (level = 0; level < pjs->stack_len; level++) {
        struct js_stack_frame *frame = pjs->stack + level;
        printf("    %llu. descr=%.*s:\n", level, (int)_frame_descr_len(frame), _frame_descr_head(pjs, frame));
        printf("        parameters len=%llu cap=%llu:\n", frame->params_len, frame->params_cap);
        js_value_list_for_each(frame->params, frame->params_len, i, v, {
            printf("            %llu: ", i);
            js_value_dump(v);
            printf("\n");
        });
        printf("        variables len=%llu cap=%llu:\n", frame->vars_len, frame->vars_cap);
        js_value_map_for_each(frame->vars, frame->vars_cap, k, kl, v, {
            printf("            %.*s = ", (int)kl, k);
            js_value_dump(v);
            printf("\n");
        });
    }
}

void js_stack_frame_clear(struct js_stack_frame *frame) {
    js_value_map_free(&(frame->vars), &(frame->vars_len), &(frame->vars_cap));
    free(frame->params);
}

// return pointer, DONT return a copy, because modify copy's such as 'len' 'cap' are useless
struct js_stack_frame *js_stack_peek(struct js *pjs) {
    return pjs->stack + pjs->stack_len - 1;
}

void js_stack_forward(struct js *pjs, size_t tok_h, size_t tok_t) {
    struct js_stack_frame frame = {tok_h, tok_t, NULL, 0, 0, NULL, 0, 0};
    buffer_push(struct js_stack_frame, pjs->stack, pjs->stack_len, pjs->stack_cap, frame);
    // js_dump_stack(pjs);
}

void js_stack_backward(struct js *pjs) {
    assert(pjs->stack_len > 1);
    js_stack_frame_clear(js_stack_peek(pjs));
    pjs->stack_len--;
    // js_dump_stack(pjs);
}

void js_stack_backward_to(struct js *pjs, size_t level) {
    assert(level > 0);
    while (pjs->stack_len > level) {
        js_stack_backward(pjs);
    }
}

void js_variable_declare(struct js *pjs, char *name, size_t name_len, struct js_value val) {
    struct js_stack_frame *frame = js_stack_peek(pjs);
    if (js_value_map_get(frame->vars, frame->vars_cap, name, name_len).type != 0) {
        printf("Variable \"%.*s\" already exists\n", (int)name_len, name);
        js_throw(pjs, "Variable name already exists");
    }
    js_value_map_put(&(frame->vars), &(frame->vars_len), &(frame->vars_cap), name, name_len, val);
}

void js_variable_declare_sz(struct js *pjs, char *name, struct js_value val) {
    js_variable_declare(pjs, name, strlen(name), val);
}

void js_variable_erase(struct js *pjs, char *name, size_t name_len) {
    struct js_stack_frame *frame = js_stack_peek(pjs);
    if (js_value_map_get(frame->vars, frame->vars_cap, name, name_len).type != 0) {
        js_value_map_put(&(frame->vars), &(frame->vars_len), &(frame->vars_cap), name, name_len, js_undefined());
        return;
    }
    // js_value_map_dump(frame->vars, frame->vars_len, frame->vars_cap);
    printf("Variable \"%.*s\" not found\n", (int)name_len, name);
    js_throw(pjs, "Variable not found");
}

void js_variable_erase_sz(struct js *pjs, char *name) {
    js_variable_erase(pjs, name, strlen(name));
}

void js_variable_assign(struct js *pjs, char *name, size_t name_len, struct js_value val) {
    int level; //  DONT use size_t because if level = 0,  level-- will become large positive number
    // log("%.*s", (int)name_len, name);
    // js_dump_stack(pjs);
    for (level = (int)pjs->stack_len - 1; level >= 0; level--) {
        struct js_stack_frame *frame = pjs->stack + level;
        if (js_value_map_get(frame->vars, frame->vars_cap, name, name_len).type != 0) {
            js_value_map_put(&(frame->vars), &(frame->vars_len), &(frame->vars_cap), name, name_len, val);
            return;
        }
    }
    js_throw(pjs, "Variable not found");
}

void js_variable_assign_sz(struct js *pjs, char *name, struct js_value val) {
    js_variable_assign(pjs, name, strlen(name), val);
}

struct js_value js_variable_fetch(struct js *pjs, char *name, size_t name_len) {
    int level;
    // log("%.*s", (int)name_len, name);
    // js_dump_stack(pjs);
    for (level = (int)pjs->stack_len - 1; level >= 0; level--) {
        struct js_stack_frame *frame = pjs->stack + level;
        struct js_value ret = js_value_map_get(frame->vars, frame->vars_cap, name, name_len);
        // log("level=%d, frame=%p, ret=%p", level, frame, ret);
        if (ret.type != 0) {
            // log("Variable found");
            return ret;
        }
    }
    log("Variable \"%.*s\" not found", (int)name_len, name);
    js_throw(pjs, "Variable not found");
}

struct js_value js_variable_fetch_sz(struct js *pjs, char *name) {
    return js_variable_fetch(pjs, name, strlen(name));
}

void js_parameter_push(struct js *pjs, struct js_value param) {
    struct js_stack_frame *frame = js_stack_peek(pjs);
    buffer_push(struct js_value, frame->params, frame->params_len, frame->params_cap, param);
}

struct js_value js_parameter_get(struct js *pjs, size_t idx) {
    struct js_stack_frame *frame = js_stack_peek(pjs);
    if (idx < frame->params_len) {
        struct js_value ret = frame->params[idx];
        return ret.type == 0 ? js_null() : ret;
    } else {
        return js_null();
    };
}

size_t js_parameter_length(struct js *pjs) {
    return js_stack_peek(pjs)->params_len;
}
