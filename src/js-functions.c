/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"
#include <time.h>

void js_function_print(struct js *pjs) {
    size_t i;
    for (i = 0; i < js_parameter_length(pjs); i++) {
        js_value_dump(js_parameter_get(pjs, i));
        printf(" ");
    }
    printf("\n");
}

void js_function_clock(struct js *pjs) {
    pjs->result = js_number_new(pjs, clock() * 1.0 / CLOCKS_PER_SEC);
}
