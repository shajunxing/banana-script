# 2025-07-28

Reconstructed dump/tostring/tojson functions. Added more std functions.

Standalized some beginner's functions: asked chatgpt, took most popular names as possible `input`, `print`, `read`, `write`

Standalized function naming rules.

Now support posix shebang

Added regular expression pattern matching

Simplified bind operator

# 2025-07-16

Reconstructed `make.c`, now support static build, and you can combine `debug` `release` with `static` `shared` as you wish. Added a new value type `vt_c_value`, which is managed, and can be notified on garbage collection, in your C code you can handle your custom data with it, see <https://github.com/shajunxing/banana-ui> for example.

More std functions.

Many bug fixes, such as js_call() a managed function with closure; throw in js_call() won't cleanup stack; in gc, c function's arguments should also be marked; op_ternary wrongly run both sides of ':', for example, 'a == null ? "Hello" : "Hello, " + a' will failed if a is null, now op_ternary is removed and replaced with op_jump family

remove 'inscription' string type, which is harmful, useless complexity, and i wan't strings always null terminated, or so many stdlib functions require null terminated string as argument will need to extra duplication and free operations, too complicated.

Finished string escape, except '\u', i think it is too complicated and useless, i keep it origin.

Finished detailed error source line number by cross reference

Linux build with readline library

# 2025-06-23

Huge reconstruction, including bytecode, project structure, `try` `catch`, enhanced C interaction ...

# 2025-02-10

After heavy code refactoring, from introducing token cache to using `js_value` instead of `js_value *` to reduce lots of memory allocation operation (structure assignment is about 10 times faster than memory allocation), to introducing bytecodes, performance vs Python reduced from dozen to about 5, now performance problems are mainly in hashmap operations such as `js_get_variable`. Maybe in the future variable access can be optimized to array operation.

# 2025-01-27

First release.