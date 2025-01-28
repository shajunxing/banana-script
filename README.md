# Banana JS, An inperpreter for a strict subset of JavaScript

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

![REPL screenshot](screenshot.png "REPL screenshot")

## Features

My goal is to weed out useless and ambiguous parts of JavaScript language that I've summarized in practice, and to create a minimal syntax interpreter by keeping only what I like and need. **Only JSON-compatible data types and function are supported, function is first-class value, and function supports closure. I don't like object-oriented programming, so everything class related are not supported.** There are no built-in immunable global variables, global functions, or object members, even contents added during interpreter initialization can be easily deleted at any time and reverted to clean empty state.

## Build

This project is C89 compatable, no other dependences, even make systems are not necessary, only need C compiler, currently tested on msvc and mingw. First, download my another project [Banana Make](https://github.com/shajunxing/banana-make), this is only one single file `make.h`, then open `make.c`, modify `#include` to correct path, then with msvc type `cl make.c && make.exe release`, or with mingw type `gcc -o make.exe make.c && ./make.exe release`. Executables are in `bin` folder, includeing REPL environment, script executor, a funny toy calculator and lexer parser test tools.

## Syntax

Data types are `null, boolean, number, string, array, object, function`, results of `typeof` correspond strictly to these names. No `undefined` because `null` is enough. Array and object are clean, no predefined members such as prototypes.

Variable declaraction use `let`, all variables are local. Access undeclared variables will cause error, and access array/object's unexisting members will get `null`. `const` is not supported because it's meaningless, for example, a constant pointed to an array/object, should it's member be immunable? if not, why should I keep `const`? if so, value must be marked immunable, but it's weird in point of garbage collector's view, because all values should be cleanable.

Function definition supports default parameter `param = value` and rest parameter `...params`. Array literal and function call support spread syntax `...`, which will not skip `null` members. No `this, arguments` in function.

Operators follow strict rule, no implicit conversion. Only boolean can do logical operations. `== !=` are strict meaning, and can be done by all types. Strings can do all relational operations and `+`. Numbers can do all relational and numerical operations. Operator precedence from low to high is:

- Ternary operator `? :`
- `&& ||`
- `== != < <= > >=`
- `+ -`
- `* / %`
- Prefix operator `+ - ! typeof`
- Array/object member access and function call operator `[] . ?. ()`

Assignment expression `= += -= *= /= %= ++ --` does not return value, Comma expression `,` is not supported.

`if`, `while`, `do while`, `for`'s condition must be boolean. `for` loop only support following syntax, `[]` means optional. `for in` and `for of` only handle non-null members:

- `for ([[let] variable = expression ] ; [condition] ; [assignment expression])`
- `for ([let] variable in array/object)`
- `for ([let] variable of array/object)`

No modules. In inperpreter's view, source code is only one large flat text.

Garbage collection is manual, you can do it at any time you need.

`delete` is semantic different from JavaScript, which removes object members, but this makes no sense because you just need to set them to `null`. Here `delete` can delete local variables on current stack level. For example, it can be used with garbage collector to empty whole execution environment to nothing left. For another example, variables added to the function closure are all local variables before return, so unused variables can be `delete`d before return to reduce closure size, run following two statements in REPL environment to see differences.

- `let f = function(a, b){let c = a + b; return function(d){return c + d;};}(1, 2); dump(); print(f(3)); delete f;`
- `let f = function(a, b){let c = a + b; delete a; delete b; return function(d){return c + d;};}(1, 2); dump(); print(f(3)); delete f;`

## Standard library and interoperability with C language

All values are `struct js_value *` type, you can create by `js_xxx_new()` functions, `xxx` is value type, and you can read c values direct from this struct, see definition in `js.h`. All created values are managed by `struct js *`, core context of this engine, following garbage collecting rules. DON'T `free()` them by yourself and DON'T directly modify primitive types `null boolean number string`, if you want to get a different value, create a new one. Compound types `array object` can be modified by `js_array_xxx() js_object_xxx()` functions.

C functions must be `void (*)(struct js *)` format, use `js_cfunction_new()` to create c function value, yes of course they are all values and can be put anywhere, for example, if put on stack root using `js_variable_declare()`, they will be global. Inside C functions, parameters passed in can be obtained by `js_parameter_length() js_parameter_get()` functions, and return value just put to `result` member of `struct js *`.

