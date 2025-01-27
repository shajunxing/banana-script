/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../src/js.h"

static void _parse_command(struct js *pjs, char *str) {
    if (strcmp(str, "/help") == 0 || strcmp(str, "/h") == 0 || strcmp(str, "/?") == 0) {
        printf("/help /h /?         show help\n");
        printf("/exit /quit /q      exit program\n");
        printf("/source /s          show plain source code\n");
        printf("/dump /d            show source, token, value and stack dumps\n");
        printf("/gc                 run garbage collector\n");
    } else if (strcmp(str, "/exit") == 0 || strcmp(str, "/quit") == 0 || strcmp(str, "/q") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(str, "/source") == 0 || strcmp(str, "/s") == 0) {
        js_print_source(pjs);
    } else if (strcmp(str, "/dump") == 0 || strcmp(str, "/d") == 0) {
        js_dump_source(pjs);
        js_dump_tokens(pjs);
        js_dump_values(pjs);
        js_dump_stack(pjs);
    } else if (strcmp(str, "/gc") == 0) {
        js_collect_garbage(pjs);
    }
}

int main() {
    struct js *pjs = js_new();
    // global functions
    struct js_value *print_func = js_cfunction_new(pjs, js_print_values);
    struct js_value *console_obj = js_object_new(pjs);
    js_variable_declare_sz(pjs, "print", print_func);
    js_variable_declare_sz(pjs, "console", console_obj);
    js_object_put_sz(pjs, console_obj, "log", print_func);
    printf("Banana JS REPL environment. Copyright (C) 2024 ShaJunXing <shajunxing@hotmail.com>.\n");
    printf("Type '/help' for more information.\n");
    for (;;) {
        // backup states
        size_t src_len_backup = pjs->src_len;
        struct js_token tok_backup = pjs->tok;
        size_t tok_cache_idx_backup = pjs->tok_cache_idx;
        size_t tok_cache_len_backup = pjs->tok_cache_len;
        char *str;
        size_t len;
        printf("> ");
        str = read_line(stdin, &len);
        if (str == NULL) {
            break;
        } else if (strlen(str) > 0) {
            if (str[0] == '/') {
                _parse_command(pjs, str);
            } else {
                if (js_try(pjs)) {
                    js_load_string(pjs, str, len);
                    // first, parse only, check syntax error, for example, declaraction and assignment expression, to prevent variable written on stack before ';' missing error occurs
                    pjs->parse_exec = false;
                    js_parse_script(pjs);
                    // then execute
                    pjs->tok_cache_idx = tok_cache_idx_backup;
                    pjs->parse_exec = true;
                    js_parse_script(pjs);
                } else {
                    puts(pjs->err_msg);
                    // restore states
                    pjs->src_len = src_len_backup;
                    pjs->tok = tok_backup;
                    pjs->tok_cache_idx = tok_cache_idx_backup;
                    pjs->tok_cache_len = tok_cache_len_backup;
                    // clear stacks, for example, function error
                    js_stack_backward_to(pjs, 1);
                }
            }
        }
        free(str);
    };
    js_delete(pjs);
    return EXIT_SUCCESS;
}