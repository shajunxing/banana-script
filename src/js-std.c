/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <time.h>
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <direct.h> // getcwd, chdir, mkdir, rmdir
    #include <string.h> // strrchr
    #include <windows.h>
    #ifndef getcwd
        #define getcwd _getcwd
    #endif
    #ifndef chdir
        #define chdir _chdir
    #endif
    #ifndef rmdir
        #define rmdir _rmdir
    #endif
static char *dirname(char *path) {
    // since windows version of _splitpath_s _makepath_s cannot handle long path, and have some bad behaviors such as path "" will crash, i implement following https://www.man7.org/linux/man-pages/man3/dirname.3p.html behaviors
    // log_debug("'%s'", path);
    char *lastsep = strrchr(path, '\\');
    if (lastsep) {
        *lastsep = '\0';
        return path;
    } else {
        return ".";
    }
}
#else
    #include <dirent.h>
    #include <libgen.h>
    #include <readline/readline.h>
    #include <signal.h>
    #include <sys/stat.h> // mkdir
    #include <sys/wait.h>
    #include <unistd.h> // getcwd, chdir, rmdir, access
static int _mkdir(const char *path) {
    return mkdir(path, 0777);
}
#endif
#include "js-std.h"

#ifdef _WIN32
// must declare under '#include "js-std.h"', or in msvc will 'error C2370: 'js_std_pathsep': redefinition; different storage class'
const char *js_std_pathsep = "\\";
#else
const char *js_std_pathsep = "/";
#endif

#define _return_null() js_return(js_null())

#define _throw_posix_error(__arg_vm) \
    js_throw(js_string_sz(&(__arg_vm->heap), (const char *)strerror(errno)))

#define _posix_zero_on_success(__arg_expr) \
    do { \
        if ((__arg_expr) != 0) { \
            _throw_posix_error(vm); \
        } \
    } while (0)

#define _one_string_argument(__arg_vm, __arg_str, __arg_statement) \
    do { \
        uint16_t __nargs = js_get_arguments_length(__arg_vm); \
        struct js_value *__argbase = js_get_arguments_base(__arg_vm); \
        js_assert(__nargs == 1); \
        js_assert(js_is_string(__argbase)); \
        char *__arg_str = js_string_base(__argbase); \
        __arg_statement; \
    } while (0);

#define _two_string_arguments(__arg_vm, __arg_str_0, __arg_str_1, __arg_statement) \
    do { \
        uint16_t __nargs = js_get_arguments_length(__arg_vm); \
        struct js_value *__argbase = js_get_arguments_base(__arg_vm); \
        js_assert(__nargs == 2); \
        js_assert(js_is_string(__argbase)); \
        js_assert(js_is_string(__argbase + 1)); \
        char *__arg_str_0 = js_string_base(__argbase); \
        char *__arg_str_1 = js_string_base(__argbase + 1); \
        __arg_statement; \
    } while (0);

struct js_result js_std_chdir(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        _posix_zero_on_success(chdir(path));
        _return_null();
    });
}

struct js_result js_std_clock(struct js_vm *vm) {
    js_return(js_number(clock() * 1.0 / CLOCKS_PER_SEC));
}

struct js_result js_std_dirname(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        js_return(js_string_sz(&(vm->heap), dirname(path)));
    });
}

struct js_result js_std_endswith(struct js_vm *vm) {
    _two_string_arguments(vm, lhs, rhs, js_return(js_boolean(string_ends_with_sz(lhs, rhs))));
}

struct js_result js_std_exists(struct js_vm *vm) {
#ifdef _WIN32
    _one_string_argument(vm, path, {
        js_return(js_boolean(GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES ? true : false));
    });
#else
    _one_string_argument(vm, path, {
        js_return(js_boolean(access(path, F_OK) == 0 ? true : false));
    });
#endif
}

struct js_result js_std_format(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs > 0);
    js_assert(js_is_string(argbase));
    struct js_value ret = js_string(&(vm->heap), NULL, 0);
    enum { _searching,
        _expect_left_brace,
        _matching } state = _searching;
    char *vbase = NULL;
    for (char *p = js_string_base(argbase); p < js_string_base(argbase) + js_string_length(argbase); p++) {
        // log_debug("%d %c", state, *p);
        switch (state) {
        case _searching:
            if (*p == '$') {
                state = _expect_left_brace;
            } else {
                string_buffer_append_ch(
                    ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity, *p);
            }
            break;
        case _expect_left_brace:
            js_assert(*p == '{');
            vbase = p + 1;
            state = _matching;
            break;
        case _matching:
            if (*p == '}') {
                uint16_t vlen = (uint16_t)(p - vbase);
                struct js_value val;
                if (isdigit(*vbase)) {
                    uint16_t idx = *vbase - '0';
                    for (char *p = vbase + 1; p < vbase + vlen; p++) {
                        js_assert(isdigit(*p));
                        idx = idx * 10 + (*p - '0');
                    }
                    val = js_get_argument(vm, idx + 1);
                } else {
                    struct js_result ret = js_get_variable(vm, vbase, vlen);
                    if (!ret.success) {
                        return ret;
                    }
                    val = ret.value;
                }
                js_assert(js_is_string(&val));
                string_buffer_append(
                    ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
                    js_string_base(&val), js_string_length(&val));
                state = _searching;
            }
            break;
        default:
            break;
        }
    }
    js_assert(state == _searching); // test if correctly ended with '}'
    js_return(ret);
}

struct js_result js_std_fread(struct js_vm *vm) {
    _one_string_argument(vm, fname, {
        FILE *fp = fopen(fname, "r");
        if (fp == NULL) {
            _throw_posix_error(vm);
        }
        if (fseek(fp, 0, SEEK_END) != 0) {
            fclose(fp);
            _throw_posix_error(vm);
        }
        long fsize = ftell(fp);
        if (fsize == -1) {
            fclose(fp);
            _throw_posix_error(vm);
        }
        if (fseek(fp, 0, SEEK_SET) != 0) {
            fclose(fp);
            _throw_posix_error(vm);
        }
        struct js_value ret = js_string(&(vm->heap), NULL, 0);
        buffer_alloc(
            ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
            fsize + 1); // always +1 to make sure null terminated
        size_t num_read = fread(ret.managed->string.base, 1, fsize, fp);
        // log_debug("fsize=%ld num_read=%zu", fsize, num_read);
        // if (feof(fp)) {
        //     log_debug("eof");
        // }
        if (ferror(fp)) {
            fclose(fp);
            _throw_posix_error(vm);
        }
        ret.managed->string.length = num_read;
        fclose(fp);
        js_return(ret);
    });
}

struct js_result js_std_fwrite(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 2);
    js_assert(js_is_string(argbase));
    js_assert(js_is_string(argbase + 1));
    char *str = js_string_base(argbase);
    size_t slen = js_string_length(argbase);
    char *fname = js_string_base(argbase + 1);
    FILE *fp = fopen(fname, "w");
    fwrite(str, 1, slen, fp);
    // size_t num_written = fwrite(str, 1, slen, fp);
    // log_debug("slen=%zu num_written=%zu", slen, num_written);
    if (ferror(fp)) {
        fclose(fp);
        _throw_posix_error(vm);
    }
    fclose(fp);
    _return_null();
}

struct js_result js_std_getcwd(struct js_vm *vm) {
    char *cwd = getcwd(NULL, 0);
    if (cwd) {
        struct js_value ret = js_string_sz(&(vm->heap), (const char *)cwd);
        free(cwd);
        js_return(ret);
    } else {
        js_throw(js_string_sz(&(vm->heap), (const char *)strerror(errno)));
    }
}

struct js_result js_std_input(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    if (nargs > 1) {
        js_throw(js_scripture_sz("Too many arguments"));
    }
    if (nargs == 1) {
        struct js_value *argbase = js_get_arguments_base(vm);
        if (!js_is_string(argbase)) {
            js_throw(js_scripture_sz("Prompt must be string"));
        }
        js_value_print(argbase);
    }
    struct js_value line = js_string(&(vm->heap), NULL, 0);
    read_line(stdin, line.managed->string.base, line.managed->string.length, line.managed->string.capacity);
    js_return(line);
}

struct js_result js_std_join(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 2);
    js_assert(argbase->type == vt_array);
    js_assert(js_is_string(argbase + 1));
    struct js_value ret = js_string(&(vm->heap), NULL, 0);
    for (size_t i = 0; i < argbase->managed->array.length; i++) {
        // be careful of vt_undefined
        struct js_value *elem = argbase->managed->array.base + i;
        js_assert(js_is_string(elem));
        if (i > 0) {
            string_buffer_append(
                ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
                js_string_base(argbase + 1), js_string_length(argbase + 1));
        }
        string_buffer_append(
            ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
            js_string_base(elem), js_string_length(elem));
    }
    js_return(ret);
}

struct js_result js_std_length(struct js_vm *vm) {
    if (js_get_arguments_length(vm) != 1) {
        js_throw(js_scripture_sz("Require exactly one argument"));
    }
    struct js_value *argbase = js_get_arguments_base(vm);
    if (argbase->type == vt_array) {
        js_return(js_number((double)argbase->managed->array.length));
    } else if (argbase->type == vt_object) {
        js_return(js_number((double)argbase->managed->object.length));
    } else if (js_is_string(argbase)) {
        js_return(js_number((double)js_string_length(argbase)));
    } else {
        js_throw(js_scripture_sz("Only string, array and object have length"));
    }
}

struct js_result js_std_listdir(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 2);
    js_assert(js_is_string(argbase));
    js_assert(js_is_function(argbase + 1));
    char *dir = js_string_base(argbase);
    // must be standardized for windows '*'
    char *standardized_dir =
        string_ends_with_sz(dir, js_std_pathsep) ? string_concat_sz(dir) : string_concat_sz(dir, js_std_pathsep);
    bool isdir;
    char *filename;
    struct js_result result = {.success = true, .value = js_null()};
    bool all_success = true;
#ifdef _WIN32
    char *pattern = string_concat_sz(standardized_dir, "*");
    HANDLE sh;
    WIN32_FIND_DATAA fd;
    sh = FindFirstFileA(pattern, &fd);
    if (sh != INVALID_HANDLE_VALUE) {
        do {
            isdir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            filename = fd.cFileName;
#else
    // https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
    struct dirent *de;
    DIR *dr = opendir(standardized_dir);
    if (dr != NULL) {
        while ((de = readdir(dr)) != NULL) {
            isdir = de->d_type == DT_DIR;
            filename = de->d_name;
#endif
            // BEGIN BLOCK
            // log_debug("isdir=%d standardized_dir=%s filename=%s", isdir, standardized_dir, filename);
            // if (isdir) {
            //     if (!string_equals_sz(filename, ".") && !string_equals_sz(filename, "..")) {
            //         char *subdir = string_concat_sz(standardized_dir, filename, js_std_pathsep);
            //         result = js_call(vm, argbase[1],
            //             (struct js_value[]){js_string_sz(&(vm->heap), subdir), js_null(), js_null()}, 3);
            //         free(subdir);
            //     } else {
            //         result = ((struct js_result){.success = true, .value = js_null()});
            //     }
            // } else {
            //     char *ext = strrchr(filename, '.');
            //     if (ext) {
            //         size_t baselen = ext - filename;
            //         char *base = (char *)calloc(baselen + 1, 1);
            //         strncpy(base, filename, baselen);
            //         result = js_call(vm, argbase[1],
            //             (struct js_value[]){
            //                 js_string_sz(&(vm->heap), standardized_dir),
            //                 js_string_sz(&(vm->heap), base),
            //                 js_string_sz(&(vm->heap), ext)},
            //             3);
            //         free(base);
            //     } else {
            //         result = js_call(vm, argbase[1],
            //             (struct js_value[]){
            //                 js_string_sz(&(vm->heap), standardized_dir),
            //                 js_string_sz(&(vm->heap), filename),
            //                 js_scripture_sz("")},
            //             3);
            //     }
            // }
            if (isdir && (string_equals_sz(filename, ".") || string_equals_sz(filename, ".."))) {
                result = ((struct js_result){.success = true, .value = js_null()});
            } else {
                result = js_call(vm, argbase[1],
                    (struct js_value[]){
                        js_string_sz(&(vm->heap), filename),
                        js_boolean(isdir)},
                    2);
            }
            if (!result.success) {
                all_success = false;
                break;
            }
            // END BLOCK
#ifdef _WIN32
        } while (FindNextFileA(sh, &fd) != 0);
        FindClose(sh);
    }
    free(pattern);
#else
        }
        closedir(dr);
    }
#endif
    free(standardized_dir);
    if (all_success) {
        _return_null();
    } else {
        return result;
    }
}

struct js_result js_std_mkdir(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        _posix_zero_on_success(_mkdir(path));
        _return_null();
    });
}

struct js_result js_std_natural_compare(struct js_vm *vm) {
    _two_string_arguments(vm, lhs, rhs, {
        js_return(js_number(string_natural_compare_sz(lhs, rhs)));
    });
}

struct js_result js_std_pop(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 1);
    js_assert(argbase->type == vt_array);
    js_assert(argbase->managed->array.length > 0);
    argbase->managed->array.length--;
    js_return(argbase->managed->array.base[argbase->managed->array.length]);
}

struct js_result js_std_print(struct js_vm *vm) {
    for (uint16_t i = 0; i < js_get_arguments_length(vm); i++) {
        js_value_print(js_get_arguments_base(vm) + i);
        printf(" ");
    }
    printf("\n");
    _return_null();
}

struct js_result js_std_push(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 2);
    js_assert(argbase->type == vt_array);
    js_array_push(argbase, argbase[1]);
    _return_null();
}

struct js_result js_std_remove(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        _posix_zero_on_success(remove(path));
        _return_null();
    });
}

struct js_result js_std_rmdir(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        _posix_zero_on_success(rmdir(path));
        _return_null();
    });
}

struct _comparator_context {
    struct js_vm *vm;
    struct js_value *func;
};

// mingw is also windows style
#ifdef _WIN32
static int _comparator(void *ctx, const void *lhs, const void *rhs) {
#else
static int _comparator(const void *lhs, const void *rhs, void *ctx) {
#endif
    struct _comparator_context *comp_ctx = (struct _comparator_context *)ctx;
    struct js_result result = js_call(
        comp_ctx->vm, *(comp_ctx->func),
        (struct js_value[]){*((struct js_value *)lhs), *((struct js_value *)rhs)}, 2);
    if (!result.success || result.value.type != vt_number) {
        return 0;
    } else {
        return (int)result.value.number;
    }
}

struct js_result js_std_sort(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 2);
    js_assert(argbase->type == vt_array);
    js_assert(js_is_function(argbase + 1));
    struct _comparator_context ctx = {.vm = vm, .func = argbase + 1};
#ifdef _WIN32
    qsort_s(argbase->managed->array.base, argbase->managed->array.length,
        sizeof(struct js_value), _comparator, &ctx);
#else
    qsort_r(argbase->managed->array.base, argbase->managed->array.length,
        sizeof(struct js_value), _comparator, &ctx);
#endif
    _return_null();
}

struct js_result js_std_split(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 2);
    js_assert(js_is_string(argbase));
    js_assert(js_is_string(argbase + 1));
    // DON'T use strtok, will modify source
    char *str = js_string_base(argbase);
    size_t slen = js_string_length(argbase);
    char *delim = js_string_base(argbase + 1);
    size_t dlen = js_string_length(argbase + 1);
    js_assert(dlen > 0);
    struct js_value ret = js_array(&(vm->heap));
    char *p, *q;
    for (p = str; p < str + slen;) {
        q = strstr(p, delim);
        // log_debug("%s %s", p, q);
        // see https://en.cppreference.com/w/c/string/byte/strstr.html for return value
        // TODO: optimize for scripture?
        // if delim is an empty string, acorrding to standard, cannot distinguish with match on first character, so empty string is forbidden here
        if (q == NULL) {
            js_array_push(&ret, js_string(&(vm->heap), p, str + slen - p));
            break;
        } else {
            js_array_push(&ret, js_string(&(vm->heap), p, q - p));
            p = q + dlen;
        }
    }
    if (q != NULL) {
        js_array_push(&ret, js_scripture_sz(""));
    }
    js_return(ret);
}

struct js_result js_std_startswith(struct js_vm *vm) {
    _two_string_arguments(vm, lhs, rhs, js_return(js_boolean(string_starts_with_sz(lhs, rhs))));
}

#define _function_list \
    X(chdir) \
    X(clock) \
    X(dirname) \
    X(endswith) \
    X(exists) \
    X(format) \
    X(fread) \
    X(fwrite) \
    X(getcwd) \
    X(input) \
    X(join) \
    X(length) \
    X(listdir) \
    X(mkdir) \
    X(natural_compare) \
    X(pop) \
    X(print) \
    X(push) \
    X(remove) \
    X(rmdir) \
    X(sort) \
    X(split) \
    X(startswith)

#ifdef DEBUG
struct js_result js_std_transponder(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs > 0);
    js_assert(js_is_function(argbase));
    return js_call(vm, *argbase, argbase + 1, nargs - 1);
}
#endif

void js_declare_std_functions(struct js_vm *vm, int argc, char *argv[]) {
#define X(name) js_declare_variable_sz(vm, #name, js_c_function(js_std_##name));
    do {
        _function_list
    } while (0);
#undef X
    // other functions and variables not included in X macro definition
    if (argc && argv) {
        struct js_value arg_vector = js_array(&(vm->heap));
        for (int i = 0; i < argc; i++) {
            js_array_push(&arg_vector, js_scripture_sz(argv[i]));
        }
        js_declare_variable_sz(vm, "argv", arg_vector);
    }
    js_declare_variable_sz(vm, "dump", js_c_function(js_vm_dump));
    js_declare_variable_sz(vm, "gc", js_c_function(js_collect_garbage));
    js_declare_variable_sz(vm, "pathsep", js_scripture_sz(js_std_pathsep));
    // compatibility purpose
    struct js_value console = js_object(&(vm->heap));
    js_declare_variable_sz(vm, "console", console);
    js_object_put_sz(&console, "log", js_c_function(js_std_print));
#ifdef DEBUG
    js_declare_variable_sz(vm, "transponder", js_c_function(js_std_transponder));
#endif
}
