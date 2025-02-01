/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../src/js.h"

int main() {
    printf("A calculator that supports arithmetic, relational, logical operations and more\n");
    printf("Powered by Banana JS\n\n");
    for (;;) {
        struct js *pjs = js_new();
        pjs->parse_exec = true;
        if (js_try(pjs)) {
            char *str;
            size_t len;
            printf("Enter expression: ");
            str = read_line(stdin, &len);
            if (str == NULL) {
                continue;
            }
            js_load_string(pjs, str, len);
            js_value_dump(js_parse_expression(pjs));
            printf("\n");
        } else {
            puts(pjs->err_msg);
        }
        js_delete(pjs);
    };
    return EXIT_SUCCESS;
}