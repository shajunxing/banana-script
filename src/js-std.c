/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <errno.h>
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
    #ifndef popen
        #define popen _popen
    #endif
    #ifndef pclose
        #define pclose _pclose
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
    #include <readline/history.h>
    #include <readline/readline.h>
    #include <sys/stat.h> // mkdir
    #include <sys/wait.h>
    #include <unistd.h> // getcwd, chdir, rmdir, access
static int _mkdir(const char *path) {
    return mkdir(path, 0777);
}
#endif
#include "js-std.h"

int js_std_argc = 0;
char **js_std_argv = NULL;

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
        char *__arg_str = js_get_string_base(__argbase); \
        __arg_statement; \
    } while (0);

#define _two_string_arguments(__arg_vm, __arg_str_0, __arg_str_1, __arg_statement) \
    do { \
        uint16_t __nargs = js_get_arguments_length(__arg_vm); \
        struct js_value *__argbase = js_get_arguments_base(__arg_vm); \
        js_assert(__nargs == 2); \
        js_assert(js_is_string(__argbase)); \
        js_assert(js_is_string(__argbase + 1)); \
        char *__arg_str_0 = js_get_string_base(__argbase); \
        char *__arg_str_1 = js_get_string_base(__argbase + 1); \
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

struct js_result js_std_close(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 1);
    js_assert(argbase->type == vt_number);
    // no error handling
    FILE *fp = (FILE *)(intptr_t)argbase->number;
    // log_debug("fclose %p", fp);
    fclose(fp);
    _return_null();
}

struct js_result js_std_dirname(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        js_return(js_string_sz(&(vm->heap), dirname(path)));
    });
}

struct js_result js_std_endswith(struct js_vm *vm) {
    _two_string_arguments(vm, lhs, rhs, js_return(js_boolean(ends_with_sz(lhs, rhs))));
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

struct js_result js_std_exit(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 1);
    js_assert(argbase->type == vt_number);
    exit((int)argbase->number);
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
    for (char *p = js_get_string_base(argbase); p < js_get_string_base(argbase) + js_get_string_length(argbase); p++) {
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
                    js_get_string_base(&val), js_get_string_length(&val));
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
    char *prompt = "";
    if (nargs == 1) {
        struct js_value *argbase = js_get_arguments_base(vm);
        if (!js_is_string(argbase)) {
            js_throw(js_scripture_sz("Prompt must be string"));
        }
        prompt = argbase->managed->string.base;
    }
    struct js_value line = js_string(&(vm->heap), NULL, 0);
#ifdef _WIN32
    printf(prompt);
    read_line(stdin, line.managed->string.base, line.managed->string.length, line.managed->string.capacity);
#else
    // disable filename completion
    rl_bind_key('\t', rl_insert);
    char *s = readline(prompt);
    string_buffer_append_sz(line.managed->string.base, line.managed->string.length, line.managed->string.capacity, s);
    free(s);
    if (line.managed->string.length > 0) {
        add_history(line.managed->string.base);
    }
#endif
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
                js_get_string_base(argbase + 1), js_get_string_length(argbase + 1));
        }
        string_buffer_append(
            ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
            js_get_string_base(elem), js_get_string_length(elem));
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
        js_return(js_number((double)js_get_string_length(argbase)));
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
    char *dir = js_get_string_base(argbase);
    // must be standardized for windows '*'
    char *standardized_dir =
        ends_with_sz(dir, js_std_pathsep) ? concat_sz(dir) : concat_sz(dir, js_std_pathsep);
    bool isdir;
    char *filename;
    struct js_result result = {.success = true, .value = js_null()};
    bool all_success = true;
#ifdef _WIN32
    char *pattern = concat_sz(standardized_dir, "*");
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
            //     if (!equals_sz(filename, ".") && !equals_sz(filename, "..")) {
            //         char *subdir = concat_sz(standardized_dir, filename, js_std_pathsep);
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
            if (isdir && (equals_sz(filename, ".") || equals_sz(filename, ".."))) {
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

static void _append_capture(struct js_vm *vm, struct js_value *arr, struct re_capture *cap) {
    if (!cap) {
        return;
    }
    if (cap->head && cap->tail) {
        js_push_array_element(arr, js_string(&(vm->heap), cap->head, cap->tail - cap->head));
    }
    buffer_for_each(cap->subs.base, cap->subs.length, cap->subs.capacity,
        i, c, _append_capture(vm, arr, c));
}

struct js_result js_std_match(struct js_vm *vm) {
    _two_string_arguments(vm, str, pat, {
        struct re_capture cap = {0};
        struct js_value ret;
        if (re_match(str, pat, &cap)) {
            ret = js_array(&(vm->heap));
            _append_capture(vm, &ret, &cap);
        } else {
            ret = js_null();
        }
        re_free_capture(&cap);
        js_return(ret);
    });
}

struct js_result js_std_mkdir(struct js_vm *vm) {
    _one_string_argument(vm, path, {
        _posix_zero_on_success(_mkdir(path));
        _return_null();
    });
}

struct js_result js_std_natural_compare(struct js_vm *vm) {
    _two_string_arguments(vm, lhs, rhs, {
        js_return(js_number(natural_compare_sz(lhs, rhs)));
    });
}

/*
generic file open
arguments can be following formats:
    string fname
    string fname, string mode
    string fname, function cb
    string fname, string mode, function cb
default mode is 'r'
if no cb, returns opened fp; if cb, call it using fp as parameter, and auto close fp after cb returns
*/
struct js_result js_std_open(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    char *fname = NULL;
    char *mode = "r";
    struct js_value *cb = NULL;
    js_assert(nargs > 0);
    js_assert(js_is_string(argbase));
    fname = js_get_string_base(argbase);
    if (nargs == 2) {
        if (js_is_string(argbase + 1)) {
            mode = js_get_string_base(argbase + 1);
        } else if (js_is_function(argbase + 1)) {
            cb = argbase + 1;
        } else {
            js_throw(js_scripture_sz("if 2 args, arg 1 must be string or function"));
        }
    } else if (nargs == 3) {
        if (js_is_string(argbase + 1) && js_is_function(argbase + 2)) {
            mode = js_get_string_base(argbase + 1);
            cb = argbase + 2;
        } else {
            js_throw(js_scripture_sz("if 3 args, arg 1 must be string and arg 2 must be function"));
        }
    } else if (nargs > 3) {
        js_throw(js_scripture_sz("num of args can only be 1 - 3"));
    }
    FILE *fp = fopen(fname, mode);
    if (fp == NULL) {
        _throw_posix_error(vm);
    }
    struct js_value jsfp = js_number((double)(intptr_t)fp);
    if (cb) {
        struct js_result ret = js_call(vm, *cb, (struct js_value[]){jsfp}, 1);
        // log_debug("fclose %p", fp);
        fclose(fp);
        return ret;
    } else {
        js_return(jsfp);
    }
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
        js_print_value(js_get_arguments_base(vm) + i);
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
    js_push_array_element(argbase, argbase[1]);
    _return_null();
}

/*
generic read for text file or process output
arguments can be following formats:
    number fp
    number fp, function cb
    string fname/cmd
    string fname/cmd, boolean iscmd
    string fname/cmd, function cb
    string fname/cmd, boolean iscmd, function cb
iscommand means use popen() instead of fopen() if arg 0 is string
if has cb, read each line and call it, pass line as parameter, or returns while content until eof
*/
struct js_result js_std_read(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    char *fname = NULL;
    FILE *fp = NULL;
    bool iscmd = false;
    struct js_value *cb = NULL;
#define __cleanup() (fname && fp ? (iscmd ? pclose(fp) : fclose(fp)) : (void)0)
    js_assert(nargs > 0);
    if (argbase->type == vt_number) {
        fp = (FILE *)(intptr_t)argbase->number;
    } else if (js_is_string(argbase)) {
        fname = js_get_string_base(argbase);
    } else {
        js_throw(js_scripture_sz("arg 0 must be number or string"));
    }
    if (nargs == 2) {
        if ((argbase + 1)->type == vt_boolean) {
            iscmd = (argbase + 1)->boolean;
        } else if (js_is_function(argbase + 1)) {
            cb = argbase + 1;
        } else {
            js_throw(js_scripture_sz("if 2 args, arg 1 must be boolean or function"));
        }
    } else if (nargs == 3) {
        if ((argbase + 1)->type == vt_boolean && js_is_function(argbase + 2)) {
            iscmd = (argbase + 1)->boolean;
            cb = argbase + 2;
        } else {
            js_throw(js_scripture_sz("if 3 args, arg 1 must be boolean and arg 2 must be function"));
        }
    } else if (nargs > 3) {
        js_throw(js_scripture_sz("num of args can only be 1 - 3"));
    }
    fname ? (fp = iscmd ? popen(fname, "r") : fopen(fname, "r")) : (void)0;
    if (fp == NULL) {
        _throw_posix_error(vm);
    }
    if (fp == stdout) {
        js_throw(js_scripture_sz("Cannot read from stdout"));
    }
    if (fp == stderr) {
        js_throw(js_scripture_sz("Cannot read from stderr"));
    }
    if (fp == stdin) {
        // no buffering, to prevent such as ctrl-z ctrl-d not responding
        // https://stackoverflow.com/questions/8699397/how-do-i-disable-buffering-in-fread
        // https://en.cppreference.com/w/c/io/setvbuf
        // tested if pressed not at line beginning, windows will add 0x1a, linux add nothing and you have to press ctrl-d twice if not at line beginning
        // https://ss64.com/nt/con.html
        // since windows ctrl-z can be in middle of string, ignore it
        setvbuf(fp, NULL, _IONBF, 0);
    }
    if (cb) {
        for (;;) {
            struct js_value line = js_string(&(vm->heap), NULL, 0);
            int c;
            for (;;) {
                c = fgetc(fp);
                if (c == EOF || c == '\n') {
                    break;
                }
                string_buffer_append_ch(
                    line.managed->string.base, line.managed->string.length, line.managed->string.capacity, c);
            }
            // buffer_dump(line.managed->string.base, line.managed->string.length, line.managed->string.capacity);
            struct js_result ret = js_call(vm, *cb, (struct js_value[]){line}, 1);
            if (!ret.success) {
                __cleanup();
                return ret;
            }
            if (feof(fp)) {
                break;
            } else if (ferror(fp)) {
                __cleanup();
                _throw_posix_error(vm);
            }
        }
        __cleanup();
        _return_null();
    } else {
        char buf[1024];
        struct js_value content = js_string(&(vm->heap), NULL, 0);
        size_t num_read;
        while ((num_read = fread(buf, sizeof(char), countof(buf), fp)) > 0) {
            string_buffer_append(
                content.managed->string.base, content.managed->string.length, content.managed->string.capacity,
                buf, num_read);
            if (feof(fp)) {
                break;
            } else if (ferror(fp)) {
                __cleanup();
                _throw_posix_error(vm);
            }
        }
        __cleanup();
        // buffer_dump(content.managed->string.base, content.managed->string.length, content.managed->string.capacity);
        js_return(content);
    }
#undef __cleanup
    // _one_string_argument(vm, fname, {
    //     FILE *fp = fopen(fname, "r");
    //     if (fp == NULL) {
    //         _throw_posix_error(vm);
    //     }
    //     if (fseek(fp, 0, SEEK_END) != 0) {
    //         fclose(fp);
    //         _throw_posix_error(vm);
    //     }
    //     long fsize = ftell(fp);
    //     if (fsize == -1) {
    //         fclose(fp);
    //         _throw_posix_error(vm);
    //     }
    //     if (fseek(fp, 0, SEEK_SET) != 0) {
    //         fclose(fp);
    //         _throw_posix_error(vm);
    //     }
    //     struct js_value ret = js_string(&(vm->heap), NULL, 0);
    //     buffer_alloc(
    //         ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
    //         fsize + 1); // always +1 to make sure null terminated
    //     size_t num_read = fread(ret.managed->string.base, 1, fsize, fp);
    //     // log_debug("fsize=%ld num_read=%zu", fsize, num_read);
    //     // if (feof(fp)) {
    //     //     log_debug("eof");
    //     // }
    //     if (ferror(fp)) {
    //         fclose(fp);
    //         _throw_posix_error(vm);
    //     }
    //     ret.managed->string.length = num_read;
    //     fclose(fp);
    //     js_return(ret);
    // });
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
    char *str = js_get_string_base(argbase);
    size_t slen = js_get_string_length(argbase);
    char *delim = js_get_string_base(argbase + 1);
    size_t dlen = js_get_string_length(argbase + 1);
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
            js_push_array_element(&ret, js_string(&(vm->heap), p, str + slen - p));
            break;
        } else {
            js_push_array_element(&ret, js_string(&(vm->heap), p, q - p));
            p = q + dlen;
        }
    }
    if (q != NULL) {
        js_push_array_element(&ret, js_scripture_sz(""));
    }
    js_return(ret);
}

struct js_result js_std_startswith(struct js_vm *vm) {
    _two_string_arguments(vm, lhs, rhs, js_return(js_boolean(starts_with_sz(lhs, rhs))));
}

struct js_result js_std_tostring(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 1);
    struct print_stream out = {.type = string_stream};
    js_serialize_value(&out, user_style, argbase);
    struct js_value ret = js_alloc_managed(&(vm->heap), vt_string);
    ret.managed->string.base = out.base;
    ret.managed->string.length = out.length;
    ret.managed->string.capacity = out.capacity;
    js_return(ret);
}

struct js_result js_std_tojson(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs == 1);
    struct print_stream out = {.type = string_stream};
    js_serialize_value(&out, json_style, argbase);
    struct js_value ret = js_alloc_managed(&(vm->heap), vt_string);
    ret.managed->string.base = out.base;
    ret.managed->string.length = out.length;
    ret.managed->string.capacity = out.capacity;
    js_return(ret);
}

struct js_result js_std_tonumber(struct js_vm *vm) {
    _one_string_argument(vm, str, {
        char *end;
        double num = strtod(str, &end);
        if (errno) {
            _throw_posix_error(vm);
        }
        if (end == str || *end != '\0') {
            js_throw(js_scripture_sz("Not valid number"));
        }
        js_return(js_number(num));
    });
}

/*
generic write for text file
arguments can be following formats:
    number fp, string str
    string fname, string str
    string fname, boolean isappend, string str
*/
struct js_result js_std_write(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    char *fname = NULL;
    FILE *fp = NULL;
    bool isappend = false;
    char *str = NULL;
    size_t slen = 0;
#define __cleanup() (fname && fp ? fclose(fp) : (void)0)
    js_assert(nargs > 1);
    if (argbase->type == vt_number) {
        fp = (FILE *)(intptr_t)argbase->number;
    } else if (js_is_string(argbase)) {
        fname = js_get_string_base(argbase);
    } else {
        js_throw(js_scripture_sz("arg 0 must be number or string"));
    }
    if (nargs == 2) {
        if (js_is_string(argbase + 1)) {
            str = js_get_string_base(argbase + 1);
            slen = js_get_string_length(argbase + 1);
        } else {
            js_throw(js_scripture_sz("if 2 args, arg 1 must be string"));
        }
    } else if (nargs == 3) {
        if ((argbase + 1)->type == vt_boolean && js_is_string(argbase + 2)) {
            isappend = (argbase + 1)->boolean;
            str = js_get_string_base(argbase + 2);
            slen = js_get_string_length(argbase + 2);
        } else {
            js_throw(js_scripture_sz("if 3 args, arg 1 must be boolean and arg 2 must be string"));
        }
    } else if (nargs > 3) {
        js_throw(js_scripture_sz("num of args can only be 2 - 3"));
    }
    fname ? (fp = fopen(fname, isappend ? "a" : "w")) : (void)0;
    if (fp == NULL) {
        _throw_posix_error(vm);
    }
    if (fp == stdin) {
        js_throw(js_scripture_sz("Cannot write to stdin"));
    }
    size_t num_written = fwrite(str, sizeof(char), slen, fp);
    __cleanup();
    if (num_written < slen) {
        _throw_posix_error(vm);
    } else {
        _return_null();
    }
#undef __cleanup
    // js_assert(js_is_string(argbase));
    // js_assert(js_is_string(argbase + 1));
    // char *fname = js_get_string_base(argbase);
    // char *str = js_get_string_base(argbase + 1);
    // size_t slen = js_get_string_length(argbase + 1);
    // FILE *fp = fopen(fname, "w");
    // fwrite(str, 1, slen, fp);
    // // size_t num_written = fwrite(str, 1, slen, fp);
    // // log_debug("slen=%zu num_written=%zu", slen, num_written);
    // if (ferror(fp)) {
    //     fclose(fp);
    //     _throw_posix_error(vm);
    // }
    // fclose(fp);
    // _return_null();
}

#define _function_list \
    X(chdir) \
    X(clock) \
    X(close) \
    X(dirname) \
    X(endswith) \
    X(exists) \
    X(exit) \
    X(format) \
    X(getcwd) \
    X(input) \
    X(join) \
    X(length) \
    X(listdir) \
    X(match) \
    X(mkdir) \
    X(natural_compare) \
    X(open) \
    X(pop) \
    X(print) \
    X(push) \
    X(read) \
    X(remove) \
    X(rmdir) \
    X(sort) \
    X(split) \
    X(startswith) \
    X(tojson) \
    X(tonumber) \
    X(tostring) \
    X(write)

#ifdef DEBUG
struct js_result js_std_forward(struct js_vm *vm) {
    uint16_t nargs = js_get_arguments_length(vm);
    struct js_value *argbase = js_get_arguments_base(vm);
    js_assert(nargs > 0);
    js_assert(js_is_function(argbase));
    return js_call(vm, *argbase, argbase + 1, nargs - 1);
}
#endif

void js_declare_std_functions(struct js_vm *vm) {
#define X(name) js_declare_variable_sz(vm, #name, js_c_function(js_std_##name));
    do {
        _function_list
    } while (0);
#undef X
    // other variables and functions not included in X macro definition
    if (js_std_argc && js_std_argv) {
        js_declare_variable_sz(vm, "argc", js_number(js_std_argc));
        struct js_value arg_vector = js_array(&(vm->heap));
        for (int i = 0; i < js_std_argc; i++) {
            js_push_array_element(&arg_vector, js_scripture_sz(js_std_argv[i]));
        }
        js_declare_variable_sz(vm, "argv", arg_vector);
    }
#ifdef _WIN32
    js_declare_variable_sz(vm, "os", js_scripture_sz("windows"));
#else
    js_declare_variable_sz(vm, "os", js_scripture_sz("posix"));
#endif
    js_declare_variable_sz(vm, "pathsep", js_scripture_sz(js_std_pathsep));
    // log_expression("%llu", stdin);
    // log_expression("%llu", stdout);
    // log_expression("%llu", stderr);
    // log_expression("%f", (double)(intptr_t)stdin);
    // log_expression("%f", (double)(intptr_t)stdout);
    // log_expression("%f", (double)(intptr_t)stderr);
    // since x64 arm64 all use 48 bit virtual address space, and double can accurately represent 53-bit integers, type cast is safe
    js_declare_variable_sz(vm, "stdin", js_number((double)(intptr_t)stdin));
    js_declare_variable_sz(vm, "stdout", js_number((double)(intptr_t)stdout));
    js_declare_variable_sz(vm, "stderr", js_number((double)(intptr_t)stderr));
    js_declare_variable_sz(vm, "dump", js_c_function(js_dump_vm));
    js_declare_variable_sz(vm, "gc", js_c_function(js_collect_garbage));
    // compatibility purpose
    struct js_value console = js_object(&(vm->heap));
    js_declare_variable_sz(vm, "console", console);
    js_put_object_value_sz(&console, "log", js_c_function(js_std_print));
#ifdef DEBUG
    js_declare_variable_sz(vm, "forward", js_c_function(js_std_forward));
#endif
}
