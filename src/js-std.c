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
    #include <windows.h> // put before lmcons.h to prevent "'PASCAL': macro redefinition"
    #include <direct.h> // getcwd, chdir, mkdir, rmdir
    #include <lmcons.h> // GetUserNameA's UNLEN
    #include <string.h> // strrchr
    #include <sys/types.h> // _stat
    #include <sys/stat.h> // _stat
    #include <process.h> // _exec family
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
    #ifndef execvp
        #define execvp _execvp
    #endif
    #ifndef stat
        #define stat _stat
typedef struct _stat struct_stat;
    #endif
static const char *_windows_error_string() {
    // https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
    static char es[256];
    memset(es, 0, sizeof(es));
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), 1033, es, sizeof(es), NULL);
    return es;
}
static double _time() {
    FILETIME ft = {0};
    GetSystemTimeAsFileTime(&ft);
    // https://programmerall.com/article/22032114313/
    return (double)(((unsigned long long)ft.dwLowDateTime + ((unsigned long long)ft.dwHighDateTime << 32)) / 10000000.0) - 11644473600;
}
    // stdlib.c already has _sleep()
    #define __sleep(__arg_secs) Sleep((DWORD)((__arg_secs) * 1000))
#else
    #include <dirent.h>
    #include <pwd.h> // getpwuid
    #include <readline/history.h>
    #include <readline/readline.h>
    #include <sys/stat.h> // mkdir
    #include <sys/wait.h>
    #include <unistd.h> // getcwd, chdir, rmdir, access
static int _mkdir(const char *path) {
    return mkdir(path, 0777);
}
typedef struct stat struct_stat;
    #include <sys/time.h>
static double _time() {
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
    #define __sleep(__arg_secs) usleep((int)((__arg_secs) * 1000000))
#endif

#include "js-std.h"

// must declare under '#include "js-std.h"', or in msvc will 'error C2370: 'js_std_pathsep': redefinition; different storage class'
int js_std_argc = 0;
char **js_std_argv = NULL;

#ifdef _WIN32
const char *js_std_os = "windows";
const char *js_std_pathsep = "\\";
static const char _pathsep_ch = '\\';
    #define _throw_windows_error(__arg_vm) \
        js_throw(js_string_sz(&(__arg_vm->heap), _windows_error_string()))
#else
const char *js_std_os = "posix";
const char *js_std_pathsep = "/";
static const char _pathsep_ch = '/';
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

struct js_result js_std_basename(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    // since windows version of _splitpath_s _makepath_s cannot handle long path, and have some bad behaviors such as path "" will crash, i implement following https://www.man7.org/linux/man-pages/man3/dirname.3p.html behaviors
    // posix version is also harmful because it will modify parameter `path`, so i implement it by myself
    // test case is in https://www.man7.org/linux/man-pages/man3/basename.3p.html
    char *path = js_get_string_base(argv);
    size_t pathlen = js_get_string_length(argv);
    if (pathlen == 0) {
        js_return(js_scripture_sz("."));
    } else {
        char *pt = path + pathlen - 1;
        while (*pt == _pathsep_ch && pt > path) {
            pt--;
        }
        if (pt == path) {
            js_return(js_scripture_sz(js_std_pathsep));
        }
        char *ph = pt;
        while (*ph != _pathsep_ch && ph > path) {
            ph--;
        }
        if (*ph == _pathsep_ch) {
            js_return(js_string(&(vm->heap), ph + 1, pt - ph));
        } else {
            js_return(js_string(&(vm->heap), ph, pt - ph + 1));
        }
    }
}

struct js_result js_std_cd(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    _posix_zero_on_success(chdir(js_get_string_base(argv)));
    _return_null();
}

struct js_result js_std_clock(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_return(js_number(clock() * 1.0 / CLOCKS_PER_SEC));
}

struct js_result js_std_close(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    // no error handling
    FILE *fp = (FILE *)(intptr_t)argv->number;
    // log_debug("fclose %p", fp);
    fclose(fp);
    _return_null();
}

struct js_result js_std_ctime(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    time_t t = (time_t)argv->number;
    char *s = ctime(&t);
    js_return(js_string(&(vm->heap), s, strlen(s) - 1)); // remove trailing '\n' of ctime() result
}

struct js_result js_std_cwd(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    char *cwd = getcwd(NULL, 0);
    if (cwd) {
        struct js_value ret = js_string_sz(&(vm->heap), (const char *)cwd);
        free(cwd);
        js_return(ret);
    } else {
        js_throw(js_string_sz(&(vm->heap), (const char *)strerror(errno)));
    }
}

struct js_result js_std_dirname(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    // since windows version of _splitpath_s _makepath_s cannot handle long path, and have some bad behaviors such as path "" will crash, i implement following https://www.man7.org/linux/man-pages/man3/dirname.3p.html behaviors
    // posix version is also harmful because it will modify parameter `path`, so i implement it by myself
    // test case is in https://www.man7.org/linux/man-pages/man3/basename.3p.html
    char *path = js_get_string_base(argv);
    size_t pathlen = js_get_string_length(argv);
    if (pathlen == 0) {
        js_return(js_scripture_sz("."));
    } else {
        char *pt = path + pathlen - 1;
        while (*pt == _pathsep_ch && pt > path) {
            pt--;
        }
        while (*pt != _pathsep_ch && pt > path) {
            pt--;
        }
        while (*pt == _pathsep_ch && pt > path) {
            pt--;
        }
        if (pt == path) {
            js_return(js_scripture_sz(*pt == _pathsep_ch ? js_std_pathsep : "."));
        } else {
            js_return(js_string(&(vm->heap), path, pt - path + 1));
        }
    }
}

struct js_result js_std_dump_vm(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_dump_vm(vm);
    _return_null();
}

struct js_result js_std_endswith(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 1);
    js_assert(js_is_string(argv));
    char *lhs = js_get_string_base(argv);
    for (uint16_t i = 1; i < argc; i++) {
        js_assert(js_is_string(argv + i));
        if (ends_with_sz(lhs, js_get_string_base(argv + i))) {
            js_return(js_boolean(true));
        }
    }
    js_return(js_boolean(false));
}

struct js_result js_std_exec(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    char *exec_argv[256] = {0};
    js_assert(argc > 0);
    js_assert(argc < sizeof(exec_argv));
    for (size_t i = 0; i < argc; i++) {
        exec_argv[i] = js_get_string_base(argv + i);
    }
    intptr_t ret = (intptr_t)execvp(js_get_string_base(argv), exec_argv);
    if (ret == -1) {
        _throw_posix_error(vm);
    }
    // acoording to windows document, _exec* will never return on success, cmd window will pause when sub-peocess ends, codes after it will not be executed, when pressed enter key, program exit.
    _return_null();
}

struct js_result js_std_exists(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
#ifdef _WIN32
    js_return(js_boolean(GetFileAttributesA(js_get_string_base(argv)) != INVALID_FILE_ATTRIBUTES ? true : false));
#else
    js_return(js_boolean(access(js_get_string_base(argv), F_OK) == 0 ? true : false));
#endif
}

struct js_result js_std_exit(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_number);
    exit((int)argv->number);
}

struct js_result js_std_format(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(js_is_string(argv));
    struct js_value ret = js_string(&(vm->heap), NULL, 0);
    enum { _searching,
        _expect_left_brace,
        _matching } state = _searching;
    char *vbase = NULL;
    for (char *p = js_get_string_base(argv); p < js_get_string_base(argv) + js_get_string_length(argv); p++) {
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
                    val = (idx + 1) < argc ? argv[idx + 1] : js_null();
                } else {
                    struct js_result ret = js_get_variable(vm, vbase, vlen);
                    if (!ret.success) {
                        return ret;
                    }
                    val = ret.value;
                }
                struct print_stream out = {.type = string_stream};
                js_serialize_value(&out, tostring_style, &val);
                string_buffer_append(
                    ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
                    out.base, out.length);
                free_stream(&out);
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

struct js_result js_std_gc(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_gc(vm);
    _return_null();
}

struct js_result js_std_input(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    if (argc > 1) {
        js_throw(js_scripture_sz("Too many arguments"));
    }
    char *prompt = "";
    if (argc == 1) {
        if (!js_is_string(argv)) {
            js_throw(js_scripture_sz("Prompt must be string"));
        }
        prompt = argv->managed->string.base;
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

struct js_result js_std_join(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_string(argv + 1));
    struct js_value ret = js_string(&(vm->heap), NULL, 0);
    for (size_t i = 0; i < argv->managed->array.length; i++) {
        // be careful of vt_undefined
        struct js_value *elem = argv->managed->array.base + i;
        js_assert(js_is_string(elem));
        if (i > 0) {
            string_buffer_append(
                ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
                js_get_string_base(argv + 1), js_get_string_length(argv + 1));
        }
        string_buffer_append(
            ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity,
            js_get_string_base(elem), js_get_string_length(elem));
    }
    js_return(ret);
}

struct js_result js_std_length(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    if (argc != 1) {
        js_throw(js_scripture_sz("Require exactly one argument"));
    }
    if (argv->type == vt_array) {
        js_return(js_number((double)argv->managed->array.length));
    } else if (argv->type == vt_object) {
        js_return(js_number((double)argv->managed->object.length));
    } else if (js_is_string(argv)) {
        js_return(js_number((double)js_get_string_length(argv)));
    } else {
        js_throw(js_scripture_sz("Only string, array and object have length"));
    }
}

struct js_result js_std_ls(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(js_is_string(argv));
    js_assert(js_is_function(argv + 1));
    char *dir = js_get_string_base(argv);
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
            if (isdir && (equals_sz(filename, ".") || equals_sz(filename, ".."))) {
                result = ((struct js_result){.success = true, .value = js_null()});
            } else {
                result = js_call(vm, argv[1],
                    2,
                    (struct js_value[]){
                        js_string_sz(&(vm->heap), filename),
                        js_boolean(isdir)});
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

struct js_result js_std_match(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(js_is_string(argv));
    js_assert(js_is_string(argv + 1));
    struct re_capture cap = {0};
    struct js_value ret;
    if (re_match(js_get_string_base(argv), js_get_string_base(argv + 1), &cap)) {
        ret = js_array(&(vm->heap));
        _append_capture(vm, &ret, &cap);
    } else {
        ret = js_null();
    }
    re_free_capture(&cap);
    js_return(ret);
}

struct js_result js_std_md(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    _posix_zero_on_success(_mkdir(js_get_string_base(argv)));
    _return_null();
}

struct js_result js_std_natural_compare(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(js_is_string(argv));
    js_assert(js_is_string(argv + 1));
    js_return(js_number(natural_compare_sz(js_get_string_base(argv), js_get_string_base(argv + 1))));
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
struct js_result js_std_open(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    char *fname = NULL;
    char *mode = "r";
    struct js_value *cb = NULL;
    js_assert(argc > 0);
    js_assert(js_is_string(argv));
    fname = js_get_string_base(argv);
    if (argc == 2) {
        if (js_is_string(argv + 1)) {
            mode = js_get_string_base(argv + 1);
        } else if (js_is_function(argv + 1)) {
            cb = argv + 1;
        } else {
            js_throw(js_scripture_sz("if 2 args, arg 1 must be string or function"));
        }
    } else if (argc == 3) {
        if (js_is_string(argv + 1) && js_is_function(argv + 2)) {
            mode = js_get_string_base(argv + 1);
            cb = argv + 2;
        } else {
            js_throw(js_scripture_sz("if 3 args, arg 1 must be string and arg 2 must be function"));
        }
    } else if (argc > 3) {
        js_throw(js_scripture_sz("num of args can only be 1 - 3"));
    }
    FILE *fp = fopen(fname, mode);
    if (fp == NULL) {
        _throw_posix_error(vm);
    }
    struct js_value jsfp = js_number((double)(intptr_t)fp);
    if (cb) {
        struct js_result ret = js_call(vm, *cb, 1, (struct js_value[]){jsfp});
        // log_debug("fclose %p", fp);
        fclose(fp);
        return ret;
    } else {
        js_return(jsfp);
    }
}

struct js_result js_std_pop(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(argv->type == vt_array);
    js_assert(argv->managed->array.length > 0);
    argv->managed->array.length--;
    js_return(argv->managed->array.base[argv->managed->array.length]);
}

struct js_result js_std_print(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    struct print_stream out = {.type = file_stream, .fp = stdout};
    for (uint16_t i = 0; i < argc; i++) {
        js_serialize_value(&out, tostring_style, argv + i);
        printf(" ");
    }
    printf("\n");
    _return_null();
}

struct js_result js_std_push(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_push_array_element(argv, argv[1]);
    _return_null();
}

struct js_result js_std_read(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    char *fname = NULL;
    FILE *fp = NULL;
    bool iscmd = false;
    struct js_value *cb = NULL;
#define __cleanup() (fname && fp ? (iscmd ? pclose(fp) : fclose(fp)) : (void)0)
    js_assert(argc > 0);
    if (argv->type == vt_number) {
        fp = (FILE *)(intptr_t)argv->number;
    } else if (js_is_string(argv)) {
        fname = js_get_string_base(argv);
    } else {
        js_throw(js_scripture_sz("arg 0 must be number or string"));
    }
    if (argc == 2) {
        if ((argv + 1)->type == vt_boolean) {
            iscmd = (argv + 1)->boolean;
        } else if (js_is_function(argv + 1)) {
            cb = argv + 1;
        } else {
            js_throw(js_scripture_sz("if 2 args, arg 1 must be boolean or function"));
        }
    } else if (argc == 3) {
        if ((argv + 1)->type == vt_boolean && js_is_function(argv + 2)) {
            iscmd = (argv + 1)->boolean;
            cb = argv + 2;
        } else {
            js_throw(js_scripture_sz("if 3 args, arg 1 must be boolean and arg 2 must be function"));
        }
    } else if (argc > 3) {
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
            struct js_result ret = js_call(vm, *cb, 1, (struct js_value[]){line});
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
}

struct js_result js_std_rd(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    _posix_zero_on_success(rmdir(js_get_string_base(argv)));
    _return_null();
}

struct js_result js_std_rm(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    _posix_zero_on_success(remove(js_get_string_base(argv)));
    _return_null();
}

struct js_result js_std_sleep(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(argv->type == vt_number);
    double timeout = argv->number;
    if (argc == 1) {
        __sleep(timeout);
    } else {
        js_assert(js_is_function(argv + 1));
        double interval = 1;
        if (argc > 2) {
            js_assert((argv + 2)->type == vt_number);
            interval = (argv + 2)->number;
        }
        double end = _time() + timeout;
        for (;;) {
            __sleep(interval);
            double remains = end - _time();
            if (remains < 0) {
                break;
            }
            js_call(vm, argv[1], 1, (struct js_value[]){js_number(remains)});
        }
    }
    _return_null();
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
        comp_ctx->vm, *(comp_ctx->func), 2,
        (struct js_value[]){*((struct js_value *)lhs), *((struct js_value *)rhs)});
    if (!result.success || result.value.type != vt_number) {
        return 0;
    } else {
        return (int)result.value.number;
    }
}

struct js_result js_std_sort(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 2);
    js_assert(argv->type == vt_array);
    js_assert(js_is_function(argv + 1));
    struct _comparator_context ctx = {.vm = vm, .func = argv + 1};
#ifdef _WIN32
    qsort_s(argv->managed->array.base, argv->managed->array.length,
        sizeof(struct js_value), _comparator, &ctx);
#else
    qsort_r(argv->managed->array.base, argv->managed->array.length,
        sizeof(struct js_value), _comparator, &ctx);
#endif
    js_return(*argv);
}

struct js_result js_std_spawn(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
#ifdef _WIN32
    char *cmd_base = NULL;
    size_t cmd_length = 0;
    size_t cmd_capacity = 0;
    bool first_arg = true;
    for (uint16_t i = 0; i < argc; i++) {
        js_assert(js_is_string(argv + i));
        if (first_arg) {
            first_arg = false;
        } else {
            string_buffer_append_ch(cmd_base, cmd_length, cmd_capacity, ' ');
        }
        // tested: quoted strings passed to argv will automatically remove quotes
        bool require_quoted = false;
        if (js_get_string_length(argv + i) == 0) {
            require_quoted = true;
        } else {
            for (char *p = js_get_string_base(argv + i); *p != '\0'; p++) {
                if (isspace(*p)) {
                    require_quoted = true;
                    break;
                }
            }
        }
        if (require_quoted) {
            string_buffer_append_ch(cmd_base, cmd_length, cmd_capacity, '"');
        }
        string_buffer_append_sz(cmd_base, cmd_length, cmd_capacity, js_get_string_base(argv + i));
        if (require_quoted) {
            string_buffer_append_ch(cmd_base, cmd_length, cmd_capacity, '"');
        }
    }
    STARTUPINFO si = {.cb = sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi = {0};
    BOOL result = CreateProcessA(NULL, cmd_base, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    buffer_free(cmd_base, cmd_length, cmd_capacity);
    if (!result) {
        _throw_windows_error(vm);
    }
    js_return(js_number((double)pi.dwProcessId));
#else
    char *exec_argv[256] = {0};
    js_assert(argc < sizeof(exec_argv));
    for (size_t i = 0; i < argc; i++) {
        js_assert(js_is_string(argv + i));
        exec_argv[i] = js_get_string_base(argv + i);
    }
    pid_t pid = fork();
    switch (pid) {
    case -1:
        _throw_posix_error(vm);
    case 0:
        int ret = execvp(js_get_string_base(argv), exec_argv);
        if (ret == -1) {
            perror(NULL);
            exit(EXIT_FAILURE);
        }
    default:
        js_return(js_number((double)pid));
    }
#endif
}

struct js_result js_std_split(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(js_is_string(argv));
    if (argc == 1) {
        struct js_value ret = js_array(&(vm->heap));
        js_push_array_element(&ret, argv[0]);
        js_return(ret);
    } else {
        js_assert(js_is_string(argv + 1));
        // DON'T use strtok, will modify source
        char *str = js_get_string_base(argv);
        size_t slen = js_get_string_length(argv);
        char *delim = js_get_string_base(argv + 1);
        size_t dlen = js_get_string_length(argv + 1);
        struct js_value ret = js_array(&(vm->heap));
        if (dlen == 0) {
            for (size_t i = 0; i < slen; i++) {
                js_push_array_element(&ret, js_string(&(vm->heap), str + i, 1));
            }
        } else {
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
        }
        js_return(ret);
    }
}

struct js_result js_std_startswith(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 1);
    js_assert(js_is_string(argv));
    char *lhs = js_get_string_base(argv);
    for (uint16_t i = 1; i < argc; i++) {
        js_assert(js_is_string(argv + i));
        if (starts_with_sz(lhs, js_get_string_base(argv + i))) {
            js_return(js_boolean(true));
        }
    }
    js_return(js_boolean(false));
}

struct js_result js_std_stat(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    struct_stat sb;
    if (stat(js_get_string_base(argv), &sb) != 0) {
        _throw_posix_error(vm);
    }
    struct js_value result = js_object(&(vm->heap));
    js_put_object_value_sz(&result, "size", js_number((double)sb.st_size));
    js_put_object_value_sz(&result, "atime", js_number((double)sb.st_atime));
    js_put_object_value_sz(&result, "ctime", js_number((double)sb.st_ctime));
    js_put_object_value_sz(&result, "mtime", js_number((double)sb.st_mtime));
    js_put_object_value_sz(&result, "uid", js_number((double)sb.st_uid));
    js_put_object_value_sz(&result, "gid", js_number((double)sb.st_gid));
    js_return(result);
}

struct js_result js_std_system(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    errno = 0; // fix windows bug: won't reset last error such as "File exists"
    int ret = system(js_get_string_base(argv));
    if (errno) {
        _throw_posix_error(vm);
    }
#ifndef _WIN32
    ret = WEXITSTATUS(ret);
#endif
    js_return(js_number((double)ret));
}

struct js_result js_std_time(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_return(js_number(_time()));
}

// _serialize already inuse in msvc, "'_serialize': intrinsic function, cannot be defined"
static struct js_result __serialize(struct js_vm *vm, uint16_t argc, struct js_value *argv, enum serialized_style to) {
    js_assert(argc == 1);
    struct print_stream out = {.type = string_stream};
    js_serialize_value(&out, to, argv);
    struct js_value ret = js_alloc_managed(&(vm->heap), vt_string);
    ret.managed->string.base = out.base;
    ret.managed->string.length = out.length;
    ret.managed->string.capacity = out.capacity;
    js_return(ret);
}

struct js_result js_std_todump(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    return __serialize(vm, argc, argv, todump_style);
}

struct js_result js_std_tojson(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    return __serialize(vm, argc, argv, tojson_style);
}

struct js_result js_std_tonumber(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 1);
    js_assert(js_is_string(argv));
    char *str = js_get_string_base(argv);
    char *end;
    errno = 0; // fix windows bug: won't reset last error such as ERANGE, linux is ok
    double num = strtod(str, &end);
    if (errno) {
        _throw_posix_error(vm);
    }
    if (end == str || *end != '\0') {
        js_throw(js_scripture_sz("Not valid number"));
    }
    js_return(js_number(num));
}

struct js_result js_std_tostring(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    return __serialize(vm, argc, argv, tostring_style);
}

struct js_result js_std_whoami(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc == 0);
#ifdef _WIN32
    char buf[UNLEN + 1] = {0};
    DWORD len = sizeof(buf);
    if (GetUserNameA(buf, &len)) {
        js_return(js_string(&(vm->heap), buf, len));
    } else {
        _throw_windows_error(vm);
    }
#else
    // DON'T use getlogin, it will keep origin user name after su
    struct passwd *pw = getpwuid(geteuid());
    if (pw) {
        js_return(js_string_sz(&(vm->heap), pw->pw_name));
    } else {
        _throw_posix_error(vm);
    }
#endif
}

struct js_result js_std_write(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    char *fname = NULL;
    FILE *fp = NULL;
    bool isappend = false;
    char *text = NULL;
    size_t tlen = 0;
#define __cleanup() (fname && fp ? fclose(fp) : (void)0)
    js_assert(argc > 1);
    if (argv->type == vt_number) {
        fp = (FILE *)(intptr_t)argv->number;
    } else if (js_is_string(argv)) {
        fname = js_get_string_base(argv);
    } else {
        js_throw(js_scripture_sz("arg 0 must be number or string"));
    }
    if (argc == 2) {
        if (js_is_string(argv + 1)) {
            text = js_get_string_base(argv + 1);
            tlen = js_get_string_length(argv + 1);
        } else {
            js_throw(js_scripture_sz("if 2 args, arg 1 must be string"));
        }
    } else if (argc == 3) {
        if ((argv + 1)->type == vt_boolean && js_is_string(argv + 2)) {
            isappend = (argv + 1)->boolean;
            text = js_get_string_base(argv + 2);
            tlen = js_get_string_length(argv + 2);
        } else {
            js_throw(js_scripture_sz("if 3 args, arg 1 must be boolean and arg 2 must be string"));
        }
    } else if (argc > 3) {
        js_throw(js_scripture_sz("num of args can only be 2 - 3"));
    }
    fname ? (fp = fopen(fname, isappend ? "a" : "w")) : (void)0;
    if (fp == NULL) {
        _throw_posix_error(vm);
    }
    if (fp == stdin) {
        js_throw(js_scripture_sz("Cannot write to stdin"));
    }
    size_t num_written = fwrite(text, sizeof(char), tlen, fp);
    fflush(fp); // in case of in linux xterm, write to stdout without \n, shall immediately display
    __cleanup();
    if (num_written < tlen) {
        _throw_posix_error(vm);
    } else {
        _return_null();
    }
#undef __cleanup
}

#define _function_list \
    X(basename) \
    X(cd) \
    X(clock) \
    X(close) \
    X(ctime) \
    X(cwd) \
    X(dirname) \
    X(dump_vm) \
    X(endswith) \
    X(exec) \
    X(exists) \
    X(exit) \
    X(format) \
    X(gc) \
    X(input) \
    X(join) \
    X(length) \
    X(ls) \
    X(match) \
    X(md) \
    X(natural_compare) \
    X(open) \
    X(pop) \
    X(print) \
    X(push) \
    X(read) \
    X(rm) \
    X(rd) \
    X(sleep) \
    X(sort) \
    X(spawn) \
    X(split) \
    X(startswith) \
    X(stat) \
    X(system) \
    X(time) \
    X(todump) \
    X(tojson) \
    X(tonumber) \
    X(tostring) \
    X(whoami) \
    X(write)

#ifdef DEBUG
struct js_result js_std_forward(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(js_is_function(argv));
    return js_call(vm, *argv, argc - 1, argv + 1);
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
    js_declare_variable_sz(vm, "os", js_scripture_sz(js_std_os));
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
    // compatibility purpose
    struct js_value console = js_object(&(vm->heap));
    js_declare_variable_sz(vm, "console", console);
    js_put_object_value_sz(&console, "log", js_c_function(js_std_print));
#ifdef DEBUG
    js_declare_variable_sz(vm, "forward", js_c_function(js_std_forward));
#endif
}
