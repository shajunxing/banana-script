# Banana Script，一个C99实现的，类JavaScript极简语法的脚本引擎

本文使用 [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) 许可。

[英文版](README.md) | [中文版](README_zhCN.md)

项目地址：<https://github.com/shajunxing/banana-script>

![REPL](screenshot3.png "REPL")

## 特色

我的目标是剔除和修改我在实践中总结的JavaScript语言的没用的和模棱两可的部分，只保留我喜欢和需要的，创建一个最小语法的解释器。函数是第一类值，函数支持闭包。我不喜欢面向对象编程，所以所有与类相关的内容都不支持，但是，我重新定义了提案里的**双冒号绑定运算符**（<https://github.com/tc39/proposal-bind-operator> <https://babeljs.io/docs/babel-plugin-proposal-function-bind>），现在 `value::function(...args)` 等价于 `function(value, ...args)`，如此Class爱好者会很开心，因为能轻松写出面向对象范儿，甚至漂亮的链式语法风格。

## 两分钟简要语法指南

数值类型为 `null` `boolean` `number` `string` `array` `object` `function`，`typeof`的结果严格对应这些名字。不支持 `undefined`，因为 `null` 已经足够。数组和对象是干净的，没有预定义的成员，比如`__proto__`。

变量声明使用 `let`，所有变量都是局部变量，不支持 `const`，因为一切都必须可删除。访问未声明的变量会引发错误，访问数组/对象不存在的成员会返回 `null` （包括数组索引是负值和非整数，但写操作是禁止的），写入`null`则为删除对应成员。

函数定义只支持`function`关键字，不支持`=>`表达式。支持默认参数 `param = value` 和剩余参数 `...args`。数组字面量和函数调用支持展开语法 `...`。函数中没有预定义的成员比如`this` `arguments`。`return` 如果在全局作用域，意为退出虚拟机。

运算符遵循严格规则，没有隐式转换。只有布尔值可以进行逻辑运算。`== !=` 是严格意义上的比较，可以应用于所有类型。数字支持所有关系和数值运算符，字符串支持所有关系运算符和 `+`。运算符的优先级从低到高为：

- 三元运算符 `?` `:`
- 逻辑或运算符 `||`
- 逻辑与运算符 `&&`
- 关系运算符 `==` `!=` `<` `<=` `>` `>=`
- 加减运算符 `+` `-`
- 乘除运算符 `*` `/` `%`
- 指数运算符 `**`
- 前缀运算符 `+` `-` `!` `typeof`
- 数组/对象成员访问和函数调用运算符 `[]` `.` `?.` `()` `::`

赋值表达式 `=` `+=` `-=` `*=` `/=` `%=` `++` `--` 不返回值。不支持逗号表达式 `,`。

条件语句是 `if`，循环语句是 `while` `do while` `for`，条件必须是布尔值。`for` 循环仅支持以下语法，`[]` 表示可选部分。`for in` 和 `for of` 只处理非 `null` 的成员：

- `for ([[let] variable = expression ] ; [condition] ; [assignment expression])`
- `for ([let] variable in array/object)`
- `for ([let] variable of array/object)`

不支持模块。在解释器的视角中，源码只是一个大的平坦文本。

垃圾回收是手动的，你可以在任何时候执行。

`delete` 语义为删除当前作用域范围的局部变量（对象成员置 `null` 即可删除）。比如，加入函数闭包的变量是声明函数变量之前的所有局部变量，可以在返回之前删掉无用的变量以减少闭包大小，在REPL环境里执行以下两条语句，可以看到区别。

- `gc();let f=function(a,b){let c=a+b;return function(d){return c+d;};}(1,2);dump_vm();print(f(3));delete f;`
- `gc();let f=function(a,b){let c=a+b;delete a;delete b;return function(d){return c+d;};}(1,2);dump_vm();print(f(3));delete f;`

`throw` 可以抛出任意值，由 `catch` （可选的）接收。不支持`finally`，因为我认为根本不需要，反而会使代码执行顺序显得怪异。

## 项目结构及与C语言的交互性

本项目兼容 C99，编译环境为 msvc/gcc/mingw，只依赖 C 编译器和<https://github.com/shajunxing/banana-nomake>，无需 make 系统。msvc 执行 `cl make.c && make.exe release`，或者 mingw/gcc 执行 `gcc -o make.exe make.c && ./make.exe release`。生成的可执行文件位于 `bin` 目录里。

项目遵循“最小依赖”原则，只包含必须的头文件，且模块之间只有单向引用，没有循环引用。模块的依赖关系和功能如下：

```
js-common   js-data     js-vm       js-syntax   js-std
    <-----------
                <-----------
                            <-----------
                            <-----------------------
```

- `js-common`： 项目通用的常量、宏定义和函数，例如日志打印、内存操作。
- `js-data`：数值类型和垃圾回收，你甚至可以在C项目里单独使用该模块操作带GC功能的高级数据结构，参见 <https://github.com/shajunxing/banana-cvar>。
- `js-vm`：字节码虚拟机，单独编译可得到不带源代码解析功能的最小足迹的解释器。
- `js-syntax`：词法解析和语法解析，将源代码转化为字节码。
- `js-std`：一些常用标准函数的参考实现，可用作编写C函数的参考。

所有值都是 `struct js_value` 类型，你可以通过 `js_...()` 函数创建，`...` 是值类型，你可以直接从这个结构体中读取 C 值，参见 `js_data.h` 中的定义。不要直接修改它们，如果你想得到不同的值，就创建新值。复合类型 `array` `object` 可以通过 `js_..._array_...()` `js_..._object_...()` 函数进行操作。

C 函数必须是 `typedef struct js_result (*js_c_function_type)(struct js_vm *vm, uint16_t argc, struct js_value *argv)` 格式，从 `argc` `argv` 读取传入参数，`struct js_result` 有两个成员，如果 `.success` 是 `true`, `.value` 就是返回值, 如果 `false`, `.value` 则是抛出的错误值。使用 `js_c_function()` 来创建 C 函数值，是的，当然它们都是值，可以放在任何地方，例如，如果使用 `js_declare_variable()` 放在堆栈根上，它们就是全局的。C函数同样也可以使用 `js_call()`、`js_call_by_name()` 和 `js_call_by_name_sz()`调用脚本函数。

## 标准库

包括语言级和操作系统级别的最常用功能。可以理解为“参考实现”，不保证在未来保持不变。更多信息请查阅 <https://github.com/shajunxing/banana-script/blob/main/examples/7-std.js> 和 <https://github.com/shajunxing/banana-script/blob/main/src/js-std.c>。

命名规则：

1. 最常用的，取用最常用的名称，例如控制台输入和输出，分别是 `input` 和 `print`。这些名称最容易记忆。我甚至请 ChatGPT 帮我查询它们的使用率。
2. 单一特征的，匹配单个 DOS/Unix 命令或 C 标准库/unistd 函数的，例如 `cd`、`md` 和 `rd`，使用最简短的，这也可以提高执行速度。
3. 非单一特征的，自定义名称。

值和函数定义约定：

- -: 空值，无返回值的函数实际上返回空值
- b: 布尔值
- n: 数字
- s: 字符串
- (): 函数
- []: 数组，或可选参数
- {}: 对象
- *: 任意类型
- /: 多种类型或名字分隔符
- ...: 不限参数

语言：

|Definition________________________|Description|
|-|-|
|n ceil(n val)|Same as C `ceil`.|
|dump_vm()|Print vm status.|
|b endswith(s str, s sub, s ...)|Determine whether string ends with any of sub strings.|
|n floor(n val)|Same as C `floor`.|
|s format(s fmt, * ...)|Format with `fmt`, there are two types of replacement field, first is `${foo}` where `foo` is variable name, second is `${0}` `${1}` `${2}` ... where numbers indicates which argument followed by, starts from 0, and will be represented as `tostring()` style.|
|gc()|Garbage collection.|
|s join([s ...] arr, s sep)|Join string array with seperator.|
|n length([* ...]/{* ...}/s val)|Returns array/object length or string length in bytes.|
|[s ...]/- match(s text, s pattern)|Regular expression matching. If matched returns all captures, otherwise returns `null`. Currently supports `^` `$` `()` `\d` `\s` `\w` `.` `[]` `*` `+` `?`.|
|[n, n] modf(n val)|Same as C `modf`, returns array of integral and fractional parts.|
|n natural_compare(s lhs, s rhs)|Natural-compare algorithm, used by `sort()`.|
|* pop([* ...] arr)|Removes array's last element and returns.|
|push([* ...] arr, * elem)|Add element to end of array.|
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

操作系统：

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
|exec(s arg, s ...)|Same as POSIX `execvp()`, but first parameter `file` is automatically filled with `argv[0]`.|
|b exists(s path)|Checks if file `path` exists.|
|exit(n status)|Same as C `exit()`, `status` will be cast to integer.|
|s input([s prompt])|Prompt (optional) and accepts line of user input. If you need number, use `tonumber()` to convert.|
|ls(s dir, cb(s fname, b isdir))|List directory and with each entry call `cb`.|
|md(s path)|Same as POSIX `mkdir()`|
|s os|`windows` or `posix`|
|s pathsep|`\` or `/`|
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
|s whoami()|Get current user name.|
|write(n fp, s text)</br>write(s fname, s text)</br>write(s fname, b isappend, s text)</br>|Generic writing function for text file, which takes file handle `fp`, or file name `fname`. `isappend` means append mode instead of overwrite mode.|

## 使用该项目的项目

<https://github.com/shajunxing/view-comic-here>

## 未整理的

详见英文版

