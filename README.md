# Banana JS, An inperpreter for a strict subset of JavaScript

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

![REPL screenshot](screenshot1.png "REPL screenshot")

![REPL screenshot](screenshot2.png "REPL screenshot")

## 2025.02.10

After heavy code refactoring, from introducing token cache to using `js_value` instead of `js_value *` to reduce lots of memory allocation operation (structure assignment is about 10 times faster than memory allocation), to introducing bytecodes, performance vs python reduced from dozen to about 5, now performance problems are mainly in hashmap operations such as `js_variable_get`. Maybe in the future variable access can be optimized to array operation.

## Introduction

My goal is to weed out useless and ambiguous parts of JavaScript language that I've summarized in practice, and to create a minimal syntax interpreter by keeping only what I like and need. **Only JSON-compatible data types and function are supported, function is first-class value, and function supports closure. I don't like object-oriented programming, so everything class related are not supported**. There are no built-in immunable global variables, global functions, or object members, even contents added during interpreter initialization can be easily deleted at any time and reverted to clean empty state.

## Build

This project is C89 compatable, no other dependences, even make systems are not necessary, only need C compiler, currently tested on msvc and mingw. First, download my another project [Banana Make](https://github.com/shajunxing/banana-make), this is only one single file `make.h`, then open `make.c`, modify `#include` to correct path, then with msvc type `cl make.c && make.exe release`, or with mingw type `gcc -o make.exe make.c && ./make.exe release`. Executables are in `bin` folder, includeing REPL environment, script executor, a funny toy calculator and lexer parser test tools.

## Syntax

Data types are `null, boolean, number, string, array, object, function`, results of `typeof` correspond strictly to these names. No `undefined` because `null` is enough. Array and object are clean, no predefined members such as prototypes.

Variable declaraction use `let`, all variables are local. Access undeclared variables will cause error, and access array/object's unexisting members will get `null`. `const` is not supported because it's meaningless, for example, a constant pointed to an array/object, should it's member be immunable? if not, why should I keep `const`? if so, value must be marked immunable, but it's weird in point of garbage collector's view, because all values should be cleanable.

Function definition supports default parameter `param = value` and rest parameter `...params`. Array literal and function call support spread syntax `...`, which will not skip `null` members. No `this, arguments` in function.

Operators follow strict rule, no implicit conversion. Only boolean can do logical operations. `== !=` are strict meaning, and can be done by all types. Strings can do all relational operations and `+`. Numbers can do all relational and numerical operations. Operator precedence from low to high is:

- Ternary operator `? :`
- Logical or operator `||`
- Logical and operator `&&`
- Relational operator `== != < <= > >=`
- Additive operator `+ -`
- Multiplicative operator `* / %`
- Exponential operator `**`
- Prefix operator `+ - ! typeof`
- Array/object member access and function call operator `[] . ?. ()`

Assignment expression `= += -= *= /= %= ++ --` does not return value, Comma expression `,` is not supported.

`if`, `while`, `do while`, `for`'s condition must be boolean. `for` loop only support following syntax, `[]` means optional. `for in` and `for of` only handle non-null members:

- `for ([[let] variable = expression ] ; [condition] ; [assignment expression])`
- `for ([let] variable in array/object)`
- `for ([let] variable of array/object)`

No modules. In inperpreter's view, source code is only one large flat text.

Garbage collection is manual, you can do it at any time you need.

`delete` is semantic different from JavaScript, which removes object members, but this makes no sense because you just need to set them to `null`. Here `delete` You can delete local variables within current scope. For example, it can be used with garbage collector to empty whole execution environment to nothing left. For another example, variables added to the function closure are all local variables before return, so unused variables can be `delete`d before return to reduce closure size, run following two statements in REPL environment to see differences.

- `let f = function(a, b){let c = a + b; return function(d){return c + d;};}(1, 2); dump(); print(f(3)); delete f;`
- `let f = function(a, b){let c = a + b; delete a; delete b; return function(d){return c + d;};}(1, 2); dump(); print(f(3)); delete f;`

## Standard library and interoperability with C language

All values are `struct js_value` type, you can create by `js_xxx()` functions, `xxx` is value type, and you can read c values direct from this struct, see definition in `js.h`. All created values are managed by `struct js *`, core context of this engine, following garbage collecting rules. DON'T directly modify their content, if you want to get different values, create new one. Compound types `array object` can be operated by `js_array_xxx() js_object_xxx()` functions.

C functions must be `void (*)(struct js *)` format, use `js_c_function()` to create c function value, yes of course they are all values and can be put anywhere, for example, if put on stack root using `js_variable_declare()`, they will be global. Inside C functions, parameters passed in can be obtained by `js_parameter_length() js_parameter_get()` functions, and return value just put to `result` of `struct js *`.

## Technical internals

There are three types of string: `vt_scripture` means immuable `const char *` written in engine c source code, eg. `typeof` result, `vt_inscription` means immuable string loaded from javascript source code such as variable identifier, string literals, they are stored in engine context's `tablet`, and `vt_string` are mutable. These three string types can be used for futher optimization, for example, string split can be optimized for `vt_scripture` and `vt_inscription`.

Value types `vt_string`, `vt_array`, `vt_object` and `vt_function` are hang on engine context's `heap`, and managed by garbage collector. Why `vt_function` is managed is because it has closure.

Variable scope is combined into call stack. Call stack has following types: `cs_root` is root stack, which is unique and not deletable, `cs_block` means block statement scope, `cs_for` is for loop scope to fit `let` keyword, `cs_function` is function scope and in which `params` and `ret_addr` are available.

Hashmap operation `js_value_map_put`'s algorithm:

    loop key      loop value      new value      operation
    -----------------------------------------------------------
    null          not null                       fatal, shouldn't happen
    null          not null                       fatal, shouldn't happen
    null          null            not null       add key value, may need rehash, return
    null          null            null           no op, return
    matched       not null        not null       replace value, return
    matched       not null        null           replace value, len--, return
    matched       null            not null       replace value, len++, return
    matched       null            null           return
    not matched   not null                       continue
    not matched   not null                       continue
    not matched   null            not null       next stage // replace key value, len++, return
    not matched   null            null           next stage // replace key, return
    whole loop ended                             fatal, shouldn't happen
    whole loop ended                             fatal, shouldn't happen

may encounter "not matched null" -> "not matched null" -> ... -> "matched", if operate with first result, will cause duplicate, so "not matched null" may enter next stage, record position, and special treat:

    null          null            not null       replace key value to recorded position, len++, return
    not matched                                  continue
    whole loop ended                             replace key value to recorded position, len++, return

when handling rehash, DONT return a new map like realloc(), that's very stupid, if this map is another data structute's element, it will become wild pointer

val can be NULL. If key does not exist, will skip. If key exists, means delete operation, set val to NULL, k unchanged, so that rehash no needed, to make sure find loop won't break if following keys exists

"len" means number of keys in map, it is meanless for user because NULL val exists, DONT use it outside

when rehash, not null key null value will not be added

EBNF

https://www.cnblogs.com/dhy2000/p/15970225.html
https://zh.wikipedia.org/wiki/%E9%80%92%E5%BD%92%E4%B8%8B%E9%99%8D%E8%A7%A3%E6%9E%90%E5%99%A8
https://gist.github.com/Chubek/0ab33e40b01a029a7195326e89646ec5

https://www.json.org/json-en.html
https://lark-parser.readthedocs.io/en/latest/json_tutorial.html

object key can be identifier, which means string, NOT identifier corresponding value

function rest parameter support:
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/rest_parameters
spread syntax support:
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax

    value = null | boolean | number | string | array | object | function | c_function
    array = '[' [expression|'...'access_call_expression (',' expression|'...'access_call_expression)] ']'
    object = '{' [string|identifier ':' expression (',' string|identifier ':' expression)] '}'
    function = 'function' _function
    _function = '(' [identifier[=expression] [,identifier[=expression]]][...identifier] ')' '{' { statement } '}'

Operator precedence
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Operator_precedence

    18: grouping: ()
    17: access and call: [] . ?. function()
    15: postfix operators ++ --
    14: prefix operators: !
    13: **
    12: * / %
    11: + -
    9: < <= > >=
    8: == !=
    4: &&
    3: ||
    2. = += ?: 
    1. ,

I modify it:

    (*high*)
    accessor = (identifier|'('expression')'|value){'['additive_expression']'|('.'|'?.')identifier|'('[expression|'...'access_call_expression[,expression|'...'access_call_expression]]')'}
    access_call_expression = accessor (* as rvalue *)
    prefix_expression = ['!'|'+'|'-'] access_call_expression
    exponentiatial_expression = prefix_expression {'**' prefix_expression}
    multiplicative_expression = exponentiation_expression {('*'|'/'|'%') exponentiation_expression}
    additive_expression = multiplicative_expression {('+'|'-') multiplicative_expression}
    relational_expression = additive_expression [('=='|'!='|'<'|'<='|'>'|'>=') additive_expression]
    logical_and_expression = relational_expression {'&&' relational_expression}
    logical_or_expression = logical_and_expression {'||' logical_and_expression}
    expression = logical_or_expression ['?' logical_or_expression ':' logical_or_expression]
    (*low*)

xxx_expression is only name from LOWEST precedence to HIGHEST, for example, relational_expression can also be numerical value, lowest name expression is shortened to be expression

only inside () [] can start from expression, elsewhere expression to prevent = , conflict

DONT add comma_expression and DONT put assignment_expression into the chain, because left value has to be lazyed, it is too complicated to deliver lazy evaluation literal representation such as 'foo["bar"]' in each chain, and more and more complicated, for example, have to save object key as dynamic, because it may be a result.

string can do following operations:

    + means strcat()
    < <= > >= == != means strcmp(), -1 is <, 0 is ==, 1 is >

https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Strict_equality

    == means strict equals
    If the operands are of different types, return false.
    If both operands are objects, return true only if they refer to the same object.
    If both operands are null or both operands are undefined, return true.
    If either operand is NaN, return false.
    Otherwise, compare the two operand's values:
    Numbers must have the same numeric values. +0 and -0 are considered to be the same value.
    Strings must have the same characters in the same order.
    Booleans must be both true or both false.

Standalone, not belong to expressions chain, used in 'for' loop
use 'expression' to prevent conflict, still can down to () to include them:

    assignment_expression = accessor(* as lvalue *) [ '='|'+='|'-='|'*='|'/='|'%='|'++'|'--' expression ]
    declaration_expression = 'let' identifier['='expression] { ',' identifier['='expression] }

    script = { statement }
    statement = ';'
            | '{' { statement } '}'
            | 'if' '(' expression ')' statement ['else' statement]
            | 'while' '(' expression ')' statement
            | 'do' statement 'while' '(' expression ')' ';'
            | 'for' '(' ( (('let' identifier)|accessor) (('='expression';'[expression]';'[assignment_expression])|('in'|'of'access_call_expression)) ) | (';'[expression]';'[assignment_expression])  ')' statement
            | 'break' ';'
            | 'continue' ';'
            | 'function' identifier _function
            | 'return' [expression] ';'
            | 'delete' identifier ';'
            | declaration_expression ';'
            | assignment_expression ';'

assignment_expression put at last, because it cannot be determined by first token

function is variable, function scope is same as variable, can only visit same or parent levels

    let a;
    function foo () {
        let b;
        function qux() {}
        function bar() {
            let c;
            function baz() {
            }
        }
    }

for loop types in parser:

    for (let a = b;
    for (let a in
    for (let a of
    for (;
    for (a = b;
    for (a in
    for (a of

DONT use inline, even slower in mingw and size increased about 10k