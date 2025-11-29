# Banana Script, a script engine with minimalist syntax similar to JavaScript, implemented in C99.

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

Project Address: <https://github.com/shajunxing/banana-script>

```
Banana Script REPL environment. Copyright (C) 2024-2025 ShaJunXing
Type '/?' for more information.

> print({"foo":true,"bar":[null,false,{"baz":function(a){return function(b){return function(c){return a+b+c;};};}}]}?.bar[2]["baz"]("How ")("are ")("you?"));
How are you?

> [1, 100, 3, 10, 2]::map(tostring)::sort(natural_compare)::join("-")::tojson()::print();
"1-2-3-10-100"

> print(format("${0}|${1}|${2}|${3}", ...match("Unknown-14886@noemail.invalid", "^([\\w\\.-]+)\\@([\\w-]+)\\.([a-zA-Z\\w]+)$")));
Unknown-14886@noemail.invalid|Unknown-14886|noemail|invalid

>
```

## Features

My goal is to remove and modify useless and ambiguous parts of JavaScript language that I've summarized in practice, and to create a minimal syntax interpreter by keeping only what I like and need. Function is first-class value, and function supports closure. I don't like object-oriented programming, so everything class-related is not supported, but I've redefined proposal's **double colon binding operator** (<https://github.com/tc39/proposal-bind-operator> <https://babeljs.io/docs/babel-plugin-proposal-function-bind>) , now `value::function(... args)` is equivalent to `function(value, ... args)`, so Class Lovers will be happy because it will make easy to write beautiful chain syntax styles.

## Two-Minutes Brief Syntax Guide

Data types are `null` `boolean` `number` `string` `array` `object` `function`, results of `typeof` correspond strictly to these names. No `undefined` because `null` is enough. Array and object are clean, no predefined members such as `__proto__`.

Variable declaraction use `let`, all variables are local, `const` is not supported because all must be deletable. Access undeclared variables will cause error, access array/object's unexisting members will get `null` (including array index are negative and non-integer, but these are forbidden for put operation), and put `null` will delete corresponding member.

Function definition supports `function` keyword, does not support `=>` expression, support default argument `param = value` and rest argument `...args`. Array literal and function call support spread syntax `...`. No predefined members such as `this` `arguments` in function. If `return` is in global scope, means exit vm.

Operators follow strict rule, no implicit conversion. Only boolean can do logical operations. `== !=` are strict meaning, and can be done by all types. Numbers can do all relational and numerical operations, strings can do all relational operations and `+`. Operator precedence from low to high is:

- Ternary operator `?` `:`
- Logical or operator `||`
- Logical and operator `&&`
- Relational operator `==` `!=` `<` `<=` `>` `>=`
- Additive operator `+` `-`
- Multiplicative operator `*` `/` `%`
- Exponential operator `**`
- Prefix operator `+` `-` `!` `typeof`
- Array/object member access and function call operator `[]` `.` `?.` `()` `::`

Assignment expression `=` `+=` `-=` `*=` `/=` `%=` `++` `--` does not return value, Comma expression `,` is not supported.

Conditional statement is `if`, loops are `while` `do while` `for`, conditions must be boolean. `for` loop only support following syntax, `[]` means optional. `for in` and `for of` only handle non-null members:

- `for ([[let] variable = expression ] ; [condition] ; [assignment expression])`
- `for ([let] variable in array/object)`
- `for ([let] variable of array/object)`

No modules. In inperpreter's view, source code is only one large flat text.

Garbage collection is manual, you can do it at any time.

`delete` means delete local variable within current scope (object members can be deleted by setting `null`). For example, variables added to the function closure are all local variables before function variable declaration, so unused variables can be deleted before return to reduce closure size, run following two statements in REPL environment to see differences.

- `gc();let f=function(a,b){let c=a+b;return function(d){return c+d;};}(1,2);dump_vm();print(f(3));delete f;`
- `gc();let f=function(a,b){let c=a+b;delete a;delete b;return function(d){return c+d;};}(1,2);dump_vm();print(f(3));delete f;`

`throw` can throw any value, which are received by optional `catch`. `finally` is not supported, because I think it's totally unecessary, and will make code execution order weird.

## Project Structure And Interoperability With C Language

This project is compatible with C99, and compilation environments are msvc/gcc/mingw, relying only on C compiler and <https://github.com/shajunxing/banana-nomake>, without need for make system. For msvc, execute `cl make.c && make.exe release`, or for mingw/gcc, execute `gcc -o make.exe make.c && ./make.exe release`. Generated executable will be located in `bin` directory.

Project follows "minimal dependency" rule, only including necessary headers. Also, there's only one-way referencing between modules, with no circular referencing. Here’s modules' dependencies and how they work:

```
js-common   js-data     js-vm       js-syntax   js-std-...
    <-----------
                <-----------
                            <-----------
                            <-----------------------
```

- `js-common`: Constants, macro definitions, and functions common to project, such as log printing, memory operations.
- `js-data`: Data types and garbage collection, you can even use this module separately in C projects to manipulate high-level data structures with GC functionality, see <https://github.com/shajunxing/banana-cvar>.
- `js-vm`: Bytecode virtual machine, compiled separately to get an interpreter with minimal footprint without source code parsing.
- `js-syntax`: Lexical parsing and syntax parsing, which converts source code into bytecode.
- `js-std-...`: Reference implementation of commonly used standard functions, which can be used as reference for writing C functions.

All values are `struct js_value` type, you can create by `js_...()` functions, `...` is value type, and you can read c values direct from this struct, see definition in `js_data.h`. DON'T directly modify their content, if you want to get different values, create new one. Compound types `array` `object` can be operated by `js_..._array_...()` `js_..._object_...()` functions.

C functions must be `typedef struct js_result (*js_c_function_type)(struct js_vm *vm, uint16_t argc, struct js_value *argv)` format, read passed arguments from `argc` `argv`, and `struct js_result` has two members, if `.success` is `true`, `.value` is return value, if `false`, `.value` is thrown error. Use `js_c_function()` to create c function value, yes of course they are all values and can be put anywhere, for example, if put on stack root using `js_declare_variable()`, they will be global. C function can also call script function using `js_call()`, `js_call_by_name()` and `js_call_by_name_sz()`.

## Standard Library

Includes most commonly used functions of language and os level. You can understand them as 'reference implementations', with no guarantee that they will remain unchanged in future. For more info, check out <https://github.com/shajunxing/banana-script/blob/main/examples/7-std.js>.

Naming rules:

1. Most commonly used ones, take most commonly used names, like console input and output, which are `input` and `print`. They're easiest to remember. I even asked ChatGPT helping me querying their usage percentage.
2. Single featured ones, match per dos/unix command, or c std/unistd function, such as `cd` `md` `rd`, use shortest one, which can also improve speed.
3. Non-single featured ones, customize names.

Values and function definition conventions:

- -: null, function with no return value actually returns null
- b: boolean
- n: number
- s: string
- (): function
- []: array, or optional parameter
- {}: object
- *: any types
- /: multiple types or names seperator
- ...: unlimited arguments

Language:

|Definition________________________|Description|
|-|-|
|n ceil(n val)|Same as C `ceil`.|
|dump_vm()|Print vm status.|
|b endswith(s str, s sub, s ...)|Determine whether string ends with any of sub strings.|
|[* ...] filter([* ...] arr, b func(* elem))|For each element of `arr`, as argument, call `func`, if returns `true`, this element will be appended to result array.|
|n floor(n val)|Same as C `floor`.|
|s format(s fmt, * ...)|Format with `fmt`, there are two types of replacement field, first is `${foo}` where `foo` is variable name, second is `${0}` `${1}` `${2}` ... where numbers indicates which argument followed by, starts from 0, and will be represented as `tostring()` style.|
|gc()|Garbage collection.|
|s join([s ...] arr, s sep)|Join string array with seperator.|
|n length([* ...]/{* ...}/s val)|Returns array/object length or string length in bytes.|
|[* ...] map([* ...] arr, * func(* elem))|For each element of `arr`, as argument, call `func`, returned value will be appended to result array.|
|[s ...]/- match(s text, s pattern)|Regular expression matching. If matched returns all captures, otherwise returns `null`. Currently supports `^` `$` `()` `\d` `\s` `\w` `.` `[]` `-` `*` `+` `?`.|
|[n, n] modf(n val)|Same as C `modf`, returns array of integral and fractional parts.|
|n natural_compare(s lhs, s rhs)|Natural-compare algorithm, used by `sort()`.|
|* pop([* ...] arr)|Removes array's last element and returns.|
|push([* ...] arr, * elem)|Add element to end of array.|
|* reduce([* ...] arr, * func(* lhs, * rhs))|Initial return value is `null`. For each element of `arr`, if is first element, replace return value, or call `func` with return value as `lhs` and element as `rhs` and replace return value with it's return value.|
|n round(n val)|Same as C `round`.|
|[* ...] sort([* ...] arr, n comp(* lhs, * rhs))|Same as C `qsort()`, array will be sorted and also be returned.|
|[s ...] split(s str, [s sep])|Split string into array. If `sep` is omitted, returns array containing original string as single element. If `sep` is empty, string will be divided into bytes.|
|b startswith(s str, s sub, s ...)|Determine whether string starts with any of sub strings.|
|s todump(* val)|Returns dump representation of any value.|
|s tojson(* val)|Returns json representation of any value.|
|s tolower(s str)|Convert `str` to lower case, use C `tolower()`.|
|n tonumber(s str)|Convert string represented number to number.|
|s tostring(* val)|Returns string representation of any value.|
|s toupper(s str)|Convert `str` to upper case, use C `toupper()`.|
|n trunc(n val)|Same as C `trunc`.|

Operating system:

|Definition________________________|Description|
|-|-|
|n argc|Same as C `main(argc, argv)`.|
|[s ...] argv|Same as C `main(argc, argv)`.|
|s basename(s path)|Same as POSIX `basename()`, returns final component of `path`.|
|cd(s path)|Same as POSIX `chdir()`.|
|n clock()|Same as C `clock()`, returns process time as second.|
|s ctime(n time)|Same as C `ctime()`, `time` is unix epoch, which means seconds elapsed since utc 1970-01-01 00:00:00 +0000|
|s cwd()|Same as POSIX `getcwd()`.|
|s dirname(s path)|Same as POSIX `dirname()`, returns parent directory of `path`.|
|exec(s arg, s ...)|Same as POSIX `execvp()`, but first parameter `file` is automatically filled with `argv[0]`. POSIX only.|
|b exists(s path)|Checks if file `path` exists.|
|exit(n status)|Same as C `exit()`, `status` will be cast to integer.|
|n fork()|Same as POSIX `fork()`. POSIX only.|
|s input([s prompt])|Prompt (optional) and accepts line of user input. If you need number, use `tonumber()` to convert.|
|ls(s dir, cb(s fname, b isdir))|List directory and with each entry call `cb`.|
|md(s path)|Same as POSIX `mkdir()`|
|s os|`windows` or `posix`|
|s pathsep|`\` or `/`|
|play(s filename)|Play .wav file. Windows only.|
|print(...)| prints zero or more values, separated by spaces, with newline at end. There are three styles for how values are represented as string, from simple to complex: `tostring()`, `tojson()` and `todump()`. Default uses first one.|
|s read(n fp)<br>s read(s fname)<br>s read(s fname, b iscmd)<br>read(n fp, cb(s line))<br>read(s fname, cb(s line))<br>read(s fname, b iscmd, cb(s line))|Generic reading function for text file or console process, which takes file handle `fp`, or file name (or command line if `iscmd` is true) `fname`. If no `cb` exist, will returns whole content, or will call it repeatly with each line as argument.|
|rd(s path)|Same as POSIX `rmdir()`|
|rm(s path)|Same as POSIX `rm()`|
|sleep(n timeout)<br>sleep(n timeout, cb(n remains))<br>sleep(n timeout, cb(n remains), n interval)|Sleep certain seconds. If `cb` exists, call it each `interval` seconds, and pass `remains` seconds as argument. Default `interval` is 1 second.|
|spawn(s arg, s ...)|Create new process, parameters are same as `exec()`.|
|{} stat(s path)|Same as POSIX `stat()`. Return value currently contains `size` `atime` `ctime` `mtime` `uid` `gid`.|
|n system(s cmd)|Same as C `system()`.|
|n stdin|Same as C `stdin`|
|n stdout|Same as C `stdout`|
|n stderr|Same as C `stderr`|
|n time()|Same as C `time()` but high precision, returns unix epoch.|
|title(s text)|Set console title. Windows only.|
|s whoami()|Get current user name.|
|write(n fp, s text)</br>write(s fname, s text)</br>write(s fname, b isappend, s text)</br>|Generic writing function for text file, which takes file handle `fp`, or file name `fname`. `isappend` means append mode instead of overwrite mode.|

## Execution Modes

There are three execution modes:

### Run In Source Code

For example, there are two source code files, 20-source-0.js：

```js
function foo() {
    print("Greetings.");
    throw "Boom!";
}
```

20-source-1.js：

```js
foo();
```

Can run these two files by following command:

```
js 20-source-0.js 20-source-1.js
```

Output:

```
Greetings.
Runtime Error: {'line':3,'message':'Boom!'}
```

### Compile To Bytecode

If add `-c` parameter, means compile:

```
js -c 20-source-0.js 20-source-1.js -c
```

Will generate following files:

```
Bytecode written to:
    examples\20-source-1-bc.bin
    examples\20-source-1-bc.txt
Cross reference written to:
    examples\20-source-1-xref.bin
    examples\20-source-1-xref.txt
```

Suffix with `-bc` are bytecode files, suffix with `-xref` are cross reference files. Can be run by following command:

```
js -b 20-source-1-bc.bin -x 20-source-1-xref.bin
```

Same output:

```
Greetings.
Runtime Error: {'line':3,'message':'Boom!'}
```

Cross reference files are optional, if not specified, will not know runtime errors corresponding lines, for example:

```
js -b 20-source-1-bc.bin
```

Output:

```
Greetings.
Runtime Error: 'Boom!'
```

### Compile To Standalone Executables

`.txt` files can be included into custom c codes, for example 20-source.c：

```c
#include "../src/js-vm.h"
#include "../src/js-std-lang.h"
#include "../src/js-std-os.h"
#pragma comment(lib, "../bin/js.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")

int main(int argc, char *argv[]) {
    uint8_t bc[] = {
#include "20-source-1-bc.txt"
    };
    uint32_t xref[] = {
#include "20-source-1-xref.txt"
    };
    struct js_vm vm = js_static_vm(bc, xref);
    js_declare_argc_argv(&vm, argc, argv);
    js_declare_std_lang_functions(&vm);
    js_declare_std_os_functions(&vm);
    return js_default_routine(&vm);
}
```

Can use msvc generate 20-source.exe and execute it：

```
cl 20-source.c && 20-source.exe
```

Same output:

```
Greetings.
Runtime Error: {'line':3,'message':'Boom!'}
```

Another example demonstrate how to declare c function `forward()`, 16-hybrid.c:

```c
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
```

This `forward()` function is same meaning as js code`function forward(func, ...args) { return func(...args); }`. It is inviked in 16-hybrid.js:

```js
forward(function(...args) {
    forward(function(...args) {
        forward(function(...args) {
            print(...args);
        }, ...args);
    }, ...args);
}, null, true, 3.14, "hello");
```

Run following command:

```
js -c 16-hybrid.js && cl 16-hybrid.c && 16-hybrid.exe
```

To see output:

```
null true 3.14 hello
```

## Projects Using This

<https://github.com/shajunxing/view-comic-here>

## Unorganized

There are 2 types of string: `vt_scripture` means immutable c string literal in engine c source code, eg. `typeof` result, and `vt_string` are mutable. They are all null terminated. They can be used for futher optimization.

Value types `vt_string`, `vt_array`, `vt_object` and `vt_function` are hang on engine context's `heap`, and managed by garbage collector. Why `vt_function` is managed is because it has closure.

Variable scope is combined into call stack. Call stack has following types: `cs_root` is root stack, which is unique and not deletable, `cs_block` means block statement scope, `cs_loop` is loop scope to fit `break` and specially to fit `let` in `for` loop, `cs_function` is function scope and in which `args` and `jmp_addr` are available.

Hashmap operation `js_map_put`'s algorithm:

    loop key      loop value      new value      operation
    -----------------------------------------------------------
    null          not null        ..             fatal, shouldn't happen
    null          not null        ..             fatal, shouldn't happen
    null          null            not null       add key value, length++, chech rehash
    null          null            null           no op, return
    matched       not null        not null       replace value, return
    matched       not null        null           replace value, length--, return
    matched       null            not null       replace value, length++, chech rehash
    matched       null            null           return
    not matched   not null        ..             continue
    not matched   not null        ..             continue
    not matched   null            not null       next stage
    not matched   null            null           next stage
    whole loop ended              ..             fatal, shouldn't happen
    whole loop ended              ..             fatal, shouldn't happen

may encounter "not matched null" -> "not matched null" -> ... -> "matched", if operate with first result, will cause duplicate, so "not matched null" may enter next stage, record position, and special treat:

    null          null            not null       replace key value to recorded position, length++, chech rehash
    not matched                                  continue
    whole loop ended                             replace key value to recorded position, length++, chech rehash

all stages must chech rehash, especially stage 2, or sometimes will fatal "Whole loop ended, this shouldn't happen" (because is full, no empty space)

test_js_value_loop generated 6.5G huge dump.txt, use tail command:

```
length=0 capacity=0
key=b, value=vt_number
length=1 capacity=2
    0 b vt_number
    1  vt_undefined
key=b, value=vt_undefined
length=0 capacity=2
    0 b vt_undefined
    1  vt_undefined
key=5zNugcVa2jZNC1oSNbqg6yd9bIYTaYisB4rv6Hpfknt0d0SVgYtYbdlVjhJ2puzSagZZs9o, value=vt_undefined
length=0 capacity=2
    0 b vt_undefined
    1  vt_undefined
key=Swy4fCH8h03lkwQwW9BxW7O3dH9EReeng80wiI37Jwid6RXMwQ0cgiPn, value=vt_scripture
length=1 capacity=2
    0 b vt_undefined
    1 Swy4fCH8h03lkwQwW9BxW7O3dH9EReeng80wiI37Jwid6RXMwQ0cgiPn vt_scripture
key=CVUcQn5KYZkjKSa1eJAsg0nUQsnZBdSNquxXsYnwIoNTEAtZBOt, value=vt_array
length=2 capacity=2
    0 CVUcQn5KYZkjKSa1eJAsg0nUQsnZBdSNquxXsYnwIoNTEAtZBOt vt_array
    1 Swy4fCH8h03lkwQwW9BxW7O3dH9EReeng80wiI37Jwid6RXMwQ0cgiPn vt_scripture
key=EFvi653FKJKm04nqvfux6YzKZhmukC7biyUhulH9eLPxZUX, value=vt_c_function
ERROR src\js-data.c:144:js_map_put: Fatal error: Whole loop ended, this shouldn't happen
```

when handling rehash, DON'T return a new map like realloc(), that's very stupid, if this map is another data structute's element, it will become wild pointer

val can be NULL. If key does not exist, will skip. If key exists, means delete operation, set val to NULL, k unchanged, so that rehash no needed, to make sure find loop won't break if following keys exists

"length" means number of keys in map, it is meanless for user because NULL val exists, DON'T use it outside

when rehash, not null key null value will not be added

EBNF

https://www.cnblogs.com/dhy2000/p/15970225.html
https://zh.wikipedia.org/wiki/%E9%80%92%E5%BD%92%E4%B8%8B%E9%99%8D%E8%A7%A3%E6%9E%90%E5%99%A8
https://gist.github.com/Chubek/0ab33e40b01a029a7195326e89646ec5

https://www.json.org/json-en.html
https://lark-parser.readthedocs.io/en/latest/json_tutorial.html

object key can be identifier, which means string, NOT identifier corresponding value

function rest argument support:
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
    prefix_expression = ['!'|'+'|'-'|'typeof'] access_call_expression
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

DON'T add comma_expression and DON'T put assignment_expression into the chain, because left value has to be lazyed, it is too complicated to deliver lazy evaluation literal representation such as 'foo["bar"]' in each chain, and more and more complicated, for example, have to save object key as dynamic, because it may be a result.

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
            | 'try' '{' { statement } '}' ['catch' '(' identifier ')' '{' { statement } '}' ]
            | 'throw' expression ';'
            | declaration_expression ';'
            | prefix_expression; (* such as 'typeof(true)::print()' *)
            | assignment_expression ';' (* put at last, because it cannot be determined by first token *)

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

DON'T use inline, even slower in mingw and size increased about 10k

Why upgrade to C99?

- Array initialization with enum indices in C <https://eli.thegreenplace.net/2011/02/15/array-initialization-with-enum-indices-in-c-but-not-c>
- vsnprintf
- anonymous struct/union
- compound literal such as (struct foo){...}
- binary literal '0b'
- __func__
- typeof (c23)

New vm instruction structure:

```
support maximum 64 opcodes, 0-3 operands, 16 operand types
instruction binary structure is (C means opcode, D E F means 1st 2nd 3rd operand type):
             low -> high
no operand : CCCCCC00
 1 operand : CCCCCC01 DDDD0000 ...operand0...
 2 operands: CCCCCC10 DDDDEEEE ...operand0... ...operand1...
 3 operands: CCCCCC11 DDDDEEEE FFFF0000 ...operand0... ...operand1... ...operand2...
```

Length limits for some fields (if not specified, will be 'size_t'):

|||
|-|-|
|uint8_t|some types|
|uint16_t|number of globals, locals, arguments, closure. object key, stack length|
|uint32_t|scripture, source, bytecode length|

Variable scope:

    local->closure->global

function parameters are not standalone scope, they will be merged into locals, so 'function(a){let a;}' is not allowed. for example, "function foo(a) { let b; return function bar() {}; }", a and b will all be put into bar's closure, they must prevent naming confliction.

Before op_call, stack layout is shown below, just fit accessor model:

    (* top *)
    sf_function(egress, arguments, ...)
    sf_value(function/c_function)
    (* bottom *)

Before function returns, push return value to stack

Map based variable speed too low problem, if using ast, maybe can change to index visit. But functions may be dynamic, so maybe only local variables can determine position?

```
Macro and function naming rule:
prefix _ means local file scope
prefix __ means function scope
Function variables do not necessarily follow this rule
Macro argument must prefix with __arg_, to prevent if some struct members have same name and appear inside macro, will wrongly be replaced
```

    /*
    test special cases
    node.js, quickjs will treat both a and b as reference
    node.js:
        [ 1, [ 1 ] ]
        [ 2, [ 2 ] ]
    quickjs:
        1,1
        2,2
    in banana script, for better performence, only strings, arrays, objects, functions are passed by reference, so a is seperated, if you want to be connected, use array or object. But after all, it is not a good practise, better not using OO style.
    */
    function foo() {
        let a;
        let b = [];
        function bar(i, j) {
            a = i;
            b[0] = j;
        }
        function qux() {
            return [a, b];
        }
        return [bar, qux];
    }
    let barqux = foo();
    barqux[0](1, 1);
    console.log(barqux[1]());
    barqux[0](2, 2);
    console.log(barqux[1]());

In gcc, DON'T use const in struct, will cause entire struct be const, see: https://stackoverflow.com/questions/34989921/assignment-of-read-only-member-error-when-assigning-to-non-const-member-of-a-s

Add '::' after using AST, have to use AST, top-down mechanism cannot pass lvalue as rvalue (function) 's first operand. AST can do transformation.

    let z = 10;
    function foo(a = z, b, ...c) { // parameter default value can be an expression, so it is not fixed
        console.log(c);
    }
    bar(1, 2, ...arr); // number of arguments cannot be determined at compile time
    function foo() {
        let a = 1;
        let bar = function (b) {
            return a + b;
        }; // test whether 'bar' will be wrongly added into closure to cause recursion
        return bar;
    }
    // test undefined array hole correctly converted to null
    function foo(a, b, c) {
        dump_vm();
    }
    foo(...[null, null, 3]);

Better remove `delete` operator, because it's variable modification action is at runtime, "if (...) { delete ...; }", but variable creating is at compile time? or runtime? "if (...) { let ...; }", if using AST to change variables visitation from hashmap to array? No, `delete` cannot be removed, because an unexistance variable's scope cannot be determined, for example, `let a = 10; {a = null; /* a is deleted */ a = 20; /* now where is a? in this scope or parent scope? */ }`

C functions have no closure, because they are always static, not dynamically created, so they don't have creation scope, unlike lua, lua's purpose is only for saving data across function call.

Known issue:

```
function doit(func) {
    func();
}
let a = 1;
doit(function() {
    let b = 2;
    doit(function() {
        let c = 3;
        doit(function() {
            print(a, b, c);
        });
    });
});
function outer() {
    print(a, b, c);
}
try {
    doit(function() {
        let b = 2;
        doit(function() {
            let c = 3;
            doit(outer); // TODO: a b c are reachable?
        });
    });
} catch (err) {
    print(err);
}
try {
    doit(function() {
        let b = 2;
        doit(function() {
            let c = 3;
            let inner = outer;
            doit(inner); // TODO: a b c are reachable?
        });
    });
} catch (err) {
    print(err);
}
```

TODO: add stack level in function value creation?
but it is complex, for example:
function foo() {
    ... {
        ...{
            ...{
                return function() { // stack level is very deep }
            }
        }
    }
}
function bar() {
    let a;
    foo()(); // can visit a?
}

So one possible solution: every function must have it's closure after created, so each value type must be referenced (all managed)?

So gc will be more than one times, for example:

```
let a = 1;
let b = 2;
function c() {}
```

a b will be referenced by c, and gc loop first round clean c, second round clean a b, third round found nothing to be cleaned and quit.

