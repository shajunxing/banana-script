#include "../src/js-vm.h"
#include "../src/js-std-lang.h"
#include "../src/js-std-os.h"
#pragma comment(lib, "../bin/js.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")

struct js_result js_std_forward(struct js_vm *vm, uint16_t argc, struct js_value *argv) {
    js_assert(argc > 0);
    js_assert(js_is_function(argv));
    return js_call(vm, *argv, argc - 1, argv + 1);
}

int main(int argc, char *argv[]) {
    uint8_t bc[] = {
#include "16-hybrid-bc.txt"
    };
    uint32_t xref[] = {
#include "16-hybrid-xref.txt"
    };
    struct js_vm vm = js_static_vm(bc, xref);
    js_declare_argc_argv(&vm, argc, argv);
    js_declare_std_lang_functions(&vm);
    js_declare_std_os_functions(&vm);
    js_declare_variable_sz(&vm, "forward", js_c_function(js_std_forward));
    return js_default_routine(&vm);
}
