#include "../src/js-vm.h"
#include "../src/js-std-lang.h"
#include "../src/js-std-os.h"
#pragma comment(lib, "../bin/js.lib")
#pragma comment(lib, "advapi32.lib")

int main(int argc, char *argv[]) {
    uint8_t bc[] = {
#include "20-source-1-bc.txt"
    };
    uint32_t xref[] = {
#include "20-source-1-xref.txt"
    };
    struct js_vm vm = {
        .bytecode = {
            .base = bc,
            .length = sizeof(bc),
            .capacity = sizeof(bc),
        },
        .cross_reference = {
            .base = xref,
            .length = sizeof(xref),
            .capacity = sizeof(xref),
        },
    };
    js_std_argc = argc;
    js_std_argv = argv;
    js_declare_std_lang_functions(&vm);
    js_declare_std_os_functions(&vm);
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
}