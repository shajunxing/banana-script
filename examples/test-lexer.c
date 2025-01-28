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
            int i;
            struct js *pjs = js_new();
            if (js_try(pjs)) {
                js_load_string(pjs, src, len);
                for (i = 0; i < pjs->tok_cache_len; i++) {
                    printf("%s\n", js_token_state_name(pjs->tok_cache[i].stat));
                }
            } else {
                js_lexer_print_error(pjs);
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