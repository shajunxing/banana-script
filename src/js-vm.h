/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef JS_VM_H
#define JS_VM_H

#include "js-data.h"

// sorted by my programming implementation order
// first implement hardest function call and exception handling
#define js_opcode_list \
    X(op_nop) /* 0 */ \
    X(op_stack_push) /* vary, frame_type, ... */ \
    X(op_stack_pop) /* 1, uint8 */ \
    X(op_variable_declare) /* 1, string */ \
    X(op_variable_delete) /* 1, string */ \
    X(op_variable_put) /* 1, string */ \
    X(op_variable_get) /* 1, string */ \
    X(op_jump) /* 1, uint32 */ \
    X(op_argument_append) /* 0 */ \
    X(op_call) /* 0. callee is function/c_function value on stack top */ \
    X(op_return) /* 0 */ \
    /* parameter's default value may be expression, so cannot be put into operand */ \
    X(op_argument_first) /* 0 */ \
    X(op_argument_get_next) /* 1, string */ \
    X(op_catch) /* 2, string, uint32 */ \
    X(op_throw) /* 0 */ \
    X(op_member_put) /* 0 */ \
    X(op_member_get) /* 0 */ \
    X(op_array_append) /* 0 */ \
    X(op_array_spread) /* 0 */ \
    X(op_object_optional) /* 0 */ \
    X(op_argument_spread) /* 0 */ \
    X(op_argument_get_rest) /* 1, string */ \
    X(op_add) /* 0 */ \
    X(op_sub) /* 0 */ \
    X(op_mul) /* 0 */ \
    X(op_pow) /* 0 */ \
    X(op_div) /* 0 */ \
    X(op_mod) /* 0 */ \
    X(op_eq) /* 0 */ \
    X(op_ne) /* 0 */ \
    X(op_lt) /* 0 */ \
    X(op_le) /* 0 */ \
    X(op_gt) /* 0 */ \
    X(op_ge) /* 0 */ \
    X(op_and) /* 0 TODO: optimize for rhs no-exec */ \
    X(op_or) /* 0 TODO: optimize for rhs no-exec */ \
    X(op_not) /* 0 */ \
    /* X(op_ternary) 0 */ \
    X(op_typeof) /* 0 */ \
    X(op_stack_dupe) /* 1, uint8, number is relative position from top to down */ \
    X(op_jump_if_false) /* 1, uint32 */ \
    X(op_jump_if_true) /* 1, uint32 */ \
    X(op_break) /* 0 */ \
    X(op_continue) /* 0 */ \
    X(op_for_in_next) /* 1, uint32 */ \
    X(op_for_of_next) /* 1, uint32 */ \
    X(op_stack_swap) /* 2, uint8, uint8, number is relative position from top to down */

#define X(name) name,
enum js_opcode { js_opcode_list };
#undef X

#define js_operand_type_list \
    X(opd_undefined) /* 0 byte, internal use, for default exception value */ \
    X(opd_null) /* 0 byte */ \
    X(opd_empty_array) /* 0 byte */ \
    X(opd_empty_object) /* 0 byte */ \
    X(opd_boolean) /* 1 byte */ \
    X(opd_uint8) /* internal use, for some enums */ \
    X(opd_uint16) /* internal use */ \
    X(opd_uint32) /* internal use */ \
    X(opd_double) /* in msvc "long double" is also 64 bit, so not supported */ \
    X(opd_string) /* length(u32), ...bytes */ \
    X(opd_function) /* ingress(u32) */

#define X(name) name,
enum js_operand_type { js_operand_type_list };
#undef X

#pragma pack(push, 1)
struct js_bytecode {
    uint8_t *base;
    uint32_t length;
    uint32_t capacity;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct js_cross_reference { // source line -> instruction offset, may be NULL if direct run from bytecode
    uint32_t *base;
    uint32_t length;
    uint32_t capacity;
};
#pragma pack(pop)

// merge call stack and eval stack together
// sf_loop is to fit all loops' 'break' 'continue' and 'for' loop's 'let' local scope
#define js_stack_frame_type_list \
    X(sf_value) \
    X(sf_function) \
    X(sf_try) \
    X(sf_block) \
    X(sf_loop)

#define X(name) name,
enum js_stack_frame_type { js_stack_frame_type_list };
#undef X

#pragma pack(push, 1)
struct js_stack_frame {
    uint8_t type;
    union {
        struct js_value value;
        struct {
            struct js_variable_map locals;
            uint32_t egress; // function, try, loop
            union {
                uint32_t ingress; // loop
                struct {
                    // for function and c_function
                    // if function, read ingress and closure from *function
                    // if c_function, only use arguments. egress and *function won't be filled
                    // due to arguments support spread syntax, number of them cannot be determined at compile time, so hard to put into stack
                    // TODO: what if number of rest arguments exceeds UINT16MAX?
                    struct js_managed_value *function;
                    struct {
                        struct js_value *base;
                        uint16_t length;
                        uint16_t capacity;
                        uint16_t index; // for parameter's getter operations
                    } arguments;
                };
            };
        };
    };
};
#pragma pack(pop)

// DON'T seperate bytecode and cross_reference outside this structure, because exception handling need these informations
#pragma pack(push, 1)
struct js_vm {
    struct js_bytecode bytecode;
    struct js_cross_reference cross_reference;
    struct js_heap heap;
    struct js_variable_map globals; // global variables, moved from stk_root
    // struct {
    //     struct js_call_stack_frame *base;
    //     uint16_t length;
    //     uint16_t capacity;
    // } call_stack;
    // struct {
    //     struct js_value *base;
    //     uint16_t length;
    //     uint16_t capacity;
    // } eval_stack;
    struct {
        struct js_stack_frame *base;
        uint16_t length;
        uint16_t capacity;
    } stack;
    uint32_t pc; // program counter, next instruction offset
};
#pragma pack(pop)

shared void js_put_instruction(struct js_bytecode *, uint32_t *, uint8_t, uint8_t, ...);
shared void js_add_instruction(struct js_bytecode *, uint8_t, uint8_t, ...);
shared void js_add_cross_reference(struct js_cross_reference *, uint32_t, uint32_t);
shared void js_bytecode_dump(struct js_bytecode *);
shared void js_dump_vm(struct js_vm *);
shared struct js_result js_declare_variable(struct js_vm *, const char *, uint16_t, struct js_value);
static inline struct js_result js_declare_variable_sz(struct js_vm *vm, const char *name, struct js_value value) {
    return js_declare_variable(vm, name, (uint16_t)strlen(name), value);
}
shared struct js_result js_delete_variable(struct js_vm *, const char *, uint16_t);
static inline struct js_result js_delete_variable_sz(struct js_vm *vm, const char *name) {
    return js_delete_variable(vm, name, (uint16_t)strlen(name));
}
shared struct js_result js_put_variable(struct js_vm *, const char *, uint16_t, struct js_value);
static inline struct js_result js_put_variable_sz(struct js_vm *vm, const char *name, struct js_value value) {
    return js_put_variable(vm, name, (uint16_t)strlen(name), value);
}
shared struct js_result js_get_variable(struct js_vm *, const char *, uint16_t);
static inline struct js_result js_get_variable_sz(struct js_vm *vm, const char *name) {
    return js_get_variable(vm, name, (uint16_t)strlen(name));
}
shared struct js_result js_run(struct js_vm *);
shared void js_gc(struct js_vm *);
shared struct js_result js_call(struct js_vm *, struct js_value, uint16_t, struct js_value *);
shared struct js_result js_call_by_name(struct js_vm *, const char *, uint16_t, uint16_t, struct js_value *);
static inline struct js_result js_call_by_name_sz(struct js_vm *vm, const char *name, uint16_t argc, struct js_value *argv) {
    return js_call_by_name(vm, name, (uint16_t)strlen(name), argc, argv);
}
typedef struct js_result (*js_c_function_type)(struct js_vm *, uint16_t, struct js_value *);
shared struct js_value js_c_function(js_c_function_type); // move from js-data to clarify function type
shared void js_free_vm(struct js_vm *);

#ifdef DEBUG

shared void test_vm_structure_size();
shared void test_instruction_get_put();
shared void test_vm_run();

#endif

#endif