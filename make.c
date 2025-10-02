/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// DON'T use <make.h> because in linux, 'include' environment variable is not configured yet
#include "../banana-nomake/src/make.h"

#if os == posix
// suppress fucking stupid readline warnings "Using 'xxx' in statically linked applications requires at runtime the shared libraries ..."
    #define ex_opts "-lm -lreadline -lncurses -ltinfo -Wl,--no-warnings"
#else
// GetUserNameA requires advapi32.lib
    #define ex_opts "advapi32.lib"
#endif

void build() {
    if (mtime(o("js-common")) < mtime(c("js-common"), h("js-common"))) {
        cc_lib(o("js-common"), c("js-common"));
    }
    if (mtime(o("js-data")) < mtime(c("js-data"), h("js-data"), h("js-common"))) {
        cc_lib(o("js-data"), c("js-data"));
    }
    if (mtime(o("js-vm")) < mtime(c("js-vm"), h("js-vm"), h("js-data"), h("js-common"))) {
        cc_lib(o("js-vm"), c("js-vm"));
    }
    if (mtime(o("js-syntax")) < mtime(c("js-syntax"), h("js-syntax"), h("js-vm"), h("js-data"), h("js-common"))) {
        cc_lib(o("js-syntax"), c("js-syntax"));
    }
    if (mtime(o("js-std-lang")) < mtime(c("js-std-lang"), h("js-std-lang"), h("js-vm"), h("js-data"), h("js-common"))) {
        cc_lib(o("js-std-lang"), c("js-std-lang"));
    }
    if (mtime(o("js-std-os")) < mtime(c("js-std-os"), h("js-std-os"), h("js-vm"), h("js-data"), h("js-common"))) {
        cc_lib(o("js-std-os"), c("js-std-os"));
    }
    if (mtime(o("js")) < mtime(c("js"), h("js-std-lang"), h("js-std-os"), h("js-syntax"), h("js-vm"), h("js-data"), h("js-common"))) {
        cc_exe(o("js"), c("js"));
    }
    await();
    if (mtime(l("js")) < mtime(o("js-common"), o("js-data"), o("js-vm"), o("js-syntax"), o("js-std-lang"), o("js-std-os"))) {
        if (shared) {
            ld_lib(d("js"), o("js-common") " " o("js-data") " " o("js-vm") " " o("js-syntax") " " o("js-std-lang") " " o("js-std-os"));
        } else {
            ld_lib(l("js"), o("js-common") " " o("js-data") " " o("js-vm") " " o("js-syntax") " " o("js-std-lang") " " o("js-std-os"));
        }
    }
    await();
    if (mtime(e("js")) < mtime(o("js"), d("js"), l("js"))) {
        if (compiler == gcc && shared) {
            ld_exe(e("js"), o("js") " " d("js") " " ex_opts);
        } else {
            ld_exe(e("js"), o("js") " " l("js") " " ex_opts);
        }
        // add utf8 support for windows
        // see https://stackoverflow.com/questions/8831143/windows-api-ansi-functions-and-utf-8
        // and https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
        // since mt.exe always failed prompt such as "file in use", maybe antivirus issues, https://stackoverflow.com/questions/3775406/mt-exe-general-error-c101008d-failed-to-write-the-updated-manifest-to-the-res, use file copy instead
        if (compiler == msvc) {
            // await();
            // async("mt.exe -nologo -manifest src\\utf-8.manifest -outputresource:bin\\js.exe;#1");
            cp("bin\\js.exe.manifest", "src\\js.exe.manifest");
        }
    }
    await();
}

int main(int argc, char *argv[]) {
    return default_main(argc, argv, build);
}