/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../src/js.h"

static void _parse_command(struct js *pjs, char *line) {
#define _match_cmd(line, cmd) (strncmp((line), (cmd), sizeof(cmd) - 1) == 0)
    if (_match_cmd(line, "/help") || _match_cmd(line, "/h") || _match_cmd(line, "/?")) {
        printf("You can execute javascript statements or run following commands:\n");
        printf("/source /u              display cached source code\n");
        printf("/write /w <filename>    write cached source to file\n");
        printf("/dump /d                display memory dump of source, tokens, values and stack\n");
        printf("/gc                     run garbage collector\n");
        printf("/help /h /?             show help\n");
        printf("/exit /quit /q          exit program\n");
    } else if (_match_cmd(line, "/source") || _match_cmd(line, "/u")) {
        js_print_source(pjs);
    } else if (_match_cmd(line, "/write") || _match_cmd(line, "/w")) {
        char *filename = strrchr(line, ' ');
        FILE *fp;
        if (filename && strlen(++filename) > 0) {
            fp = fopen(filename, "w");
            if (!fp) {
                printf("Failed to open \"%s\"\n", filename);
            } else {
                fwrite(pjs->src, 1, pjs->src_len, fp);
                fclose(fp);
                printf("Done writing to \"%s\"\n", filename);
            }
        } else {
            printf("Expect filename\n");
        }
    } else if (_match_cmd(line, "/dump") || _match_cmd(line, "/d")) {
        js_dump_source(pjs);
        js_dump_tokens(pjs);
        js_dump_values(pjs);
        js_dump_stack(pjs);
    } else if (_match_cmd(line, "/gc")) {
        js_collect_garbage(pjs);
    } else if (_match_cmd(line, "/exit") || _match_cmd(line, "/quit") || _match_cmd(line, "/q")) {
        exit(EXIT_SUCCESS);
    } else {
        printf("Unknown command \"%s\"\n", line);
    }
}

int main() {
    struct js *pjs = js_new();
    // predefined functions
    struct js_value *print_func = js_cfunction_new(pjs, js_print_values);
    struct js_value *console_obj = js_object_new(pjs);
    js_variable_declare_sz(pjs, "print", print_func);
    js_variable_declare_sz(pjs, "console", console_obj);
    js_object_put_sz(pjs, console_obj, "log", print_func);
    js_variable_declare_sz(pjs, "gc", js_cfunction_new(pjs, js_collect_garbage));
    js_variable_declare_sz(pjs, "dump", js_cfunction_new(pjs, js_dump_stack));
    printf("Banana JS REPL environment. Copyright (C) 2024 ShaJunXing <shajunxing@hotmail.com>.\n");
    printf("Type '/help' for more information.\n\n");
    for (;;) {
        // backup states
        size_t src_len_backup = pjs->src_len;
        struct js_token tok_backup = pjs->tok;
        size_t tok_cache_idx_backup = pjs->tok_cache_idx;
        size_t tok_cache_len_backup = pjs->tok_cache_len;
        char *line;
        printf("> ");
        line = read_line(stdin, NULL);
        if (line == NULL) {
            break;
        } else if (strlen(line) > 0) {
            if (line[0] == '/') {
                _parse_command(pjs, line);
            } else {
                if (js_try(pjs)) {
                    js_load_string_sz(pjs, line);
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
        free(line);
    };
    js_delete(pjs);
    return EXIT_SUCCESS;
}