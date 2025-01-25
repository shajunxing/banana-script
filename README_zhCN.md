# Banana JS，一个严格子集 JavaScript 的解释器

本文使用 [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) 许可。

[英文版](README.md) | [中文版](README_zhCN.md)

## 语法

不支持模块。在解释器的视角中，源码只是一个大的扁平文本。我不喜欢面向对象编程，所以所有与类相关的内容都不支持。

支持 JSON 兼容的数据类型：`null、boolean、number、string、array、object、function、c function`。不支持 `undefined`，因为 `null` 已经足够。数组和对象是干净的，没有预定义的成员，比如原型。

变量声明使用 `let`，所有变量都是局部变量。访问未声明的变量会引发错误，访问数组/对象不存在的成员会返回 `null`。不支持 `const`，因为它没有意义。例如，一个常量指向一个数组/对象，那么它的成员是否应该不可变？如果不是，那我为什么要使用 `const`？如果是，那值必须被标记为不可变，但这在垃圾回收器的视角下显得很奇怪，因为所有的值都应该是可清理的。

函数定义支持默认参数 `param = value` 和剩余参数 `...params`。数组字面量和函数调用支持扩展语法 `...`。函数中没有 `this, arguments`。函数支持闭包。

运算符遵循严格规则，没有隐式转换。只有布尔值可以进行逻辑运算。`== !=` 是严格意义上的比较，可以应用于所有类型。字符串支持所有关系运算符和 `+`。数字支持所有关系和数值运算符。运算符的优先级从低到高为：

- 三元运算符 `? :`
- `&& ||`
- `== != < <= > >=`
- `+ -`
- `* / %`
- 前缀运算符 `+ - !`
- 数组/对象成员访问和函数调用运算符 `[] . ?. ()`

赋值表达式 `= += -= *= /= %= ++ --` 不返回值。不支持逗号表达式 `,`。

`if`、`while`、`do while`、`for` 的条件必须是布尔值。`for` 循环仅支持以下语法，`[]` 表示可选部分。`for in` 和 `for of` 只处理非 `null` 的成员：

- `for ([[let] variable = expression ] ; [condition] ; [assignment expression])`
- `for ([let] variable in array/object)`
- `for ([let] variable of array/object)`
