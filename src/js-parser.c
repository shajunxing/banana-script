/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

static char *_token_head(struct js *pjs) {
    return pjs->src + pjs->tok.h;
}

static size_t _token_length(struct js *pjs) {
    return pjs->tok.t - pjs->tok.h;
}

static char *_token_string_head(struct js *pjs) {
    return _token_head(pjs) + 1;
}

static size_t _token_string_length(struct js *pjs) {
    return _token_length(pjs) - 2;
}

static void _next_token(struct js *pjs) {
    js_next_token(pjs);
    // js_token_dump(pjs);
    // printf("\n");
}

#define _add_bytecode(pjs, op, ...) (js_add_bytecode((pjs), js_bytecode((op), ##__VA_ARGS__)))

#define _next_bytecode_position(pjs) (pjs->bytecodes_len)

static struct js_value _inscr(struct js *pjs, char *p, size_t len) {
    return js_inscription(js_inscribe_tablet(pjs, p, len), len);
}

static bool _accept(struct js *pjs, enum js_token_state stat) {
    if (pjs->tok.stat == stat) {
        _next_token(pjs);
        return true;
    }
    return false;
}

// use macro instead of  function, to correctly record error position
// DONT use '_pjs' inside do while, or msvc will cause lots of 'warning C4700: uninitialized local variable '_pjs' used', don't know why, maybe bug
#define _expect(pjs, stat, msg)        \
    do {                               \
        struct js *__pjs = (pjs);      \
        if (!_accept(__pjs, (stat))) { \
            js_throw(__pjs, (msg));    \
        }                              \
    } while (0)

static void _parse_statement(struct js *pjs, size_t break_pos, size_t continue_pos);

static void _parse_function(struct js *pjs) {
    size_t p0, p1, p2;
    size_t i;
    p0 = _add_bytecode(pjs, op_jump, js_number(0));
    p1 = _next_bytecode_position(pjs);
    _expect(pjs, ts_left_parenthesis, "Expect (");
    i = 0;
    if (!_accept(pjs, ts_right_parenthesis)) {
        for (;;) {
            if (_accept(pjs, ts_spread)) {
                if (pjs->tok.stat != ts_identifier) {
                    js_throw(pjs, "Expect parameter name");
                }
                _add_bytecode(pjs, op_value, _inscr(pjs, _token_head(pjs), _token_length(pjs)));
                i++;
                _next_token(pjs);
                _expect(pjs, ts_right_parenthesis, "Expect )");
                break;
            } else {
                if (pjs->tok.stat != ts_identifier) {
                    js_throw(pjs, "Expect parameter name");
                }
                _add_bytecode(pjs, op_value, _inscr(pjs, _token_head(pjs), _token_length(pjs)));
                i++;
                _next_token(pjs);
                if (_accept(pjs, ts_assignment)) {
                    js_parse_expression(pjs);
                } else {
                    _add_bytecode(pjs, op_value, js_null());
                }
                i++;
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
    _add_bytecode(pjs, op_parameter_get, js_number((double)i));
    _expect(pjs, ts_left_brace, "Expect {");
    while (pjs->tok.stat != ts_right_brace) {
        _parse_statement(pjs, UINT_MAX, UINT_MAX);
    }
    _next_token(pjs);
    _add_bytecode(pjs, op_value, js_null());
    _add_bytecode(pjs, op_return);
    p2 = _add_bytecode(pjs, op_function, js_number((double)p1));
    (pjs->bytecodes + p0)->operand = js_number((double)p2);
}

static void _parse_value(struct js *pjs) {
    switch (pjs->tok.stat) {
    case ts_null:
        _add_bytecode(pjs, op_value, js_null());
        _next_token(pjs);
        break;
    case ts_true:
        _add_bytecode(pjs, op_value, js_boolean(true));
        _next_token(pjs);
        break;
    case ts_false:
        _add_bytecode(pjs, op_value, js_boolean(false));
        _next_token(pjs);
        break;
    case ts_number:
        _add_bytecode(pjs, op_value, js_number(pjs->tok.num));
        _next_token(pjs);
        break;
    case ts_string:
        _add_bytecode(pjs, op_value, _inscr(pjs, _token_string_head(pjs), _token_string_length(pjs)));
        _next_token(pjs);
        break;
    case ts_left_bracket:
        _add_bytecode(pjs, op_array);
        _next_token(pjs);
        if (!_accept(pjs, ts_right_bracket)) {
            for (;;) {
                if (_accept(pjs, ts_spread)) {
                    js_parse_expression(pjs);
                    _add_bytecode(pjs, op_array_spread);
                } else {
                    js_parse_expression(pjs);
                    _add_bytecode(pjs, op_array_push);
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
        break;
    case ts_left_brace:
        _add_bytecode(pjs, op_object);
        _next_token(pjs);
        if (!_accept(pjs, ts_right_brace)) {
            for (;;) {
                if (pjs->tok.stat == ts_string) {
                    _add_bytecode(pjs, op_value, _inscr(pjs, _token_string_head(pjs), _token_string_length(pjs)));
                } else if (pjs->tok.stat == ts_identifier) {
                    _add_bytecode(pjs, op_value, _inscr(pjs, _token_head(pjs), _token_length(pjs)));
                } else {
                    js_throw(pjs, "Expect string or identifier");
                }
                _next_token(pjs);
                _expect(pjs, ts_colon, "Expect :");
                js_parse_expression(pjs);
                _add_bytecode(pjs, op_member_put);
                if (_accept(pjs, ts_comma)) {
                    continue;
                } else if (_accept(pjs, ts_right_brace)) {
                    break;
                } else {
                    js_throw(pjs, "Expect , or }");
                }
            }
        }
        break;
    case ts_function:
        _next_token(pjs);
        _parse_function(pjs);
        break;
    default:
        js_throw(pjs, "Not a value literal");
    }
}

static void _parse_additive_expression(struct js *pjs);

#define _accessor_type_list \
    X(at_value)             \
    X(at_identifier)        \
    X(at_member_access)     \
    X(at_optional_chaining)

#define X(name) name,
enum _accessor_type { _accessor_type_list };
#undef X

struct _accessor {
    enum _accessor_type type;
    struct js_value value;
};

static const char *_accessor_type_name(enum _accessor_type type) {
#define X(name) #name,
    static const char *const names[] = {_accessor_type_list};
#undef X
    if (type >= 0 && type < countof(names)) {
        return names[type];
    } else {
        return "???";
    }
}

static void _accessor_put(struct js *pjs, struct _accessor acc) {
    switch (acc.type) {
    case at_identifier:
        _add_bytecode(pjs, op_variable_put, acc.value);
        break;
    case at_member_access:
        _add_bytecode(pjs, op_member_put);
        _add_bytecode(pjs, op_eval_stack_pop, js_number(1));
        break;
    default:
        js_throw(pjs, "Illegal accessor type for put operation");
    }
}

static void _accessor_get(struct js *pjs, struct _accessor acc) {
    switch (acc.type) {
    case at_identifier:
        _add_bytecode(pjs, op_variable_get, acc.value);
        break;
    case at_member_access:
        _add_bytecode(pjs, op_member_get);
        break;
    case at_optional_chaining:
        _add_bytecode(pjs, op_object_optional);
        break;
    default:
        break;
    }
}

static struct _accessor _parse_accessor(struct js *pjs) {
    struct _accessor acc;
    size_t p0, p1;
    if (_accept(pjs, ts_left_parenthesis)) {
        js_parse_expression(pjs);
        _expect(pjs, ts_right_parenthesis, "Expect )");
        acc.type = at_value;
    } else if (pjs->tok.stat == ts_identifier) {
        acc.value = _inscr(pjs, _token_head(pjs), _token_length(pjs));
        _next_token(pjs);
        acc.type = at_identifier;
    } else {
        _parse_value(pjs);
        acc.type = at_value;
    }
    for (;;) {
        if (_accept(pjs, ts_left_bracket)) {
            _accessor_get(pjs, acc);
            _parse_additive_expression(pjs);
            _expect(pjs, ts_right_bracket, "Expect ]");
            acc.type = at_member_access;
        } else if (_accept(pjs, ts_member_access)) {
            _accessor_get(pjs, acc);
            if (pjs->tok.stat == ts_identifier) {
                _add_bytecode(pjs, op_value, _inscr(pjs, _token_head(pjs), _token_length(pjs)));
                _next_token(pjs);
                acc.type = at_member_access;
            } else {
                js_throw(pjs, "Must be object.identifier");
            }
        } else if (_accept(pjs, ts_optional_chaining)) {
            _accessor_get(pjs, acc);
            if (pjs->tok.stat == ts_identifier) {
                _add_bytecode(pjs, op_value, _inscr(pjs, _token_head(pjs), _token_length(pjs)));
                _next_token(pjs);
                acc.type = at_optional_chaining;
            } else {
                js_throw(pjs, "Must be object.identifier");
            }
        } else if (_accept(pjs, ts_left_parenthesis)) { // function call
            _accessor_get(pjs, acc);
            p0 = _add_bytecode(pjs, op_call_stack_push_function, js_number(0)); // return address
            if (!_accept(pjs, ts_right_parenthesis)) {
                for (;;) {
                    if (_accept(pjs, ts_spread)) {
                        js_parse_expression(pjs);
                        _add_bytecode(pjs, op_parameter_spread);
                    } else {
                        js_parse_expression(pjs);
                        _add_bytecode(pjs, op_parameter_push);
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
            _add_bytecode(pjs, op_call);
            p1 = _next_bytecode_position(pjs);
            (pjs->bytecodes + p0)->operand = js_number((double)p1);
            _add_bytecode(pjs, op_call_stack_pop);
            _add_bytecode(pjs, op_get_result);
            acc.type = at_value; // it is value now
        } else {
            break;
        }
    }
    return acc;
}

static void _parse_access_call_expression(struct js *pjs) {
    _accessor_get(pjs, _parse_accessor(pjs));
}

static void _parse_prefix_expression(struct js *pjs) {
    enum js_token_state stat = pjs->tok.stat;
    if (stat == ts_typeof || stat == ts_not || stat == ts_plus || stat == ts_minus) {
        if (stat == ts_minus) {
            _add_bytecode(pjs, op_value, js_number(0));
        }
        _next_token(pjs);
        _parse_access_call_expression(pjs);
        switch (stat) {
        case ts_typeof:
            _add_bytecode(pjs, op_typeof);
            break;
        case ts_not:
            _add_bytecode(pjs, op_not);
            break;
        case ts_minus:
            _add_bytecode(pjs, op_sub);
            break;
        default:
            break;
        }
    } else {
        _parse_access_call_expression(pjs);
    }
}

static void _parse_exponential_expression(struct js *pjs) {
    _parse_prefix_expression(pjs);
    while (_accept(pjs, ts_exponentiation)) {
        _parse_prefix_expression(pjs);
        _add_bytecode(pjs, op_pow);
    }
}

static void _parse_multiplicative_expression(struct js *pjs) {
    enum js_token_state stat;
    _parse_exponential_expression(pjs);
    while (pjs->tok.stat == ts_multiplication || pjs->tok.stat == ts_division || pjs->tok.stat == ts_mod) {
        stat = pjs->tok.stat;
        _next_token(pjs);
        _parse_exponential_expression(pjs);
        switch (stat) {
        case ts_multiplication:
            _add_bytecode(pjs, op_mul);
            break;
        case ts_division:
            _add_bytecode(pjs, op_div);
            break;
        case ts_mod:
            _add_bytecode(pjs, op_mod);
            break;
        default:
            break;
        }
    }
}

// needed by _parse_accessor()
static void _parse_additive_expression(struct js *pjs) {
    enum js_token_state stat;
    _parse_multiplicative_expression(pjs);
    while (pjs->tok.stat == ts_plus || pjs->tok.stat == ts_minus) {
        stat = pjs->tok.stat;
        _next_token(pjs);
        _parse_multiplicative_expression(pjs);
        switch (stat) {
        case ts_plus:
            _add_bytecode(pjs, op_add);
            break;
        case ts_minus:
            _add_bytecode(pjs, op_sub);
            break;
        default:
            break;
        }
    }
}

static void _parse_relational_expression(struct js *pjs) {
    enum js_token_state stat;
    _parse_additive_expression(pjs);
    stat = pjs->tok.stat;
    if (stat == ts_equal_to || stat == ts_not_equal_to || stat == ts_less_than || stat == ts_less_than_or_equal_to || stat == ts_greater_than || stat == ts_greater_than_or_equal_to) {
        _next_token(pjs);
        _parse_additive_expression(pjs);
        switch (stat) {
        case ts_equal_to:
            _add_bytecode(pjs, op_eq);
            break;
        case ts_not_equal_to:
            _add_bytecode(pjs, op_ne);
            break;
        case ts_less_than:
            _add_bytecode(pjs, op_lt);
            break;
        case ts_less_than_or_equal_to:
            _add_bytecode(pjs, op_le);
            break;
        case ts_greater_than:
            _add_bytecode(pjs, op_gt);
            break;
        case ts_greater_than_or_equal_to:
            _add_bytecode(pjs, op_ge);
            break;
        default:
            break;
        }
    }
}

static void _parse_logical_and_expression(struct js *pjs) {
    _parse_relational_expression(pjs);
    while (_accept(pjs, ts_and)) {
        _parse_relational_expression(pjs);
        _add_bytecode(pjs, op_and);
    }
}

static void _parse_logical_or_expression(struct js *pjs) {
    _parse_logical_and_expression(pjs);
    while (_accept(pjs, ts_or)) {
        _parse_logical_and_expression(pjs);
        _add_bytecode(pjs, op_or);
    }
}

// ternary expression as root
void js_parse_expression(struct js *pjs) {
    _parse_logical_or_expression(pjs);
    if (_accept(pjs, ts_question)) {
        _parse_logical_or_expression(pjs);
        _expect(pjs, ts_colon, "Expect :");
        _parse_logical_or_expression(pjs);
        _add_bytecode(pjs, op_ternary);
    }
}

static void _parse_assignment_expression(struct js *pjs) {
    enum js_token_state stat;
    struct _accessor acc;
    acc = _parse_accessor(pjs);
    stat = pjs->tok.stat;
    if (stat == ts_assignment) { // assignment is optional, for example, function call is l-value but not need assignment
        if (acc.type != at_identifier && acc.type != at_member_access) {
            js_throw(pjs, "Assignment expression's l-value can only be identifier or member access");
        }
        _next_token(pjs);
        js_parse_expression(pjs);
        _accessor_put(pjs, acc);
    } else if (stat == ts_plus_assignment || stat == ts_minus_assignment || stat == ts_multiplication_assignment || stat == ts_exponentiation_assignment || stat == ts_division_assignment || stat == ts_mod_assignment || stat == ts_plus_plus || stat == ts_minus_minus) {
        if (acc.type != at_identifier && acc.type != at_member_access) {
            js_throw(pjs, "Assignment expression's l-value can only be identifier or member access");
        }
        // first, get value
        if (acc.type == at_member_access) {
            _add_bytecode(pjs, op_eval_stack_duplicate, js_number(-1));
            _add_bytecode(pjs, op_eval_stack_duplicate, js_number(-1));
        }
        _accessor_get(pjs, acc);
        _next_token(pjs);
        switch (stat) {
        case ts_plus_assignment:
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_add);
            break;
        case ts_minus_assignment:
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_sub);
            break;
        case ts_multiplication_assignment:
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_mul);
            break;
        case ts_division_assignment:
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_div);
            break;
        case ts_mod_assignment:
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_mod);
            break;
        case ts_exponentiation_assignment:
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_pow);
            break;
        case ts_plus_plus:
            _add_bytecode(pjs, op_value, js_number(1));
            _add_bytecode(pjs, op_add);
            break;
        case ts_minus_minus:
            _add_bytecode(pjs, op_value, js_number(1));
            _add_bytecode(pjs, op_sub);
            break;
        default:
            break;
        }
        _accessor_put(pjs, acc);
    } else { // no assignment, just clear ev stack
        switch (acc.type) {
        case at_value:
            _add_bytecode(pjs, op_eval_stack_pop, js_number(1));
            break;
        case at_member_access:
        case at_optional_chaining:
            _add_bytecode(pjs, op_eval_stack_pop, js_number(2));
            break;
        default:
            break;
        }
    }
}

static void _parse_declaration_expression(struct js *pjs) {
    struct js_value var_name;
    _expect(pjs, ts_let, "Expect let");
    for (;;) {
        if (pjs->tok.stat != ts_identifier) {
            js_throw(pjs, "Expect variable name");
        }
        var_name = _inscr(pjs, _token_head(pjs), _token_length(pjs));
        _next_token(pjs);
        if (_accept(pjs, ts_assignment)) {
            js_parse_expression(pjs);
        } else {
            _add_bytecode(pjs, op_value, js_null());
        }
        _add_bytecode(pjs, op_variable_declare, var_name);
        if (_accept(pjs, ts_comma)) {
            continue;
        } else {
            break;
        }
    }
}

// designed for break continue, to parse twice, first time know loop end position and second time fill to break jmp
struct _parser_state {
    struct js_token tok;
    size_t bytecode_pos;
};

static struct _parser_state _backup_state(struct js *pjs) {
    struct _parser_state stat;
    stat.tok = pjs->tok;
    stat.bytecode_pos = pjs->bytecodes_len;
    return stat;
}

static void _restore_state(struct js *pjs, struct _parser_state stat) {
    pjs->tok = stat.tok;
    pjs->bytecodes_len = stat.bytecode_pos;
}

// needed by _parse_function()
static void _parse_statement(struct js *pjs, size_t break_pos, size_t continue_pos) {
    struct js_value var_name;
    size_t p0, p1, p2, p3, p4, p5, p6, p7;
    struct _parser_state s0, s1;
    enum { classic_for,
           for_in,
           for_of } for_type;
    struct _accessor acc;
    if (_accept(pjs, ts_semicolon)) {
        ;
    } else if (pjs->tok.stat == ts_left_brace) { // DONT use _accept, _stack_forward will record token
        _add_bytecode(pjs, op_call_stack_push_block);
        _next_token(pjs);
        while (pjs->tok.stat != ts_right_brace) {
            _parse_statement(pjs, break_pos, continue_pos);
        }
        _add_bytecode(pjs, op_call_stack_pop);
        _next_token(pjs);
    } else if (_accept(pjs, ts_if)) {
        _expect(pjs, ts_left_parenthesis, "Expect (");
        js_parse_expression(pjs);
        p0 = _add_bytecode(pjs, op_jump_if_false, js_number(0)); // jmp_f to 'else' or last instruction + 1
        _expect(pjs, ts_right_parenthesis, "Expect )");
        _parse_statement(pjs, break_pos, continue_pos);
        p1 = _add_bytecode(pjs, op_jump, js_number(0)); // jmp tp last instruction + 1
        if (_accept(pjs, ts_else)) {
            _parse_statement(pjs, break_pos, continue_pos);
        }
        p2 = _next_bytecode_position(pjs); // last instruction + 1
        // printf("p0=%d, p1=%d, p2=%d\n", p0, p1, p2);
        (pjs->bytecodes + p0)->operand = js_number((double)(p1 + 1));
        (pjs->bytecodes + p1)->operand = js_number((double)p2);
    } else if (_accept(pjs, ts_while)) {
        p0 = _add_bytecode(pjs, op_call_stack_backup);
        _expect(pjs, ts_left_parenthesis, "Expect (");
        js_parse_expression(pjs);
        p1 = _add_bytecode(pjs, op_jump_if_false, js_number(0));
        _expect(pjs, ts_right_parenthesis, "Expect )");
        s0 = _backup_state(pjs); // backup for second parsing to fullfill 'break' 'continue' address
        _parse_statement(pjs, 0, 0);
        p2 = _add_bytecode(pjs, op_call_stack_restore);
        _add_bytecode(pjs, op_jump, js_number((double)p0));
        p3 = _add_bytecode(pjs, op_call_stack_restore);
        (pjs->bytecodes + p1)->operand = js_number((double)p3);
        s1 = _backup_state(pjs);
        _restore_state(pjs, s0); // second parsing
        _parse_statement(pjs, p3, p2);
        _restore_state(pjs, s1);
    } else if (_accept(pjs, ts_do)) {
        p0 = _add_bytecode(pjs, op_call_stack_backup);
        s0 = _backup_state(pjs);
        _parse_statement(pjs, 0, 0);
        _expect(pjs, ts_while, "Expect while");
        _expect(pjs, ts_left_parenthesis, "Expect (");
        js_parse_expression(pjs);
        _expect(pjs, ts_right_parenthesis, "Expect )");
        _expect(pjs, ts_semicolon, "Expect ;");
        p1 = _add_bytecode(pjs, op_jump_if_false, js_number(0));
        p2 = _add_bytecode(pjs, op_call_stack_restore);
        _add_bytecode(pjs, op_jump, js_number((double)p0));
        p3 = _add_bytecode(pjs, op_call_stack_restore);
        (pjs->bytecodes + p1)->operand = js_number((double)p3);
        s1 = _backup_state(pjs);
        _restore_state(pjs, s0);
        _parse_statement(pjs, p3, p2);
        _restore_state(pjs, s1);
    } else if (_accept(pjs, ts_for)) {
        _add_bytecode(pjs, op_call_stack_push_for);
        _expect(pjs, ts_left_parenthesis, "Expect (");
        if (_accept(pjs, ts_let)) {
            if (pjs->tok.stat != ts_identifier) {
                js_throw(pjs, "Expect variable name");
            }
            var_name = _inscr(pjs, _token_head(pjs), _token_length(pjs));
            _next_token(pjs);
            if (_accept(pjs, ts_assignment)) {
                js_parse_expression(pjs);
                _add_bytecode(pjs, op_variable_declare, var_name);
                _expect(pjs, ts_semicolon, "Expect ;");
                for_type = classic_for;
            } else if (_accept(pjs, ts_in)) {
                _add_bytecode(pjs, op_value, js_null());
                _add_bytecode(pjs, op_variable_declare, var_name);
                acc.type = at_identifier;
                acc.value = var_name;
                for_type = for_in;
            } else if (_accept(pjs, ts_of)) {
                _add_bytecode(pjs, op_value, js_null());
                _add_bytecode(pjs, op_variable_declare, var_name);
                acc.type = at_identifier;
                acc.value = var_name;
                for_type = for_of;
            } else {
                js_throw(pjs, "Unknown for loop type");
            }
        } else if (_accept(pjs, ts_semicolon)) {
            for_type = classic_for;
        } else {
            acc = _parse_accessor(pjs);
            if (_accept(pjs, ts_assignment)) {
                js_parse_expression(pjs);
                _accessor_put(pjs, acc);
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
            p0 = _add_bytecode(pjs, op_call_stack_backup);
            if (_accept(pjs, ts_semicolon)) { // section 2
                _add_bytecode(pjs, op_value, js_boolean(true));
            } else {
                js_parse_expression(pjs);
                _expect(pjs, ts_semicolon, "Expect ;");
            }
            p1 = _add_bytecode(pjs, op_jump_if_false, js_number(0));
            p2 = _add_bytecode(pjs, op_jump, js_number(0));
            p3 = _next_bytecode_position(pjs);
            if (!_accept(pjs, ts_right_parenthesis)) { // section 3
                _parse_assignment_expression(pjs);
                _expect(pjs, ts_right_parenthesis, "Expect )");
            }
            p4 = _add_bytecode(pjs, op_jump, js_number(0));
            p5 = _next_bytecode_position(pjs);
            s0 = _backup_state(pjs);
            _parse_statement(pjs, 0, 0); // statement
            _add_bytecode(pjs, op_jump, js_number((double)p3));
            p6 = _add_bytecode(pjs, op_call_stack_restore);
            _add_bytecode(pjs, op_jump, js_number((double)p0));
            p7 = _add_bytecode(pjs, op_call_stack_restore);
            (pjs->bytecodes + p1)->operand = js_number((double)p7);
            (pjs->bytecodes + p2)->operand = js_number((double)p5);
            (pjs->bytecodes + p4)->operand = js_number((double)p6);
            s1 = _backup_state(pjs);
            _restore_state(pjs, s0);
            _parse_statement(pjs, p7, p3);
            _restore_state(pjs, s1);
        } else {
            _parse_access_call_expression(pjs);
            _add_bytecode(pjs, op_value, js_number(0));
            p0 = _add_bytecode(pjs, op_call_stack_backup);
            p1 = _add_bytecode(pjs, for_type == for_in ? op_for_in : op_for_of, js_number(0));
            printf("acc = %s\n", _accessor_type_name(acc.type));
            if (acc.type == at_identifier) {
                _accessor_put(pjs, acc);
            } else {
                _add_bytecode(pjs, op_eval_stack_duplicate, js_number(-4));
                _add_bytecode(pjs, op_eval_stack_duplicate, js_number(-4));
                _add_bytecode(pjs, op_eval_stack_duplicate, js_number(-2));
                _accessor_put(pjs, acc);
                _add_bytecode(pjs, op_eval_stack_pop, js_number(1));
            }
            _expect(pjs, ts_right_parenthesis, "Expect )");
            s0 = _backup_state(pjs);
            _parse_statement(pjs, 0, 0);
            p2 = _add_bytecode(pjs, op_call_stack_restore);
            _add_bytecode(pjs, op_jump, js_number((double)p0));
            p3 = _add_bytecode(pjs, op_call_stack_restore);
            _add_bytecode(pjs, op_eval_stack_pop, js_number(acc.type == at_identifier ? 2 : 4));
            (pjs->bytecodes + p1)->operand = js_number((double)p3);
            s1 = _backup_state(pjs);
            _restore_state(pjs, s0);
            _parse_statement(pjs, p3, p2);
            _restore_state(pjs, s1);
        }
        _add_bytecode(pjs, op_call_stack_pop);
    } else if (_accept(pjs, ts_break)) {
        _expect(pjs, ts_semicolon, "Expect ;");
        if (break_pos == UINT_MAX) {
            js_throw(pjs, "Statement 'break' can't be outside loop");
        }
        p0 = _next_bytecode_position(pjs);
        _add_bytecode(pjs, op_jump, js_number((double)break_pos));
    } else if (_accept(pjs, ts_continue)) {
        _expect(pjs, ts_semicolon, "Expect ;");
        if (continue_pos == UINT_MAX) {
            js_throw(pjs, "Statement 'continue' can't be outside loop");
        }
        p0 = _next_bytecode_position(pjs);
        _add_bytecode(pjs, op_jump, js_number((double)continue_pos));
    } else if (pjs->tok.stat == ts_function) {
        _next_token(pjs);
        if (pjs->tok.stat != ts_identifier) {
            js_throw(pjs, "Expect function name");
        }
        var_name = _inscr(pjs, _token_head(pjs), _token_length(pjs));
        _next_token(pjs);
        _parse_function(pjs);
        _add_bytecode(pjs, op_variable_declare, var_name);
    } else if (_accept(pjs, ts_return)) {
        if (!_accept(pjs, ts_semicolon)) {
            js_parse_expression(pjs);
            _add_bytecode(pjs, op_return);
            _expect(pjs, ts_semicolon, "Expect ;");
        }
    } else if (_accept(pjs, ts_delete)) {
        if (pjs->tok.stat == ts_identifier) {
            var_name = _inscr(pjs, _token_head(pjs), _token_length(pjs));
            _add_bytecode(pjs, op_variable_delete, var_name);
        } else {
            js_throw(pjs, "Expect identifier");
        }
        _next_token(pjs);
        _expect(pjs, ts_semicolon, "Expect ;");
    } else if (pjs->tok.stat == ts_let) {
        _parse_declaration_expression(pjs);
        _expect(pjs, ts_semicolon, "Expect ;");
    } else {
        _parse_assignment_expression(pjs);
        _expect(pjs, ts_semicolon, "Expect ;");
    }
}

void js_parse_script(struct js *pjs) {
    while (pjs->tok.stat != ts_end_of_file) {
        _parse_statement(pjs, UINT_MAX, UINT_MAX);
    }
}
