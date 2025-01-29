/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../banana-make/make.h"

#define prefix "js"
#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define src_dir "src" pathsep
#define examples_dir "examples" pathsep
#define private_dir "private" pathsep
#define header_file src_dir prefix ".h"
#define dll_file_name prefix dllext
#define dll_file_path bin_dir dll_file_name
#define lib_file bin_dir prefix libext
double header_mtime = -DBL_MAX;
char *obj_files = NULL;
char *cc_msvc = NULL;
char *cc_gcc = NULL;
char *link_msvc = NULL;
char *link_gcc = NULL;
int link_required = 0;

void compile_file(const char *src_file, const char *obj_file) {
    char *cmd = compiler == msvc ? concat(cc_msvc, " /DDLL /DEXPORT /Fo", obj_file, " ", src_file) : concat(cc_gcc, " -D DLL -D EXPORT -o ", obj_file, " ", src_file);
    run(cmd);
    free(cmd);
}

void compile(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *src_file = concat(dir, base, ext);
        double src_mtime = mtime(src_file);
        char *obj_file = concat(build_dir, base, objext);
        append(&obj_files, " ", obj_file);
        if (max(header_mtime, src_mtime) > mtime(obj_file)) {
            compile_file(src_file, obj_file);
            link_required = 1;
        }
        free(obj_file);
        free(src_file);
    }
}

void build_example(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *src_file = concat(dir, base, ext);
        double src_mtime = mtime(src_file);
        char *obj_file = concat(build_dir, base, objext);
        char *exe_file = concat(bin_dir, base, exeext);
        if (max(header_mtime, src_mtime) > mtime(exe_file) || link_required) {
            char *cmd;
            compile_file(src_file, obj_file);
            cmd = compiler == msvc ? concat(link_msvc, " /out:", exe_file, " ", obj_file, " ", lib_file) : concat(link_gcc, " -o ", exe_file, " ", obj_file, " -L", bin_dir, " -l:", dll_file_name);
            run(cmd);
            free(cmd);
        }
        free(exe_file);
        free(obj_file);
        free(src_file);
    }
}

void build() {
    header_mtime = mtime(header_file);
    obj_files = (char *)calloc(1, 1);
    // for msvc, use /W3 instead of /Wall
    // https://stackoverflow.com/questions/4001736/whats-up-with-the-thousands-of-warnings-in-standard-headers-in-msvc-wall
    // https://stackoverflow.com/questions/28985515/is-warning-c4127-conditional-expression-is-constant-ever-helpful
    // https://stackoverflow.com/questions/12501392/why-does-the-compiler-complain-about-the-alignment
    // cl use /link to pass parameters to linker, but must place at the end, so give up
    // https://learn.microsoft.com/en-us/cpp/build/reference/link-pass-options-to-linker?view=msvc-170
    listdir(src_dir, compile); // compilation stage
    if (link_required || mtime(dll_file_path) == 0) { // linking stage
        char *cmd = compiler == msvc ? concat(link_msvc, " /dll /out:", dll_file_path, obj_files) : concat(link_gcc, " -shared -o ", dll_file_path, obj_files);
        run(cmd);
        free(cmd);
    }
    free(obj_files);
    listdir(examples_dir, build_example); // examples
    listdir(private_dir, build_example);
}

void build_debug() {
    cc_msvc = "cl /nologo /c /W3 /MD";
    cc_gcc = "gcc -c -Wall -std=gnu2x";
    link_msvc = "link /nologo /debug";
    link_gcc = "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
    build();
}

void build_ndebug() {
    cc_msvc = "cl /nologo /c /W3 /MD /DNDEBUG";
    cc_gcc = "gcc -c -Wall -std=gnu2x -DNDEBUG";
    link_msvc = "link /nologo /debug";
    link_gcc = "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
    build();
}

void build_release() {
    cc_msvc = "cl /nologo /c /O2 /W3 /MD /DNDEBUG";
    cc_gcc = "gcc -c -O3 -Wall -std=gnu2x -DNDEBUG";
    link_msvc = "link /nologo";
    link_gcc = "gcc -s -Wl,--exclude-all-symbols -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
    build();
}

void cleanup(const char *dir, const char *base, const char *ext) {
    if (base) {
        char *file_name = concat(dir, base, ext);
        printf("Delete %s\n", file_name);
        remove(file_name);
        free(file_name);
    } else {
        listdir(dir, cleanup);
        printf("Delete %s\n", dir);
        rmdir(dir);
    }
}

void clean() {
    listdir(bin_dir, cleanup);
    listdir(build_dir, cleanup);
}

void install() {
    puts("Install");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        build_debug();
    } else if (argc == 2) {
        if (equals(argv[1], "-h") || equals(argv[1], "--help")) {
            printf("Usage: %s [debug|ndebug|release|clean|install]\n", argv[0]);
        } else if (equals(argv[1], "debug")) {
            build_debug();
        } else if (equals(argv[1], "ndebug")) {
            build_ndebug();
        } else if (equals(argv[1], "release")) {
            build_release();
        } else if (equals(argv[1], "clean")) {
            clean();
        } else if (equals(argv[1], "install")) {
            install();
        }
    }
}
