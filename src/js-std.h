/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_STD_H
#define JS_STD_H

#include "js-vm.h"

/*
function naming rules:
1. simple unique feature, match one dos/unix command/function, such as 'cd' 'md' 'rd', use shortest name, can also improve speed
2. more complicated, customize it's name
*/

shared int js_std_argc;
shared char **js_std_argv;
shared const char *js_std_pathsep;
shared struct js_result js_std_chdir(struct js_vm *);
shared struct js_result js_std_clock(struct js_vm *);
shared struct js_result js_std_close(struct js_vm *);
shared struct js_result js_std_dirname(struct js_vm *);
shared struct js_result js_std_endswith(struct js_vm *);
shared struct js_result js_std_exists(struct js_vm *);
shared struct js_result js_std_exit(struct js_vm *);
shared struct js_result js_std_format(struct js_vm *);
shared struct js_result js_std_getcwd(struct js_vm *);
shared struct js_result js_std_input(struct js_vm *);
shared struct js_result js_std_join(struct js_vm *);
shared struct js_result js_std_length(struct js_vm *);
shared struct js_result js_std_listdir(struct js_vm *);
shared struct js_result js_std_match(struct js_vm *);
shared struct js_result js_std_mkdir(struct js_vm *);
shared struct js_result js_std_natural_compare(struct js_vm *);
shared struct js_result js_std_open(struct js_vm *);
shared struct js_result js_std_pop(struct js_vm *);
shared struct js_result js_std_print(struct js_vm *);
shared struct js_result js_std_push(struct js_vm *);
shared struct js_result js_std_read(struct js_vm *);
shared struct js_result js_std_remove(struct js_vm *);
shared struct js_result js_std_rmdir(struct js_vm *);
shared struct js_result js_std_sort(struct js_vm *);
shared struct js_result js_std_split(struct js_vm *);
shared struct js_result js_std_startswith(struct js_vm *);
shared struct js_result js_std_tojson(struct js_vm *);
shared struct js_result js_std_tonumber(struct js_vm *);
shared struct js_result js_std_tostring(struct js_vm *);
shared struct js_result js_std_write(struct js_vm *);
shared void js_declare_std_functions(struct js_vm *);

#ifdef DEBUG

shared struct js_result js_std_forward(struct js_vm *);

#endif

#endif