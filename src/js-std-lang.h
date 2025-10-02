/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_STD_LANG_H
#define JS_STD_LANG_H

#include "js-vm.h"

shared struct js_result js_std_ceil(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_dump_vm(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_endswith(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_filter(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_floor(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_format(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_gc(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_join(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_length(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_map(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_match(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_modf(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_natural_compare(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_pop(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_push(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_reduce(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_round(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_sort(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_split(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_startswith(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_todump(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_tolower(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_tojson(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_tonumber(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_tostring(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_toupper(struct js_vm *, uint16_t, struct js_value *);
shared struct js_result js_std_trunc(struct js_vm *, uint16_t, struct js_value *);
shared void js_declare_std_lang_functions(struct js_vm *);

#endif