/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../src/js.h"

int main(int argc, char *argv[]) {
    if (argc == 2) {
        size_t len;
        char *src = read_file(argv[1], &len);
        if (src) {
            struct js *pjs = js_new();
            int stage = 0;
            if (js_try(pjs)) {
                // predefined functions
                struct js_value *print_func = js_cfunction_new(pjs, js_function_print);
                struct js_value *console_obj = js_object_new(pjs);
                js_variable_declare_sz(pjs, "print", print_func);
                js_variable_declare_sz(pjs, "console", console_obj);
                js_object_put_sz(pjs, console_obj, "log", print_func);
                js_variable_declare_sz(pjs, "gc", js_cfunction_new(pjs, js_collect_garbage));
                js_variable_declare_sz(pjs, "dump", js_cfunction_new(pjs, js_dump_stack));
                js_variable_declare_sz(pjs, "stat", js_cfunction_new(pjs, js_print_statistics));
                js_variable_declare_sz(pjs, "clock", js_cfunction_new(pjs, js_function_clock));
                js_load_string(pjs, src, len);
                stage = 1;
                pjs->parse_exec = true;
                js_parse_script(pjs);
            } else {
                if (stage == 0) {
                    js_lexer_print_error(pjs);
                } else {
                    js_parser_print_error(pjs);
                }
            }
            js_delete(pjs);
            free(src);
            return EXIT_SUCCESS;
        }
        printf("Failed to read \"%s\"\n", argv[1]);
    }
    printf("Usage: %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
}