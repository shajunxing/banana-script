/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <math.h>
#include "js-vm.h"

#define X(name) #name,
static const char *const _opcode_names[] = {js_opcode_list};
#undef X

#pragma pack(push, 1)
struct _operand {
    uint8_t type;
    union {
        bool value_bool;
        uint8_t value_uint8;
        uint16_t value_uint16;
        int32_t value_int32;
        uint32_t value_uint32;
        double value_double;
        struct {
            uint32_t offset;
            uint32_t length;
        } value_string;
        struct {
            uint32_t ingress;
        } value_function;
    };
};
#pragma pack(pop)

#pragma pack(push, 1)
struct _instruction {
    uint8_t opcode : 6;
    uint8_t num_operands : 2;
    struct _operand operands[3];
};
#pragma pack(pop)

static bool _get_instruction(struct js_bytecode *bytecode, uint32_t *offset /* in,out */, struct _instruction *instruction /* out */) {
#define __safe_forward(__arg_num_bytes, __arg_statement) \
    do { \
        if ((bytecode->length - *offset) < __arg_num_bytes) { \
            log_warning("__safe_forward failed"); \
            return false; \
        } \
        __arg_statement; \
        (*offset) += __arg_num_bytes; \
    } while (0)
#define __current_position (bytecode->base + *offset)
    if ((bytecode->length - *offset) < 1) {
        return false; // end of bytecode, skip warning
    }
    __safe_forward(1, *((uint8_t *)instruction) = *__current_position);
    if (instruction->num_operands > 0) {
        __safe_forward(1, {
            instruction->operands[0].type = *__current_position & 0b00001111;
            instruction->operands[1].type = *__current_position >> 4;
        });
    }
    if (instruction->num_operands > 2) {
        __safe_forward(1, instruction->operands[2].type = *__current_position & 0b00001111);
    }
    for (uint8_t i = 0; i < instruction->num_operands; i++) { // be careful: 0 means 1 operand
        switch (instruction->operands[i].type) {
        case opd_undefined:
        case opd_null:
        case opd_empty_array:
        case opd_empty_object:
            break;
        case opd_boolean:
            __safe_forward(1, instruction->operands[i].value_bool = *__current_position != 0);
            break;
        case opd_uint8:
            __safe_forward(1, instruction->operands[i].value_uint8 = *__current_position);
            break;
        case opd_uint16:
            __safe_forward(2, instruction->operands[i].value_uint16 = *((uint16_t *)__current_position));
            break;
        case opd_uint32:
            __safe_forward(4, instruction->operands[i].value_uint32 = *((uint32_t *)__current_position));
            break;
        case opd_double:
            __safe_forward(8, instruction->operands[i].value_double = *((double *)__current_position));
            break;
        case opd_string:
            __safe_forward(4, instruction->operands[i].value_string.length = *((uint32_t *)__current_position));
            instruction->operands[i].value_string.offset = *offset;
            __safe_forward(instruction->operands[i].value_string.length, );
            break;
        case opd_function:
            __safe_forward(4, instruction->operands[i].value_function.ingress = *((uint32_t *)__current_position));
            break;
        default:
            printf("Unknown type %u\n", instruction->operands[i].type);
            return false;
        }
    }
    return true;
#undef __current_position
#undef __safe_forward
}

// args are: operand types, operands, and with string, are length and base address
static void _put_instruction(struct js_bytecode *bytecode /* in,out */, uint32_t *offset /* in,out */, uint8_t opcode, uint8_t num_operands, va_list args) {
#define __safe_forward(__arg_num_bytes, __arg_statement) \
    do { \
        buffer_alloc(bytecode->base, bytecode->length, bytecode->capacity, *offset + __arg_num_bytes); \
        __arg_statement; \
        (*offset) += __arg_num_bytes; \
        if (bytecode->length < *offset) { \
            bytecode->length = *offset; \
        } \
    } while (0)
#define __current_position (bytecode->base + *offset)
    enforce(opcode < 64);
    enforce(num_operands < 4);
    __safe_forward(1, *__current_position = opcode + (num_operands << 6));
    uint8_t types[3] = {0};
    for (uint8_t i = 0; i < num_operands; i++) {
        types[i] = (uint8_t)va_arg(args, int);
    }
    if (num_operands > 0) {
        __safe_forward(1, *__current_position = types[0] + (types[1] << 4));
    }
    if (num_operands > 2) {
        __safe_forward(1, *__current_position = types[2]);
    }
    for (uint8_t i = 0; i < num_operands; i++) {
        switch (types[i]) {
        case opd_undefined:
        case opd_null:
        case opd_empty_array:
        case opd_empty_object:
            break;
        case opd_boolean:
            __safe_forward(1, *__current_position = (bool)va_arg(args, int) ? 1 : 0); // is promoted to 'int'
            break;
        case opd_uint8:
            __safe_forward(1, *((uint8_t *)__current_position) = (uint8_t)va_arg(args, int)); // is promoted to 'int'
            break;
        case opd_uint16:
            __safe_forward(2, *((uint16_t *)__current_position) = (uint16_t)va_arg(args, int)); // is promoted to 'int'
            break;
        case opd_uint32:
            __safe_forward(4, *((uint32_t *)__current_position) = va_arg(args, uint32_t));
            break;
        case opd_double:
            __safe_forward(8, *((double *)__current_position) = va_arg(args, double));
            break;
        case opd_string:
            uint32_t slen = va_arg(args, uint32_t);
            __safe_forward(4, *((uint32_t *)__current_position) = slen);
            const char *s = va_arg(args, const char *);
            __safe_forward(slen, memcpy(__current_position, s, slen));
            break;
        case opd_function:
            __safe_forward(4, *((uint32_t *)__current_position) = (uint32_t)va_arg(args, int));
            break;
        default:
            fatal("Unknown type %u", types[i]);
        }
    }
#undef __current_position
#undef __safe_forward
}

void js_put_instruction(struct js_bytecode *bytecode /* in,out */, uint32_t *offset /* in,out */, uint8_t opcode, uint8_t num_operands, ...) {
    va_list args;
    va_start(args, num_operands);
    _put_instruction(bytecode, offset, opcode, num_operands, args);
    va_end(args);
}

void js_add_instruction(struct js_bytecode *bytecode /* in,out */, uint8_t opcode, uint8_t num_operands, ...) {
    uint32_t offset = bytecode->length;
    va_list args;
    va_start(args, num_operands);
    _put_instruction(bytecode, &offset, opcode, num_operands, args);
    va_end(args);
}

void js_add_cross_reference(struct js_cross_reference *cross_reference, uint32_t source_line, uint32_t bytecode_offset) {
    buffer_put(cross_reference->base, cross_reference->length, cross_reference->capacity, source_line, bytecode_offset);
}

// #define X(name) #name,
// static const char *const _call_stack_frame_type_names[] = {js_call_stack_frame_type_list};
// #undef X

#define X(name) #name,
static const char *const _stack_frame_type_names[] = {js_stack_frame_type_list};
#undef X

static void _instruction_dump(uint8_t *base, struct _instruction *instruction) {
    int n = 0;
    printf("%-24s    ", _opcode_names[instruction->opcode]);
    for (uint8_t i = 0; i < instruction->num_operands; i++) {
        if (i > 0) {
            n += printf("  ");
        }
        switch (instruction->operands[i].type) {
        case opd_undefined:
            n += printf("undefined");
            break;
        case opd_null:
            n += printf("null");
            break;
        case opd_empty_array:
            n += printf("[]");
            break;
        case opd_empty_object:
            n += printf("{}");
            break;
        case opd_boolean:
            n += printf("%s", instruction->operands[i].value_bool ? "true" : "false");
            break;
        case opd_uint8:
            n += printf("<u8 %u>", instruction->operands[i].value_uint8);
            break;
        case opd_uint16:
            n += printf("<u16 %u>", instruction->operands[i].value_uint16);
            break;
        case opd_uint32:
            n += printf("<u32 %u>", instruction->operands[i].value_uint32);
            break;
        case opd_double:
            n += printf("%g", instruction->operands[i].value_double);
            break;
        case opd_string:
            n += printf("''%.*s''", instruction->operands[i].value_string.length, base + instruction->operands[i].value_string.offset);
            break;
        case opd_function:
            n += printf("<function %u>", instruction->operands[i].value_function.ingress);
            break;
        default:
            break;
        }
    }
    printf("%*s", max(24 - n, 2), "");
    // comment for special types
    switch (instruction->opcode) {
    case op_stack_push:
        enforce(instruction->num_operands > 0);
        enforce(instruction->operands[0].type == opd_uint8);
        enforce(instruction->operands[0].value_uint8 < countof(_stack_frame_type_names));
        if (instruction->operands[0].value_uint8 != sf_value) {
            printf("; %s", _stack_frame_type_names[instruction->operands[0].value_uint8]);
        }
        break;
    default:
        break;
    }
}

void js_bytecode_dump(struct js_bytecode *bytecode) {
    struct _instruction instruction;
    for (uint32_t offset = 0, next_offset = 0; _get_instruction(bytecode, &next_offset, &instruction); offset = next_offset) {
        printf("%10u  ", offset);
        _instruction_dump(bytecode->base, &instruction);
        printf("\n");
    };
    printf("\n");
}

void js_dump_vm(struct js_vm *vm) {
    struct print_stream out = {.type = file_stream, .fp = stdout};
    printf("heap base=%p length=%zu capacity=%zu\n", vm->heap.base, vm->heap.length, vm->heap.capacity);
    buffer_for_each(vm->heap.base, vm->heap.length, vm->heap.capacity, i, v, {
        printf("    %zu. ", i);
        js_serialize_managed_value(&out, todump_style, *v, 0);
        printf("\n");
    });
    // printf("globals base=%p length=%u capacity=%u\n", vm->globals.base, vm->globals.length, vm->globals.capacity);
    // js_map_for_each(vm->globals.base, _, vm->globals.capacity, k, kl, v, {
    //     printf("    %.*s = ", (int)kl, k);
    //     js_serialize_value(&out, todump_style, v, 0);
    //     printf("\n");
    // });
    printf("stack base=%p length=%u capacity=%u\n", vm->stack.base, vm->stack.length, vm->stack.capacity);
    for (uint16_t depth = 0; depth < vm->stack.length; depth++) {
        struct js_stack_frame *frame = vm->stack.base + depth;
        printf("    %u: (%u)%s", depth, frame->type, _stack_frame_type_names[frame->type]);
        switch (frame->type) {
        case sf_value:
            printf(": ");
            js_serialize_value(&out, todump_style, &(frame->value), 0);
            printf("\n");
            break;
        case sf_block:
        case sf_loop:
        case sf_try:
        case sf_function:
            printf("\n");
            printf("        locals: base=%p length=%u capacity=%u\n", frame->locals.base, frame->locals.length, frame->locals.capacity);
            js_map_for_each(frame->locals.base, _, frame->locals.capacity, k, kl, v, {
                printf("            %.*s = ", (int)kl, k);
                js_serialize_value(&out, todump_style, v, 0);
                printf("\n");
            });
            printf("        egress: %u\n", frame->egress);
            if (frame->type == sf_function) {
                printf("        function: %p ", frame->function);
                if (frame->function != NULL) {
                    js_serialize_managed_value(&out, todump_style, frame->function, 0);
                }
                printf("\n");
                printf("        arguments: base=%p length=%u capacity=%u index=%u\n", frame->arguments.base, frame->arguments.length, frame->arguments.capacity, frame->arguments.index);
                buffer_for_each(frame->arguments.base, frame->arguments.length, frame->arguments.capacity, i, v, {
                    printf("            %u. ", i);
                    js_serialize_value(&out, todump_style, v, 0);
                    printf("\n");
                });
            } else {
                printf("        ingress: %u\n", frame->ingress);
            }
            break;
        default:
            printf("    Unknown frame type %u\n", frame->type);
            break;
        }
    }
    printf("pc=%u\n", vm->pc);
    printf("\n");
}

// reverse order, from top to down
#define _stack_for_each(__arg_vm, __arg_frame, __arg_statement) \
    do { \
        /* DON'T use "for (uint16_t i = vm->stack.length - 1; i >= 0; i--)", \
        because turn unsigned i into negative will result a huge positive value */ \
        for (uint16_t i = 0; i < __arg_vm->stack.length; i++) { \
            struct js_stack_frame *__arg_frame = __arg_vm->stack.base + __arg_vm->stack.length - 1 - i; \
            __arg_statement; \
        } \
    } while (0)

#define _call_stack_for_each(__arg_vm, __arg_frame, __arg_statement) \
    _stack_for_each(__arg_vm, __arg_frame, { \
        if (__arg_frame->type != sf_value) { \
            __arg_statement; \
        } \
    })

static struct js_variable_map *_get_current_scope(struct js_vm *vm) {
    _call_stack_for_each(vm, frame, return &(frame->locals));
    return &(vm->globals);
}

struct js_result js_declare_variable(struct js_vm *vm, const char *name, uint16_t name_length, struct js_value value) {
    struct js_variable_map *scope = _get_current_scope(vm);
    if (js_map_get(scope->base, scope->length, scope->capacity, name, name_length).type != 0) {
        js_throw(js_string_f(&(vm->heap),
            "Variable \"%.*s\" already exists", (int)name_length, name));
    }
    js_map_put(scope->base, scope->length, scope->capacity, name, name_length, value);
    js_return(js_null());
}

struct js_result js_delete_variable(struct js_vm *vm, const char *name, uint16_t name_length) {
    struct js_variable_map *scope = _get_current_scope(vm);
    if (js_map_get(scope->base, scope->length, scope->capacity, name, name_length).type == 0) {
        js_throw(js_string_f(&(vm->heap), "Variable \"%.*s\" not found", (int)name_length, name));
    }
    js_map_put(scope->base, scope->length, scope->capacity, name, name_length, (struct js_value){0});
    js_return(js_null());
}

struct js_result js_put_variable(struct js_vm *vm, const char *name, uint16_t name_length, struct js_value value) {
    // first, check current stack variables
    // second, check current stack closure if is function
    // there may be multiple nested functions, so each stack should check closure
    _call_stack_for_each(vm, frame, {
        if (js_map_get(frame->locals.base, frame->locals.length, frame->locals.capacity, name, name_length).type != 0) {
            js_map_put(frame->locals.base, frame->locals.length, frame->locals.capacity, name, name_length, value);
            js_return(js_null());
        }
        if (frame->type == sf_function && frame->function != NULL) {
            if (js_map_get(frame->function->function.closure.base, frame->function->function.closure.length, frame->function->function.closure.capacity, name, name_length).type != 0) {
                js_map_put(frame->function->function.closure.base, frame->function->function.closure.length, frame->function->function.closure.capacity, name, name_length, value);
                js_return(js_null());
            }
        }
    });
    // at last, check globals
    if (js_map_get(vm->globals.base, vm->globals.length, vm->globals.capacity, name, name_length).type != 0) {
        js_map_put(vm->globals.base, vm->globals.length, vm->globals.capacity, name, name_length, value);
        js_return(js_null());
    }
    js_throw(js_string_f(&(vm->heap), "Variable \"%.*s\" not found", (int)name_length, name));
}

struct js_result js_get_variable(struct js_vm *vm, const char *name, uint16_t name_length) {
    // first, check current stack variables
    // second, check current stack closure if is function
    // there may be multiple nested functions, so each stack should check closure
    struct js_value ret;
    _call_stack_for_each(vm, frame, {
        ret = js_map_get(frame->locals.base, frame->locals.length, frame->locals.capacity, name, name_length);
        if (ret.type != 0) {
            js_return(ret);
        }
        if (frame->type == sf_function && frame->function != NULL) {
            ret = js_map_get(frame->function->function.closure.base, frame->function->function.closure.length, frame->function->function.closure.capacity, name, name_length);
            if (ret.type != 0) {
                js_return(ret);
            }
        }
    });
    // at last, check globals
    ret = js_map_get(vm->globals.base, vm->globals.length, vm->globals.capacity, name, name_length);
    if (ret.type != 0) {
        js_return(ret);
    }
    js_throw(js_string_f(&(vm->heap), "Variable \"%.*s\" not found", (int)name_length, name));
}

static void _stack_push(struct js_vm *vm, struct js_stack_frame frame) {
    // compound literal which contains comma can not be used inside macro
    // https://stackoverflow.com/questions/5558159/compound-literals-and-function-like-macros-bug-in-gcc-or-the-c-standard
    // finally it can, just surround with extra parentheses
    // such as: ((struct foo){.a = 1, .b = 2, .c = 3})
    buffer_push(vm->stack.base, vm->stack.length, vm->stack.capacity, frame);
}

static struct js_stack_frame *_stack_peek(struct js_vm *vm, uint16_t depth) { // depth from 0 (top) to length-1 (bottom)
    // js_dump_vm(vm);
    // js_bytecode_dump(&(vm->bytecode));
    enforce(vm->stack.length > depth);
    return vm->stack.base + vm->stack.length - 1 - depth;
}

static void _stack_swap(struct js_vm *vm, uint16_t depth_1, uint16_t depth_2) {
    enforce(vm->stack.length > depth_1);
    enforce(vm->stack.length > depth_2);
    struct js_stack_frame swap = vm->stack.base[vm->stack.length - 1 - depth_1];
    vm->stack.base[vm->stack.length - 1 - depth_1] = vm->stack.base[vm->stack.length - 1 - depth_2];
    vm->stack.base[vm->stack.length - 1 - depth_2] = swap;
}

static void _stack_frame_free(struct js_stack_frame *frame) {
    if (frame->type != sf_value) {
        js_map_free(frame->locals.base, frame->locals.length, frame->locals.capacity);
        if (frame->type == sf_function) {
            buffer_free(frame->arguments.base, frame->arguments.length, frame->arguments.capacity);
        }
    }
}

static void _stack_pop(struct js_vm *vm, uint16_t depth) {
    for (uint16_t i = 0; i < depth; i++) {
        _stack_frame_free(_stack_peek(vm, i));
    }
    vm->stack.length -= depth;
}

static struct js_value _stack_peek_value(struct js_vm *vm, uint16_t depth) { // depth from 0 (top) to length-1 (bottom)
    struct js_stack_frame *frame = _stack_peek(vm, depth);
    enforce(frame->type == sf_value);
    return frame->value;
}

static struct js_value _stack_pop_value(struct js_vm *vm) {
    struct js_value value = _stack_peek_value(vm, 0);
    _stack_pop(vm, 1); // clear this value before following stack operations
    return value;
}

static void _stack_pop_to(struct js_vm *vm, enum js_stack_frame_type type) {
    uint16_t depth = 0;
    _stack_for_each(vm, frame, {
        if (frame->type == type) {
            break;
        }
        depth++;
    });
    _stack_pop(vm, depth);
}

static void _stack_push_value(struct js_vm *vm, struct js_value value) {
    _stack_push(vm, (struct js_stack_frame){.type = sf_value, .value = value});
}

static const char *const _typeof_table[] = {"undefined", "null", "boolean", "number", "string", "string", "string", "array", "object", "function", "function", "c_data"};

static struct js_value *_get_arguments_base(struct js_vm *vm) {
    struct js_stack_frame *frame = _stack_peek(vm, 0);
    enforce(frame->type == sf_function);
    return frame->arguments.base;
}

static uint16_t _get_arguments_length(struct js_vm *vm) {
    struct js_stack_frame *frame = _stack_peek(vm, 0);
    enforce(frame->type == sf_function);
    return frame->arguments.length;
}

static struct js_value _get_argument(struct js_vm *vm, uint16_t index) {
    struct js_stack_frame *frame = _stack_peek(vm, 0);
    enforce(frame->type == sf_function);
    if (index < frame->arguments.length) {
        return frame->arguments.base[index];
    } else {
        return js_null();
    }
}

static struct js_value js_error(struct js_vm *vm, uint32_t curr_offset, struct js_value message) {
    // see https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Global_Objects/Error
    /*
        A special case:

        //...
        //...
        //...
        throw greetings;

        throws variable not found error at curr_offset=0, but there are many lines corresponding to offset 0, because these 0 are array holes during buffer_put, so line will wrongly break at first 0;
    */
    if (vm->cross_reference.base) {
        struct js_value error = js_object(&(vm->heap));
        js_put_object_value_sz(&error, "message", message);
        // for (uint32_t line = 0; line < vm->cross_reference.length; line++) {
        //     log_debug("line=%lu, offset=%lu, curr_offset=%lu", line, vm->cross_reference.base[line], curr_offset);
        // }
        for (uint32_t line = 0; line < vm->cross_reference.length; line++) {
            uint32_t offset = vm->cross_reference.base[line];
            if (curr_offset == 0 && offset == 0) {
                continue;
            }
            if (curr_offset <= offset) {
                // log_debug("line=%lu, break", line);
                js_put_object_value_sz(&error, "line", js_number(line + 1));
                break;
            }
        }
        return error;
    } else {
        return message;
    }
}

struct js_result js_run(struct js_vm *vm) {
    struct _instruction instruction;
    struct js_stack_frame *frame;
    struct js_value container, selector, value;
    struct js_result result;
    size_t index;
    bool yes;
    uint32_t curr_offset;
#define __debug() \
    do { \
        _instruction_dump(vm->bytecode.base, &instruction); \
        printf("\n"); \
        js_dump_vm(vm); \
    } while (0)
#define __operand_offset(__arg_i) (char *)(vm->bytecode.base + instruction.operands[__arg_i].value_string.offset)
#define __operand_length(__arg_i) (instruction.operands[__arg_i].value_string.length)
#define __throw(__arg_message) \
    do { \
        /* side effect: if __arg_message is _stack_pop_value, it will disappear after _stack_pop_to */ \
        typeof(__arg_message) __error = js_error(vm, curr_offset, __arg_message); \
        /* if is in c_function, must exit loop, for example, a 'try' 'c function' function' chain, \
        function 'throw's, stack will be emptied to 'try' and vm_run will return to c function, \
        and stack is corrupted now */ \
        for (; vm->stack.length > 0; _stack_pop(vm, 1)) { \
            frame = _stack_peek(vm, 0); \
            if (frame->type == sf_try) { \
                vm->pc = frame->egress; \
                _stack_pop(vm, 1); \
                _stack_push_value(vm, __error); \
                goto end_of_while_loop; /* DON'T use 'break' because it may be in another loop */ \
                /* function != NULL but egress == 0 means called by c function, see js_call() */ \
            } else if (frame->type == sf_function && frame->egress == 0) { \
                js_throw(__error); \
            } \
        } \
        if (vm->stack.length == 0) { \
            js_throw(__error); \
        } \
    } while (0)
#define __do_try(__arg_expr) /* if using __try, clang_format will add new line after it */ \
    do { \
        result = (__arg_expr); \
        if (!result.success) { \
            __throw(result.value); \
        } \
    } while (0);
#define __lhs container
#define __rhs selector
    curr_offset = vm->pc;
    while (_get_instruction(&(vm->bytecode), &(vm->pc), &instruction)) {
        switch (instruction.opcode) {
        case op_nop:
            break;
        case op_stack_push:
            // __debug();
            enforce(instruction.num_operands > 0);
            enforce(instruction.operands[0].type == opd_uint8);
            switch (instruction.operands[0].value_uint8) {
            case sf_value:
                enforce(instruction.num_operands == 2);
                switch (instruction.operands[1].type) {
                case opd_undefined:
                    _stack_push(vm, (struct js_stack_frame){0});
                    break;
                case opd_null:
                    _stack_push_value(vm, js_null());
                    break;
                case opd_empty_array:
                    _stack_push_value(vm, js_array(&(vm->heap)));
                    break;
                case opd_empty_object:
                    _stack_push_value(vm, js_object(&(vm->heap)));
                    break;
                case opd_boolean:
                    _stack_push_value(vm, js_boolean(instruction.operands[1].value_bool));
                    break;
                case opd_double:
                    _stack_push_value(vm, js_number(instruction.operands[1].value_double));
                    break;
                case opd_string:
                    _stack_push_value(vm, js_string(&(vm->heap), __operand_offset(1), __operand_length(1)));
                    break;
                case opd_function:
                    // closure must be added just before return, NOT here, because closure = local variables before return
                    // NONONO, closure must be added just after function definition, not before return, for example, returning function is declared outside this function, shoun't carry this function's local variable as closure.
                    value = js_function(&(vm->heap), instruction.operands[1].value_function.ingress);
                    yes = false; // if is created inside another function (if not, closure is not necessary)
                    // if c function is in call stack chain, definitely there are atleast 1 script function stack after c stack
                    _call_stack_for_each(vm, frame, {
                        if (frame->type == sf_function) {
                            yes = true;
                            break;
                        }
                    });
                    if (yes) {
#define __put_to_closure() \
    do { \
        /* add only when not equals to value (DON'T add self) and closure not have it */ \
        if ((v->type != vt_function || v->managed != value.managed) && \
            js_map_get(value.managed->function.closure.base, \
                value.managed->function.closure.length, \
                value.managed->function.closure.capacity, k, kl) \
                    .type == 0) { \
            js_map_put(value.managed->function.closure.base, \
                value.managed->function.closure.length, \
                value.managed->function.closure.capacity, k, kl, *v); \
        } \
    } while (0)
                        // reverse traverse all call stack until first function stack
                        _call_stack_for_each(vm, frame, {
                            js_map_for_each(frame->locals.base, _, frame->locals.capacity, k, kl, v, {
                                __put_to_closure();
                            });
                            if (frame->type == sf_function && frame->function != NULL) {
                                // also put frame's recorded closure into (return closure function from another closure function)
                                js_map_for_each(frame->function->function.closure.base, _, frame->function->function.closure.capacity, k, kl, v, {
                                    __put_to_closure();
                                });
                                // and stop traversel
                                break;
                            }
                        });
#undef __put_to_closure
                    }
                    _stack_push_value(vm, value);
                    break;
                default:
                    fatal("Invalid value type %u", instruction.operands[1].type);
                }
                break;
            case sf_function:
            case sf_try:
                enforce(instruction.num_operands = 2);
                enforce(instruction.operands[1].type == opd_uint32);
                _stack_push(vm, (struct js_stack_frame){.type = instruction.operands[0].value_uint8, .egress = instruction.operands[1].value_uint32});
                break;
            case sf_block:
                enforce(instruction.num_operands = 1);
                _stack_push(vm, (struct js_stack_frame){.type = instruction.operands[0].value_uint8});
                break;
            case sf_loop:
                enforce(instruction.num_operands = 3);
                enforce(instruction.operands[1].type == opd_uint32);
                enforce(instruction.operands[2].type == opd_uint32);
                _stack_push(vm, (struct js_stack_frame){.type = instruction.operands[0].value_uint8, .ingress = instruction.operands[1].value_uint32, .egress = instruction.operands[2].value_uint32});
                break;
            default:
                fatal("Invalid stack type %u", instruction.operands[0].value_uint8);
                break;
            }
            // __debug();
            break;
        case op_stack_pop:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_uint8);
            _stack_pop(vm, instruction.operands[0].value_uint8);
            break;
        case op_variable_declare:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_string);
            __do_try(js_declare_variable(vm, __operand_offset(0), __operand_length(0), _stack_pop_value(vm)));
            break;
        case op_variable_delete:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_string);
            __do_try(js_delete_variable(vm, __operand_offset(0), __operand_length(0)));
            break;
        case op_variable_put:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_string);
            __do_try(js_put_variable(vm, __operand_offset(0), __operand_length(0), _stack_pop_value(vm)));
            break;
        case op_variable_get:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_string);
            __do_try(js_get_variable(vm, __operand_offset(0), __operand_length(0)));
            _stack_push_value(vm, result.value);
            break;
        case op_jump:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_uint32);
            vm->pc = instruction.operands[0].value_uint32;
            break;
        case op_argument_append:
            value = _stack_pop_value(vm);
            frame = _stack_peek(vm, 0);
            enforce(frame->type == sf_function);
            buffer_push(frame->arguments.base, frame->arguments.length, frame->arguments.capacity, value);
            break;
        case op_call:
            frame = _stack_peek(vm, 1);
            enforce(frame->type == sf_value);
            value = frame->value;
            frame = _stack_peek(vm, 0);
            enforce(frame->type == sf_function);
            switch (value.type) {
            case vt_function:
                frame->function = value.managed; // complete sf_function
                vm->pc = value.managed->function.ingress;
                // __debug();
                break;
            case vt_c_function:
                // result = ((js_c_function_type)value.c_function)(vm, _get_arguments_length(vm), _get_arguments_base(vm));
                // if (result.success) {
                //     _stack_pop(vm, 2);
                //     _stack_push_value(vm, result.value);
                // } else {
                //     // exception handling
                //     __throw(result.value);
                // }
                __do_try(((js_c_function_type)value.c_function)(vm, _get_arguments_length(vm), _get_arguments_base(vm)));
                _stack_pop(vm, 2);
                _stack_push_value(vm, result.value);
                // __debug();
                break;
            default:
                fatal("Value type %u is not function", value.type);
                break;
            }
            break;
        case op_return:
            if (_stack_peek(vm, 0)->type == sf_value) {
                value = _stack_pop_value(vm); // return value
            } else {
                value = js_null(); // if there are no return value, return NULL
            }
            // DON'T add closure here, for example, returning function is defined outside this function
            // 'return' may be in deep call stack level, all cleanup
            _stack_pop_to(vm, sf_function);
            if (vm->stack.length == 0) {
                // if there are no stacks, which means 'return' outside function, exit loop
                js_return(value);
            } else if (_stack_peek(vm, 0)->egress == 0) {
                // is called by c function, exit loop
                _stack_pop(vm, 2); // cleanup
                js_return(value);
            } else {
                // else jump to function egress
                vm->pc = _stack_peek(vm, 0)->egress;
                _stack_pop(vm, 2); // cleanup
                _stack_push_value(vm, value); // push return value
            }
            // __debug();
            break;
        case op_argument_first:
            frame = _stack_peek(vm, 0);
            enforce(frame->type == sf_function);
            frame->arguments.index = 0;
            break;
        case op_argument_get_next:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_string);
            // default value
            if (_stack_peek(vm, 0)->type == sf_value) {
                __lhs = _stack_pop_value(vm);
            } else {
                __lhs = js_null();
            }
            __rhs = _get_argument(vm, _stack_peek(vm, 0)->arguments.index++);
            if (__rhs.type != vt_null) {
                __lhs = __rhs;
            }
            __do_try(js_declare_variable(vm, __operand_offset(0), __operand_length(0), __lhs));
            break;
        case op_catch:
            enforce(instruction.num_operands == 2);
            enforce(instruction.operands[0].type == opd_string);
            enforce(instruction.operands[1].type == opd_uint32);
            value = _stack_pop_value(vm);
            if (value.type == 0) { // no exception
                vm->pc = instruction.operands[1].value_uint32;
            } else {
                _stack_push(vm, (struct js_stack_frame){.type = sf_block, .egress = instruction.operands[1].value_uint32});
                __do_try(js_declare_variable(vm, __operand_offset(0), __operand_length(0), value));
            }
            break;
        case op_throw:
            enforce(instruction.num_operands == 0);
            __throw(_stack_pop_value(vm));
            break;
        case op_member_put:
            value = _stack_pop_value(vm);
            selector = _stack_pop_value(vm);
            container = _stack_peek_value(vm, 0);
            if (container.type == vt_array && selector.type == vt_number) {
                index = (size_t)selector.number;
                if (index != selector.number) {
                    __throw(js_scripture_sz("Invalid array index, must be positive integer"));
                }
                js_put_array_element(&container, index, value);
            } else if (container.type == vt_object && js_is_string(&selector)) {
                js_put_object_value(&container, js_get_string_base(&selector), (uint16_t)js_get_string_length(&selector), value);
            } else {
                __throw(js_scripture_sz("Must be array[number] or object[string]"));
            }
            break;
        case op_member_get:
            selector = _stack_pop_value(vm);
            container = _stack_pop_value(vm);
            if (container.type == vt_array && selector.type == vt_number) {
                if (selector.number < 0) {
                    _stack_push_value(vm, js_null());
                } else {
                    index = (size_t)selector.number;
                    if (index != selector.number) {
                        // __throw(js_scripture_sz("Invalid array index, must be positive integer"));
                        _stack_push_value(vm, js_null());
                    } else {
                        _stack_push_value(vm, js_get_array_element(&container, index));
                    }
                }
            } else if (container.type == vt_object && js_is_string(&selector)) {
                _stack_push_value(vm, js_get_object_value(&container, js_get_string_base(&selector), (uint16_t)js_get_string_length(&selector)));
            } else {
                __throw(js_scripture_sz("Must be array[number] or object[string]"));
            }
            break;
        case op_array_append:
            value = _stack_pop_value(vm);
            container = _stack_peek_value(vm, 0);
            if (container.type == vt_array) {
                js_push_array_element(&container, value);
            } else {
                __throw(js_scripture_sz("Must be array"));
            }
            break;
        case op_array_spread:
            value = _stack_pop_value(vm);
            container = _stack_peek_value(vm, 0);
            if (container.type == vt_array && value.type == vt_array) {
                // no skip null
                buffer_for_each(value.managed->array.base, value.managed->array.length, _, i, v, {
                    js_push_array_element(&container, *v);
                });
            } else {
                __throw(js_scripture_sz("Must be array[...array]"));
            }
            break;
        case op_object_optional:
            selector = _stack_pop_value(vm);
            container = _stack_pop_value(vm);
            if (container.type == vt_object && js_is_string(&selector)) {
                _stack_push_value(vm, js_get_object_value(&container, js_get_string_base(&selector), (uint8_t)js_get_string_length(&selector)));
            } else {
                _stack_push_value(vm, js_null());
            }
            break;
        case op_argument_spread:
            value = _stack_pop_value(vm);
            frame = _stack_peek(vm, 0);
            enforce(frame->type == sf_function);
            if (value.type != vt_array) {
                __throw(js_scripture_sz("Parameter to be spreaded must be array"));
            }
            buffer_for_each(value.managed->array.base, value.managed->array.length, _, i, v, {
                // arguments will be used by 3rd-party c functions, so special treat js_undefined here
                buffer_push(frame->arguments.base, frame->arguments.length, frame->arguments.capacity, v->type == 0 ? js_null() : *v);
            });
            break;
        case op_argument_get_rest:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_string);
            value = js_array(&(vm->heap));
            frame = _stack_peek(vm, 0);
            enforce(frame->type == sf_function);
            while (frame->arguments.index < frame->arguments.length) {
                // special treat js_undefined come from argument spread
                // if (frame->arguments.base[frame->arguments.index].type == 0) {
                //     js_push_array_element(&value, js_null());
                //     frame->arguments.index++;
                // } else {
                js_push_array_element(&value, frame->arguments.base[frame->arguments.index++]);
                // }
            };
            __do_try(js_declare_variable(vm, __operand_offset(0), __operand_length(0), value));
            break;
        case op_add:
            __rhs = _stack_pop_value(vm);
            __lhs = _stack_pop_value(vm);
            __do_try(js_add(&(vm->heap), &__lhs, &__rhs));
            _stack_push_value(vm, result.value);
            break;
        case op_sub:
        case op_mul:
        case op_pow:
        case op_div:
        case op_mod:
            __rhs = _stack_pop_value(vm);
            __lhs = _stack_pop_value(vm);
            if (__lhs.type != vt_number || __rhs.type != vt_number) {
                __throw(js_scripture_sz("Arithmatic operand must be number"));
            }
            switch (instruction.opcode) {
            case op_sub:
                _stack_push_value(vm, js_number(__lhs.number - __rhs.number));
                break;
            case op_mul:
                _stack_push_value(vm, js_number(__lhs.number * __rhs.number));
                break;
            case op_pow:
                _stack_push_value(vm, js_number(pow(__lhs.number, __rhs.number)));
                break;
            case op_div:
                _stack_push_value(vm, js_number(__lhs.number / __rhs.number));
                break;
            case op_mod:
                _stack_push_value(vm, js_number(fmod(__lhs.number, __rhs.number)));
                break;
            default:
                break;
            }
            break;
        case op_eq:
        case op_ne:
            __rhs = _stack_pop_value(vm);
            __lhs = _stack_pop_value(vm);
            if (memcmp(&__lhs, &__rhs, sizeof(struct js_value)) == 0) {
                yes = true;
            } else if (__lhs.type == vt_number && __rhs.type == vt_number) {
                yes = __lhs.number == __rhs.number; // DONT memcmp two double, same value may be different memory content, for example, mod result 0 == 0 may be false perhaps -0 +0
            } else if (js_is_string(&__lhs) && js_is_string(&__rhs)) {
                yes = js_compare_string(&__lhs, &__rhs) == 0;
            } else {
                yes = false;
            }
            if (instruction.opcode == op_ne) {
                yes = !yes;
            }
            _stack_push_value(vm, js_boolean(yes));
            break;
        case op_lt:
        case op_le:
        case op_gt:
        case op_ge:
            __rhs = _stack_pop_value(vm);
            __lhs = _stack_pop_value(vm);
            if (__lhs.type == vt_number && __rhs.type == vt_number) {
                switch (instruction.opcode) {
                case op_lt:
                    yes = __lhs.number < __rhs.number;
                    break;
                case op_le:
                    yes = __lhs.number <= __rhs.number;
                    break;
                case op_gt:
                    yes = __lhs.number > __rhs.number;
                    break;
                case op_ge:
                    yes = __lhs.number >= __rhs.number;
                    break;
                default:
                    break;
                }
            } else if (js_is_string(&__lhs) && js_is_string(&__rhs)) {
                switch (instruction.opcode) {
                case op_lt:
                    yes = js_compare_string(&__lhs, &__rhs) < 0;
                    break;
                case op_le:
                    yes = js_compare_string(&__lhs, &__rhs) <= 0;
                    break;
                case op_gt:
                    yes = js_compare_string(&__lhs, &__rhs) > 0;
                    break;
                case op_ge:
                    yes = js_compare_string(&__lhs, &__rhs) >= 0;
                    break;
                default:
                    break;
                }
            } else {
                __throw(js_scripture_sz("Relational operand must be number or string"));
            }
            _stack_push_value(vm, js_boolean(yes));
            break;
        case op_and:
        case op_or:
            __rhs = _stack_pop_value(vm);
            __lhs = _stack_pop_value(vm);
            if (__lhs.type != vt_boolean || __rhs.type != vt_boolean) {
                __throw(js_scripture_sz("Logical operand must be boolean"));
            }
            switch (instruction.opcode) {
            case op_and:
                __lhs.boolean = __lhs.boolean && __rhs.boolean; //  there are no &&= ||= operators
                break;
            case op_or:
                __lhs.boolean = __lhs.boolean || __rhs.boolean;
                break;
            default:
                break;
            }
            _stack_push_value(vm, __lhs);
            break;
        case op_not:
            __rhs = _stack_pop_value(vm);
            if (__rhs.type != vt_boolean) {
                __throw(js_scripture_sz("Logical operand must be boolean"));
            }
            __rhs.boolean = !__rhs.boolean;
            _stack_push_value(vm, __rhs);
            break;
        // case op_ternary:
        //     __rhs = _stack_pop_value(vm);
        //     __lhs = _stack_pop_value(vm);
        //     value = _stack_pop_value(vm);
        //     if (value.type != vt_boolean) {
        //         __throw(js_scripture_sz("Ternary condition must be boolean"));
        //     }
        //     _stack_push_value(vm, value.boolean ? __lhs : __rhs);
        //     break;
        case op_typeof:
            __rhs = _stack_pop_value(vm);
            enforce(__rhs.type < countof(_typeof_table));
            _stack_push_value(vm, js_scripture_sz(_typeof_table[__rhs.type]));
            break;
        case op_stack_dupe: // duplicate value from stack count from top to down
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_uint8);
            _stack_push_value(vm, _stack_peek_value(vm, instruction.operands[0].value_uint8));
            break;
        case op_jump_if_false:
        case op_jump_if_true:
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_uint32);
            value = _stack_pop_value(vm); // DONT place multiple conditional jumps together, because condition is poped
            if (value.type != vt_boolean) {
                __throw(js_scripture_sz("Conditional jump needs boolean"));
            }
            yes = instruction.opcode == op_jump_if_true ? value.boolean : !value.boolean;
            if (yes) {
                vm->pc = instruction.operands[0].value_uint32;
            }
            break;
        case op_break:
            _stack_pop_to(vm, sf_loop);
            enforce(vm->stack.length > 0);
            frame = _stack_peek(vm, 0);
            vm->pc = frame->egress;
            _stack_pop(vm, 1);
            break;
        case op_continue:
            _stack_pop_to(vm, sf_loop);
            enforce(vm->stack.length > 0);
            frame = _stack_peek(vm, 0);
            vm->pc = frame->ingress;
            break;
        case op_for_in_next: // push next value into stack top
        case op_for_of_next: // push next value into stack top
            enforce(instruction.num_operands == 1);
            enforce(instruction.operands[0].type == opd_uint32);
            value = _stack_pop_value(vm);
            index = (size_t)value.number; // loop number
            container = _stack_peek_value(vm, 0); // array/object to be looped
            yes = false; // whether success
            if (container.type == vt_array) {
                for (; index < container.managed->array.length; index++) {
                    value = container.managed->array.base[index];
                    if (value.type != vt_undefined && value.type != vt_null) {
                        if (instruction.opcode == op_for_in_next) {
                            value = js_number((double)index);
                        }
                        yes = true;
                        break;
                    }
                }
            } else if (container.type == vt_object) {
                // js_value_map_dump(value->value.object->p, value->value.object->len, value->value.object->cap);
                for (; index < container.managed->object.capacity; index++) {
                    struct js_kv_pair *kv = container.managed->object.base + index;
                    if (kv->key.base != NULL && kv->value.type != vt_undefined && kv->value.type != vt_null) {
                        // printf("index = %llu\index", index);
                        if (instruction.opcode == op_for_in_next) {
                            value = js_string(&(vm->heap), kv->key.base, kv->key.length);
                        } else {
                            value = kv->value;
                        }
                        yes = true;
                        break;
                    }
                }
            } else {
                __throw(js_scripture_sz("'for in/of' operand must be array or object"));
            }
            _stack_push_value(vm, js_number((double)(index + 1))); // write back loop number
            if (yes) {
                _stack_push_value(vm, value);
            } else {
                vm->pc = instruction.operands[0].value_uint32;
            }
            break;
        case op_stack_swap:
            enforce(instruction.num_operands == 2);
            enforce(instruction.operands[0].type == opd_uint8);
            enforce(instruction.operands[1].type == opd_uint8);
            _stack_swap(vm, instruction.operands[0].value_uint8, instruction.operands[1].value_uint8);
            break;
        default:
            fatal("Unknown opcode %u", instruction.opcode);
            break;
        }
    end_of_while_loop:
        // (void)0;
        curr_offset = vm->pc;
    }
    js_return(js_null());
#undef __rhs
#undef __lhs
#undef __throw
#undef __operand_0_length
#undef __operand_0_offset
#undef __debug
}

void js_gc(struct js_vm *vm) {
#define __mark_map(__arg_map) \
    js_map_for_each((__arg_map).base, (__arg_map).length, (__arg_map).capacity, k, kl, v, { \
        (void)k; \
        (void)kl; \
        js_mark(v); \
    })
#define __mark_list(__arg_map) \
    js_list_for_each((__arg_map).base, (__arg_map).length, (__arg_map).capacity, i, v, { \
        (void)i; \
        js_mark(v); \
    })
    __mark_map(vm->globals);
    _call_stack_for_each(vm, frame, {
        __mark_map(frame->locals);
        if (frame->type == sf_function) {
            // some anonumous functions which are in use by callee
            // c function's arguments must also be marked, for example, in an anonymous callback of c function, invoked gc(), this callback be sweeped, boom!
            __mark_list(frame->arguments);
            if (frame->function != NULL) {
                __mark_map(frame->function->function.closure);
            }
        }
    });
    js_sweep(&(vm->heap));
#undef __mark_list
#undef __mark_map
}

struct js_result js_call(struct js_vm *vm, struct js_value fv, uint16_t argc, struct js_value *argv) {
    if (fv.type == vt_function) {
        // backup stack depth, in callee, may throw error, stack won't be cleaned up, if not cleaned here and return at upper vm's 'op_call', and '__do_try' will check stack and found leftover .egress=0 stack, and exit vm, this shouldn't happen
        uint16_t stack_length_backup = vm->stack.length;
        buffer_push(vm->stack.base, vm->stack.length, vm->stack.capacity,
            ((struct js_stack_frame){.type = sf_value, .value = fv}));
        struct js_stack_frame frame = (struct js_stack_frame){.type = sf_function, .function = fv.managed, .egress = 0}; // 0 indicates called by c function
        // prepare arguments
        for (uint16_t i = 0; i < argc; i++) {
            // js_dump_value(argv + i);
            // printf("\n");
            // special treat for vt_undefined from such as array element passed to sort callback
            struct js_value arg = argv[i];
            if (arg.type == 0) {
                arg = js_null();
            }
            buffer_push(frame.arguments.base, frame.arguments.length, frame.arguments.capacity, arg);
        }
        buffer_push(vm->stack.base, vm->stack.length, vm->stack.capacity, frame);
        // backup program counter, jump to function ingress, wait for function completion
        uint32_t pc_backup = vm->pc;
        vm->pc = fv.managed->function.ingress;
        struct js_result result = js_run(vm);
        vm->pc = pc_backup;
        // restore to backuped stack depth
        if (vm->stack.length > stack_length_backup) {
            _stack_pop(vm, vm->stack.length - stack_length_backup);
        }
        return result;
    } else if (fv.type == vt_c_function) {
        // TODO: still need stack?
        buffer_push(vm->stack.base, vm->stack.length, vm->stack.capacity,
            ((struct js_stack_frame){.type = sf_value, .value = fv}));
        struct js_stack_frame frame = (struct js_stack_frame){.type = sf_function};
        for (uint16_t i = 0; i < argc; i++) {
            // is it necessart to special treat for vt_undefined like above? maybe not, c_function can handle it
            buffer_push(frame.arguments.base, frame.arguments.length, frame.arguments.capacity, argv[i]);
        }
        buffer_push(vm->stack.base, vm->stack.length, vm->stack.capacity, frame);
        struct js_result result = ((js_c_function_type)fv.c_function)(vm, _get_arguments_length(vm), _get_arguments_base(vm));
        _stack_pop(vm, 2);
        return result;
    } else {
        js_throw(js_scripture_sz("Not a function"));
    }
}

struct js_result js_call_by_name(struct js_vm *vm, const char *name, uint16_t name_length, uint16_t argc, struct js_value *argv) {
    struct js_result result = js_get_variable(vm, name, name_length);
    if (!result.success) {
        return result;
    }
    return js_call(vm, result.value, argc, argv);
}

struct js_value js_c_function(js_c_function_type c_function) {
    return (struct js_value){.type = vt_c_function, .c_function = c_function};
}

void js_free_vm(struct js_vm *vm) {
    buffer_free(vm->bytecode.base, vm->bytecode.length, vm->bytecode.capacity);
    buffer_free(vm->cross_reference.base, vm->cross_reference.length, vm->cross_reference.capacity);
    js_sweep(&(vm->heap));
    js_sweep(&(vm->heap));
    js_map_free(vm->globals.base, vm->globals.length, vm->globals.capacity);
    _stack_pop(vm, vm->stack.length);
}

#ifdef DEBUG

void test_vm_structure_size() {
    // log_expression("%zu", sizeof(struct js_call_stack_frame));
    log_expression("%zu", sizeof(struct js_bytecode));
    log_expression("%zu", sizeof(struct js_cross_reference));
    log_expression("%zu", sizeof(struct js_stack_frame));
    log_expression("%zu", sizeof(struct js_vm));
    log_expression("%zu", sizeof(struct _operand));
    log_expression("%zu", sizeof(struct _instruction));
}

void test_instruction_get_put() {
    // {
    //     uint8_t base[] = {
    //         0b11000000, // nop, null, false, true
    //         0b00010000,
    //         0b00000001,
    //         0b00000000,
    //         0b00000001,
    //         0b11000000, // nop, uint8, int32, uint32
    //         0b01000010,
    //         0b00000101,
    //         0b00000000, // uint8
    //         0b00000000, // int32
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         0b00000000, // uint32
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         0b11000000, // nop, double, string, function
    //         0b01110110,
    //         0b00001000,
    //         0b00000000, // double
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         0b10000000,
    //         0b00000101, // string
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //         'h',
    //         'e',
    //         'l',
    //         'l',
    //         'o',
    //         0b00000011, // function
    //         0b00000000,
    //         0b00000000,
    //         0b00000000,
    //     };
    //     struct js_bytecode bytecode = {
    //         .base = base,
    //         .length = countof(base),
    //         .capacity = countof(base),
    //     };
    //     buffer_dump(bytecode.base, bytecode.length, bytecode.capacity);
    //     js_bytecode_dump(&bytecode);
    // }
    {
        struct js_bytecode bytecode = {0};
        uint32_t offset = 0;
        js_put_instruction(&bytecode, &offset, op_nop, 0);
        js_put_instruction(&bytecode, &offset, op_nop, 1, opd_null);
        js_put_instruction(&bytecode, &offset, op_nop, 2, opd_boolean, opd_boolean, false, true);
        js_put_instruction(&bytecode, &offset, op_nop, 3, opd_uint8, opd_uint16, opd_uint32, UINT8_MAX, UINT16_MAX, UINT32_MAX);
        js_put_instruction(&bytecode, &offset, op_nop, 3, opd_double, opd_string, opd_function, -0.123456, 12, "Hello,World!", 666);
        buffer_dump(bytecode.base, bytecode.length, bytecode.capacity);
        js_bytecode_dump(&bytecode);
    }
}

void test_vm_run() {
    struct js_vm vm = {};
    js_add_instruction(&(vm.bytecode), op_nop, 0);
    js_add_instruction(&(vm.bytecode), op_stack_push, 2, opd_uint8, opd_boolean, sf_value, false);
    js_add_instruction(&(vm.bytecode), op_variable_declare, 1, opd_string, 3, "foo");
    js_add_instruction(&(vm.bytecode), op_stack_push, 2, opd_uint8, opd_double, sf_value, 1.23456);
    js_add_instruction(&(vm.bytecode), op_variable_declare, 1, opd_string, 3, "bar");
    js_add_instruction(&(vm.bytecode), op_stack_push, 2, opd_uint8, opd_string, sf_value, 5, "hello");
    js_add_instruction(&(vm.bytecode), op_variable_declare, 1, opd_string, 3, "baz");
    js_bytecode_dump(&(vm.bytecode));
    js_run(&vm);
    js_dump_vm(&vm);
}

#endif