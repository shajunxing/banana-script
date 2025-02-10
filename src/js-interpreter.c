/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

static bool _is_str(struct js_value *pv) {
    return pv->type == vt_scripture || pv->type == vt_inscription || pv->type == vt_string;
}

static char *_str_pos(struct js *pjs, struct js_value *pv) {
    switch (pv->type) {
    case vt_scripture:
        return (char *)pv->value.scripture.p;
    case vt_inscription:
        return pjs->tablet + pv->value.inscription.h;
    case vt_string:
        return pv->value.string->p;
    default:
        return NULL;
    }
}

static size_t _str_len(struct js_value *pv) {
    switch (pv->type) {
    case vt_scripture:
        return pv->value.scripture.len;
    case vt_inscription:
        return pv->value.inscription.len;
    case vt_string:
        return pv->value.string->len;
    default:
        return 0;
    }
}

static int _str_cmp(struct js *pjs, struct js_value *pvl, struct js_value *pvr) {
    char *pl = _str_pos(pjs, pvl);
    size_t ll = _str_len(pvl);
    char *pr = _str_pos(pjs, pvr);
    size_t lr = _str_len(pvr);
    return strncmp(pl, pr, max(ll, lr));
}

void js_interpret(struct js *pjs) {
    intptr_t p0;
    struct js_bytecode *ir;
    enum js_opcode op;
    struct js_value v0, v1, *pv0, *pv1, *pv2;
    struct js_key_value *pkv;
    bool b;
    int i;
    size_t n0, n1;
    register size_t pc = pjs->bytecodes_idx;
    while (pc < pjs->bytecodes_len) {
        ir = pjs->bytecodes + pc;
        op = ir->opcode;
        // printf("%6llu  ", pc);
        // js_dump_bytecode(pjs, ir);
        // printf("\n");
        switch (op) {
        case op_value:
            switch (ir->operand.type) {
            case vt_null:
            case vt_boolean:
            case vt_number:
            case vt_scripture:
            case vt_inscription:
                js_evaluation_stack_push(pjs, ir->operand);
                break;
            default:
                js_throw(pjs, "This op_value type is forbidden, because it will be garbage collected");
            }
            break;
        case op_array:
            js_evaluation_stack_push(pjs, js_array(pjs));
            break;
        case op_object:
            js_evaluation_stack_push(pjs, js_object(pjs));
            break;
        case op_variable_declare:
        case op_variable_delete:
        case op_variable_put:
        case op_variable_get:
            v0 = ir->operand; // var name
            if (!_is_str(&v0)) {
                js_throw(pjs, "Identifier must be string");
            }
            switch (op) {
            case op_variable_declare:
                js_variable_declare(pjs, _str_pos(pjs, &v0), _str_len(&v0), *js_evaluation_stack_peek(pjs, 0));
                js_evaluation_stack_pop(pjs, 1);
                break;
            case op_variable_delete:
                js_variable_delete(pjs, _str_pos(pjs, &v0), _str_len(&v0));
                break;
            case op_variable_put:
                js_variable_put(pjs, _str_pos(pjs, &v0), _str_len(&v0), *js_evaluation_stack_peek(pjs, 0));
                js_evaluation_stack_pop(pjs, 1);
                break;
            case op_variable_get:
                v1 = js_variable_get(pjs, _str_pos(pjs, &v0), _str_len(&v0));
                js_evaluation_stack_push(pjs, v1);
                break;
            default:
                break;
            }
            break;
        case op_member_get:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type == vt_array && pv0->type == vt_number) {
                n0 = (size_t)pv0->value.number;
                if (n0 != pv0->value.number) {
                    js_throw(pjs, "Invalid array index, must be positive integer");
                }
                v0 = js_array_get(pjs, pv1, n0);
                js_evaluation_stack_pop(pjs, 2);
                js_evaluation_stack_push(pjs, v0);
            } else if (pv1->type == vt_object && _is_str(pv0)) {
                v0 = js_object_get(pjs, pv1, _str_pos(pjs, pv0), _str_len(pv0));
                js_evaluation_stack_pop(pjs, 2);
                js_evaluation_stack_push(pjs, v0);
            } else {
                js_throw(pjs, "Must be array[number] or object[string]");
            }
            break;
        case op_member_put:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            pv2 = js_evaluation_stack_peek(pjs, -2);
            if (pv2->type == vt_array && pv1->type == vt_number) {
                n0 = (size_t)pv1->value.number;
                if (n0 != pv1->value.number) {
                    js_throw(pjs, "Invalid array index, must be positive integer");
                }
                js_array_put(pjs, pv2, n0, *pv0);
                js_evaluation_stack_pop(pjs, 2);
            } else if (pv2->type == vt_object && _is_str(pv1)) {
                js_object_put(pjs, pv2, _str_pos(pjs, pv1), _str_len(pv1), *pv0);
                js_evaluation_stack_pop(pjs, 2);
            } else {
                js_throw(pjs, "Must be array[number] or object[string]");
            }
            break;
        case op_array_push:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type == vt_array) {
                js_array_push(pjs, pv1, *pv0);
                js_evaluation_stack_pop(pjs, 1);
            } else {
                js_throw(pjs, "Must be array");
            }
            break;
        case op_array_spread:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type == vt_array && pv0->type == vt_array) {
                for (n0 = 0; n0 < pv0->value.array->len; n0++) {
                    js_array_push(pjs, pv1, pv0->value.array->p[n0]);
                }
                js_evaluation_stack_pop(pjs, 1);
            } else {
                js_throw(pjs, "Must be array[...array]");
            }
            break;
        case op_object_optional:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type == vt_object && _is_str(pv0)) {
                v0 = js_object_get(pjs, pv1, _str_pos(pjs, pv0), _str_len(pv0));
            } else {
                v0 = js_null();
            }
            js_evaluation_stack_pop(pjs, 2);
            js_evaluation_stack_push(pjs, v0);
            break;
        case op_eval_stack_duplicate: // duplicate value from evstack top down i
            i = (int)ir->operand.value.number;
            js_evaluation_stack_push(pjs, *js_evaluation_stack_peek(pjs, i));
            break;
        case op_eval_stack_pop:
            i = (int)ir->operand.value.number;
            js_evaluation_stack_pop(pjs, i);
            break;
        case op_eval_stack_clear:
            js_evaluation_stack_clear(pjs);
            break;
        case op_add:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type == vt_number && pv0->type == vt_number) {
                pv1->value.number += pv0->value.number;
                js_evaluation_stack_pop(pjs, 1);
            } else if (_is_str(pv1) && _is_str(pv0)) {
                v0 = js_string(pjs, _str_pos(pjs, pv1), _str_len(pv1));
                string_buffer_append(v0.value.string->p, v0.value.string->len, v0.value.string->cap, _str_pos(pjs, pv0), _str_len(pv0));
                js_evaluation_stack_pop(pjs, 2);
                js_evaluation_stack_push(pjs, v0);
            } else {
                js_throw(pjs, "Add operand must be number or string");
            }
            break;
        case op_sub:
        case op_mul:
        case op_pow:
        case op_div:
        case op_mod:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type != vt_number || pv0->type != vt_number) {
                // js_value_dump(pjs, *pv0);
                // js_value_dump(pjs, *pv1);
                js_throw(pjs, "Arithmatic operand must be number");
            }
            switch (op) {
            case op_sub:
                pv1->value.number -= pv0->value.number;
                break;
            case op_mul:
                pv1->value.number *= pv0->value.number;
                break;
            case op_pow:
                pv1->value.number = pow(pv1->value.number, pv0->value.number);
                break;
            case op_div:
                pv1->value.number /= pv0->value.number;
                break;
            case op_mod:
                pv1->value.number = fmod(pv1->value.number, pv0->value.number);
                break;
            default:
                break;
            }
            js_evaluation_stack_pop(pjs, 1);
            break;
        case op_eq:
        case op_ne:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (memcmp(pv1, pv0, sizeof(struct js_value)) == 0) {
                b = true;
            } else if (pv1->type == vt_number && pv0->type == vt_number) {
                b = pv1->value.number == pv0->value.number; // DONT memcmp two double, same value may be different memory content, for example, mod result 0 == 0 may be false perhaps -0 +0
            } else if (_is_str(pv1) && _is_str(pv0)) {
                b = _str_cmp(pjs, pv1, pv0) == 0;
            } else {
                b = false;
            }
            if (op == op_ne) {
                b = !b;
            }
            js_evaluation_stack_pop(pjs, 2);
            js_evaluation_stack_push(pjs, js_boolean(b));
            break;
        case op_lt:
        case op_le:
        case op_gt:
        case op_ge:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type == vt_number && pv0->type == vt_number) {
                switch (op) {
                case op_lt:
                    b = pv1->value.number < pv0->value.number;
                    break;
                case op_le:
                    b = pv1->value.number <= pv0->value.number;
                    break;
                case op_gt:
                    b = pv1->value.number > pv0->value.number;
                    break;
                case op_ge:
                    b = pv1->value.number >= pv0->value.number;
                    break;
                default:
                    break;
                }
            } else if (_is_str(pv1) && _is_str(pv0)) {
                switch (op) {
                case op_lt:
                    b = _str_cmp(pjs, pv1, pv0) < 0;
                    break;
                case op_le:
                    b = _str_cmp(pjs, pv1, pv0) <= 0;
                    break;
                case op_gt:
                    b = _str_cmp(pjs, pv1, pv0) > 0;
                    break;
                case op_ge:
                    b = _str_cmp(pjs, pv1, pv0) >= 0;
                    break;
                default:
                    break;
                }
            } else {
                js_throw(pjs, "Relational operand must be number or string");
            }
            js_evaluation_stack_pop(pjs, 2);
            js_evaluation_stack_push(pjs, js_boolean(b));
            break;
        case op_and:
        case op_or:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            if (pv1->type != vt_boolean || pv0->type != vt_boolean) {
                js_throw(pjs, "Logical operand must be boolean");
            }
            switch (op) {
            case op_and:
                pv1->value.boolean = pv1->value.boolean && pv0->value.boolean;
                break;
            case op_or:
                pv1->value.boolean = pv1->value.boolean || pv0->value.boolean;
                break;
            default:
                break;
            }
            js_evaluation_stack_pop(pjs, 1);
            break;
        case op_not:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            if (pv0->type != vt_boolean) {
                js_throw(pjs, "Logical operand must be boolean");
            }
            pv0->value.boolean = !pv0->value.boolean;
            break;
        case op_ternary:
            pv0 = js_evaluation_stack_peek(pjs, -0);
            pv1 = js_evaluation_stack_peek(pjs, -1);
            pv2 = js_evaluation_stack_peek(pjs, -2);
            if (pv2->type != vt_boolean) {
                js_throw(pjs, "Ternary condition must be boolean");
            }
            v0 = pv2->value.boolean ? *pv1 : *pv0;
            js_evaluation_stack_pop(pjs, 3);
            js_evaluation_stack_push(pjs, v0);
            break;
        case op_typeof:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            v0 = js_scripture_sz(js_value_type_name(pv0->type));
            js_evaluation_stack_pop(pjs, 1);
            js_evaluation_stack_push(pjs, v0);
            break;
        case op_jump:
            pc = (size_t)ir->operand.value.number;
            continue;
        case op_jump_if_false:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            if (pv0->type != vt_boolean) {
                js_throw(pjs, "Conditional jump needs boolean");
            }
            b = pv0->value.boolean;
            js_evaluation_stack_pop(pjs, 1); //  DONT place multiple conditional jumps together, because condition is poped
            if (op == op_jump_if_false) {
                b = !b;
            }
            if (b) {
                pc = (size_t)ir->operand.value.number;
                continue;
            }
            break;
        case op_call_stack_push_block:
            js_call_stack_push(pjs, cs_block, 0);
            break;
        case op_call_stack_push_for:
            js_call_stack_push(pjs, cs_for, 0);
            break;
        case op_call_stack_push_function:
            js_call_stack_push(pjs, cs_function, (size_t)ir->operand.value.number); // return address
            break;
        case op_call_stack_pop:
            js_call_stack_pop(pjs);
            break;
        case op_call_stack_backup:
            js_call_stack_backup(pjs);
            break;
        case op_call_stack_restore:
            js_call_stack_restore(pjs);
            break;
        case op_for_in:
        case op_for_of:
            // js_dump_evaluation_stack(pjs);
            pv0 = js_evaluation_stack_peek(pjs, -0); // loop number
            if (pv0->type != vt_number) {
                js_throw(pjs, "Must be loop number");
            }
            pv1 = js_evaluation_stack_peek(pjs, -1); // array/object
            b = false; // whether success
            n0 = (size_t)pv0->value.number;
            if (pv1->type == vt_array) {
                for (; n0 < pv1->value.array->len; n0++) {
                    pv2 = pv1->value.array->p + n0;
                    if (pv2->type != vt_undefined && pv2->type != vt_null) {
                        if (op == op_for_in) {
                            js_evaluation_stack_push(pjs, js_number((double)n0));
                        } else {
                            js_evaluation_stack_push(pjs, *pv2);
                        }
                        b = true;
                        break;
                    }
                }
            } else if (pv1->type == vt_object) {
                // js_value_map_dump(pv1->value.object->p, pv1->value.object->len, pv1->value.object->cap);
                for (; n0 < pv1->value.object->cap; n0++) {
                    pkv = pv1->value.object->p + n0;
                    if (pkv->k.p != NULL && pkv->v.type != vt_undefined && pkv->v.type != vt_null) {
                        // printf("n0 = %llu\n0", n0);
                        if (op == op_for_in) {
                            js_evaluation_stack_push(pjs, js_string(pjs, pkv->k.p, pkv->k.len));
                        } else {
                            js_evaluation_stack_push(pjs, pkv->v);
                        }
                        b = true;
                        break;
                    }
                }
            } else {
                js_throw(pjs, "'for in/of' operand must be array or object");
            }
            if (b) {
                n0++;
                js_evaluation_stack_peek(pjs, -1)->value.number = (double)n0; // DONT use pv0, because evstack may be reallocated
                // js_dump_evaluation_stack(pjs);
                break;
            } else {
                pc = (size_t)ir->operand.value.number;
                continue;
            }
        case op_nop:
            break;
        case op_function:
            js_evaluation_stack_push(pjs, js_function(pjs, (size_t)ir->operand.value.number));
            break;
        case op_parameter_get:
            n0 = (size_t)ir->operand.value.number; // ev stack depth
            p0 = -((intptr_t)n0 - 1); // ev stack index
            n1 = 0; // param index
            while (p0 <= 0) {
                // printf("n0=%llu p0=%lld n1=%llu\n", n0, p0, n1);
                // js_dump_evaluation_stack(pjs);
                pv0 = js_evaluation_stack_peek(pjs, p0);
                // js_value_dump(pjs, *pv0);
                // printf("\n");
                if (!_is_str(pv0)) {
                    js_throw(pjs, "Parameter name must be string");
                }
                p0++;
                if (p0 <= 0) {
                    // normal param
                    pv1 = js_evaluation_stack_peek(pjs, p0);
                    // js_value_dump(pjs, *pv1);
                    // printf("\n");
                    js_variable_declare(pjs, _str_pos(pjs, pv0), _str_len(pv0), *pv1);
                    v0 = js_parameter_get(pjs, n1);
                    if (v0.type != vt_null) {
                        js_variable_put(pjs, _str_pos(pjs, pv0), _str_len(pv0), v0);
                    }
                    p0++;
                    n1++;
                } else {
                    // spread param
                    v0 = js_array(pjs);
                    while (n1 < js_parameter_length(pjs)) {
                        v1 = js_parameter_get(pjs, n1);
                        js_array_push(pjs, &v0, v1);
                        n1++;
                    }
                    js_variable_declare(pjs, _str_pos(pjs, pv0), _str_len(pv0), v0);
                    p0++;
                }
            }
            js_evaluation_stack_pop(pjs, n0);
            break;
        case op_parameter_push:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            js_parameter_push(pjs, *pv0);
            js_evaluation_stack_pop(pjs, 1);
            break;
        case op_parameter_spread:
            pv0 = js_evaluation_stack_peek(pjs, 0);
            if (pv0->type != vt_array) {
                js_throw(pjs, "Must be array[...array]");
            }
            for (n0 = 0; n0 < pv0->value.array->len; n0++) {
                js_parameter_push(pjs, pv0->value.array->p[n0]);
            }
            js_evaluation_stack_pop(pjs, 1);
            break;
        case op_call:
            pjs->result = js_null(); // reset result
            v0 = *js_evaluation_stack_peek(pjs, 0);
            if (v0.type == vt_function) {
                js_evaluation_stack_pop(pjs, 1);
                // before executing function, put all closure variables into current stack
                js_value_map_for_each(v0.value.function->closure.p, v0.value.function->closure.cap, k, kl, v, js_variable_declare(pjs, k, kl, v));
                pc = (size_t)v0.value.function->index;
                continue;
            } else if (v0.type == vt_c_function) {
                js_evaluation_stack_pop(pjs, 1);
                v0.value.c_function(pjs);
            } else {
                js_throw(pjs, "Must be function");
            }
            break;
        case op_return:
            v0 = *js_evaluation_stack_peek(pjs, 0);
            // if return value is function, put all variables in current stack into this function's closure
            if (v0.type == vt_function) {
                struct js_call_stack_frame *frame = js_call_stack_peek(pjs);
                js_value_map_for_each(frame->vars, frame->vars_cap, k, kl, v, js_value_map_put(&(v0.value.function->closure.p), &(v0.value.function->closure.len), &(v0.value.function->closure.cap), k, kl, v));
            }
            pjs->result = v0;
            js_evaluation_stack_pop(pjs, 1);
            while (js_call_stack_peek(pjs)->type != cs_function) {
                js_call_stack_pop(pjs);
            }
            pc = js_call_stack_peek(pjs)->ret_addr;
            continue;
        case op_get_result:
            js_evaluation_stack_push(pjs, pjs->result);
            break;
        default:
            fatal("Operator %d is under construction", ir->opcode);
        }
        pc++;
    }
    pjs->bytecodes_idx = pc;
}