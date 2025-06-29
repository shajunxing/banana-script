/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../banana-nomake/make.h"

#define proj "js"
#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define src_dir "src" pathsep
#define dll_file_name proj dllext
#define dll_file_path bin_dir dll_file_name
#define lib_file bin_dir proj libext
#define msvc_nodefaultlib " msvcrt.lib libvcruntime.lib ucrt.lib kernel32.lib user32.lib"
char *cc = NULL;
char *ld = NULL;

// use macro instead of function to output correct line number
#define compile_library(obj_file, c_file)                                             \
    do {                                                                              \
        char *cmd = compiler == msvc                                                  \
                        ? concat(cc, " /DDLL /DEXPORT /Fo", obj_file, " ", c_file)    \
                        : concat(cc, " -D DLL -D EXPORT -o ", obj_file, " ", c_file); \
        async(cmd);                                                                   \
        free(cmd);                                                                    \
    } while (0)

#define compile_executable(obj_file, c_file)                                \
    do {                                                                    \
        char *cmd = compiler == msvc                                        \
                        ? concat(cc, " /DDLL /Fo", obj_file, " ", c_file)   \
                        : concat(cc, " -D DLL -o ", obj_file, " ", c_file); \
        async(cmd);                                                         \
        free(cmd);                                                          \
    } while (0)

#define link_executable(exe_file, obj_file)                                   \
    do {                                                                      \
        char *cmd =                                                           \
            compiler == msvc                                                  \
                ? concat(ld, " /nodefaultlib /out:", exe_file, " ", obj_file, \
                         " ", lib_file, msvc_nodefaultlib)                    \
                : concat(ld, " -o ", exe_file, " ", obj_file,                 \
                         " -L", bin_dir, " -l:", dll_file_name);              \
        async(cmd);                                                           \
        free(cmd);                                                            \
    } while (0)

#define e(__arg_base) (bin_dir __arg_base exeext)
#define o(__arg_base) (build_dir __arg_base objext)
#define c(__arg_base) (src_dir __arg_base ".c")
#define h(__arg_base) (src_dir __arg_base ".h")

void build() {
    mkdir(bin_dir);
    mkdir(build_dir);
    if (mtime(o("js-common")) < mtime(c("js-common"), h("js-common"))) {
        compile_library(o("js-common"), c("js-common"));
    }
    if (mtime(o("js-data")) < mtime(c("js-data"), h("js-data"), h("js-common"))) {
        compile_library(o("js-data"), c("js-data"));
    }
    if (mtime(o("js-vm")) < mtime(c("js-vm"), h("js-vm"), h("js-data"), h("js-common"))) {
        compile_library(o("js-vm"), c("js-vm"));
    }
    if (mtime(o("js-syntax")) < mtime(c("js-syntax"), h("js-syntax"), h("js-vm"), h("js-data"), h("js-common"))) {
        compile_library(o("js-syntax"), c("js-syntax"));
    }
    if (mtime(o("js")) < mtime(c("js"), h("js-syntax"), h("js-vm"), h("js-data"), h("js-common"))) {
        compile_executable(o("js"), c("js"));
    }
    await();
    if (mtime(dll_file_path) < mtime(o("js-common"), o("js-data"), o("js-vm"), o("js-syntax"))) {
        char *objs = join(" ", o("js-common"), o("js-data"), o("js-vm"), o("js-syntax"));
        char *cmd = compiler == msvc
                        ? concat(ld, " /nodefaultlib /dll /out:", dll_file_path, " ", objs, msvc_nodefaultlib)
                        : concat(ld, " -shared -o ", dll_file_path, " ", objs);
        async(cmd);
        free(cmd);
        free(objs);
    }
    await();
    if (mtime(e("js")) < mtime(o("js"), dll_file_path)) {
        link_executable(e("js"), o("js"));
    }
    await();
}

void cleanup(const char *dir, const char *base, const char *ext) {
    // printf("cleanup: %s%s%s\n", dir, base, ext);
    if (base) {
        char *file_name = concat(dir, base, ext);
        remove(file_name);
        free(file_name);
    } else {
        listdir(dir, cleanup);
        rmdir(dir);
    }
}

// DON'T use global pack option such as '/Zp', see banana-ui's make.c for explanation
#if compiler == msvc
    #define cc_debug "cl /nologo /c /W3 /MD /utf-8 /std:clatest" // "/Zp" means #pragma pack(1), DON'T use it
    #define cc_release cc_debug " /O2 /DNOLOGINFO /DNOTEST"
    #define ld_release "link /nologo /incremental:no" // LINK : xxx not found or not built by the last incremental link; performing full link
    #define ld_debug ld_release " /debug"
#else
    #define cc_debug "gcc -c -Wall" // "-fpack-struct" also means #pragma pack(1), but will cause lot's of "taking address of packed member may result in an unaligned pointer value" warning
    #define cc_release cc_debug " -O3 -DNOLOGINFO -DNOTEST"
    #define ld_debug "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc"
    #define ld_release ld_debug " -s -Wl,--exclude-all-symbols"
#endif

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && equals(argv[1], "debug"))) {
        cc = cc_debug;
        ld = ld_debug;
        build();
        return EXIT_SUCCESS;
    } else if (argc == 2) {
        if (equals(argv[1], "release")) {
            cc = cc_release;
            ld = ld_release;
            build();
            return EXIT_SUCCESS;
        } else if (equals(argv[1], "clean")) {
            listdir(bin_dir, cleanup);
            listdir(build_dir, cleanup);
            return EXIT_SUCCESS;
        } else if (equals(argv[1], "-h", "--help")) {
            ;
        } else {
            printf("Invalid target: %s\n", argv[1]);
        }
    } else {
        printf("Too many arguments\n");
    }
    printf("Usage: %s [debug|release|clean|-h|--help], default is debug\n", argv[0]);
    return EXIT_FAILURE;
}
