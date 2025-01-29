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
        return "";
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

static enum js_token_state _match_keyword_fast(const char *id, size_t idlen) {
    static const int keywords[] = {12, 'b', 25, 'c', 39, 'd', 62, 'e', 83, 'f', 94, 'i', 137, 'l', 146, 'n', 154, 'o', 165, 'r', 170, 't', 187, 'w', 214, 1, 'r', 28, 1, 'e', 31, 1, 'a', 34, 1, 'k', 37, 0, ts_break, 1, 'o', 42, 1, 'n', 45, 1, 't', 48, 1, 'i', 51, 1, 'n', 54, 1, 'u', 57, 1, 'e', 60, 0, ts_continue, 2, 'e', 67, 'o', 81, 1, 'l', 70, 1, 'e', 73, 1, 't', 76, 1, 'e', 79, 0, ts_delete, 0, ts_do, 1, 'l', 86, 1, 's', 89, 1, 'e', 92, 0, ts_else, 3, 'a', 101, 'u', 112, 'o', 132, 1, 'l', 104, 1, 's', 107, 1, 'e', 110, 0, ts_false, 1, 'n', 115, 1, 'c', 118, 1, 't', 121, 1, 'i', 124, 1, 'o', 127, 1, 'n', 130, 0, ts_function, 1, 'r', 135, 0, ts_for, 2, 'f', 142, 'n', 144, 0, ts_if, 0, ts_in, 1, 'e', 149, 1, 't', 152, 0, ts_let, 1, 'u', 157, 1, 'l', 160, 1, 'l', 163, 0, ts_null, 1, 'f', 168, 0, ts_of, 1, 'e', 173, 1, 't', 176, 1, 'u', 179, 1, 'r', 182, 1, 'n', 185, 0, ts_return, 2, 'y', 192, 'r', 206, 1, 'p', 195, 1, 'e', 198, 1, 'o', 201, 1, 'f', 204, 0, ts_typeof, 1, 'u', 209, 1, 'e', 212, 0, ts_true, 1, 'h', 217, 1, 'i', 220, 1, 'l', 223, 1, 'e', 226, 0, ts_while};
    size_t i;
    size_t k = 0;
    for (i = 0; i < idlen; i++) {
        char c = id[i];
        bool matched = false;
        int j;
        int jmax = keywords[k];
        for (j = 0; j < jmax; j++) {
            k++;
            if (c == keywords[k]) {
                k++;
                k = keywords[k];
                matched = true;
                break;
            }
            k++;
        }
        if (!matched) {
            return ts_identifier;
        }
    }
    if (keywords[k] == 0) {
        k++;
        return keywords[k];
    } else {
        return ts_identifier;
    }
}

void js_lexer_next_token(struct js *pjs) {
    // use DFA because fit my thinking habits, is simple for me
    // DONT use switch/case because "break" will conflict with "for"
    // following firefox Syntax error text, use F12 to check
    // at first, tok.h = NULL
    // DONT calculate tok.line in one place because when tok.t=\n and return, and call again, \n may be calcualted more than once
    while (pjs->tok.t < pjs->src_len) {
        // always make sure [tok.h, tok.t) is printable, eg. tok.t point to next outside, and correctly count new line
        // log("%2d %3s  %s", pjs->tok.line, ascii_abbreviation(tok_t_char), js_token_state_name(pjs->tok.stat));
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
                } else {
                    pjs->tok.stat = ts_multiplication;
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
            pjs->tok.stat = _match_keyword_fast(tok_hdr, tok_len);
        }
        // DONT declare a variable = js_token_state_name(pjs->tok.stat), or in release mode will cause "warning: unused variable" because log() is not exist
        switch (pjs->tok.stat) {
        case ts_identifier:
        case ts_string:
            log("%ld:%s:%.*s", pjs->tok.line, js_token_state_name(pjs->tok.stat), (int)tok_len, tok_hdr);
            break;
        case ts_number:
            log("%ld:%s:%g", pjs->tok.line, js_token_state_name(pjs->tok.stat), pjs->tok.num);
            break;
        default:
            log("%ld:%s", pjs->tok.line, js_token_state_name(pjs->tok.stat));
            break;
        }
    }
    return;
}

void js_lexer_print_error(struct js *pjs) {
    printf("%s:%ld %ld:%s:%.*s: %s\n", pjs->err_file, pjs->err_line,
           pjs->tok.line, js_token_state_name(pjs->tok.stat),
           (int)_token_length(pjs), _token_head(pjs), pjs->err_msg);
}