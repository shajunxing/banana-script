/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

static inline enum js_token_state _get_token_state(struct js *pjs) {
    // check end here
    if (pjs->tok_cache_idx < pjs->tok_cache_len) {
        return pjs->tok_cache[pjs->tok_cache_idx].stat;
    } else {
        return ts_end_of_file;
    }
}

static inline char *_get_token_head(struct js *pjs) {
    return pjs->src + pjs->tok_cache[pjs->tok_cache_idx].h;
}

static inline size_t _get_token_length(struct js *pjs) {
    return pjs->tok_cache[pjs->tok_cache_idx].t - pjs->tok_cache[pjs->tok_cache_idx].h;
}

static inline long _get_token_line(struct js *pjs) {
    return pjs->tok_cache[pjs->tok_cache_idx].line;
}

static inline char *_get_token_string_head(struct js *pjs) {
    return _get_token_head(pjs) + 1;
}

static inline size_t _get_token_string_length(struct js *pjs) {
    return _get_token_length(pjs) - 2;
}

static inline double _get_token_number(struct js *pjs) {
    return pjs->tok_cache[pjs->tok_cache_idx].num;
}

static inline void _next_token(struct js *pjs) {
    // DONT check end here, because _accept() will step next on success, if end here, many operations will not done.
    pjs->tok_cache_idx++;
}

static inline void _stack_forward(struct js *pjs) {
    js_stack_forward(pjs, pjs->tok_cache[pjs->tok_cache_idx].h, pjs->tok_cache[pjs->tok_cache_idx].t);
}

static inline bool _accept(struct js *pjs, enum js_token_state stat) {
    if (_get_token_state(pjs) == stat) {
        _next_token(pjs);
        return true;
    }
    return false;
}

// use macro instead of inline function, to correctly record error position
// DONT use '_pjs' inside do while, or msvc will cause lots of 'warning C4700: uninitialized local variable '_pjs' used', don't know why, maybe bug
#define _expect(pjs, stat, msg)        \
    do {                               \
        struct js *__pjs = (pjs);      \
        if (!_accept(__pjs, (stat))) { \
            js_throw(__pjs, (msg));    \
        }                              \
    } while (0)

struct js_value *js_parse_value(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret;
        switch (_get_token_state(pjs)) {
        case ts_null:
            ret = js_null_new(pjs);
            _next_token(pjs);
            break;
        case ts_true:
            ret = js_boolean_new(pjs, true);
            _next_token(pjs);
            break;
        case ts_false:
            ret = js_boolean_new(pjs, false);
            _next_token(pjs);
            break;
        case ts_number:
            ret = js_number_new(pjs, _get_token_number(pjs));
            _next_token(pjs);
            break;
        case ts_string:
            ret = js_string_new(pjs, _get_token_string_head(pjs), _get_token_string_length(pjs));
            _next_token(pjs);
            break;
        case ts_left_bracket:
            ret = js_parse_array(pjs);
            break;
        case ts_left_brace:
            ret = js_parse_object(pjs);
            break;
        case ts_function: {
            _next_token(pjs);
            ret = js_function_new(pjs, pjs->tok_cache_idx);
            // only walk through, no exec
            pjs->parse_exec = false;
            js_parse_function(pjs);
            pjs->parse_exec = true;
            break;
        }
        default:
            js_throw(pjs, "Not a value literal");
        }
        return ret;
    } else {
        switch (_get_token_state(pjs)) {
        case ts_null:
        case ts_true:
        case ts_false:
        case ts_number:
        case ts_string:
            _next_token(pjs);
            break;
        case ts_left_bracket:
            js_parse_array(pjs);
            break;
        case ts_left_brace:
            js_parse_object(pjs);
            break;
        case ts_function: {
            _next_token(pjs);
            js_parse_function(pjs);
            break;
        }
        default:
            js_throw(pjs, "Not a value literal");
        }
        return NULL;
    }
}

struct js_value *js_parse_array(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_array_new(pjs);
        struct js_value *spread;
        size_t i;
        _expect(pjs, ts_left_bracket, "Expect [");
        if (!_accept(pjs, ts_right_bracket)) {
            for (;;) {
                if (_accept(pjs, ts_spread)) {
                    spread = js_parse_expression(pjs);
                    if (spread->type != vt_array) {
                        js_throw(pjs, "Operator ... requires array operand");
                    }
                    for (i = 0; i < spread->value.array->len; i++) {
                        js_array_push(pjs, ret, js_array_get(pjs, spread, i));
                    }
                } else {
                    js_array_push(pjs, ret, js_parse_expression(pjs));
                }
                if (_accept(pjs, ts_comma)) {
                    continue;
                } else if (_accept(pjs, ts_right_bracket)) {
                    break;
                } else {
                    js_throw(pjs, "Expect , or ]");
                }
            }
        }
        return ret;
    } else {
        _expect(pjs, ts_left_bracket, "Expect [");
        if (!_accept(pjs, ts_right_bracket)) {
            for (;;) {
                _accept(pjs, ts_spread); // accept or not
                js_parse_expression(pjs);
                if (_accept(pjs, ts_comma)) {
                    continue;
                } else if (_accept(pjs, ts_right_bracket)) {
                    break;
                } else {
                    js_throw(pjs, "Expect , or ]");
                }
            }
        }
        return NULL;
    }
}

struct js_value *js_parse_object(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_object_new(pjs);
        char *k;
        size_t klen;
        _expect(pjs, ts_left_brace, "Expect {");
        if (!_accept(pjs, ts_right_brace)) {
            for (;;) {
                if (_get_token_state(pjs) == ts_string) {
                    k = _get_token_string_head(pjs);
                    klen = _get_token_string_length(pjs);
                } else if (_get_token_state(pjs) == ts_identifier) {
                    k = _get_token_head(pjs);
                    klen = _get_token_length(pjs);
                } else {
                    js_throw(pjs, "Expect string or identifier");
                }
                _next_token(pjs);
                _expect(pjs, ts_colon, "Expect :");
                js_object_put(pjs, ret, k, klen, js_parse_expression(pjs));
                if (_accept(pjs, ts_comma)) {
                    continue;
                } else if (_accept(pjs, ts_right_brace)) {
                    break;
                } else {
                    js_throw(pjs, "Expect , or }");
                }
            }
        }
        return ret;
    } else {
        _expect(pjs, ts_left_brace, "Expect {");
        if (!_accept(pjs, ts_right_brace)) {
            for (;;) {
                if (_get_token_state(pjs) != ts_string && _get_token_state(pjs) != ts_identifier) {
                    js_throw(pjs, "Expect string or identifier");
                }
                _next_token(pjs);
                _expect(pjs, ts_colon, "Expect :");
                js_parse_expression(pjs);
                if (_accept(pjs, ts_comma)) {
                    continue;
                } else if (_accept(pjs, ts_right_brace)) {
                    break;
                } else {
                    js_throw(pjs, "Expect , or }");
                }
            }
        }
        return NULL;
    }
}

// ternary expression as root
struct js_value *js_parse_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_parse_logical_expression(pjs);
        if (_accept(pjs, ts_question)) {
            if (ret->type != vt_boolean) {
                js_throw(pjs, "Operator ?: requires condition operand be boolean");
            }
            if (ret->value.boolean) {
                ret = js_parse_logical_expression(pjs);
                _expect(pjs, ts_colon, "Expect :");
                pjs->parse_exec = false;
                js_parse_logical_expression(pjs);
                pjs->parse_exec = true;
            } else {
                pjs->parse_exec = false;
                js_parse_logical_expression(pjs);
                pjs->parse_exec = true;
                _expect(pjs, ts_colon, "Expect :");
                ret = js_parse_logical_expression(pjs);
            }
        }
        return ret;
    } else {
        js_parse_logical_expression(pjs);
        if (_accept(pjs, ts_question)) {
            js_parse_logical_expression(pjs);
            _expect(pjs, ts_colon, "Expect :");
            js_parse_logical_expression(pjs);
        }
        return NULL;
    }
}

struct js_value *js_parse_logical_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_parse_relational_expression(pjs);
        enum js_token_state stat;
        struct js_value *val_r;
        while (_get_token_state(pjs) == ts_and || _get_token_state(pjs) == ts_or) {
            stat = _get_token_state(pjs);
            if (ret->type != vt_boolean) {
                js_throw(pjs, "Operators && || require left operand be boolean");
            }
            _next_token(pjs);
            val_r = js_parse_relational_expression(pjs);
            if (val_r->type != vt_boolean) {
                js_throw(pjs, "Operators && || require right operand be boolean");
            }
            if (stat == ts_and) {
                ret = js_boolean_new(pjs, ret->value.boolean && val_r->value.boolean);
            } else {
                ret = js_boolean_new(pjs, ret->value.boolean || val_r->value.boolean);
            }
        }
        return ret;
    } else {
        js_parse_relational_expression(pjs);
        while (_get_token_state(pjs) == ts_and || _get_token_state(pjs) == ts_or) {
            _next_token(pjs);
            js_parse_relational_expression(pjs);
        }
        return NULL;
    }
}

struct js_value *js_parse_relational_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_parse_additive_expression(pjs);
        enum js_token_state stat;
        struct js_value *val_r;
        bool eq;
        double num_l, num_r;
        if (_get_token_state(pjs) == ts_equal_to || _get_token_state(pjs) == ts_not_equal_to) {
            stat = _get_token_state(pjs);
            _next_token(pjs);
            val_r = js_parse_additive_expression(pjs);
            if (ret == val_r) {
                eq = true;
            } else if (ret->type == val_r->type) {
                switch (ret->type) {
                case vt_null:
                    eq = true;
                    break;
                case vt_boolean:
                    eq = ret->value.boolean == val_r->value.boolean;
                    break;
                case vt_number:
                    eq = ret->value.number == val_r->value.number;
                    break;
                case vt_string:
                    eq = string_compare(ret->value.string, val_r->value.string) == 0;
                    break;
                default:
                    eq = false;
                    break;
                }
            } else {
                eq = false;
            }
            if (stat == ts_equal_to) {
                ret = js_boolean_new(pjs, eq);
            } else {
                ret = js_boolean_new(pjs, !eq);
            }
        } else if (_get_token_state(pjs) == ts_less_than || _get_token_state(pjs) == ts_less_than_or_equal_to || _get_token_state(pjs) == ts_greater_than || _get_token_state(pjs) == ts_greater_than_or_equal_to) {
            stat = _get_token_state(pjs);
            if (ret->type != vt_number && ret->type != vt_string) {
                js_throw(pjs, "Operators < <= > >= require left operand be number or string");
            }
            _next_token(pjs);
            val_r = js_parse_additive_expression(pjs);
            if (val_r->type != ret->type) {
                js_throw(pjs, "Operators < <= > >= require right operand be same type");
            }
            if (ret->type == vt_number) {
                num_l = ret->value.number;
                num_r = val_r->value.number;
            } else {
                num_l = (double)string_compare(ret->value.string, val_r->value.string);
                num_r = 0;
            }
            if (stat == ts_less_than) {
                ret = js_boolean_new(pjs, num_l < num_r);
            } else if (stat == ts_less_than_or_equal_to) {
                ret = js_boolean_new(pjs, num_l <= num_r);
            } else if (stat == ts_greater_than) {
                ret = js_boolean_new(pjs, num_l > num_r);
            } else if (stat == ts_greater_than_or_equal_to) {
                ret = js_boolean_new(pjs, num_l >= num_r);
            } else if (stat == ts_equal_to) {
                ret = js_boolean_new(pjs, num_l == num_r);
            } else {
                ret = js_boolean_new(pjs, num_l != num_r);
            }
        }
        return ret;
    } else {
        js_parse_additive_expression(pjs);
        if (_get_token_state(pjs) == ts_equal_to || _get_token_state(pjs) == ts_not_equal_to || _get_token_state(pjs) == ts_less_than || _get_token_state(pjs) == ts_less_than_or_equal_to || _get_token_state(pjs) == ts_greater_than || _get_token_state(pjs) == ts_greater_than_or_equal_to) {
            _next_token(pjs);
            js_parse_additive_expression(pjs);
        }
        return NULL;
    }
}

struct js_value *js_parse_additive_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_parse_multiplicative_expression(pjs);
        enum js_token_state stat;
        struct js_value *val_r;
        for (;;) {
            stat = _get_token_state(pjs);
            if (stat == ts_plus) {
                if (ret->type != vt_number && ret->type != vt_string) {
                    js_throw(pjs, "Operator + requires left operand be number or string");
                }
            } else if (stat == ts_minus) {
                if (ret->type != vt_number) {
                    js_throw(pjs, "Operator - requires left operand be number");
                }
            } else {
                break;
            }
            _next_token(pjs);
            val_r = js_parse_multiplicative_expression(pjs);
            if (val_r->type != ret->type) {
                js_throw(pjs, "Operators + - require right operand be same type");
            }
            if (stat == ts_plus) {
                if (ret->type == vt_number) {
                    ret = js_number_new(pjs, ret->value.number + val_r->value.number);
                } else {
                    ret = js_string_new(pjs, ret->value.string->p, ret->value.string->len);
                    string_append(ret->value.string, val_r->value.string->p, val_r->value.string->len);
                }
            } else {
                ret = js_number_new(pjs, ret->value.number - val_r->value.number);
            }
        }
        return ret;
    } else {
        js_parse_multiplicative_expression(pjs);
        while (_get_token_state(pjs) == ts_plus || _get_token_state(pjs) == ts_minus) {
            _next_token(pjs);
            js_parse_multiplicative_expression(pjs);
        }
        return NULL;
    }
}

struct js_value *js_parse_multiplicative_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret = js_parse_prefix_expression(pjs);
        enum js_token_state stat;
        struct js_value *val_r;
        while (_get_token_state(pjs) == ts_multiplication || _get_token_state(pjs) == ts_division || _get_token_state(pjs) == ts_mod) {
            stat = _get_token_state(pjs);
            if (ret->type != vt_number) {
                js_throw(pjs, "Operators * / % require left operand be number");
            }
            _next_token(pjs);
            val_r = js_parse_prefix_expression(pjs);
            if (val_r->type != vt_number) {
                js_throw(pjs, "Operators * / % require right operand be number");
            }
            if (stat == ts_multiplication) {
                ret = js_number_new(pjs, ret->value.number * val_r->value.number);
            } else if (stat == ts_division) {
                ret = js_number_new(pjs, ret->value.number / val_r->value.number);
            } else {
                ret = js_number_new(pjs, fmod(ret->value.number, val_r->value.number));
            }
        }
        return ret;
    } else {
        js_parse_prefix_expression(pjs);
        while (_get_token_state(pjs) == ts_multiplication || _get_token_state(pjs) == ts_division || _get_token_state(pjs) == ts_mod) {
            _next_token(pjs);
            js_parse_prefix_expression(pjs);
        }
        return NULL;
    }
}

struct js_value *js_parse_prefix_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *ret;
        double sign = 0;
        if (_accept(pjs, ts_typeof)) {
            ret = js_parse_access_call_expression(pjs);
            ret = js_string_new_sz(pjs, js_value_type_name(ret));
        } else if (_accept(pjs, ts_not)) {
            ret = js_parse_access_call_expression(pjs);
            if (ret->type != vt_boolean) {
                js_throw(pjs, "Operator ! requires boolean operand");
            }
            ret = js_boolean_new(pjs, !ret->value.boolean);
        } else {
            if (_get_token_state(pjs) == ts_plus) {
                sign = 1;
                _next_token(pjs);
            } else if (_get_token_state(pjs) == ts_minus) {
                sign = -1;
                _next_token(pjs);
            }
            ret = js_parse_access_call_expression(pjs);
            if (sign != 0) {
                if (ret->type != vt_number) {
                    js_throw(pjs, "Prefix operators + - require number operand");
                }
                ret = js_number_new(pjs, sign * ret->value.number);
            }
        }
        return ret;
    } else {
        if (_get_token_state(pjs) == ts_typeof || _get_token_state(pjs) == ts_not || _get_token_state(pjs) == ts_plus || _get_token_state(pjs) == ts_minus) {
            _next_token(pjs);
        }
        js_parse_access_call_expression(pjs);
        return NULL;
    }
}

static inline void _accessor_dump(struct js_value_accessor acc) {
    printf("acc.type= %d\nacc.v= ", acc.type);
    js_value_dump(acc.v);
    printf("\nacc.p= %p\nacc.n= %llu\n", acc.p, acc.n);
    print_hex(acc.p, acc.n);
}

static inline void _accessor_put(struct js *pjs, struct js_value_accessor acc, struct js_value *v) {
    switch (acc.type) {
    case at_value:
        js_throw(pjs, "Can not put value to accessor value type");
    case at_identifier:
        js_variable_assign(pjs, acc.p, acc.n, v);
        return;
    case at_array_member:
        js_array_put(pjs, acc.v, acc.n, v);
        return;
    case at_object_member:
        js_object_put(pjs, acc.v, acc.p, acc.n, v);
        return;
    case at_optional_member:
        if (acc.v->type == vt_object) {
            js_object_put(pjs, acc.v, acc.p, acc.n, v);
        } else {
            js_throw(pjs, "Not object");
        }
        return;
    default:
        js_throw(pjs, "Unknown accessor type");
    }
}

static inline struct js_value *_accessor_get(struct js *pjs, struct js_value_accessor acc) {
    switch (acc.type) {
    case at_value:
        return acc.v;
    case at_identifier:
        return js_variable_fetch(pjs, acc.p, acc.n);
    case at_array_member:
        return js_array_get(pjs, acc.v, acc.n);
    case at_object_member:
        return js_object_get(pjs, acc.v, acc.p, acc.n);
    case at_optional_member:
        if (acc.v->type == vt_object) {
            return js_object_get(pjs, acc.v, acc.p, acc.n);
        } else {
            return js_null_new(pjs);
        }
    default:
        js_throw(pjs, "Unknown accessor type");
    }
}

struct js_value *js_parse_access_call_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        return _accessor_get(pjs, js_parse_accessor(pjs));
    } else {
        js_parse_accessor(pjs);
        return NULL;
    }
}

struct js_value_accessor js_parse_accessor(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value_accessor acc;
        struct js_value *idx = NULL;
        size_t tok_idx_backup;
        struct js_value *spread;
        size_t i;
        if (_accept(pjs, ts_left_parenthesis)) {
            acc.type = at_value;
            acc.v = js_parse_expression(pjs);
            _expect(pjs, ts_right_parenthesis, "Expect )");
        } else if (_get_token_state(pjs) == ts_identifier) {
            acc.type = at_identifier;
            acc.p = _get_token_head(pjs);
            acc.n = _get_token_length(pjs);
            _next_token(pjs);
        } else {
            acc.type = at_value;
            acc.v = js_parse_value(pjs);
        }
        for (;;) {
            if (_accept(pjs, ts_left_bracket)) {
                acc.v = _accessor_get(pjs, acc);
                idx = js_parse_additive_expression(pjs);
                if (acc.v->type == vt_array && idx->type == vt_number) {
                    acc.type = at_array_member;
                    acc.n = (size_t)idx->value.number;
                    if (acc.n != idx->value.number) {
                        js_throw(pjs, "Invalid array index, must be positive integer");
                    }
                } else if (acc.v->type == vt_object && idx->type == vt_string) {
                    acc.type = at_object_member;
                    acc.p = idx->value.string->p;
                    acc.n = idx->value.string->len;
                } else {
                    js_throw(pjs, "Must be array[number] or object[string]");
                }
                _expect(pjs, ts_right_bracket, "Expect ]");
            } else if (_accept(pjs, ts_member_access)) {
                acc.v = _accessor_get(pjs, acc);
                if (acc.v->type == vt_object && _get_token_state(pjs) == ts_identifier) {
                    acc.type = at_object_member;
                    acc.p = _get_token_head(pjs);
                    acc.n = _get_token_length(pjs);
                    _next_token(pjs);
                } else {
                    js_throw(pjs, "Must be object.identifier");
                }
            } else if (_accept(pjs, ts_optional_chaining)) {
                acc.v = _accessor_get(pjs, acc);
                if (_get_token_state(pjs) == ts_identifier) {
                    acc.type = at_optional_member;
                    acc.p = _get_token_head(pjs);
                    acc.n = _get_token_length(pjs);
                    _next_token(pjs);
                } else {
                    js_throw(pjs, "Must be value?.identifier");
                }
            } else if (_get_token_state(pjs) == ts_left_parenthesis) {
                acc.v = _accessor_get(pjs, acc);
                _stack_forward(pjs); // record function call stack descriptor '('
                _next_token(pjs);
                if (!_accept(pjs, ts_right_parenthesis)) {
                    for (;;) {
                        if (_accept(pjs, ts_spread)) {
                            spread = js_parse_expression(pjs);
                            if (spread->type != vt_array) {
                                js_throw(pjs, "Operator ... requires array operand");
                            }
                            for (i = 0; i < spread->value.array->len; i++) {
                                js_parameter_push(pjs, js_array_get(pjs, spread, i));
                            }
                        } else {
                            js_parameter_push(pjs, js_parse_expression(pjs));
                        }
                        if (_accept(pjs, ts_comma)) {
                            continue;
                        } else if (_accept(pjs, ts_right_parenthesis)) {
                            break;
                        } else {
                            js_throw(pjs, "Expect , or )");
                        }
                    }
                }
                if (acc.v->type == vt_function) {
                    tok_idx_backup = pjs->tok_cache_idx;
                    pjs->tok_cache_idx = acc.v->value.function.index;
                    // before executing function, put all closure variables into current stack
                    map_for_each(acc.v->value.function.closure, k, v, js_variable_declare(pjs, k->p, k->len, v));
                    js_parse_function(pjs);
                    pjs->tok_cache_idx = tok_idx_backup;
                } else if (acc.v->type == vt_cfunction) {
                    acc.v->value.cfunction(pjs);
                } else {
                    js_throw(pjs, "Must be function");
                }
                // js_dump_stack(pjs);
                acc.type = at_value;
                acc.v = pjs->result ? pjs->result : js_null_new(pjs);
                pjs->result = NULL;
                // if return value is function, put all variables in current stack into this function's closure
                if (acc.v->type == vt_function) {
                    struct map *variables = js_stack_peek(pjs)->variables;
                    map_for_each(variables, k, v, map_put(acc.v->value.function.closure, k->p, k->len, v));
                }
                js_stack_backward(pjs);
                pjs->mark_return = false;
            } else {
                break;
            }
        }
        return acc;
    } else {
        struct js_value_accessor acc = {0, NULL, NULL, 0};
        if (_accept(pjs, ts_left_parenthesis)) {
            js_parse_expression(pjs);
            _expect(pjs, ts_right_parenthesis, "Expect )");
        } else if (_accept(pjs, ts_identifier)) {
            ;
        } else {
            js_parse_value(pjs);
        }
        for (;;) {
            if (_accept(pjs, ts_left_bracket)) {
                js_parse_additive_expression(pjs);
                _expect(pjs, ts_right_bracket, "Expect ]");
            } else if (_accept(pjs, ts_member_access)) {
                _expect(pjs, ts_identifier, "Must be object.identifier");
            } else if (_accept(pjs, ts_optional_chaining)) {
                _expect(pjs, ts_identifier, "Must be object.identifier");
            } else if (_accept(pjs, ts_left_parenthesis)) {
                if (!_accept(pjs, ts_right_parenthesis)) {
                    for (;;) {
                        _accept(pjs, ts_spread); // accept or not
                        js_parse_expression(pjs);
                        if (_accept(pjs, ts_comma)) {
                            continue;
                        } else if (_accept(pjs, ts_right_parenthesis)) {
                            break;
                        } else {
                            js_throw(pjs, "Expect , or )");
                        }
                    }
                }
            } else {
                break;
            }
        }
        return acc;
    }
}

void js_parse_assignment_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value_accessor acc = js_parse_accessor(pjs);
        enum js_token_state stat;
        struct js_value *val, *varval;
        if (_get_token_state(pjs) == ts_assignment || _get_token_state(pjs) == ts_plus_assignment || _get_token_state(pjs) == ts_minus_assignment || _get_token_state(pjs) == ts_multiplication_assignment || _get_token_state(pjs) == ts_division_assignment || _get_token_state(pjs) == ts_mod_assignment) {
            stat = _get_token_state(pjs);
            _next_token(pjs);
            val = js_parse_expression(pjs);
            if (stat == ts_assignment) {
                _accessor_put(pjs, acc, val);
            } else {
                varval = _accessor_get(pjs, acc);
                if (stat == ts_plus_assignment) {
                    if (varval->type != vt_number && varval->type != vt_string) {
                        js_throw(pjs, "Operator += requires left operand be number or string");
                    }
                } else {
                    if (varval->type != vt_number) {
                        js_throw(pjs, "Operators -= *= /= %= require left operand be number");
                    }
                }
                if (val->type != varval->type) {
                    js_throw(pjs, "Operators += -= *= /= %= require right operand be same type");
                }
                if (stat == ts_plus_assignment) {
                    if (varval->type == vt_number) {
                        varval = js_number_new(pjs, varval->value.number + val->value.number);
                    } else {
                        varval = js_string_new(pjs, varval->value.string->p, varval->value.string->len);
                        string_append(varval->value.string, val->value.string->p, val->value.string->len);
                    }
                } else if (stat == ts_minus_assignment) {
                    varval = js_number_new(pjs, varval->value.number - val->value.number);
                } else if (stat == ts_multiplication_assignment) {
                    varval = js_number_new(pjs, varval->value.number * val->value.number);
                } else if (stat == ts_division_assignment) {
                    varval = js_number_new(pjs, varval->value.number / val->value.number);
                } else { // ts_mod_assignment
                    varval = js_number_new(pjs, fmod(varval->value.number, val->value.number));
                }
                _accessor_put(pjs, acc, varval);
            }
        } else if (_get_token_state(pjs) == ts_plus_plus || _get_token_state(pjs) == ts_minus_minus) {
            varval = _accessor_get(pjs, acc);
            if (varval->type != vt_number) {
                js_throw(pjs, "Operators ++ -- require operand be number");
            }
            if (_get_token_state(pjs) == ts_plus_plus) {
                varval = js_number_new(pjs, varval->value.number + 1);
            } else {
                varval = js_number_new(pjs, varval->value.number - 1);
            }
            _accessor_put(pjs, acc, varval);
            _next_token(pjs);
        }
    } else {
        js_parse_accessor(pjs);
        if (_get_token_state(pjs) == ts_assignment || _get_token_state(pjs) == ts_plus_assignment || _get_token_state(pjs) == ts_minus_assignment || _get_token_state(pjs) == ts_multiplication_assignment || _get_token_state(pjs) == ts_division_assignment || _get_token_state(pjs) == ts_mod_assignment) {
            _next_token(pjs);
            js_parse_expression(pjs);
        } else if (_get_token_state(pjs) == ts_plus_plus || _get_token_state(pjs) == ts_minus_minus) {
            _next_token(pjs);
        }
    }
}

void js_parse_declaration_expression(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        char *ident_h;
        size_t ident_len;
        _expect(pjs, ts_let, "Expect let");
        for (;;) {
            if (_get_token_state(pjs) != ts_identifier) {
                js_throw(pjs, "Expect variable name");
            }
            ident_h = _get_token_head(pjs);
            ident_len = _get_token_length(pjs);
            _next_token(pjs);
            if (_accept(pjs, ts_assignment)) {
                js_variable_declare(pjs, ident_h, ident_len, js_parse_expression(pjs));
            } else {
                js_variable_declare(pjs, ident_h, ident_len, js_null_new(pjs));
            }
            if (_accept(pjs, ts_comma)) {
                continue;
            } else {
                break;
            }
        }
    } else {
        _expect(pjs, ts_let, "Expect let");
        for (;;) {
            _expect(pjs, ts_identifier, "Expect variable name");
            if (_accept(pjs, ts_assignment)) {
                js_parse_expression(pjs);
            }
            if (_accept(pjs, ts_comma)) {
                continue;
            } else {
                break;
            }
        }
    }
}

void js_parse_script(struct js *pjs) {
    while (_get_token_state(pjs) != ts_end_of_file) {
        js_parse_statement(pjs);
    }
}

void js_parse_statement(struct js *pjs) {
    enum { classic_for,
           for_in,
           for_of } for_type;
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        struct js_value *cond = NULL, *val;
        size_t tok_idx_backup, tok_idx_backup_2, tok_idx_backup_3;
        struct js_value_accessor acc;
        int loop_count;
        char *ident_h;
        size_t ident_len;
        if (_accept(pjs, ts_semicolon)) {
            ;
        } else if (_get_token_state(pjs) == ts_left_brace) { // DONT use _accept, _stack_forward will record token
            _stack_forward(pjs);
            _next_token(pjs);
            while (_get_token_state(pjs) != ts_right_brace) {
                js_parse_statement(pjs);
            }
            js_stack_backward(pjs);
            _next_token(pjs);
        } else if (_accept(pjs, ts_if)) {
            struct js_value *cond;
            _expect(pjs, ts_left_parenthesis, "Expect (");
            cond = js_parse_expression(pjs);
            if (cond->type != vt_boolean) {
                js_throw(pjs, "Condition must be boolean");
            }
            _expect(pjs, ts_right_parenthesis, "Expect )");
            pjs->parse_exec = cond->value.boolean;
            js_parse_statement(pjs);
            if (_accept(pjs, ts_else)) {
                pjs->parse_exec = !cond->value.boolean;
                js_parse_statement(pjs);
            }
            pjs->parse_exec = true; // restore execute
        } else if (_accept(pjs, ts_while)) {
            _expect(pjs, ts_left_parenthesis, "Expect (");
            tok_idx_backup = pjs->tok_cache_idx; // backup token
            while (pjs->parse_exec && !pjs->mark_break) {
                pjs->tok_cache_idx = tok_idx_backup; // restore token
                cond = js_parse_expression(pjs);
                if (cond->type != vt_boolean) {
                    js_throw(pjs, "Condition must be boolean");
                }
                _expect(pjs, ts_right_parenthesis, "Expect )");
                pjs->parse_exec = cond->value.boolean;
                js_parse_statement(pjs); // skip if cond == false
                pjs->mark_continue = false; // if not set to false, next cond may be NULL
            }
            pjs->parse_exec = true; // restore execute
            pjs->mark_break = false;
            pjs->mark_continue = false;
        } else if (_accept(pjs, ts_do)) {
            tok_idx_backup = pjs->tok_cache_idx;
            do {
                pjs->tok_cache_idx = tok_idx_backup;
                js_parse_statement(pjs);
                pjs->mark_continue = false; // https://stackoverflow.com/questions/64120214/continue-statement-in-a-do-while-loop
                _expect(pjs, ts_while, "Expect while");
                _expect(pjs, ts_left_parenthesis, "Expect (");
                cond = js_parse_expression(pjs); // may be NULL due to 'break' mark
                if (cond && cond->type != vt_boolean) {
                    js_throw(pjs, "Condition must be boolean");
                }
                _expect(pjs, ts_right_parenthesis, "Expect )");
            } while ((cond && cond->value.boolean) && !pjs->mark_break);
            pjs->mark_break = false;
            pjs->mark_continue = false;
            _expect(pjs, ts_semicolon, "Expect ;");
        } else if (_get_token_state(pjs) == ts_for) { // DONT use _accept, _stack_forward will record token
            _stack_forward(pjs);
            _next_token(pjs);
            _expect(pjs, ts_left_parenthesis, "Expect (");
            if (_accept(pjs, ts_let)) {
                acc = js_parse_accessor(pjs);
                if (acc.type != at_identifier) {
                    js_throw(pjs, "Expect identifier");
                }
                if (_accept(pjs, ts_assignment)) {
                    js_variable_declare(pjs, acc.p, acc.n, js_parse_expression(pjs));
                    _expect(pjs, ts_semicolon, "Expect ;");
                    for_type = classic_for;
                } else if (_accept(pjs, ts_in)) {
                    js_variable_declare(pjs, acc.p, acc.n, js_null_new(pjs));
                    for_type = for_in;
                } else if (_accept(pjs, ts_of)) {
                    js_variable_declare(pjs, acc.p, acc.n, js_null_new(pjs));
                    for_type = for_of;
                } else {
                    js_throw(pjs, "Unknown for loop type");
                }
            } else if (_accept(pjs, ts_semicolon)) {
                for_type = classic_for;
            } else {
                acc = js_parse_accessor(pjs);
                if (_accept(pjs, ts_assignment)) {
                    _accessor_put(pjs, acc, js_parse_expression(pjs));
                    _expect(pjs, ts_semicolon, "Expect ;");
                    for_type = classic_for;
                } else if (_accept(pjs, ts_in)) {
                    for_type = for_in;
                } else if (_accept(pjs, ts_of)) {
                    for_type = for_of;
                } else {
                    js_throw(pjs, "Unknown for loop type");
                }
            }
            // printf("for_type= %d\n", for_type);
            if (for_type == classic_for) {
                tok_idx_backup = pjs->tok_cache_idx;
                while (pjs->parse_exec && !pjs->mark_break) {
                    pjs->tok_cache_idx = tok_idx_backup;
                    if (_accept(pjs, ts_semicolon)) {
                        cond = js_boolean_new(pjs, true);
                    } else {
                        cond = js_parse_expression(pjs);
                        if (cond->type != vt_boolean) {
                            js_throw(pjs, "Condition must be boolean");
                        }
                        _expect(pjs, ts_semicolon, "Expect ;");
                    }
                    tok_idx_backup_2 = pjs->tok_cache_idx;
                    pjs->parse_exec = false; // first, skip the 3rd part
                    if (!_accept(pjs, ts_right_parenthesis)) {
                        js_parse_assignment_expression(pjs);
                        _expect(pjs, ts_right_parenthesis, "Expect )");
                    }
                    pjs->parse_exec = cond->value.boolean;
                    js_parse_statement(pjs);
                    pjs->mark_continue = false;
                    tok_idx_backup_3 = pjs->tok_cache_idx;
                    pjs->tok_cache_idx = tok_idx_backup_2; // then, exec the 3rd pard
                    if (!_accept(pjs, ts_right_parenthesis)) {
                        js_parse_assignment_expression(pjs);
                    }
                    pjs->tok_cache_idx = tok_idx_backup_3; // restore end position or will stay at ')' when leaving while loop, will cause next statement parse error 'ts_right_parenthesis:): Expect identifier'
                }
                pjs->parse_exec = true;
                pjs->mark_break = false;
                pjs->mark_continue = false;
            } else {
                val = js_parse_access_call_expression(pjs);
                _expect(pjs, ts_right_parenthesis, "Expect )");
                tok_idx_backup = pjs->tok_cache_idx;
                loop_count = 0;
                if (val->type == vt_array) {
                    list_for_each(val->value.array, i, v, {
                        if (((struct js_value *)v)->type != vt_null) { // skip null
                            pjs->tok_cache_idx = tok_idx_backup;
                            if (for_type == for_in) {
                                _accessor_put(pjs, acc, js_number_new(pjs, (double)i));
                            } else {
                                _accessor_put(pjs, acc, (struct js_value *)v);
                            }
                            js_parse_statement(pjs);
                            loop_count++;
                            pjs->mark_continue = false;
                            if (pjs->mark_break) {
                                break;
                            }
                        }
                    });
                    pjs->mark_break = false;
                } else if (val->type == vt_object) {
                    map_for_each(val->value.object, k, v, {
                        if (((struct js_value *)v)->type != vt_null) { // skip null
                            pjs->tok_cache_idx = tok_idx_backup;
                            if (for_type == for_in) {
                                _accessor_put(pjs, acc, js_string_new(pjs, k->p, k->len));
                            } else {
                                _accessor_put(pjs, acc, (struct js_value *)v);
                            }
                            js_parse_statement(pjs);
                            loop_count++;
                            pjs->mark_continue = false;
                            if (pjs->mark_break) {
                                break;
                            }
                        }
                    });
                    pjs->mark_break = false;
                } else {
                    js_throw(pjs, "For in/of loop require array or object");
                }
                if (!loop_count) { // if loop didn't happen, still should walk through loop body
                    pjs->parse_exec = false;
                    js_parse_statement(pjs);
                    pjs->parse_exec = true;
                }
            }
            js_stack_backward(pjs);
            // puts("****** END OF FOR ******");
            // js_dump_stack(pjs);
        } else if (_accept(pjs, ts_break)) {
            pjs->mark_break = true;
            _expect(pjs, ts_semicolon, "Expect ;");
        } else if (_accept(pjs, ts_continue)) {
            pjs->mark_continue = true;
            _expect(pjs, ts_semicolon, "Expect ;");
        } else if (_get_token_state(pjs) == ts_function) {
            _expect(pjs, ts_function, "Expect 'function'");
            if (_get_token_state(pjs) != ts_identifier) {
                js_throw(pjs, "Expect function name");
            }
            ident_h = _get_token_head(pjs);
            ident_len = _get_token_length(pjs);
            _next_token(pjs);
            js_variable_declare(pjs, ident_h, ident_len, js_function_new(pjs, pjs->tok_cache_idx));
            // only walk through, no exec
            pjs->parse_exec = false;
            js_parse_function(pjs);
            pjs->parse_exec = true;
        } else if (_accept(pjs, ts_return)) {
            if (!_accept(pjs, ts_semicolon)) {
                pjs->result = js_parse_expression(pjs);
                _expect(pjs, ts_semicolon, "Expect ;");
            }
            pjs->mark_return = true;
        } else if (_get_token_state(pjs) == ts_let) {
            js_parse_declaration_expression(pjs);
            _expect(pjs, ts_semicolon, "Expect ;");
            // puts("****** END OF LET ******");
            // js_dump_stack(pjs);
        } else {
            js_parse_assignment_expression(pjs);
            _expect(pjs, ts_semicolon, "Expect ;");
        }
    } else {
        if (_accept(pjs, ts_semicolon)) {
            ;
        } else if (_accept(pjs, ts_left_brace)) {
            while (_get_token_state(pjs) != ts_right_brace) {
                js_parse_statement(pjs);
            }
            _next_token(pjs);
        } else if (_accept(pjs, ts_if)) {
            _expect(pjs, ts_left_parenthesis, "Expect (");
            js_parse_expression(pjs);
            _expect(pjs, ts_right_parenthesis, "Expect )");
            js_parse_statement(pjs);
            if (_accept(pjs, ts_else)) {
                js_parse_statement(pjs);
            }
        } else if (_accept(pjs, ts_while)) {
            _expect(pjs, ts_left_parenthesis, "Expect (");
            js_parse_expression(pjs);
            _expect(pjs, ts_right_parenthesis, "Expect )");
            js_parse_statement(pjs);
        } else if (_accept(pjs, ts_do)) {
            js_parse_statement(pjs);
            _expect(pjs, ts_while, "Expect while");
            _expect(pjs, ts_left_parenthesis, "Expect (");
            js_parse_expression(pjs);
            _expect(pjs, ts_right_parenthesis, "Expect )");
            _expect(pjs, ts_semicolon, "Expect ;");
        } else if (_accept(pjs, ts_for)) {
            _expect(pjs, ts_left_parenthesis, "Expect (");
            if (_accept(pjs, ts_let)) {
                _expect(pjs, ts_identifier, "Expect identifier");
                if (_accept(pjs, ts_assignment)) {
                    js_parse_expression(pjs);
                    _expect(pjs, ts_semicolon, "Expect ;");
                    for_type = classic_for;
                } else if (_accept(pjs, ts_in)) {
                    for_type = for_in;
                } else if (_accept(pjs, ts_of)) {
                    for_type = for_of;
                } else {
                    js_throw(pjs, "Unknown for loop type");
                }
            } else if (_accept(pjs, ts_semicolon)) {
                for_type = classic_for;
            } else {
                js_parse_accessor(pjs);
                if (_accept(pjs, ts_assignment)) {
                    js_parse_expression(pjs);
                    _expect(pjs, ts_semicolon, "Expect ;");
                    for_type = classic_for;
                } else if (_accept(pjs, ts_in)) {
                    for_type = for_in;
                } else if (_accept(pjs, ts_of)) {
                    for_type = for_of;
                } else {
                    js_throw(pjs, "Unknown for loop type");
                }
            }
            if (for_type == classic_for) {
                if (!_accept(pjs, ts_semicolon)) {
                    js_parse_expression(pjs);
                    _expect(pjs, ts_semicolon, "Expect ;");
                }
                if (!_accept(pjs, ts_right_parenthesis)) {
                    js_parse_assignment_expression(pjs);
                    _expect(pjs, ts_right_parenthesis, "Expect )");
                }
                js_parse_statement(pjs);
            } else {
                js_parse_access_call_expression(pjs);
                _expect(pjs, ts_right_parenthesis, "Expect )");
                js_parse_statement(pjs);
            }
        } else if (_accept(pjs, ts_break)) {
            _expect(pjs, ts_semicolon, "Expect ;");
        } else if (_accept(pjs, ts_continue)) {
            _expect(pjs, ts_semicolon, "Expect ;");
        } else if (_get_token_state(pjs) == ts_function) {
            _expect(pjs, ts_function, "Expect 'function'");
            _expect(pjs, ts_identifier, "Expect function name");
            js_parse_function(pjs);
        } else if (_accept(pjs, ts_return)) {
            if (!_accept(pjs, ts_semicolon)) {
                js_parse_expression(pjs);
                _expect(pjs, ts_semicolon, "Expect ;");
            }
        } else if (_get_token_state(pjs) == ts_let) {
            js_parse_declaration_expression(pjs);
            _expect(pjs, ts_semicolon, "Expect ;");
        } else {
            js_parse_assignment_expression(pjs);
            _expect(pjs, ts_semicolon, "Expect ;");
        }
    }
}

void js_parse_function(struct js *pjs) {
    bool exec = pjs->parse_exec && !pjs->mark_break && !pjs->mark_continue && !pjs->mark_return;
    log("%s", exec ? "Parse and execute" : "Parse only");
    if (exec) {
        char *ident_h;
        size_t ident_len;
        size_t i = 0, j;
        struct js_value *param;
        _expect(pjs, ts_left_parenthesis, "Expect (");
        if (!_accept(pjs, ts_right_parenthesis)) {
            for (;; i++) {
                if (_accept(pjs, ts_spread)) {
                    // rest parameters
                    if (_get_token_state(pjs) != ts_identifier) {
                        js_throw(pjs, "Expect variable name");
                    }
                    ident_h = _get_token_head(pjs);
                    ident_len = _get_token_length(pjs);
                    _next_token(pjs);
                    param = js_array_new(pjs);
                    for (j = i; j < js_parameter_length(pjs); j++) {
                        js_array_push(pjs, param, js_parameter_get(pjs, j));
                    }
                    js_variable_declare(pjs, ident_h, ident_len, param);
                    _expect(pjs, ts_right_parenthesis, "Expect )");
                    break;
                } else {
                    if (_get_token_state(pjs) != ts_identifier) {
                        js_throw(pjs, "Expect variable name");
                    }
                    ident_h = _get_token_head(pjs);
                    ident_len = _get_token_length(pjs);
                    _next_token(pjs);
                    if (_accept(pjs, ts_assignment)) {
                        js_variable_declare(pjs, ident_h, ident_len, js_parse_expression(pjs));
                    } else {
                        js_variable_declare(pjs, ident_h, ident_len, js_null_new(pjs));
                    }
                    param = js_parameter_get(pjs, i);
                    if (param->type != vt_null) {
                        js_variable_assign(pjs, ident_h, ident_len, param);
                    }
                    if (_accept(pjs, ts_comma)) {
                        continue;
                    } else if (_accept(pjs, ts_right_parenthesis)) {
                        break;
                    } else {
                        js_throw(pjs, "Expect , or )");
                    }
                }
            }
        }
        _expect(pjs, ts_left_brace, "Expect {");
        while (_get_token_state(pjs) != ts_right_brace) {
            js_parse_statement(pjs);
        }
        _next_token(pjs);
    } else {
        _expect(pjs, ts_left_parenthesis, "Expect (");
        if (!_accept(pjs, ts_right_parenthesis)) {
            for (;;) {
                if (_accept(pjs, ts_spread)) {
                    _expect(pjs, ts_identifier, "Expect variable name");
                    _expect(pjs, ts_right_parenthesis, "Expect )");
                    break;
                } else {
                    _expect(pjs, ts_identifier, "Expect variable name");
                    if (_accept(pjs, ts_assignment)) {
                        js_parse_expression(pjs);
                    }
                    if (_accept(pjs, ts_comma)) {
                        continue;
                    } else if (_accept(pjs, ts_right_parenthesis)) {
                        break;
                    } else {
                        js_throw(pjs, "Expect , or )");
                    }
                }
            }
        }
        _expect(pjs, ts_left_brace, "Expect {");
        while (_get_token_state(pjs) != ts_right_brace) {
            js_parse_statement(pjs);
        }
        _next_token(pjs);
    }
}

void js_parser_print_error(struct js *pjs) {
    // Here, cache index may exceeds cache boundary, so DONT use token head and length
    // printf("%d %p\n", (int)_get_token_length(pjs), _get_token_head(pjs));
    printf("%s:%ld %ld:%s: %s\n", pjs->err_file, pjs->err_line,
           _get_token_line(pjs), js_token_state_name(_get_token_state(pjs)), pjs->err_msg);
}
