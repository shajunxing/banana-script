/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

const char *js_token_state_name(enum js_token_state stat) {
// if extern const array, sizeof is not useable anymore
// https://www.reddit.com/r/C_Programming/comments/1169mv6/best_way_to_write_constant_arrays_to_be_included/
#define X(name) #name,
    static const char *const names[] = {js_token_state_list};
#undef X
    if (stat >= 0 && stat < sizeof(names)) {
        return names[stat];
    } else {
        return "???";
    }
}

static char *_token_head(struct js *pjs) {
    return pjs->src + pjs->tok.h;
}

static size_t _token_length(struct js *pjs) {
    return pjs->tok.t - pjs->tok.h;
}

static bool _is_identifier_first_character(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

static bool _is_identifier_rest_characters(char ch) {
    return _is_identifier_first_character(ch) || (ch >= '0' && ch <= '9');
}

static void _reset_token_number(struct js *pjs) {
    pjs->tok_num_sign = 1;
    pjs->tok_num_inte = 0;
    pjs->tok_num_frac = 0;
    pjs->tok_num_frac_depth = 0.1;
    pjs->tok_num_exp_sign = 1;
    pjs->tok_num_exp = 0;
    // log("_reset_token_number");
}

static double _char_to_digit(char ch) {
    return ch - '0';
}

static void _accumulate_token_number_integer(struct js *pjs, char ch) {
    pjs->tok_num_inte = pjs->tok_num_inte * 10 + _char_to_digit(ch);
    // log("%lg", pjs->tok_num_inte);
}

static void _accumulate_token_number_fraction(struct js *pjs, char ch) {
    pjs->tok_num_frac = pjs->tok_num_frac + _char_to_digit(ch) * pjs->tok_num_frac_depth;
    pjs->tok_num_frac_depth *= 0.1;
}

static void _accumulate_token_number_exponent(struct js *pjs, char ch) {
    pjs->tok_num_exp = pjs->tok_num_exp * 10 + _char_to_digit(ch);
}

static void _calculate_token_number(struct js *pjs) {
    // must include math.h, or pow() result is 0
    double num = pjs->tok_num_sign * (pjs->tok_num_inte + pjs->tok_num_frac) * pow(10.0, pjs->tok_num_exp_sign * pjs->tok_num_exp);
    // log("%d %g %g %d %g = %g", pjs->tok_num_sign, pjs->tok_num_inte, pjs->tok_num_frac, pjs->tok_num_exp_sign, pjs->tok_num_exp, num);
    pjs->tok.num = num;
}

static enum js_token_state _match_keyword(const char *id, size_t idlen) {
#define kw_entry(kw) \
    { #kw, sizeof(#kw) - 1, ts_##kw }
    static const struct {
        const char *name;
        size_t len;
        enum js_token_state token;
    } keywords[] = {
                    kw_entry(null),
                    kw_entry(true),
                    kw_entry(false),
                    kw_entry(let),
                    kw_entry(if),
                    kw_entry(else),
                    kw_entry(while),
                    kw_entry(do),
                    kw_entry(for),
                    kw_entry(break),
                    kw_entry(continue),
                    kw_entry(function),
                    kw_entry(return),
                    kw_entry(in),
                    kw_entry(of),
                    kw_entry(typeof),
                    kw_entry(delete),
    };
    int i;
    for (i = 0; i < countof(keywords); i++) {
        // match keyword len, DONT use token len, for example, "fals"
        if (strncmp(id, keywords[i].name, max(idlen, keywords[i].len)) == 0) {
            return keywords[i].token;
        }
    }
    return ts_identifier;
}

static void _next_token(struct js *pjs) {
    // use DFA because fit my thinking habits, is simple for me
    // DONT use switch/case because "break" will conflict with "for"
    // following firefox Syntax error text, use F12 to check
    // at first, tok.h = NULL
    // DONT calculate tok.line in one place because when tok.t=\n and return, and call again, \n may be calcualted more than once
    while (pjs->tok.t < pjs->src_len) {
        // always make sure [tok.h, tok.t) is printable, eg. tok.t point to next outside, and correctly count new line
        if (pjs->tok.stat == ts_searching) {
            // searching state matches tok.h, which is inside scope
            char tok_h_char;
            pjs->tok.h = pjs->tok.t;
            pjs->tok.t++;
            tok_h_char = *_token_head(pjs);
            if (isspace(tok_h_char)) {
                if (tok_h_char == '\n') {
                    pjs->tok.line++;
                }
            } else if (_is_identifier_first_character(tok_h_char)) {
                pjs->tok.stat = ts_identifier_matching;
            } else if (tok_h_char == '/') {
                pjs->tok.stat = ts_division_matching;
            } else if (tok_h_char == '-') {
                // slightly modified, json negative value not supported, to prevent such as '1-2' parsed to '1' '-2' illegal expression error, but json still can be parsed because '-2' is legal expression
                pjs->tok.stat = ts_minus_matching;
            } else if (isdigit(tok_h_char)) {
                _reset_token_number(pjs);
                _accumulate_token_number_integer(pjs, tok_h_char);
                pjs->tok.stat = ts_number_matching;
            } else if (tok_h_char == '+') {
                pjs->tok.stat = ts_plus_matching;
            } else if (tok_h_char == '*') {
                pjs->tok.stat = ts_multiplication_matching;
            } else if (tok_h_char == '(') {
                pjs->tok.stat = ts_left_parenthesis;
                goto matched;
            } else if (tok_h_char == ')') {
                pjs->tok.stat = ts_right_parenthesis;
                goto matched;
            } else if (tok_h_char == '[') {
                pjs->tok.stat = ts_left_bracket;
                goto matched;
            } else if (tok_h_char == ']') {
                pjs->tok.stat = ts_right_bracket;
                goto matched;
            } else if (tok_h_char == '{') {
                pjs->tok.stat = ts_left_brace;
                goto matched;
            } else if (tok_h_char == '}') {
                pjs->tok.stat = ts_right_brace;
                goto matched;
            } else if (tok_h_char == '\"') {
                pjs->tok.stat = ts_string_matching;
            } else if (tok_h_char == ':') {
                pjs->tok.stat = ts_colon;
                goto matched;
            } else if (tok_h_char == ',') {
                pjs->tok.stat = ts_comma;
                goto matched;
            } else if (tok_h_char == '=') {
                pjs->tok.stat = ts_assignment_matching;
            } else if (tok_h_char == '<') {
                pjs->tok.stat = ts_less_than_matching;
            } else if (tok_h_char == '>') {
                pjs->tok.stat = ts_greater_than_matching;
            } else if (tok_h_char == '!') {
                pjs->tok.stat = ts_not_matching;
            } else if (tok_h_char == '&') {
                pjs->tok.stat = ts_and_matching;
            } else if (tok_h_char == '|') {
                pjs->tok.stat = ts_or_matching;
            } else if (tok_h_char == '%') {
                pjs->tok.stat = ts_mod_matching;
            } else if (tok_h_char == ';') {
                pjs->tok.stat = ts_semicolon;
                goto matched;
            } else if (tok_h_char == '.') {
                pjs->tok.stat = ts_one_dot_matching;
            } else if (tok_h_char == '?') {
                pjs->tok.stat = ts_question_matching;
            } else {
                js_throw(pjs, "Illegal character");
            }
        } else {
            char tok_t_char = *(pjs->src + pjs->tok.t);
            if (pjs->tok.stat == ts_identifier_matching) {
                // most other states determined by "next" character tok.t, which is outside scope
                // but some will check inside scope, for example: block comment
                if (_is_identifier_rest_characters(tok_t_char)) {
                    pjs->tok.t++;
                } else {
                    pjs->tok.stat = ts_identifier;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_division_matching) {
                if (tok_t_char == '/') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_line_comment_matching_new_line;
                } else if (tok_t_char == '*') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_block_comment_matching_end_star;
                } else if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_division_assignment;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_division;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_line_comment_matching_new_line) {
                if (tok_t_char == '\n') {
                    pjs->tok.line++;
                    pjs->tok.t++;
                    pjs->tok.stat = ts_line_comment;
                    goto matched;
                } else {
                    pjs->tok.t++;
                }
            } else if (pjs->tok.stat == ts_block_comment_matching_end_star) {
                if (tok_t_char == '*') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_block_comment_matching_end_slash;
                } else {
                    if (tok_t_char == '\n') {
                        pjs->tok.line++;
                    }
                    pjs->tok.t++;
                }
            } else if (pjs->tok.stat == ts_block_comment_matching_end_slash) {
                if (tok_t_char == '/') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_block_comment;
                    goto matched;
                } else {
                    if (tok_t_char == '\n') {
                        pjs->tok.line++;
                    }
                    pjs->tok.t++;
                    pjs->tok.stat = ts_block_comment_matching_end_star;
                }
            } else if (pjs->tok.stat == ts_number_matching) { // match from 2nd integral digit to check leading 0
                if (isdigit(tok_t_char)) {
                    if (pjs->tok_num_inte == 0) { //  json rule
                        js_throw(pjs, "Interger part starting with 0 cannot follow any other digits");
                    }
                    _accumulate_token_number_integer(pjs, tok_t_char);
                    pjs->tok.t++;
                } else if (tok_t_char == '.') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_number_matching_fraction_first;
                } else if (tok_t_char == 'E' || tok_t_char == 'e') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_number_matching_exponent_first;
                } else if (_is_identifier_first_character(tok_t_char)) {
                    pjs->tok.t++;
                    js_throw(pjs, "Identifier starts immediately after numeric literal");
                } else {
                    _calculate_token_number(pjs);
                    pjs->tok.stat = ts_number;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_number_matching_fraction_first) {
                // fraction may start with myltiple 0, so it cannot check missing using == 0 like integral part
                if (isdigit(tok_t_char)) {
                    _accumulate_token_number_fraction(pjs, tok_t_char);
                    pjs->tok.stat = ts_number_matching_fraction_rest;
                    pjs->tok.t++;
                } else {
                    js_throw(pjs, "Missing fraction");
                }
            } else if (pjs->tok.stat == ts_number_matching_fraction_rest) {
                if (isdigit(tok_t_char)) {
                    _accumulate_token_number_fraction(pjs, tok_t_char);
                    pjs->tok.t++;
                } else if (tok_t_char == 'E' || tok_t_char == 'e') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_number_matching_exponent_first;
                } else if (_is_identifier_first_character(tok_t_char)) {
                    pjs->tok.t++;
                    js_throw(pjs, "Identifier starts immediately after numeric literal");
                } else {
                    _calculate_token_number(pjs);
                    pjs->tok.stat = ts_number;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_number_matching_exponent_first) {
                // exponent may start with myltiple 0, so it cannot check missing using == 0 like integral part
                if (tok_t_char == '+' || tok_t_char == '-') {
                    if (tok_t_char == '-') {
                        pjs->tok_num_exp_sign = -1;
                    }
                    pjs->tok.t++;
                    pjs->tok.stat = ts_number_matching_exponent_number_first;
                } else if (isdigit(tok_t_char)) {
                    _accumulate_token_number_exponent(pjs, tok_t_char);
                    pjs->tok.t++;
                    pjs->tok.stat = ts_number_matching_exponent_rest;
                } else {
                    js_throw(pjs, "Missing exponent");
                }
            } else if (pjs->tok.stat == ts_number_matching_exponent_number_first) {
                if (isdigit(tok_t_char)) {
                    _accumulate_token_number_exponent(pjs, tok_t_char);
                    pjs->tok.t++;
                    pjs->tok.stat = ts_number_matching_exponent_rest;
                } else {
                    js_throw(pjs, "Missing exponent");
                }
            } else if (pjs->tok.stat == ts_number_matching_exponent_rest) {
                if (isdigit(tok_t_char)) {
                    _accumulate_token_number_exponent(pjs, tok_t_char);
                    pjs->tok.t++;
                } else if (_is_identifier_first_character(tok_t_char)) {
                    pjs->tok.t++;
                    js_throw(pjs, "Identifier starts immediately after numeric literal");
                } else {
                    _calculate_token_number(pjs);
                    pjs->tok.stat = ts_number;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_string_matching) {
                if (tok_t_char == '\\') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_string_matching_control;
                } else if (tok_t_char == '\"') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_string;
                    goto matched;
                } else if (tok_t_char == '\n') {
                    js_throw(pjs, "Line break is not allowed inside string literal");
                } else {
                    pjs->tok.t++;
                }
            } else if (pjs->tok.stat == ts_string_matching_control) {
                pjs->tok.t++;
                pjs->tok.stat = ts_string_matching;
            } else if (pjs->tok.stat == ts_assignment_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_equal_to;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_assignment;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_less_than_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_less_than_or_equal_to;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_less_than;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_greater_than_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_greater_than_or_equal_to;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_greater_than;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_not_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_not_equal_to;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_not;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_and_matching) {
                if (tok_t_char == '&') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_and;
                    goto matched;
                } else {
                    js_throw(pjs, "Unfinished logical && operator");
                }
            } else if (pjs->tok.stat == ts_or_matching) {
                if (tok_t_char == '|') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_or;
                    goto matched;
                } else {
                    js_throw(pjs, "Unfinished logical || operator");
                }
            } else if (pjs->tok.stat == ts_one_dot_matching) {
                if (tok_t_char == '.') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_two_dot_matching;
                } else {
                    pjs->tok.stat = ts_member_access;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_two_dot_matching) {
                if (tok_t_char == '.') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_spread;
                    goto matched;
                } else {
                    js_throw(pjs, "Unfinished spread ... operator");
                }
            } else if (pjs->tok.stat == ts_question_matching) {
                if (tok_t_char == '.') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_optional_chaining;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_question;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_minus_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_minus_assignment;
                    goto matched;
                } else if (tok_t_char == '-') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_minus_minus;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_minus;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_plus_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_plus_assignment;
                    goto matched;
                } else if (tok_t_char == '+') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_plus_plus;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_plus;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_multiplication_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_multiplication_assignment;
                    goto matched;
                } else if (tok_t_char == '*') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_exponentiation_matching;
                } else {
                    pjs->tok.stat = ts_multiplication;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_exponentiation_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_exponentiation_assignment;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_exponentiation;
                    goto matched;
                }
            } else if (pjs->tok.stat == ts_mod_matching) {
                if (tok_t_char == '=') {
                    pjs->tok.t++;
                    pjs->tok.stat = ts_mod_assignment;
                    goto matched;
                } else {
                    pjs->tok.stat = ts_mod;
                    goto matched;
                }
            } else { // back to searching state
                pjs->tok.stat = ts_searching;
            }
        }
    }
    // remaining content before eof
    switch (pjs->tok.stat) {
    case ts_identifier_matching:
        pjs->tok.stat = ts_identifier;
        goto matched;
    case ts_division_matching:
        pjs->tok.stat = ts_division;
        goto matched;
    case ts_line_comment_matching_new_line:
        pjs->tok.stat = ts_line_comment;
        goto matched;
    case ts_block_comment_matching_end_star:
    case ts_block_comment_matching_end_slash:
        js_throw(pjs, "Unfinished block comment");
    case ts_number_matching_fraction_first:
        js_throw(pjs, "Missing fraction");
    case ts_number_matching_exponent_first:
    case ts_number_matching_exponent_number_first:
        js_throw(pjs, "Missing exponent");
    case ts_number_matching:
    case ts_number_matching_fraction_rest:
    case ts_number_matching_exponent_rest:
        _calculate_token_number(pjs);
        pjs->tok.stat = ts_number;
        goto matched;
    case ts_string_matching:
    case ts_string_matching_control:
        js_throw(pjs, "Unfinished string");
    case ts_assignment_matching:
        pjs->tok.stat = ts_assignment;
        goto matched;
    case ts_less_than_matching:
        pjs->tok.stat = ts_less_than;
        goto matched;
    case ts_greater_than_matching:
        pjs->tok.stat = ts_greater_than;
        goto matched;
    case ts_not_matching:
        pjs->tok.stat = ts_not;
        goto matched;
    case ts_and_matching:
        js_throw(pjs, "Unfinished logical && operator");
    case ts_or_matching:
        js_throw(pjs, "Unfinished logical || operator");
    case ts_one_dot_matching:
        pjs->tok.stat = ts_member_access;
        goto matched;
    case ts_two_dot_matching:
        js_throw(pjs, "Unfinished spread ... operator");
    case ts_question_matching:
        pjs->tok.stat = ts_question;
        goto matched;
    case ts_minus_matching:
        pjs->tok.stat = ts_minus;
        goto matched;
    case ts_plus_matching:
        pjs->tok.stat = ts_plus;
        goto matched;
    case ts_multiplication_matching:
        pjs->tok.stat = ts_multiplication;
        goto matched;
    case ts_exponentiation_matching:
        pjs->tok.stat = ts_exponentiation;
        goto matched;
    case ts_mod_matching:
        pjs->tok.stat = ts_mod;
        goto matched;
    case ts_end_of_file:
        js_throw(pjs, "End of file");
    default:
        // after final ready state processed, will set eof state for syntax parser to end loop
        // recall after eof state will cause eof error
        pjs->tok.h = pjs->tok.t;
        pjs->tok.stat = ts_end_of_file;
        goto matched;
    }
matched:
    // replace "return" with "goto matched" so that extra works can be done before return
    {
        // match keywords
        char *tok_hdr = _token_head(pjs);
        size_t tok_len = _token_length(pjs);
        if (pjs->tok.stat == ts_identifier) {
            pjs->tok.stat = _match_keyword(tok_hdr, tok_len);
        }
    }
    return;
}

void js_next_token(struct js *pjs) {
    do {
        _next_token(pjs);
    } while (pjs->tok.stat == ts_block_comment || pjs->tok.stat == ts_line_comment);
}

void js_token_dump(struct js *pjs) {
    const char *tok_name = js_token_state_name(pjs->tok.stat);
    char *tok_hdr = _token_head(pjs);
    size_t tok_len = _token_length(pjs);
    switch (pjs->tok.stat) {
    case ts_identifier:
    case ts_string:
        printf("%ld:%s:%.*s", pjs->tok.line, tok_name, (int)tok_len, tok_hdr);
        break;
    case ts_number:
        printf("%ld:%s:%g", pjs->tok.line, tok_name, pjs->tok.num);
        break;
    default:
        printf("%ld:%s", pjs->tok.line, tok_name);
        break;
    }
}

void js_print_error(struct js *pjs) {
    printf("%s:%ld %ld:%s:%.*s: %s\n", pjs->err_file, pjs->err_line,
           pjs->tok.line, js_token_state_name(pjs->tok.stat),
           (int)_token_length(pjs), _token_head(pjs), pjs->err_msg);
}