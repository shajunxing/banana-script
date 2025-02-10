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
    // call_stack root
    js_call_stack_push(pjs, cs_root, 0);
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
    int depth;
    free(pjs->src);
    link_for_each(&(pjs->heap), v, {
        link_remove(&(pjs->heap), &(pjs->heap_len), v);
        _free_managed((struct js_managed_head *)v);
    });
    for (depth = 0; depth < pjs->call_stack_len; depth++) {
        js_call_stack_frame_clear(pjs->call_stack + depth);
    }
    free(pjs->call_stack);
    free(pjs->call_stack_depths);
    free(pjs->eval_stack);
    free(pjs->bytecodes);
    free(pjs->tablet);
    free(pjs);
}

static void _mark_in_use(struct js *pjs, struct js_value val) {
    // printf("_mark_in_use: ");
    // js_value_dump(pjs, val);
    // printf("\n");
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
                _mark_in_use(pjs, v);
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
                _mark_in_use(pjs, v);
            });
        }
        break;
    case vt_function:
        if (!val.value.function->h.in_use) {
            val.value.function->h.in_use = true;
            js_value_map_for_each(val.value.function->closure.p, val.value.function->closure.cap, k, kl, v, {
                (void)k;
                (void)kl;
                _mark_in_use(pjs, v);
            });
        }
        break;
    default:
        // puts("skip");
        break;
    }
}

void js_collect_garbage(struct js *pjs) {
    size_t depth;
    // mark
    for (depth = 0; depth < pjs->call_stack_len; depth++) {
        struct js_call_stack_frame *frame = pjs->call_stack + depth;
        js_value_map_for_each(frame->vars, frame->vars_cap, k, kl, v, {
            (void)k;
            (void)kl;
            _mark_in_use(pjs, v);
        });
    }
    // js_dump_heap(pjs);
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
    // js_dump_heap(pjs);
}

void js_load_string(struct js *pjs, const char *p, size_t len) {
    string_buffer_append(pjs->src, pjs->src_len, pjs->src_cap, p, len);
}

void js_load_string_sz(struct js *pjs, const char *p) {
    js_load_string(pjs, p, strlen(p));
}

void js_print_source(struct js *pjs) {
    printf("%.*s\n", (int)pjs->src_len, pjs->src);
}

void js_print_statistics(struct js *pjs) {
    printf("heap len=%llu\n", pjs->heap_len);
    printf("stack len=%llu\n", pjs->call_stack_len);
}

void js_dump_source(struct js *pjs) {
    printf("source len=%llu cap=%llu:\n", pjs->src_len, pjs->src_cap);
    print_hex(pjs->src, pjs->src_len);
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
                js_value_dump(pjs, v);
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
                js_value_dump(pjs, v);
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
                js_value_dump(pjs, v);
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

void js_dump_call_stack(struct js *pjs) {
#define X(name) #name,
    static const char *const names[] = {js_call_stack_frame_type_list};
#undef X
    size_t depth;
    printf("call stack len=%llu cap=%llu:\n", pjs->call_stack_len, pjs->call_stack_cap);
    for (depth = 0; depth < pjs->call_stack_len; depth++) {
        struct js_call_stack_frame *frame = pjs->call_stack + depth;
        printf("    depth=%llu type=%s ret_addr=%llu:\n", depth, names[frame->type], frame->ret_addr);
        printf("        parameters len=%llu cap=%llu:\n", frame->params_len, frame->params_cap);
        js_value_list_for_each(frame->params, frame->params_len, i, v, {
            printf("            %llu: ", i);
            js_value_dump(pjs, v);
            printf("\n");
        });
        printf("        variables len=%llu cap=%llu:\n", frame->vars_len, frame->vars_cap);
        js_value_map_for_each(frame->vars, frame->vars_cap, k, kl, v, {
            printf("            %.*s = ", (int)kl, k);
            js_value_dump(pjs, v);
            printf("\n");
        });
    }
}

void js_call_stack_frame_clear(struct js_call_stack_frame *frame) {
    js_value_map_free(&(frame->vars), &(frame->vars_len), &(frame->vars_cap));
    free(frame->params);
}

// return pointer, DONT return a copy, because modify copy's such as 'len' 'cap' are useless
struct js_call_stack_frame *js_call_stack_peek(struct js *pjs) {
    return pjs->call_stack + pjs->call_stack_len - 1;
}

void js_call_stack_push(struct js *pjs, enum js_call_stack_frame_type type, size_t ret_addr) {
    struct js_call_stack_frame frame = {type, NULL, 0, 0, NULL, 0, 0, ret_addr};
    buffer_push(struct js_call_stack_frame, pjs->call_stack, pjs->call_stack_len, pjs->call_stack_cap, frame);
    // js_dump_call_stack(pjs);
}

void js_call_stack_pop(struct js *pjs) {
    assert(pjs->call_stack_len > 1);
    js_call_stack_frame_clear(js_call_stack_peek(pjs));
    pjs->call_stack_len--;
    // js_dump_call_stack(pjs);
}

void js_call_stack_backup(struct js *pjs) {
    // printf("pjs->call_stack_len=%llu\n", pjs->call_stack_len);
    buffer_push(size_t, pjs->call_stack_depths, pjs->call_stack_depths_len, pjs->call_stack_depths_cap, pjs->call_stack_len);
}

void js_call_stack_restore(struct js *pjs) {
    size_t depth;
    assert(pjs->call_stack_depths_len > 0);
    pjs->call_stack_depths_len--;
    depth = pjs->call_stack_depths[pjs->call_stack_depths_len];
    // printf("depth=%llu\n", depth);
    assert(depth > 0);
    while (pjs->call_stack_len > depth) {
        js_call_stack_pop(pjs);
    }
}

void js_variable_declare(struct js *pjs, char *name, size_t name_len, struct js_value val) {
    struct js_call_stack_frame *frame = js_call_stack_peek(pjs);
    if (js_value_map_get(frame->vars, frame->vars_cap, name, name_len).type != 0) {
        printf("Variable \"%.*s\" already exists\n", (int)name_len, name);
        js_throw(pjs, "Variable name already exists");
    }
    js_value_map_put(&(frame->vars), &(frame->vars_len), &(frame->vars_cap), name, name_len, val);
}

void js_variable_declare_sz(struct js *pjs, char *name, struct js_value val) {
    js_variable_declare(pjs, name, strlen(name), val);
}

void js_variable_delete(struct js *pjs, char *name, size_t name_len) {
    struct js_call_stack_frame *frame = js_call_stack_peek(pjs);
    if (js_value_map_get(frame->vars, frame->vars_cap, name, name_len).type != 0) {
        js_value_map_put(&(frame->vars), &(frame->vars_len), &(frame->vars_cap), name, name_len, js_undefined());
        return;
    }
    // js_value_map_dump(frame->vars, frame->vars_len, frame->vars_cap);
    printf("Variable \"%.*s\" not found\n", (int)name_len, name);
    js_throw(pjs, "Variable not found");
}

void js_variable_delete_sz(struct js *pjs, char *name) {
    js_variable_delete(pjs, name, strlen(name));
}

void js_variable_put(struct js *pjs, char *name, size_t name_len, struct js_value val) {
    int depth; //  DONT use size_t because if depth = 0,  depth-- will become large positive number
    // log("%.*s", (int)name_len, name);
    // js_dump_call_stack(pjs);
    for (depth = (int)pjs->call_stack_len - 1; depth >= 0; depth--) {
        struct js_call_stack_frame *frame = pjs->call_stack + depth;
        if (js_value_map_get(frame->vars, frame->vars_cap, name, name_len).type != 0) {
            js_value_map_put(&(frame->vars), &(frame->vars_len), &(frame->vars_cap), name, name_len, val);
            return;
        }
    }
    js_throw(pjs, "Variable not found");
}

void js_variable_put_sz(struct js *pjs, char *name, struct js_value val) {
    js_variable_put(pjs, name, strlen(name), val);
}

struct js_value js_variable_get(struct js *pjs, char *name, size_t name_len) {
    int depth;
    // log("%.*s", (int)name_len, name);
    // js_dump_call_stack(pjs);
    for (depth = (int)pjs->call_stack_len - 1; depth >= 0; depth--) {
        struct js_call_stack_frame *frame = pjs->call_stack + depth;
        struct js_value ret = js_value_map_get(frame->vars, frame->vars_cap, name, name_len);
        // log("depth=%d, frame=%p, ret=%p", depth, frame, ret);
        if (ret.type != 0) {
            // log("Variable found");
            return ret;
        }
    }
    log("Variable \"%.*s\" not found", (int)name_len, name);
    js_throw(pjs, "Variable not found");
}

struct js_value js_variable_get_sz(struct js *pjs, char *name) {
    return js_variable_get(pjs, name, strlen(name));
}

void js_parameter_push(struct js *pjs, struct js_value param) {
    struct js_call_stack_frame *frame = js_call_stack_peek(pjs);
    buffer_push(struct js_value, frame->params, frame->params_len, frame->params_cap, param);
}

struct js_value js_parameter_get(struct js *pjs, size_t idx) {
    struct js_call_stack_frame *frame = js_call_stack_peek(pjs);
    if (idx < frame->params_len) {
        struct js_value ret = frame->params[idx];
        return ret.type == 0 ? js_null() : ret;
    } else {
        return js_null();
    };
}

size_t js_parameter_length(struct js *pjs) {
    return js_call_stack_peek(pjs)->params_len;
}

void js_evaluation_stack_push(struct js *pjs, struct js_value item) {
    buffer_push(struct js_value, pjs->eval_stack, pjs->eval_stack_len, pjs->eval_stack_cap, item);
}

void js_evaluation_stack_pop(struct js *pjs, size_t count) {
    if (pjs->eval_stack_len < count) {
        fatal("evaluation stack count %llu less than %llu, failed to pop", pjs->eval_stack_len, count);
    } else {
        pjs->eval_stack_len -= count;
    }
}

void js_evaluation_stack_clear(struct js *pjs) {
    pjs->eval_stack_len = 0;
}

struct js_value *js_evaluation_stack_peek(struct js *pjs, intptr_t inv) {
    intptr_t pos = pjs->eval_stack_len - 1 + inv;
    assert(pos >= 0);
    assert(pos < (intptr_t)pjs->eval_stack_len);
    return pjs->eval_stack + pos;
}

size_t js_add_bytecode(struct js *pjs, struct js_bytecode bytecode) {
    size_t pos = pjs->bytecodes_len;
    buffer_push(struct js_bytecode, pjs->bytecodes, pjs->bytecodes_len, pjs->bytecodes_cap, bytecode);
    return pos;
}

// DONT use vaarg because msvc only support >0 args
struct js_bytecode js_bytecode(enum js_opcode op, ...) {
    struct js_bytecode bytecode;
    va_list args;
    bytecode.opcode = op;
    va_start(args, op);
    switch (op) {
    case op_value:
    case op_variable_declare:
    case op_variable_delete:
    case op_variable_put:
    case op_variable_get:
    case op_eval_stack_duplicate:
    case op_eval_stack_pop:
    case op_jump:
    case op_jump_if_false:
    case op_call_stack_push_function:
    case op_for_in:
    case op_for_of:
    case op_nop:
    case op_function:
    case op_parameter_get:
        bytecode.operand = va_arg(args, struct js_value);
        break;
    default:
        break;
    }
    va_end(args);
    return bytecode;
}

void js_dump_bytecode(struct js *pjs, struct js_bytecode *pcode) {
#define X(name) #name,
    static const char *const names[] = {js_opcode_list};
#undef X
    printf("%-27s  ", names[pcode->opcode]);
    switch (pcode->opcode) {
    case op_value:
    case op_variable_declare:
    case op_variable_delete:
    case op_variable_put:
    case op_variable_get:
    case op_eval_stack_duplicate:
    case op_eval_stack_pop:
    case op_jump:
    case op_jump_if_false:
    case op_call_stack_push_function:
    case op_for_in:
    case op_for_of:
    case op_nop:
    case op_function:
    case op_parameter_get:
        js_value_dump(pjs, pcode->operand);
        break;
    default:
        break;
    }
}

void js_dump_bytecodes(struct js *pjs) {
    size_t i;
    printf("bytecodes len=%llu cap=%llu:\n", pjs->bytecodes_len, pjs->bytecodes_cap);
    for (i = 0; i < pjs->bytecodes_len; i++) {
        struct js_bytecode *pcode = pjs->bytecodes + i;
        printf("%6llu  ", i);
        js_dump_bytecode(pjs, pcode);
        printf("\n");
    }
}

void js_dump_evaluation_stack(struct js *pjs) {
    size_t i;
    printf("evaluation stack len=%llu cap=%llu:\n", pjs->eval_stack_len, pjs->eval_stack_cap);
    for (i = 0; i < pjs->eval_stack_len; i++) {
        struct js_value *pval = pjs->eval_stack + i;
        printf("%6llu  ", i);
        js_value_dump(pjs, *pval);
        printf("\n");
    }
}

void js_dump_tablet(struct js *pjs) {
    printf("tablet len=%llu cap=%llu:\n", pjs->tablet_len, pjs->tablet_cap);
    print_hex(pjs->tablet, pjs->tablet_len);
}

size_t js_inscribe_tablet(struct js *pjs, char *p, size_t len) {
    char *cmp_h;
    size_t i, cmp_len = 0;
    for (i = 0; i <= pjs->tablet_len; i++) {
        cmp_h = pjs->tablet + i;
        cmp_len = min(pjs->tablet_len - i, len);
        if (memcmp(cmp_h, p, cmp_len) == 0) {
            break;
        }
    }
    string_buffer_append(pjs->tablet, pjs->tablet_len, pjs->tablet_cap, p + cmp_len, len - cmp_len);
    return i;
}