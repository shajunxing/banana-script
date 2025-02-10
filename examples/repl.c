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
        printf("/unassemble /u          display bytecodes\n");
        printf("/dump /d                display memory dump of source cache, heap and stacks\n");
        printf("/write /w <filename>    write cached source code to disk file\n");
        printf("/gc                     run garbage collector\n");
        printf("/help /h /?             show help\n");
        printf("/exit /quit /q          exit program\n");
    } else if (_match_cmd(line, "/unassemble") || _match_cmd(line, "/u")) {
        js_dump_bytecodes(pjs);
        js_dump_tablet(pjs);
    } else if (_match_cmd(line, "/dump") || _match_cmd(line, "/d")) {
        js_dump_source(pjs);
        js_dump_heap(pjs);
        js_dump_call_stack(pjs);
        js_dump_evaluation_stack(pjs);
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
    struct js_value print_func = js_c_function(pjs, js_c_print);
    struct js_value console_obj = js_object(pjs);
    js_variable_declare_sz(pjs, "print", print_func);
    js_variable_declare_sz(pjs, "console", console_obj);
    js_object_put_sz(pjs, &console_obj, "log", print_func);
    js_variable_declare_sz(pjs, "gc", js_c_function(pjs, js_collect_garbage));
    js_variable_declare_sz(pjs, "dump", js_c_function(pjs, js_dump_call_stack));
    js_variable_declare_sz(pjs, "stat", js_c_function(pjs, js_print_statistics));
    js_variable_declare_sz(pjs, "clock", js_c_function(pjs, js_c_clock));
    printf("Banana JS REPL environment. Copyright (C) 2024 ShaJunXing <shajunxing@hotmail.com>.\n");
    printf("Type '/help' for more information.\n\n");
    for (;;) {
        size_t src_len_backup;
        struct js_token tok_backup;
        size_t bytecodes_len_backup;
        size_t bytecodes_idx_backup;
        char *line;
        // backup states
        src_len_backup = pjs->src_len;
        tok_backup = pjs->tok;
        bytecodes_len_backup = pjs->bytecodes_len;
        bytecodes_idx_backup = pjs->bytecodes_idx;
        js_call_stack_backup(pjs);
        printf("> ");
        line = read_line(stdin, NULL);
        if (line != NULL && strlen(line) > 0) {
            if (line[0] == '/') {
                _parse_command(pjs, line);
            } else {
                if (js_try(pjs)) {
                    js_load_string_sz(pjs, line);
                    js_next_token(pjs);
                    js_parse_script(pjs);
                    js_interpret(pjs);
                } else {
                    puts(pjs->err_msg);
                    // restore states
                    pjs->src_len = src_len_backup;
                    pjs->tok = tok_backup;
                    pjs->bytecodes_len = bytecodes_len_backup;
                    pjs->bytecodes_idx = bytecodes_idx_backup;
                    // clear stacks, for example, function error
                    js_call_stack_restore(pjs);
                }
            }
        }
        free(line);
    };
    js_delete(pjs);
    return EXIT_SUCCESS;
}