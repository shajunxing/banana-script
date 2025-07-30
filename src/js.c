/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <time.h>
#ifndef _WIN32
    #include <readline/history.h>
    #include <readline/readline.h>
#endif
#include "js-syntax.h"
#include "js-std.h"

static char *_make_filename(char *source_filename, const char *ext, char *binary_filename) {
    if (binary_filename) {
        return dupe_sz(binary_filename);
    } else {
        char *dot = strrchr(source_filename, '.');
        if (dot) {
            *dot = 0;
        }
        char *ret = (char *)calloc(strlen(source_filename) + strlen(ext) + 1, 1);
        strcat(ret, source_filename);
        strcat(ret, ext);
        return ret;
    }
}

static void _write_binary(struct js_vm *vm, char *source_filename, char *bytecode_filename, char *xref_filename) {
    // if bytecode_filename or xref_filename is NULL, default is source filename base + extension
    char *fname;
    fname = _make_filename(source_filename, ".bin", bytecode_filename);
    write_file(fname, "wb", vm->bytecode.base, vm->bytecode.length, vm->bytecode.capacity);
    printf("Bytecode written to: %s\n", fname);
    free(fname);
    fname = _make_filename(source_filename, ".xref", xref_filename);
    write_file(fname, "wb", vm->cross_reference.base, vm->cross_reference.length, vm->cross_reference.capacity);
    printf("Cross reference written to: %s\n", fname);
    free(fname);
}

struct js_source source = {0};
struct js_token token = {0};
struct js_vm vm = {0};

static int _repl() {
    struct js_source line = {0};
    js_declare_std_functions(&vm);
    printf("Banana Script REPL environment. Copyright (C) 2024-2025 ShaJunXing\n");
    printf("Type '/?' for more information.\n\n");
    for (;;) {
#ifdef _WIN32
        printf("> ");
        read_line(stdin, line.base, line.length, line.capacity);
#else
        // disable filename completion
        rl_bind_key('\t', rl_insert);
        char *s = readline("> ");
        string_buffer_append_sz(line.base, line.length, line.capacity, s);
        free(s);
        if (line.length > 0) {
            add_history(line.base);
        }
#endif
        // buffer_dump(line.base, line.length, line.capacity);
        if (line.length > 0) {
            if (line.base[0] == '/') {
#define __line_eq(__arg_0) (strcmp(line.base, (__arg_0)) == 0)
                if (__line_eq("/?")) {
                    printf("Enter script statements or following commands:\n");
                    printf("  /?                       show help\n");
                    printf("  /d                       dump memory\n");
                    printf("  /q                       quit program\n");
                    printf("  /u                       unassemble bytecodes\n");
                    printf("  /w                       write source and binaries to file\n");
                } else if (__line_eq("/d")) {
                    buffer_dump(source.base, source.length, source.capacity);
                    buffer_dump(vm.bytecode.base, vm.bytecode.length, vm.bytecode.capacity);
                    js_dump_vm(&vm);
                } else if (__line_eq("/q")) {
                    printf("Bye.\n\n");
                    exit(EXIT_SUCCESS);
                } else if (__line_eq("/u")) {
                    js_bytecode_dump(&(vm.bytecode));
                } else if (__line_eq("/w")) {
                    time_t now = time(NULL);
                    char source_filename[64] = {0};
                    strftime(source_filename, sizeof(source_filename), "%Y-%m-%d-%H-%M-%S.js", localtime(&now));
                    write_file(source_filename, "w", source.base, source.length, source.capacity);
                    printf("Source written to: %s\n", source_filename);
                    _write_binary(&vm, source_filename, NULL, NULL);
                } else {
                    printf("Unknown command \"%s\"\n", line.base);
                }
#undef __line_eq
            } else {
                uint32_t src_len_bak = source.length;
                struct js_token tok_bak = token;
                uint32_t bc_len_bak = vm.bytecode.length;
                uint32_t xref_len_bak = vm.cross_reference.length;
                uint32_t pc_bak = vm.pc;
                // concat line to source to make sure next_token works correctly
                string_buffer_append(source.base, source.length, source.capacity, line.base, line.length);
                // set from eof state to searching state
                // token.state = ts_searching;
                // if runtime error happens, for example, if happens inside a function with no return value need to process, for example, print(...), after op_call there is an op_stack_pop, next statement will continue run at following op_stack_pop will failed because stack has been cleared. there are 2 solution: 1. move vm->pc to the end, 2. rollback
                bool rollback = false;
                if (js_compile(&source, &token, &(vm.bytecode), &(vm.cross_reference))) {
                    // log_debug("OK, %u,%u-%u,%u", token.head_line, token.head_offset, token.tail_line, token.tail_offset);
                    struct js_result result = js_run(&vm);
                    if (!result.success) {
                        printf("Runtime Error: ");
                        js_dump_value(&(result.value));
                        printf("\n");
                        rollback = true;
                    }
                } else {
                    rollback = true;
                }
                if (rollback) {
                    // log_debug("Failed");
                    source.length = src_len_bak;
                    token = tok_bak;
                    vm.bytecode.length = bc_len_bak;
                    vm.cross_reference.length = xref_len_bak;
                    vm.pc = pc_bak;
                }
            }
        }
        buffer_free(line.base, line.length, line.capacity);
    }
    return EXIT_SUCCESS;
}

static int _help(char *arg_0) {
    printf("Usage: %s [filenames] [options]\n", arg_0);
    printf("\n");
    printf("If both options and filenames are not specified, enter repl environment.\n");
    printf("If only filenames specified, load them sequentially as source code, and run.\n");
    printf("\n");
    printf("Action options:\n");
    printf("  -c, --compile            compile only\n");
    printf("  -h, --help               show help\n");
    printf("  -o, --optimize           optimize for code generation\n");
    printf("  -r, --run                run source or bytecode\n");
#ifdef DEBUG
    printf("  -t, --test               run test suit\n");
#endif
    printf("  -u, --unassemble         show disassembly only\n");
    printf("\n");
    printf("File options:\n");
    printf("  -b, --binary <filename>  bytecode binary filename\n");
    printf("  -s, --source <filenames> one or more source filenames\n");
    printf("  -x, --xref <filename>    cross reference filename\n");
    printf("\n");
    printf("Relationship between actions and files:\n");
    printf("  action             file\n");
    printf("  ---------------------------------------------------------\n");
    printf("  compile            source    in   required\n");
    printf("                     bytecode  out  required\n");
    printf("                     xref      out  optional\n");
    printf("  run, unassemble    source    in   required first priority\n");
    printf("                     bytecode  in   required if no source\n");
    printf("                     xref      in   optional if no source\n");
    return EXIT_SUCCESS;
}

#ifdef DEBUG

    #define test_function_list \
        X(test_random_sz) \
        X(test_random_double) \
        X(test_buffer) \
        X(test_string_buffer) \
        X(test_string_buffer_loop) \
        /* X(test_string_buffer_append_f) */ \
        X(test_memmove) \
        X(test_string_dupe) \
        /* X(test_read_file) */ \
        X(test_read_line) \
        X(test_natural_compare) \
        X(test_print_stream) \
        X(test_regex) \
        X(test_data_structure_size) \
        X(test_js_map) \
        X(test_js_map_loop) \
        X(test_js_value) \
        X(test_js_value_loop) \
        X(test_js_value_bug) \
        X(test_js_string_family) \
        X(test_js_string_f) \
        X(test_vm_structure_size) \
        X(test_instruction_get_put) \
        X(test_vm_run) \
        X(test_lexer) \
        X(test_parser) \
        X(test_c_function) \
        X(test_unescape_string) \
        X(test_free_vm) \
        X(test_read_source_file)

    #define X(name) #name,
static const char *test_function_names[] = {test_function_list};
    #undef X

    #define X(name) name,
static const void (*test_functions[])() = {test_function_list};
    #undef X

static int _test(char *arg_0, int argc, char *argv[]) {
    int selected_id = argc > 0 ? atoi(argv[0]) : -1;
    int id;
    for (id = 0; id < countof(test_functions); id++) {
        if (selected_id == id) {
            printf("----> %-34s", test_function_names[id]);
        } else {
            printf("%4d. %-34s", id, test_function_names[id]);
        }
        if (id % 2 == 1) {
            printf("\n");
        }
    }
    if (id % 2 == 1) {
        printf("\n");
    }
    if (selected_id >= 0 && selected_id < countof(test_functions)) {
        srand((unsigned)time(NULL));
        printf("\n");
        test_functions[selected_id]();
        return EXIT_SUCCESS;
    } else {
        printf("Usage: %s -t,--test <id>, id must be one of the above\n", arg_0);
        return EXIT_FAILURE;
    }
}

#endif

// #ifdef _WIN32
//     #include <windows.h>
// #endif

int main(int argc, char *argv[]) {
    // #ifdef _WIN32
    //     SetConsoleOutputCP(65001);
    // #endif
    js_std_argc = argc;
    js_std_argv = argv;
    if (argc == 1) { // no arguments, enter repl
        return _repl();
    }
    enum { t_bytecode,
        t_source,
        t_xref
    } file_type = t_source;
    struct {
        char **base;
        size_t length;
        size_t capacity;
    } source_filenames = {0};
    char *bytecode_filename = NULL;
    char *xref_filename = NULL;
    enum { a_compile,
        a_run,
        a_unassemble
    } action = a_run;
    bool optimize = false;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
#define __arg_eq(__arg_0) (strcmp(arg, (__arg_0)) == 0)
        if (arg[0] == '-') {
            if (__arg_eq("-b") || __arg_eq("--bytecode")) {
                file_type = t_bytecode;
            } else if (__arg_eq("-s") || __arg_eq("--source")) {
                file_type = t_source;
            } else if (__arg_eq("-x") || __arg_eq("--xref")) {
                file_type = t_xref;
            } else if (__arg_eq("-c") || __arg_eq("--compile")) {
                action = a_compile;
            } else if (__arg_eq("-h") || __arg_eq("--help")) {
                return _help(argv[0]);
            } else if (__arg_eq("-o") || __arg_eq("--optimize")) {
                optimize = true;
            } else if (__arg_eq("-r") || __arg_eq("--run")) {
                action = a_run;
#ifdef DEBUG
            } else if (__arg_eq("-t") || __arg_eq("--test")) {
                return _test(argv[0], argc - i - 1, argv + i + 1);
#endif
            } else if (__arg_eq("-u") || __arg_eq("--unassemble")) {
                action = a_unassemble;
            } else {
                // rest arguments are handled by script, shouldn't fatal
                // fatal("Unknown option: %s", arg);
            }
        } else {
            // handle file names
            if (file_type == t_bytecode) {
                bytecode_filename = arg;
            } else if (file_type == t_source) {
                buffer_push(source_filenames.base, source_filenames.length, source_filenames.capacity, arg);
            } else {
                xref_filename = arg;
            }
        }
#undef __arg_eq
    }
    // do actions
    js_declare_std_functions(&vm);
    if (source_filenames.base != NULL) {
        for (size_t i = 0; i < source_filenames.length; i++) {
            if (!js_read_source_file(&source, source_filenames.base[i])) {
                return EXIT_FAILURE;
            }
        }
        // TODO: add optimization
        (void)optimize;
        if (!js_compile(&source, &token, &(vm.bytecode), &(vm.cross_reference))) {
            return EXIT_FAILURE;
        }
    }
    if (action == a_compile) {
        if (source_filenames.base == NULL) {
            fatal("Require source files");
        }
        _write_binary(&vm, source_filenames.base[source_filenames.length - 1], bytecode_filename, xref_filename);
    } else {
        // source.base may be NULL if source file size == 0, so use source_filenames.base to check
        if (source_filenames.base == NULL) {
            if (bytecode_filename == NULL) {
                fatal("If no source files specified, bytecode file is required");
            } else {
                read_binary_file(bytecode_filename, vm.bytecode.base, vm.bytecode.length, vm.bytecode.capacity);
            }
            if (xref_filename != NULL) {
                read_binary_file(xref_filename, vm.cross_reference.base, vm.cross_reference.length, vm.cross_reference.capacity);
            }
        }
        if (action == a_run) {
            struct js_result result = js_run(&vm);
            if (result.success) {
                switch (result.value.type) {
                case vt_number:
                    return (int)result.value.number;
                    break;
                case vt_boolean:
                    return result.value.boolean ? EXIT_SUCCESS : EXIT_FAILURE;
                    break;
                default:
                    return EXIT_SUCCESS;
                    break;
                }
            } else {
                printf("Runtime Error: ");
                js_dump_value(&(result.value));
                printf("\n");
                return EXIT_FAILURE;
            }
        } else if (action == a_unassemble) {
            js_bytecode_dump(&(vm.bytecode));
        }
    }
    return EXIT_SUCCESS;
}
