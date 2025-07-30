/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include "js-syntax.h"

// if extern const array, sizeof is not useable anymore
// https://www.reddit.com/r/C_Programming/comments/1169mv6/best_way_to_write_constant_arrays_to_be_included/
#define X(name) #name,
static const char *const _token_state_names[] = {js_token_state_list};
#undef X

#define _token_head(__arg_source, __arg_token) ((__arg_source)->base + (__arg_token)->head_offset)
#define _token_length(__arg_token) ((__arg_token)->tail_offset - (__arg_token)->head_offset)
#define _token_string_head(__arg_source, __arg_token) (_token_head(__arg_source, __arg_token) + 1)
#define _token_string_length(__arg_token) (_token_length(__arg_token) - 2)

static enum js_token_state _match_keyword(const char *identitier, size_t identifier_length) {
#define __keyword_entry(kw) {#kw, sizeof(#kw) - 1, ts_##kw}
    static const struct {
        const char *name;
        size_t len;
        enum js_token_state token;
    } keywords[] = {
        __keyword_entry(null),
        __keyword_entry(true),
        __keyword_entry(false),
        __keyword_entry(let),
        __keyword_entry(if),
        __keyword_entry(else),
        __keyword_entry(while),
        __keyword_entry(do),
        __keyword_entry(for),
        __keyword_entry(break),
        __keyword_entry(continue),
        __keyword_entry(function),
        __keyword_entry(return),
        __keyword_entry(in),
        __keyword_entry(of),
        __keyword_entry(typeof),
        __keyword_entry(delete),
        __keyword_entry(try),
        __keyword_entry(catch),
        __keyword_entry(finally),
        __keyword_entry(throw),
    };
#undef __keyword_entry
    int i;
    for (i = 0; i < countof(keywords); i++) {
        // match keyword len, DON'T use token len, for example, "fals"
        if (strncmp(identitier, keywords[i].name, max(identifier_length, keywords[i].len)) == 0) {
            return keywords[i].token;
        }
    }
    return ts_identifier;
}

#define _return_false(__arg_source, __arg_token, __arg_message) \
    do { \
        log_warning("%u-%u:%u-%u:%s:%.*s: %s", __arg_token->head_line, __arg_token->tail_line, \
            __arg_token->head_offset, __arg_token->tail_offset, \
            _token_state_names[__arg_token->state], \
            _token_length(__arg_token), _token_head(__arg_source, __arg_token), \
            __arg_message); \
        return false; \
    } while (0)

static bool _get_token_unfiltered(struct js_source *source, struct js_token *token /* out */) {
#define __is_identifier_first_character(__arg_ch) \
    ((__arg_ch >= 'a' && __arg_ch <= 'z') || (__arg_ch >= 'A' && __arg_ch <= 'Z') || (__arg_ch == '_'))
#define __is_identifier_rest_characters(__arg_ch) \
    (__is_identifier_first_character(__arg_ch) || (__arg_ch >= '0' && __arg_ch <= '9'))
#define __reset_token_number() \
    do { \
        token->integer = 0; \
        token->fraction = 0; \
        token->fraction_depth = 0.1; \
        token->exponent_sign = 1; \
        token->exponent = 0; \
    } while (0)
#define __char_to_digit(__arg_ch) (__arg_ch - '0')
#define __accumulate_token_number_integer(__arg_ch) \
    do { \
        token->integer = token->integer * 10 + __char_to_digit(__arg_ch); \
    } while (0)
#define __accumulate_token_number_fraction(__arg_ch) \
    do { \
        token->fraction = token->fraction + __char_to_digit(__arg_ch) * token->fraction_depth; \
        token->fraction_depth *= 0.1; \
    } while (0)
#define __accumulate_token_number_exponent(__arg_ch) \
    do { \
        token->exponent = token->exponent * 10 + __char_to_digit(__arg_ch); \
    } while (0)
// must include math.h, or pow() result is 0
#define __calculate_token_number() \
    do { \
        token->number = \
            (token->integer + token->fraction) * pow(10.0, token->exponent_sign * token->exponent); \
    } while (0)
    // use DFA because fit my thinking habits, is simple for me
    // DON'T use switch/case because "break" will conflict with "for"
    // following firefox Syntax error text, use F12 to check
    // at first, tok.h = NULL
    // DON'T calculate tok.line in one place because when tok.t=\n and return, and call again, \n may be calcualted more than once
    while (token->tail_offset < source->length) {
        // always make sure [tok.h, tok.t) is printable, eg. tok.t point to next outside, and correctly count new line
        if (token->state == ts_searching) {
            // searching state matches tok.h, which is inside scope
            token->head_offset = token->tail_offset;
            token->head_line = token->tail_line;
            (token->tail_offset)++;
            char tok_h_char = *_token_head(source, token);
            if (isspace(tok_h_char)) {
                if (tok_h_char == '\n') {
                    (token->tail_line)++;
                }
            } else if (__is_identifier_first_character(tok_h_char)) {
                token->state = ts_identifier_matching;
            } else if (tok_h_char == '/') {
                token->state = ts_division_matching;
            } else if (tok_h_char == '-') {
                // slightly modified, json negative value not supported, to prevent such as '1-2' parsed to '1' '-2' illegal expression error, but json still can be parsed because '-2' is legal expression
                token->state = ts_minus_matching;
            } else if (isdigit(tok_h_char)) {
                __reset_token_number();
                __accumulate_token_number_integer(tok_h_char);
                token->state = ts_number_matching;
            } else if (tok_h_char == '+') {
                token->state = ts_plus_matching;
            } else if (tok_h_char == '*') {
                token->state = ts_multiplication_matching;
            } else if (tok_h_char == '(') {
                token->state = ts_left_parenthesis;
                goto matched;
            } else if (tok_h_char == ')') {
                token->state = ts_right_parenthesis;
                goto matched;
            } else if (tok_h_char == '[') {
                token->state = ts_left_bracket;
                goto matched;
            } else if (tok_h_char == ']') {
                token->state = ts_right_bracket;
                goto matched;
            } else if (tok_h_char == '{') {
                token->state = ts_left_brace;
                goto matched;
            } else if (tok_h_char == '}') {
                token->state = ts_right_brace;
                goto matched;
            } else if (tok_h_char == '\"') {
                token->state = ts_string_matching;
            } else if (tok_h_char == ':') {
                token->state = ts_colon_matching;
            } else if (tok_h_char == ',') {
                token->state = ts_comma;
                goto matched;
            } else if (tok_h_char == '=') {
                token->state = ts_assignment_matching;
            } else if (tok_h_char == '<') {
                token->state = ts_less_than_matching;
            } else if (tok_h_char == '>') {
                token->state = ts_greater_than_matching;
            } else if (tok_h_char == '!') {
                token->state = ts_not_matching;
            } else if (tok_h_char == '&') {
                token->state = ts_and_matching;
            } else if (tok_h_char == '|') {
                token->state = ts_or_matching;
            } else if (tok_h_char == '%') {
                token->state = ts_mod_matching;
            } else if (tok_h_char == ';') {
                token->state = ts_semicolon;
                goto matched;
            } else if (tok_h_char == '.') {
                token->state = ts_one_dot_matching;
            } else if (tok_h_char == '?') {
                token->state = ts_question_matching;
            } else if (tok_h_char == '\0') { // sometimes final '\0' is included in source, eg. sizeof
                token->state = ts_end_of_file;
                goto matched;
            } else {
                _return_false(source, token, "Illegal character");
            }
        } else {
            char tok_t_char = *(source->base + token->tail_offset);
            if (token->state == ts_identifier_matching) {
                // most other states determined by "next" character tok.t, which is outside scope
                // but some will check inside scope, for example: block comment
                if (__is_identifier_rest_characters(tok_t_char)) {
                    (token->tail_offset)++;
                } else {
                    token->state = ts_identifier;
                    goto matched;
                }
            } else if (token->state == ts_division_matching) {
                if (tok_t_char == '/') {
                    (token->tail_offset)++;
                    token->state = ts_line_comment_matching_new_line;
                } else if (tok_t_char == '*') {
                    (token->tail_offset)++;
                    token->state = ts_block_comment_matching_end_star;
                } else if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_division_assignment;
                    goto matched;
                } else {
                    token->state = ts_division;
                    goto matched;
                }
            } else if (token->state == ts_line_comment_matching_new_line) {
                if (tok_t_char == '\n') {
                    (token->tail_line)++;
                    (token->tail_offset)++;
                    token->state = ts_line_comment;
                    goto matched;
                } else {
                    (token->tail_offset)++;
                }
            } else if (token->state == ts_block_comment_matching_end_star) {
                if (tok_t_char == '*') {
                    (token->tail_offset)++;
                    token->state = ts_block_comment_matching_end_slash;
                } else {
                    if (tok_t_char == '\n') {
                        (token->tail_line)++;
                    }
                    (token->tail_offset)++;
                }
            } else if (token->state == ts_block_comment_matching_end_slash) {
                if (tok_t_char == '/') {
                    (token->tail_offset)++;
                    token->state = ts_block_comment;
                    goto matched;
                } else {
                    if (tok_t_char == '\n') {
                        (token->tail_line)++;
                    }
                    (token->tail_offset)++;
                    token->state = ts_block_comment_matching_end_star;
                }
            } else if (token->state == ts_number_matching) { // match from 2nd integral digit to check leading 0
                if (isdigit(tok_t_char)) {
                    if (token->integer == 0) { //  json rule
                        _return_false(source, token, "Interger part starting with 0 cannot follow any other digits");
                    }
                    __accumulate_token_number_integer(tok_t_char);
                    (token->tail_offset)++;
                } else if (tok_t_char == '.') {
                    (token->tail_offset)++;
                    token->state = ts_number_matching_fraction_first;
                } else if (tok_t_char == 'E' || tok_t_char == 'e') {
                    (token->tail_offset)++;
                    token->state = ts_number_matching_exponent_first;
                } else if (__is_identifier_first_character(tok_t_char)) {
                    (token->tail_offset)++;
                    _return_false(source, token, "Identifier starts immediately after numeric literal");
                } else {
                    __calculate_token_number();
                    token->state = ts_number;
                    goto matched;
                }
            } else if (token->state == ts_number_matching_fraction_first) {
                // fraction may start with myltiple 0, so it cannot check missing using == 0 like integral part
                if (isdigit(tok_t_char)) {
                    __accumulate_token_number_fraction(tok_t_char);
                    token->state = ts_number_matching_fraction_rest;
                    (token->tail_offset)++;
                } else {
                    _return_false(source, token, "Missing fraction");
                }
            } else if (token->state == ts_number_matching_fraction_rest) {
                if (isdigit(tok_t_char)) {
                    __accumulate_token_number_fraction(tok_t_char);
                    (token->tail_offset)++;
                } else if (tok_t_char == 'E' || tok_t_char == 'e') {
                    (token->tail_offset)++;
                    token->state = ts_number_matching_exponent_first;
                } else if (__is_identifier_first_character(tok_t_char)) {
                    (token->tail_offset)++;
                    _return_false(source, token, "Identifier starts immediately after numeric literal");
                } else {
                    __calculate_token_number();
                    token->state = ts_number;
                    goto matched;
                }
            } else if (token->state == ts_number_matching_exponent_first) {
                // exponent may start with myltiple 0, so it cannot check missing using == 0 like integral part
                if (tok_t_char == '+' || tok_t_char == '-') {
                    if (tok_t_char == '-') {
                        token->exponent_sign = -1;
                    }
                    (token->tail_offset)++;
                    token->state = ts_number_matching_exponent_number_first;
                } else if (isdigit(tok_t_char)) {
                    __accumulate_token_number_exponent(tok_t_char);
                    (token->tail_offset)++;
                    token->state = ts_number_matching_exponent_rest;
                } else {
                    _return_false(source, token, "Missing exponent");
                }
            } else if (token->state == ts_number_matching_exponent_number_first) {
                if (isdigit(tok_t_char)) {
                    __accumulate_token_number_exponent(tok_t_char);
                    (token->tail_offset)++;
                    token->state = ts_number_matching_exponent_rest;
                } else {
                    _return_false(source, token, "Missing exponent");
                }
            } else if (token->state == ts_number_matching_exponent_rest) {
                if (isdigit(tok_t_char)) {
                    __accumulate_token_number_exponent(tok_t_char);
                    (token->tail_offset)++;
                } else if (__is_identifier_first_character(tok_t_char)) {
                    (token->tail_offset)++;
                    _return_false(source, token, "Identifier starts immediately after numeric literal");
                } else {
                    __calculate_token_number();
                    token->state = ts_number;
                    goto matched;
                }
            } else if (token->state == ts_string_matching) {
                if (tok_t_char == '\\') {
                    (token->tail_offset)++;
                    token->state = ts_string_matching_control;
                } else if (tok_t_char == '\"') {
                    (token->tail_offset)++;
                    token->state = ts_string;
                    goto matched;
                } else if (tok_t_char == '\n') {
                    _return_false(source, token, "Line break is not allowed inside string literal");
                } else {
                    (token->tail_offset)++;
                }
            } else if (token->state == ts_string_matching_control) {
                (token->tail_offset)++;
                token->state = ts_string_matching;
            } else if (token->state == ts_colon_matching) {
                if (tok_t_char == ':') {
                    (token->tail_offset)++;
                    token->state = ts_double_colon;
                    goto matched;
                } else {
                    token->state = ts_colon;
                    goto matched;
                }
            } else if (token->state == ts_assignment_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_equal_to;
                    goto matched;
                } else {
                    token->state = ts_assignment;
                    goto matched;
                }
            } else if (token->state == ts_less_than_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_less_than_or_equal_to;
                    goto matched;
                } else {
                    token->state = ts_less_than;
                    goto matched;
                }
            } else if (token->state == ts_greater_than_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_greater_than_or_equal_to;
                    goto matched;
                } else {
                    token->state = ts_greater_than;
                    goto matched;
                }
            } else if (token->state == ts_not_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_not_equal_to;
                    goto matched;
                } else {
                    token->state = ts_not;
                    goto matched;
                }
            } else if (token->state == ts_and_matching) {
                if (tok_t_char == '&') {
                    (token->tail_offset)++;
                    token->state = ts_and;
                    goto matched;
                } else {
                    _return_false(source, token, "Unfinished logical && operator");
                }
            } else if (token->state == ts_or_matching) {
                if (tok_t_char == '|') {
                    (token->tail_offset)++;
                    token->state = ts_or;
                    goto matched;
                } else {
                    _return_false(source, token, "Unfinished logical || operator");
                }
            } else if (token->state == ts_one_dot_matching) {
                if (tok_t_char == '.') {
                    (token->tail_offset)++;
                    token->state = ts_two_dot_matching;
                } else {
                    token->state = ts_member_access;
                    goto matched;
                }
            } else if (token->state == ts_two_dot_matching) {
                if (tok_t_char == '.') {
                    (token->tail_offset)++;
                    token->state = ts_spread;
                    goto matched;
                } else {
                    _return_false(source, token, "Unfinished spread ... operator");
                }
            } else if (token->state == ts_question_matching) {
                if (tok_t_char == '.') {
                    (token->tail_offset)++;
                    token->state = ts_optional_chaining;
                    goto matched;
                } else {
                    token->state = ts_question;
                    goto matched;
                }
            } else if (token->state == ts_minus_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_minus_assignment;
                    goto matched;
                } else if (tok_t_char == '-') {
                    (token->tail_offset)++;
                    token->state = ts_minus_minus;
                    goto matched;
                } else {
                    token->state = ts_minus;
                    goto matched;
                }
            } else if (token->state == ts_plus_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_plus_assignment;
                    goto matched;
                } else if (tok_t_char == '+') {
                    (token->tail_offset)++;
                    token->state = ts_plus_plus;
                    goto matched;
                } else {
                    token->state = ts_plus;
                    goto matched;
                }
            } else if (token->state == ts_multiplication_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_multiplication_assignment;
                    goto matched;
                } else if (tok_t_char == '*') {
                    (token->tail_offset)++;
                    token->state = ts_exponentiation_matching;
                } else {
                    token->state = ts_multiplication;
                    goto matched;
                }
            } else if (token->state == ts_exponentiation_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_exponentiation_assignment;
                    goto matched;
                } else {
                    token->state = ts_exponentiation;
                    goto matched;
                }
            } else if (token->state == ts_mod_matching) {
                if (tok_t_char == '=') {
                    (token->tail_offset)++;
                    token->state = ts_mod_assignment;
                    goto matched;
                } else {
                    token->state = ts_mod;
                    goto matched;
                }
            } else { // back to searching state
                token->state = ts_searching;
            }
        }
    }
    // remaining content before eof
    switch (token->state) {
    case ts_identifier_matching:
        token->state = ts_identifier;
        goto matched;
    case ts_division_matching:
        token->state = ts_division;
        goto matched;
    case ts_line_comment_matching_new_line:
        token->state = ts_line_comment;
        goto matched;
    case ts_block_comment_matching_end_star:
    case ts_block_comment_matching_end_slash:
        _return_false(source, token, "Unfinished block comment");
    case ts_number_matching_fraction_first:
        _return_false(source, token, "Missing fraction");
    case ts_number_matching_exponent_first:
    case ts_number_matching_exponent_number_first:
        _return_false(source, token, "Missing exponent");
    case ts_number_matching:
    case ts_number_matching_fraction_rest:
    case ts_number_matching_exponent_rest:
        __calculate_token_number();
        token->state = ts_number;
        goto matched;
    case ts_string_matching:
    case ts_string_matching_control:
        _return_false(source, token, "Unfinished string");
    case ts_colon_matching:
        token->state = ts_colon;
        goto matched;
    case ts_assignment_matching:
        token->state = ts_assignment;
        goto matched;
    case ts_less_than_matching:
        token->state = ts_less_than;
        goto matched;
    case ts_greater_than_matching:
        token->state = ts_greater_than;
        goto matched;
    case ts_not_matching:
        token->state = ts_not;
        goto matched;
    case ts_and_matching:
        _return_false(source, token, "Unfinished logical && operator");
    case ts_or_matching:
        _return_false(source, token, "Unfinished logical || operator");
    case ts_one_dot_matching:
        token->state = ts_member_access;
        goto matched;
    case ts_two_dot_matching:
        _return_false(source, token, "Unfinished spread ... operator");
    case ts_question_matching:
        token->state = ts_question;
        goto matched;
    case ts_minus_matching:
        token->state = ts_minus;
        goto matched;
    case ts_plus_matching:
        token->state = ts_plus;
        goto matched;
    case ts_multiplication_matching:
        token->state = ts_multiplication;
        goto matched;
    case ts_exponentiation_matching:
        token->state = ts_exponentiation;
        goto matched;
    case ts_mod_matching:
        token->state = ts_mod;
        goto matched;
    case ts_end_of_file:
        _return_false(source, token, "End of file");
    default:
        // after final ready state processed, will set eof state for syntax parser to end loop
        // recall after eof state will cause eof error
        token->head_offset = token->tail_offset;
        token->head_line = token->tail_line;
        token->state = ts_end_of_file;
        goto matched;
    }
matched:
    // replace "return" with "goto matched" so that extra works can be done before return
    {
        // match keywords
        if (token->state == ts_identifier) {
            token->state = _match_keyword(_token_head(source, token), _token_length(token));
        }
    }
    return true;
#undef __calculate_token_number
#undef __accumulate_token_number_exponent
#undef __accumulate_token_number_fraction
#undef __accumulate_token_number_integer
#undef __char_to_digit
#undef __reset_token_number
#undef __is_identifier_rest_characters
#undef __is_identifier_first_character
}

static bool _get_token_filtered(struct js_source *source, struct js_token *token /* out */) {
    bool success;
    do {
        success = _get_token_unfiltered(source, token);
    } while (token->state == ts_block_comment || token->state == ts_line_comment);
    return success;
}

static struct js_source _unescape(char *base, uint32_t length) {
    struct js_source ret = {0};
    bool matching_control = false;
    for (char *p = base; p < (base + length); p++) {
        if (matching_control) {
            // following c escaping rules https://en.cppreference.com/w/c/language/escape.html
            // compared to json, add '\a' '\v'
            if (*p == 'u') {
                string_buffer_append_sz(ret.base, ret.length, ret.capacity, "\\u");
            } else {
                char c;
                switch (*p) {
                case 'a':
                    c = '\a';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'v':
                    c = '\v';
                    break;
                default:
                    c = *p;
                    break;
                }
                string_buffer_append_ch(ret.base, ret.length, ret.capacity, c);
            }
            matching_control = false;
        } else if (*p == '\\') {
            matching_control = true;
        } else {
            string_buffer_append_ch(ret.base, ret.length, ret.capacity, *p);
        }
    }
    return ret;
}

#define _try(__arg_expr) \
    do { \
        if (!(__arg_expr)) { \
            return false; \
        } \
    } while (0);

#define _next_token(__arg_source, __arg_token) \
    do { \
        typeof(__arg_token) __token = (__arg_token); \
        _try(_get_token_filtered((__arg_source), __token)); \
    } while (0)

// use macro instead of  function, to correctly record error position
#define _expect(__arg_source, __arg_token, __arg_state) \
    do { \
        typeof(__arg_token) __token = (__arg_token); \
        if (__token->state == (__arg_state)) { \
            _next_token(__arg_source, __arg_token); \
        } else { \
            _return_false(__arg_source, __token, "Expect " #__arg_state); \
        } \
    } while (0)

#define _add_instruction(token, bytecode, xref, ...) \
    do { \
        /* log_debug("js_add_cross_reference %lu %lu", token->head_line, bytecode->length); */ \
        js_add_cross_reference(xref, token->head_line, bytecode->length); \
        js_add_instruction(bytecode, ##__VA_ARGS__); \
    } while (0)

static bool _parse_statement(struct js_source *, struct js_token *, struct js_bytecode *, struct js_cross_reference *, uint32_t, uint32_t);

static bool _parse_expression(struct js_source *, struct js_token *, struct js_bytecode *, struct js_cross_reference *);

static bool _parse_function(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    char *identifier_head;
    uint32_t identifier_length;
    uint32_t d0, d1, d2; // d means delta
    uint16_t i;
    d0 = bytecode->length;
    _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, 0);
    d1 = bytecode->length;
    _expect(source, token, ts_left_parenthesis);
    i = 0;
    if (token->state == ts_right_parenthesis) {
        _next_token(source, token);
    } else {
        _add_instruction(token, bytecode, xref, op_argument_first, 0);
        for (;;) {
            if (token->state == ts_spread) {
                _next_token(source, token);
                if (token->state != ts_identifier) {
                    _return_false(source, token, "Expect parameter name");
                }
                // _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string, sf_value, _token_length(token), _token_head(source, token));
                _add_instruction(token, bytecode, xref, op_argument_get_rest, 1, opd_string, _token_length(token), _token_head(source, token));
                i++;
                _next_token(source, token);
                _expect(source, token, ts_right_parenthesis);
                break;
            } else {
                if (token->state != ts_identifier) {
                    _return_false(source, token, "Expect parameter name");
                }
                // _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string, sf_value, _token_length(token), _token_head(source, token));
                identifier_head = _token_head(source, token);
                identifier_length = _token_length(token);
                i++;
                _next_token(source, token);
                if (token->state == ts_assignment) {
                    _next_token(source, token);
                    _try(_parse_expression(source, token, bytecode, xref));
                } else {
                    // _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_null, sf_value);
                }
                _add_instruction(token, bytecode, xref, op_argument_get_next, 1, opd_string, identifier_length, identifier_head);
                i++;
                if (token->state == ts_comma) {
                    _next_token(source, token);
                    continue;
                } else if (token->state == ts_right_parenthesis) {
                    _next_token(source, token);
                    break;
                } else {
                    _return_false(source, token, "Expect , or )");
                }
            }
        }
    }
    // _add_instruction(token, bytecode, xref, op_argument_get, 1, opd_uint16, i);
    _expect(source, token, ts_left_brace);
    while (token->state != ts_right_brace) {
        _try(_parse_statement(source, token, bytecode, xref, UINT32_MAX, UINT32_MAX));
    }
    _next_token(source, token);
    // _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_null, sf_value);
    _add_instruction(token, bytecode, xref, op_return, 0); // add a default 'return' at function end
    d2 = bytecode->length;
    _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_function, sf_value, d1);
    js_put_instruction(bytecode, &d0, op_jump, 1, opd_uint32, d2);
    return true;
}

static bool _parse_value(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    struct js_source unescaped = {0};
    switch (token->state) {
    case ts_null:
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_null, sf_value);
        _next_token(source, token);
        break;
    case ts_true:
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_boolean, sf_value, true);
        _next_token(source, token);
        break;
    case ts_false:
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_boolean, sf_value, false);
        _next_token(source, token);
        break;
    case ts_number:
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_double, sf_value, token->number);
        _next_token(source, token);
        break;
    case ts_string:
        unescaped = _unescape(_token_string_head(source, token), _token_string_length(token));
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string,
            sf_value, unescaped.length, unescaped.base);
        buffer_free(unescaped.base, unescaped.length, unescaped.capacity);
        _next_token(source, token);
        break;
    case ts_left_bracket:
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_empty_array, sf_value);
        _next_token(source, token);
        if (token->state == ts_right_bracket) {
            _next_token(source, token);
        } else {
            for (;;) {
                if (token->state == ts_spread) {
                    _next_token(source, token);
                    _try(_parse_expression(source, token, bytecode, xref));
                    _add_instruction(token, bytecode, xref, op_array_spread, 0);
                } else {
                    _try(_parse_expression(source, token, bytecode, xref));
                    _add_instruction(token, bytecode, xref, op_array_append, 0);
                }
                if (token->state == ts_comma) {
                    _next_token(source, token);
                    continue;
                } else if (token->state == ts_right_bracket) {
                    _next_token(source, token);
                    break;
                } else {
                    _return_false(source, token, "Expect , or ]");
                }
            }
        }
        break;
    case ts_left_brace:
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_empty_object, sf_value);
        _next_token(source, token);
        if (token->state == ts_right_brace) {
            _next_token(source, token);
        } else {
            for (;;) {
                if (token->state == ts_string) {
                    unescaped = _unescape(_token_string_head(source, token), _token_string_length(token));
                    _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string,
                        sf_value, unescaped.length, unescaped.base);
                    buffer_free(unescaped.base, unescaped.length, unescaped.capacity);
                } else if (token->state == ts_identifier) {
                    _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string, sf_value, _token_length(token), _token_head(source, token));
                } else {
                    _return_false(source, token, "Expect string or identifier");
                }
                _next_token(source, token);
                _expect(source, token, ts_colon);
                _try(_parse_expression(source, token, bytecode, xref));
                _add_instruction(token, bytecode, xref, op_member_put, 0);
                if (token->state == ts_comma) {
                    _next_token(source, token);
                    continue;
                } else if (token->state == ts_right_brace) {
                    _next_token(source, token);
                    break;
                } else {
                    _return_false(source, token, "Expect , or }");
                }
            }
        }
        break;
    case ts_function:
        _next_token(source, token);
        _try(_parse_function(source, token, bytecode, xref));
        break;
    default:
        _return_false(source, token, "Not a value literal");
    }
    return true;
}

#define _accessor_type_list \
    X(at_value) \
    X(at_identifier) \
    X(at_member_access) \
    X(at_optional_chaining)

#define X(name) name,
enum _accessor_type { _accessor_type_list };
#undef X

#pragma pack(push, 1)
struct _accessor {
    enum _accessor_type type;
    char *identifier_head;
    uint32_t identifier_length;
};
#pragma pack(pop)

static bool _accessor_put(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref, struct _accessor acc) {
    switch (acc.type) { // previous parsed type
    case at_identifier:
        _add_instruction(token, bytecode, xref, op_variable_put, 1, opd_string, acc.identifier_length, acc.identifier_head);
        break;
    case at_member_access:
        _add_instruction(token, bytecode, xref, op_member_put, 0);
        _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1); // is lvalue, cleanup evstack
        break;
    default:
        _return_false(source, token, "Illegal accessor type for put operation");
    }
    return true;
}

static bool _accessor_get(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref, struct _accessor acc) {
    switch (acc.type) { // previous parsed type
    case at_identifier:
        _add_instruction(token, bytecode, xref, op_variable_get, 1, opd_string, acc.identifier_length, acc.identifier_head);
        break;
    case at_member_access:
        _add_instruction(token, bytecode, xref, op_member_get, 0);
        break;
    case at_optional_chaining:
        _add_instruction(token, bytecode, xref, op_object_optional, 0);
        break;
    default:
        break;
    }
    return true;
}

static bool _parse_additive_expression(struct js_source *, struct js_token *, struct js_bytecode *, struct js_cross_reference *);

static bool _parse_accessor(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref, struct _accessor *acc /* out */) {
    uint32_t d0, d1;
    bool bind = false; // indicate next chaining function call must consume binding value
beginning:
    if (token->state == ts_left_parenthesis) {
        _next_token(source, token);
        _try(_parse_expression(source, token, bytecode, xref));
        _expect(source, token, ts_right_parenthesis);
        acc->type = at_value;
    } else if (token->state == ts_identifier) {
        acc->identifier_head = _token_head(source, token);
        acc->identifier_length = _token_length(token);
        _next_token(source, token);
        acc->type = at_identifier;
    } else {
        _try(_parse_value(source, token, bytecode, xref));
        acc->type = at_value;
    }
    for (;;) {
        if (token->state == ts_left_bracket) {
            _next_token(source, token);
            _try(_accessor_get(source, token, bytecode, xref, *acc));
            _try(_parse_additive_expression(source, token, bytecode, xref));
            _expect(source, token, ts_right_bracket);
            acc->type = at_member_access;
        } else if (token->state == ts_member_access) {
            _next_token(source, token);
            _try(_accessor_get(source, token, bytecode, xref, *acc));
            if (token->state == ts_identifier) {
                _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string, sf_value, _token_length(token), _token_head(source, token));
                _next_token(source, token);
                acc->type = at_member_access;
            } else {
                _return_false(source, token, "Must be object.identifier");
            }
        } else if (token->state == ts_optional_chaining) {
            _next_token(source, token);
            _try(_accessor_get(source, token, bytecode, xref, *acc));
            if (token->state == ts_identifier) {
                _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_string, sf_value, _token_length(token), _token_head(source, token));
                _next_token(source, token);
                acc->type = at_optional_chaining;
            } else {
                _return_false(source, token, "Must be object?.identifier");
            }
        } else if (token->state == ts_double_colon) {
            _next_token(source, token);
            _try(_accessor_get(source, token, bytecode, xref, *acc));
            bind = true;
            goto beginning; // following parse must reset to beginning state
        } else if (token->state == ts_left_parenthesis) {
            _next_token(source, token); // function call
            _try(_accessor_get(source, token, bytecode, xref, *acc));
            d0 = bytecode->length;
            _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_uint32, sf_function, 0); // return address
            if (bind) { // treat bind value as first argument
                _add_instruction(token, bytecode, xref, op_stack_swap, 2, opd_uint8, opd_uint8, 0, 2);
                _add_instruction(token, bytecode, xref, op_stack_swap, 2, opd_uint8, opd_uint8, 1, 2);
                _add_instruction(token, bytecode, xref, op_argument_append, 0);
            }
            bind = false; // bind value is consumed
            if (token->state == ts_right_parenthesis) {
                _next_token(source, token);
            } else {
                for (;;) {
                    if (token->state == ts_spread) {
                        _next_token(source, token);
                        _try(_parse_expression(source, token, bytecode, xref));
                        _add_instruction(token, bytecode, xref, op_argument_spread, 0);
                    } else {
                        _try(_parse_expression(source, token, bytecode, xref));
                        _add_instruction(token, bytecode, xref, op_argument_append, 0);
                    }
                    if (token->state == ts_comma) {
                        _next_token(source, token);
                        continue;
                    } else if (token->state == ts_right_parenthesis) {
                        _next_token(source, token);
                        break;
                    } else {
                        _return_false(source, token, "Expect , or )");
                    }
                }
            }
            _add_instruction(token, bytecode, xref, op_call, 0);
            d1 = bytecode->length;
            js_put_instruction(bytecode, &d0, op_stack_push, 2, opd_uint8, opd_uint32, sf_function, d1); // return address
            // _add_instruction(token, bytecode, xref, op_call_stack_pop, 0);
            // _add_instruction(token, bytecode, xref, op_get_result, 0);
            acc->type = at_value; // it is value now
        } else {
            break;
        }
    }
    if (bind) {
        _return_false(source, token, "No function consume bind value");
    }
    return true;
}

static bool _parse_access_call_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    struct _accessor acc;
    _try(_parse_accessor(source, token, bytecode, xref, &acc));
    _try(_accessor_get(source, token, bytecode, xref, acc));
    return true;
}

static bool _parse_prefix_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    enum js_token_state stat = token->state;
    if (stat == ts_typeof || stat == ts_not || stat == ts_plus || stat == ts_minus) {
        if (stat == ts_minus) {
            _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_double, sf_value, 0.0);
        }
        _next_token(source, token);
        _try(_parse_access_call_expression(source, token, bytecode, xref));
        switch (stat) {
        case ts_typeof:
            _add_instruction(token, bytecode, xref, op_typeof, 0);
            break;
        case ts_not:
            _add_instruction(token, bytecode, xref, op_not, 0);
            break;
        case ts_minus:
            _add_instruction(token, bytecode, xref, op_sub, 0);
            break;
        default:
            break;
        }
    } else {
        _try(_parse_access_call_expression(source, token, bytecode, xref));
    }
    return true;
}

static bool _parse_exponential_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    _try(_parse_prefix_expression(source, token, bytecode, xref));
    while (token->state == ts_exponentiation) {
        _next_token(source, token);
        _try(_parse_prefix_expression(source, token, bytecode, xref));
        _add_instruction(token, bytecode, xref, op_pow, 0);
    }
    return true;
}

static bool _parse_multiplicative_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    enum js_token_state stat;
    _try(_parse_exponential_expression(source, token, bytecode, xref));
    while (token->state == ts_multiplication || token->state == ts_division || token->state == ts_mod) {
        stat = token->state;
        _next_token(source, token);
        _try(_parse_exponential_expression(source, token, bytecode, xref));
        switch (stat) {
        case ts_multiplication:
            _add_instruction(token, bytecode, xref, op_mul, 0);
            break;
        case ts_division:
            _add_instruction(token, bytecode, xref, op_div, 0);
            break;
        case ts_mod:
            _add_instruction(token, bytecode, xref, op_mod, 0);
            break;
        default:
            break;
        }
    }
    return true;
}

// needed by _parse_accessor()
static bool _parse_additive_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    enum js_token_state stat;
    _try(_parse_multiplicative_expression(source, token, bytecode, xref));
    while (token->state == ts_plus || token->state == ts_minus) {
        stat = token->state;
        _next_token(source, token);
        _try(_parse_multiplicative_expression(source, token, bytecode, xref));
        switch (stat) {
        case ts_plus:
            _add_instruction(token, bytecode, xref, op_add, 0);
            break;
        case ts_minus:
            _add_instruction(token, bytecode, xref, op_sub, 0);
            break;
        default:
            break;
        }
    }
    return true;
}

static bool _parse_relational_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    enum js_token_state stat;
    _try(_parse_additive_expression(source, token, bytecode, xref));
    stat = token->state;
    if (stat == ts_equal_to || stat == ts_not_equal_to || stat == ts_less_than || stat == ts_less_than_or_equal_to || stat == ts_greater_than || stat == ts_greater_than_or_equal_to) {
        _next_token(source, token);
        _try(_parse_additive_expression(source, token, bytecode, xref));
        switch (stat) {
        case ts_equal_to:
            _add_instruction(token, bytecode, xref, op_eq, 0);
            break;
        case ts_not_equal_to:
            _add_instruction(token, bytecode, xref, op_ne, 0);
            break;
        case ts_less_than:
            _add_instruction(token, bytecode, xref, op_lt, 0);
            break;
        case ts_less_than_or_equal_to:
            _add_instruction(token, bytecode, xref, op_le, 0);
            break;
        case ts_greater_than:
            _add_instruction(token, bytecode, xref, op_gt, 0);
            break;
        case ts_greater_than_or_equal_to:
            _add_instruction(token, bytecode, xref, op_ge, 0);
            break;
        default:
            break;
        }
    }
    return true;
}

static bool _parse_logical_and_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    uint32_t* d_base = NULL; // because of while loop, there are many jumps, each address must be modified
    size_t d_length = 0;
    size_t d_capacity = 0;
    uint32_t d0;
    _try(_parse_relational_expression(source, token, bytecode, xref));
    while (token->state == ts_and) {
        _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 0); // jump will eat value, so dupe one
        buffer_push(d_base, d_length, d_capacity, bytecode->length);
        _add_instruction(token, bytecode, xref, op_jump_if_false, 1, opd_uint32, 0);
        _next_token(source, token);
        _try(_parse_relational_expression(source, token, bytecode, xref));
        _add_instruction(token, bytecode, xref, op_and, 0);
    }
    d0 = bytecode->length;
    buffer_for_each(d_base, d_length, d_capacity, i, d, {
        js_put_instruction(bytecode, d, op_jump_if_false, 1, opd_uint32, d0);
    });
    buffer_free(d_base, d_length, d_capacity);
    return true;
}

static bool _parse_logical_or_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    uint32_t* d_base = NULL; // because of while loop, there are many jumps, each address must be modified
    size_t d_length = 0;
    size_t d_capacity = 0;
    uint32_t d0;
    _try(_parse_logical_and_expression(source, token, bytecode, xref));
    while (token->state == ts_or) {
        _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 0); // jump will eat value, so dupe one
        buffer_push(d_base, d_length, d_capacity, bytecode->length);
        _add_instruction(token, bytecode, xref, op_jump_if_true, 1, opd_uint32, 0);
        _next_token(source, token);
        _try(_parse_logical_and_expression(source, token, bytecode, xref));
        _add_instruction(token, bytecode, xref, op_or, 0);
    }
    d0 = bytecode->length;
    buffer_for_each(d_base, d_length, d_capacity, i, d, {
        js_put_instruction(bytecode, d, op_jump_if_true, 1, opd_uint32, d0);
    });
    buffer_free(d_base, d_length, d_capacity);
    return true;
}

// ternary expression as root
static bool _parse_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    uint32_t d0, d1, d2, d3;
    _try(_parse_logical_or_expression(source, token, bytecode, xref));
    if (token->state == ts_question) {
        // op_ternary wrongly run both sides of ':', for example, 'a == null ? "Hello" : "Hello, " + a' will failed if a is null, now op_ternary removed and replaced with op_jump family
        // _next_token(source, token);
        // _try(_parse_logical_or_expression(source, token, bytecode, xref));
        // _expect(source, token, ts_colon);
        // _try(_parse_logical_or_expression(source, token, bytecode, xref));
        // _add_instruction(token, bytecode, xref, op_ternary, 0);
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_jump_if_false, 1, opd_uint32, 0); // jmp_f to right side of ':'
        _next_token(source, token);
        _try(_parse_logical_or_expression(source, token, bytecode, xref));
        d1 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, 0); // jmp tp last instruction + 1
        d2 = bytecode->length;
        _expect(source, token, ts_colon);
        _try(_parse_logical_or_expression(source, token, bytecode, xref));
        d3 = bytecode->length; // last instruction + 1
        // printf("d0=%d, d1=%d, d2=%d\n", d0, d1, d2);
        js_put_instruction(bytecode, &d0, op_jump_if_false, 1, opd_uint32, d2);
        js_put_instruction(bytecode, &d1, op_jump, 1, opd_uint32, d3);
    }
    return true;
}

static bool _parse_assignment_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    enum js_token_state stat;
    struct _accessor acc;
    _try(_parse_accessor(source, token, bytecode, xref, &acc));
    stat = token->state;
    if (stat == ts_assignment) { // assignment is optional, for example, function call is l-value but not need assignment
        if (acc.type != at_identifier && acc.type != at_member_access) {
            _return_false(source, token, "Assignment expression's l-value can only be identifier or member access");
        }
        _next_token(source, token);
        _try(_parse_expression(source, token, bytecode, xref));
        _try(_accessor_put(source, token, bytecode, xref, acc));
    } else if (stat == ts_plus_assignment || stat == ts_minus_assignment || stat == ts_multiplication_assignment || stat == ts_exponentiation_assignment || stat == ts_division_assignment || stat == ts_mod_assignment || stat == ts_plus_plus || stat == ts_minus_minus) {
        if (acc.type != at_identifier && acc.type != at_member_access) {
            _return_false(source, token, "Assignment expression's l-value can only be identifier or member access");
        }
        // first, get value
        if (acc.type == at_member_access) {
            _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 1);
            _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 1);
        }
        _try(_accessor_get(source, token, bytecode, xref, acc));
        _next_token(source, token);
        switch (stat) {
        case ts_plus_assignment:
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_add, 0);
            break;
        case ts_minus_assignment:
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_sub, 0);
            break;
        case ts_multiplication_assignment:
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_mul, 0);
            break;
        case ts_division_assignment:
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_div, 0);
            break;
        case ts_mod_assignment:
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_mod, 0);
            break;
        case ts_exponentiation_assignment:
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_pow, 0);
            break;
        case ts_plus_plus:
            _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_double, sf_value, 1.0);
            _add_instruction(token, bytecode, xref, op_add, 0);
            break;
        case ts_minus_minus:
            _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_double, sf_value, 1.0);
            _add_instruction(token, bytecode, xref, op_sub, 0);
            break;
        default:
            break;
        }
        _try(_accessor_put(source, token, bytecode, xref, acc));
    } else { // no assignment, just clear ev stack
        switch (acc.type) {
        case at_value:
            _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
            break;
        case at_member_access:
        case at_optional_chaining:
            _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 2);
            break;
        default:
            break;
        }
    }
    return true;
}

static bool _parse_declaration_expression(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    char *identifier_head;
    uint32_t identifier_length;
    _expect(source, token, ts_let);
    for (;;) {
        if (token->state != ts_identifier) {
            _return_false(source, token, "Expect variable name");
        }
        identifier_head = _token_head(source, token);
        identifier_length = _token_length(token);
        _next_token(source, token);
        if (token->state == ts_assignment) {
            _next_token(source, token);
            _try(_parse_expression(source, token, bytecode, xref));
        } else {
            _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_null, sf_value);
        }
        _add_instruction(token, bytecode, xref, op_variable_declare, 1, opd_string, identifier_length, identifier_head);
        if (token->state == ts_comma) {
            _next_token(source, token);
            continue;
        } else {
            break;
        }
    }
    return true;
}

// TODO: remove break_pos continue_pos, replace with a boolean

// needed by _parse_function()
static bool _parse_statement(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref, uint32_t break_pos, uint32_t continue_pos) {
    char *identifier_head;
    uint32_t identifier_length;
    uint32_t d0, d1, d2, d3, d4, d5, d6, d7;
    // struct _parser_state s0, s1;
    enum { classic_for,
        for_in,
        for_of } for_type;
    struct _accessor acc;
    // log_debug("%u:%u:%.*s", token->head_line, token->tail_line, _token_length(token), _token_head(source, token));
    if (token->state == ts_semicolon) {
        _next_token(source, token);
    } else if (token->state == ts_left_brace) { // DONT use _accept, _stack_forward will record token
        _add_instruction(token, bytecode, xref, op_stack_push, 1, opd_uint8, sf_block);
        _next_token(source, token);
        while (token->state != ts_right_brace) {
            _try(_parse_statement(source, token, bytecode, xref, break_pos, continue_pos));
        }
        _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
        _next_token(source, token);
    } else if (token->state == ts_if) {
        _next_token(source, token);
        _expect(source, token, ts_left_parenthesis);
        _try(_parse_expression(source, token, bytecode, xref));
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_jump_if_false, 1, opd_uint32, 0); // jmp_f to 'else' or last instruction + 1
        _expect(source, token, ts_right_parenthesis);
        _try(_parse_statement(source, token, bytecode, xref, break_pos, continue_pos));
        d1 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, 0); // jmp tp last instruction + 1
        d2 = bytecode->length;
        if (token->state == ts_else) {
            _next_token(source, token);
            _try(_parse_statement(source, token, bytecode, xref, break_pos, continue_pos));
        }
        d3 = bytecode->length; // last instruction + 1
        // printf("d0=%d, d1=%d, d2=%d\n", d0, d1, d2);
        js_put_instruction(bytecode, &d0, op_jump_if_false, 1, opd_uint32, d2);
        js_put_instruction(bytecode, &d1, op_jump, 1, opd_uint32, d3);
    } else if (token->state == ts_while) {
        _next_token(source, token);
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, 0, 0);
        _expect(source, token, ts_left_parenthesis);
        d1 = bytecode->length; // ingress
        _try(_parse_expression(source, token, bytecode, xref));
        d2 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_jump_if_false, 1, opd_uint32, 0);
        _expect(source, token, ts_right_parenthesis);
        _try(_parse_statement(source, token, bytecode, xref, 0, 0));
        _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, d1);
        d3 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
        d4 = bytecode->length;
        js_put_instruction(bytecode, &d0, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, d1, d4);
        js_put_instruction(bytecode, &d2, op_jump_if_false, 1, opd_uint32, d3);
    } else if (token->state == ts_do) {
        _next_token(source, token);
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, 0, 0);
        d1 = bytecode->length;
        _try(_parse_statement(source, token, bytecode, xref, 0, 0));
        _expect(source, token, ts_while);
        _expect(source, token, ts_left_parenthesis);
        _try(_parse_expression(source, token, bytecode, xref));
        _add_instruction(token, bytecode, xref, op_jump_if_true, 1, opd_uint32, d1);
        _expect(source, token, ts_right_parenthesis);
        _expect(source, token, ts_semicolon);
        _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
        d2 = bytecode->length;
        js_put_instruction(bytecode, &d0, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, d1, d2);
    } else if (token->state == ts_for) {
        _next_token(source, token);
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, 0, 0);
        _expect(source, token, ts_left_parenthesis);
        if (token->state == ts_let) {
            _next_token(source, token);
            if (token->state != ts_identifier) {
                _return_false(source, token, "Expect variable name");
            }
            identifier_head = _token_head(source, token);
            identifier_length = _token_length(token);
            _next_token(source, token);
            if (token->state == ts_assignment) {
                _next_token(source, token);
                _try(_parse_expression(source, token, bytecode, xref));
                _add_instruction(token, bytecode, xref, op_variable_declare, 1, opd_string, identifier_length, identifier_head);
                _expect(source, token, ts_semicolon);
                for_type = classic_for;
            } else if (token->state == ts_in) {
                _next_token(source, token);
                _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_null, sf_value);
                _add_instruction(token, bytecode, xref, op_variable_declare, 1, opd_string, identifier_length, identifier_head);
                acc.type = at_identifier;
                acc.identifier_head = identifier_head;
                acc.identifier_length = identifier_length;
                for_type = for_in;
            } else if (token->state == ts_of) {
                _next_token(source, token);
                _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_null, sf_value);
                _add_instruction(token, bytecode, xref, op_variable_declare, 1, opd_string, identifier_length, identifier_head);
                acc.type = at_identifier;
                acc.identifier_head = identifier_head;
                acc.identifier_length = identifier_length;
                for_type = for_of;
            } else {
                _return_false(source, token, "Unknown for loop type");
            }
        } else if (token->state == ts_semicolon) {
            _next_token(source, token);
            for_type = classic_for;
        } else {
            _try(_parse_accessor(source, token, bytecode, xref, &acc));
            if (token->state == ts_assignment) {
                _next_token(source, token);
                _try(_parse_expression(source, token, bytecode, xref));
                _try(_accessor_put(source, token, bytecode, xref, acc));
                _expect(source, token, ts_semicolon);
                for_type = classic_for;
            } else if (token->state == ts_in) {
                _next_token(source, token);
                for_type = for_in;
            } else if (token->state == ts_of) {
                _next_token(source, token);
                for_type = for_of;
            } else {
                _return_false(source, token, "Unknown for loop type");
            }
        }
        if (for_type == classic_for) {
            d1 = bytecode->length;
            if (token->state == ts_semicolon) { // section 2 has no content, default value is null
                _next_token(source, token);
                _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_boolean, sf_value, true);
            } else {
                _try(_parse_expression(source, token, bytecode, xref));
                _expect(source, token, ts_semicolon);
            }
            d2 = bytecode->length;
            _add_instruction(token, bytecode, xref, op_jump_if_false, 1, opd_uint32, 0);
            d3 = bytecode->length;
            _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, 0);
            d4 = bytecode->length;
            if (token->state == ts_right_parenthesis) { // section 3 has no contents
                _next_token(source, token);
            } else {
                _try(_parse_assignment_expression(source, token, bytecode, xref));
                _expect(source, token, ts_right_parenthesis);
            }
            _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, d1);
            d5 = bytecode->length;
            _try(_parse_statement(source, token, bytecode, xref, 0, 0));
            _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, d4);
            d6 = bytecode->length;
            _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
            d7 = bytecode->length;
            js_put_instruction(bytecode, &d0, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, d4, d7);
            js_put_instruction(bytecode, &d2, op_jump_if_false, 1, opd_uint32, d6);
            js_put_instruction(bytecode, &d3, op_jump, 1, opd_uint32, d5);
        } else {
            _try(_parse_access_call_expression(source, token, bytecode, xref)); // restrict array or object
            _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_double, sf_value, 0.0); // iterator
            d1 = bytecode->length;
            _add_instruction(token, bytecode, xref, for_type == for_in ? op_for_in_next : op_for_of_next, 1, opd_uint32, 0);
            // printf("acc = %u\n", acc.type);
            if (acc.type == at_identifier) {
                _try(_accessor_put(source, token, bytecode, xref, acc));
            } else {
                _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 4);
                _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 4);
                _add_instruction(token, bytecode, xref, op_stack_dupe, 1, opd_uint8, 2);
                _try(_accessor_put(source, token, bytecode, xref, acc));
                _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
            }
            _expect(source, token, ts_right_parenthesis);
            _try(_parse_statement(source, token, bytecode, xref, 0, 0));
            _add_instruction(token, bytecode, xref, op_jump, 1, opd_uint32, d1);
            d2 = bytecode->length;
            _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, acc.type == at_identifier ? 2 : 4);
            _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1); // call stack
            d3 = bytecode->length;
            js_put_instruction(bytecode, &d0, op_stack_push, 3, opd_uint8, opd_uint32, opd_uint32, sf_loop, d1, d3);
            js_put_instruction(bytecode, &d1, for_type == for_in ? op_for_in_next : op_for_of_next, 1, opd_uint32, d2);
        }
    } else if (token->state == ts_break) {
        _next_token(source, token);
        _expect(source, token, ts_semicolon);
        if (break_pos == UINT32_MAX) {
            _return_false(source, token, "Statement 'break' can't be outside loop");
        }
        _add_instruction(token, bytecode, xref, op_break, 1, opd_uint32, break_pos);
    } else if (token->state == ts_continue) {
        _next_token(source, token);
        _expect(source, token, ts_semicolon);
        if (continue_pos == UINT32_MAX) {
            _return_false(source, token, "Statement 'continue' can't be outside loop");
        }
        _add_instruction(token, bytecode, xref, op_continue, 1, opd_uint32, 0);
    } else if (token->state == ts_function) {
        _next_token(source, token);
        if (token->state != ts_identifier) {
            _return_false(source, token, "Expect function name");
        }
        identifier_head = _token_head(source, token);
        identifier_length = _token_length(token);
        _next_token(source, token);
        _try(_parse_function(source, token, bytecode, xref));
        _add_instruction(token, bytecode, xref, op_variable_declare, 1, opd_string, identifier_length, identifier_head);
    } else if (token->state == ts_return) {
        _next_token(source, token);
        if (token->state == ts_semicolon) {
            _next_token(source, token);
        } else {
            _try(_parse_expression(source, token, bytecode, xref));
            _add_instruction(token, bytecode, xref, op_return, 0);
            _expect(source, token, ts_semicolon);
        }
    } else if (token->state == ts_delete) {
        _next_token(source, token);
        if (token->state == ts_identifier) {
            identifier_head = _token_head(source, token);
            identifier_length = _token_length(token);
            _add_instruction(token, bytecode, xref, op_variable_delete, 1, opd_string, identifier_length, identifier_head);
        } else {
            _return_false(source, token, "Expect identifier");
        }
        _next_token(source, token);
        _expect(source, token, ts_semicolon);
    } else if (token->state == ts_try) {
        _next_token(source, token);
        _expect(source, token, ts_left_brace);
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_uint32, sf_try, 0);
        while (token->state != ts_right_brace) {
            _try(_parse_statement(source, token, bytecode, xref, break_pos, continue_pos));
        }
        _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
        _add_instruction(token, bytecode, xref, op_stack_push, 2, opd_uint8, opd_undefined, sf_value);
        d1 = bytecode->length;
        js_put_instruction(bytecode, &d0, op_stack_push, 2, opd_uint8, opd_uint32, sf_try, d1);
        _next_token(source, token);
        // acorrding to js standard, 'catch' 'finally' at least one, since 'finally' not supported, 'catch' is necessary
        _expect(source, token, ts_catch);
        _expect(source, token, ts_left_parenthesis);
        if (token->state != ts_identifier) {
            _return_false(source, token, "Expect variable name");
        }
        identifier_head = _token_head(source, token);
        identifier_length = _token_length(token);
        _next_token(source, token);
        _expect(source, token, ts_right_parenthesis);
        _expect(source, token, ts_left_brace);
        d0 = bytecode->length;
        _add_instruction(token, bytecode, xref, op_catch, 2, opd_string, opd_uint32, identifier_length, identifier_head, 0);
        while (token->state != ts_right_brace) {
            _try(_parse_statement(source, token, bytecode, xref, break_pos, continue_pos));
        }
        _add_instruction(token, bytecode, xref, op_stack_pop, 1, opd_uint8, 1);
        d1 = bytecode->length;
        js_put_instruction(bytecode, &d0, op_catch, 2, opd_string, opd_uint32, identifier_length, identifier_head, d1);
        _next_token(source, token);
    } else if (token->state == ts_throw) {
        _next_token(source, token);
        _try(_parse_expression(source, token, bytecode, xref));
        _add_instruction(token, bytecode, xref, op_throw, 0);
        _expect(source, token, ts_semicolon);
    } else if (token->state == ts_let) {
        _try(_parse_declaration_expression(source, token, bytecode, xref));
        _expect(source, token, ts_semicolon);
    } else {
        _try(_parse_assignment_expression(source, token, bytecode, xref));
        _expect(source, token, ts_semicolon);
    }
    return true;
}

bool js_compile(struct js_source *source, struct js_token *token, struct js_bytecode *bytecode, struct js_cross_reference *xref) {
    // ONLY determine EOF here, and in other functions, treat ts_end_of_file as a normal token in judgement
    _try(_get_token_filtered(source, token));
    if (token->state == ts_end_of_file) {
        return true;
    }
    for (;;) {
        _try(_parse_statement(source, token, bytecode, xref, UINT32_MAX, UINT32_MAX));
        if (token->state == ts_end_of_file) {
            break;
        }
    }
    // prevent loop's break address outside scope
    // _add_instruction(token, bytecode, xref, op_nop, 0);
    return true;
}

bool js_read_source_file(struct js_source *source, const char *filename) {
    // make sure at least \0 in ret
    if (source->base == NULL && source->length == 0 && source->capacity == 0) {
        source->base = alloc(char, 1);
        source->capacity = 1;
    }
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        log_warning("Cannot open \"%s\": %s", filename, strerror(errno));
        return false;
    }
    int ch = fgetc(fp);
    if (ch == '#') { // shebang
        if (fgetc(fp) != '!') {
            log_warning("No '!' after '#' in shebang");
            goto close_on_error;
        }
        for (;;) {
            ch = fgetc(fp);
            if (ch == EOF) {
                log_warning("Unexpected EOF in shebang");
                goto close_on_error;
            }
            if (ch == '\n') {
                break;
            }
        }
    } else {
        string_buffer_append_ch(source->base, source->length, source->capacity, (char)ch);
    }
    char buf[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buf, sizeof(char), countof(buf), fp))) {
        string_buffer_append(source->base, source->length, source->capacity, buf, bytes_read);
    }
    fclose(fp);
    return true;
close_on_error:
    fclose(fp);
    return false;
}

#ifdef DEBUG

void test_lexer() {
    char tokens[] =
        "//ss\n"
        "//2\n"
        "/* foo\n"
        " * bar\n"
        " */\n"
        "// baz\n"
        "true false null\n"
        "1 23 34.5 -6.78e-9\n"
        "_a23_zoS__ baaLJoi887 ___i 9 k s - / // foo\n"
        "-123.45E-2\n"
        "/*\n"
        "*\n"
        "*/\n"
        "asd 3.2E2\n"
        "{[(123.22)]}\n"
        "\"\" \"Hello\" \"\\\"--\\\\--\\/--\\b--\\f--\\n--\\r--\\t--\" \"\" \"\\ud800\\udc00 \\udbff\\udfff\"\n"
        "\"\\uD83D\\uDE10\\u0001\\u0002\\ufffe\\uffff\" \"asd\\\"\" = ===\n"
        "<<=>>=!!=&&|| \n"
        "% 1 % 2\n"
        "let if while\n"
        ". ? ?. \n"
        "+ += - -= * *= / /= % %= ++ --\n"
        "break continue function return\n"
        ". . ... in of typeof delete * *= ** **= try catch finally throw\n"
        ": :: \n"
        ":\n";
    struct js_source source = {
        .base = tokens,
        .length = sizeof(tokens), // use sizeof to test last '\0' -> ts_end_of_file
        .capacity = sizeof(tokens),
    };
    struct js_token token = {0};
    for (;;) {
        bool success = _get_token_filtered(&source, &token);
        if (token.state == ts_end_of_file) {
            return;
        }
        if (!success) {
            return;
        }
        const char *t_name = _token_state_names[token.state];
        switch (token.state) {
        case ts_identifier:
        case ts_string:
            printf("%4u: %s: %.*s\n", token.head_line, t_name, (int)_token_length(&token), _token_head(&source, &token));
            break;
        case ts_number:
            printf("%4u: %s: %g\n", token.head_line, t_name, token.number);
            break;
        default:
            printf("%4u: %s\n", token.head_line, t_name);
            break;
        }
    }
}

void test_parser() {
    // test token error or unexpected end inside parser for _next_token macro:
    // "& null false & true 12.3e-4 \"Hello\"\n"
    // "& null false \0 true 12.3e-4 \"Hello\"\n"
    char *snippets[] = {
        "le", "let", "let a", "let a;", "let a =", "let a = 10", "let a = 10;",
        "let a,", "let a, b", "let a, b;", "let a =,", "let a = 10,",
        "let a = 10, b", "let a = 10, b;", "let a, b = 10;"};
    for (int i = 0; i < countof(snippets); i++) {
        printf("TESTING: \"%s\": \n", snippets[i]);
        struct js_source source = {
            .base = snippets[i],
            .length = (uint32_t)strlen(snippets[i]),
            .capacity = (uint32_t)strlen(snippets[i]),
        };
        struct js_token token = {0};
        struct js_bytecode bytecode = {0};
        struct js_cross_reference xref = {0};
        bool success = js_compile(&source, &token, &bytecode, &xref);
        puts(success ? "success" : "failed");
        js_bytecode_dump(&bytecode);
        // buffer_dump(xref.base, xref.length, xref.capacity);
        putchar('\n');
    }
}

static struct js_result f_native(struct js_vm *vm) {
    // struct js_result ret = js_call_by_name_sz(vm, "f_managed", (struct js_value[]){js_scripture_sz("Hello, "), js_scripture_sz("World, ")}, 2);
    struct js_result ret = {.success = false, .value = js_number(3.14)};
    js_dump_vm(vm);
    return ret;
}

void test_c_function() {
    struct js_source source = {0};
    struct js_token token = {0};
    struct js_vm vm = {0};
    js_declare_variable_sz(&vm, "dump", js_c_function(js_dump_vm));
    js_declare_variable_sz(&vm, "f_native", js_c_function(f_native));
    js_dump_vm(&vm);
    // Test interoperability between C function and Script function, transitivity and closure
    // const char *test = "try { let f_managed = function(){ let c = \"Folks!\"; return function(a, b) {throw a + b + c;}; }(); let a = f_native(); } catch(ex) { dump_vm(); }";
    const char *test = "function f_managed(){throw 3.14;} try { let a = f_native(); } catch(ex) { dump_vm(); }";
    string_buffer_append_sz(source.base, source.length, source.capacity, test);
    if (js_compile(&source, &token, &(vm.bytecode), &(vm.cross_reference))) {
        js_bytecode_dump(&(vm.bytecode));
        struct js_result result = js_run(&vm);
        printf("result is: %s, ", result.success ? "true" : "false");
        js_dump_value(&(result.value));
        printf("\n\n");
    }
    // buffer_dump(vm.bytecode.base, vm.bytecode.length, vm.bytecode.capacity);
    js_dump_vm(&vm);
}

void test_unescape_string() {
    for (;;) {
        char *in = "\\a\\b\\f\\n\\r\\t\\v-\\'-\\\"-\\?-\\\\-\\u1234";
        struct js_source out = _unescape(in, (uint32_t)strlen(in));
        // buffer_dump(out.base, out.length, out.capacity);
        buffer_free(out.base, out.length, out.capacity);
    }
}

void test_free_vm() {
    struct js_source source = {0};
    struct js_token token = {0};
    struct js_vm vm = {0};
    const char *src = "function foo() {let b = \"world\"; return function(a){return a + b;};} let a = \"hello\"; foo()(a);";
    for (;;) {
        string_buffer_append_sz(source.base, source.length, source.capacity, src);
        if (js_compile(&source, &token, &(vm.bytecode), &(vm.cross_reference))) {
            js_run(&vm);
        }
        js_free_vm(&vm);
        token = (struct js_token){0};
        buffer_free(source.base, source.length, source.capacity);
    }
}

void test_read_source_file() {
    for (;;) {
        struct js_source source = {0};
        js_read_source_file(&source, "no-exists");
        buffer_dump(source.base, source.length, source.capacity);
        js_read_source_file(&source, "examples/13-invalid-shebang.js");
        buffer_dump(source.base, source.length, source.capacity);
        js_read_source_file(&source, "examples/14-only-shebang.js");
        buffer_dump(source.base, source.length, source.capacity);
        js_read_source_file(&source, "examples/15-shebang.js");
        buffer_dump(source.base, source.length, source.capacity);
        buffer_free(source.base, source.length, source.capacity);
    }
}

#endif