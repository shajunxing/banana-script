/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_STD_OS_H
#define JS_STD_OS_H

#include "js-vm.h"

shared const char *js_std_os;
shared const char *js_std_pathsep;
shared struct js_result js_std_basename(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_cd(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_clock(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_close(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_ctime(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_cwd(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_dirname(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_exists(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_exit(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_input(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_ls(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_md(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_open(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_print(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_read(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_rd(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_rm(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_sleep(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_spawn(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_stat(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_system(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_time(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_whoami(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_write(struct js_vm *, uint16_t, struct js_value *);
#ifdef _WIN32
shared struct js_result js_std_play(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_title(struct js_vm *, uint16_t, struct js_value *);
#else
shared struct js_result js_std_exec(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_fork(struct js_vm *, uint16_t, struct js_value *);
#endif
shared void js_declare_std_os_functions(struct js_vm *);

#endif