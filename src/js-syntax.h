/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_SYNTAX_H
#define JS_SYNTAX_H

#include "js-vm.h"

struct js_source {
    char *base;
    uint32_t length;
    uint32_t capacity;
};

// https://codelucky.com/c-macros/
// https://en.wikipedia.org/wiki/X_Macro
// must use X, others will cause clang-format wrong indent, don't know why
#define js_token_state_list                                              \
    X(ts_searching)                                                      \
    X(ts_end_of_file)                                                    \
    X(ts_identifier_matching)                                            \
    X(ts_identifier)                                                     \
    X(ts_division_matching)                                              \
    X(ts_division)                                                       \
    X(ts_line_comment_matching_new_line)                                 \
    X(ts_line_comment)                                                   \
    X(ts_block_comment_matching_end_star)                                \
    X(ts_block_comment_matching_end_slash)                               \
    X(ts_block_comment)                                                  \
    X(ts_minus)                                                          \
    X(ts_number_matching)                                                \
    X(ts_number_matching_fraction_first)                                 \
    X(ts_number_matching_fraction_rest)                                  \
    X(ts_number_matching_exponent_first)                                 \
    X(ts_number_matching_exponent_number_first)                          \
    X(ts_number_matching_exponent_rest)                                  \
    X(ts_number)                                                         \
    X(ts_plus)                                                           \
    X(ts_multiplication)                                                 \
    X(ts_left_parenthesis)                                               \
    X(ts_right_parenthesis)                                              \
    X(ts_left_bracket)                                                   \
    X(ts_right_bracket)                                                  \
    X(ts_left_brace)                                                     \
    X(ts_right_brace)                                                    \
    X(ts_string_matching)                                                \
    X(ts_string_matching_control)                                        \
    X(ts_string)                                                         \
    X(ts_null)                                                           \
    X(ts_true)                                                           \
    X(ts_false)                                                          \
    X(ts_colon)                                                          \
    X(ts_comma)                                                          \
    X(ts_assignment_matching)                                            \
    X(ts_equal_to)                                                       \
    X(ts_assignment)                                                     \
    X(ts_less_than_matching)                                             \
    X(ts_less_than_or_equal_to)                                          \
    X(ts_less_than)                                                      \
    X(ts_greater_than_matching)                                          \
    X(ts_greater_than_or_equal_to)                                       \
    X(ts_greater_than)                                                   \
    X(ts_not_matching)                                                   \
    X(ts_not_equal_to)                                                   \
    X(ts_not)                                                            \
    X(ts_and_matching)                                                   \
    X(ts_and)                                                            \
    X(ts_or_matching)                                                    \
    X(ts_or)                                                             \
    X(ts_mod)                                                            \
    X(ts_let)                                                            \
    X(ts_if)                                                             \
    X(ts_else)                                                           \
    X(ts_while)                                                          \
    X(ts_semicolon)                                                      \
    X(ts_do)                                                             \
    X(ts_for)                                                            \
    X(ts_one_dot_matching)                                               \
    X(ts_member_access)                                                  \
    X(ts_two_dot_matching)                                               \
    X(ts_spread)                                                         \
    X(ts_question_matching)                                              \
    X(ts_optional_chaining)                                              \
    X(ts_question)                                                       \
    X(ts_division_assignment)                                            \
    X(ts_minus_matching)                                                 \
    X(ts_minus_assignment)                                               \
    X(ts_minus_minus)                                                    \
    X(ts_plus_matching)                                                  \
    X(ts_plus_assignment)                                                \
    X(ts_plus_plus)                                                      \
    X(ts_multiplication_matching)                                        \
    X(ts_multiplication_assignment)                                      \
    X(ts_exponentiation_matching)                                        \
    X(ts_exponentiation_assignment)                                      \
    X(ts_exponentiation)                                                 \
    X(ts_mod_matching)                                                   \
    X(ts_mod_assignment)                                                 \
    X(ts_break)                                                          \
    X(ts_continue)                                                       \
    X(ts_function)                                                       \
    X(ts_return)                                                         \
    X(ts_in)                                                             \
    X(ts_of)                                                             \
    X(ts_typeof)                                                         \
    X(ts_delete)                                                         \
    X(ts_try)                                                            \
    X(ts_catch)                                                          \
    X(ts_finally) /* keep it to prevent wrongly be used as identifier */ \
    X(ts_throw)

#define X(name) name,
enum js_token_state { js_token_state_list };
#undef X

// move outside to be a struct, for backup restore token position in repl
struct js_token {
    uint8_t state;
    // src support increasement at run time, may be reallocated, so token position use offset instead of pointer
    uint32_t head_offset; // source may be reallocated, record offset
    uint32_t head_line;
    uint32_t tail_offset; // means current position
    uint32_t tail_line;
    // string
    // DON'T parse escape here, keep it simple, prevent memory copy and possible memory leak in syntax pasing stage
    // string head is always token head + 1, and length is token length - 2
    double integer; // temp data for calculating number
    double fraction;
    double fraction_depth;
    int exponent_sign;
    double exponent;
    double number; // final number result
};

shared bool js_compile(struct js_source *, struct js_token *, struct js_bytecode *, struct js_cross_reference *);

#ifndef NOTEST

shared int test_lexer(int, char *[]);
shared int test_parser(int, char *[]);
shared int test_c_function(int, char *[]);

#endif

#endif