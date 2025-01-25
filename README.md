# Banana JS, An inperpreter for a strict subset of JavaScript

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

## syntax

No modules. In inperpreter's view, source code is only one large flat text. I don't like object oriented, so all class related content are not supported.

Json compatible data types: `null, boolean, number, string, array, object, function, c function`. No `undefined` because `null` is enough. Array and object are clean, no predefined members such as prototypes.

Variable declaraction use `let`, all variables are local. Access undeclared variables will cause error, and access array/object's unexisting members will get `null`. `const` is not supported because it's meaningless, for example, a constant pointed to an array/object, should it's member be immunable? if not, why should I keep `const`? if so, value must be marked immunable, but it's weird in point of garbage collector's view, because all values should be cleanable.

Function definition supports default parameter `param = value` and rest parameter `...params`. Array literal and function call support spread syntax `...`. No `this, arguments` in function. Function supports closure.

Operators follow strict rule, no implicit conversion. Only boolean can do logical operations. `== !=` are strict meaning, and can be done by all types. Strings can do all relational operations and `+`. Numbers can do all relational and numerical operations. Operator precedence from low to high is:

- Ternary operator `? :`
- `&& ||`
- `== != < <= > >=`
- `+ -`
- `* / %`
- Prefix operator `+ - !`
- Array/object member access and function call operator `[] . ?. ()`

Assignment expression `= += -= *= /= %= ++ --` does not return value, Comma expression `,` is not supported.

`if`, `while`, `do while`, `for`'s condition must be boolean. `for` loop only support following syntax, `[]` means optional. `for in` and `for of` only handle non-null members:

- `for ([[let] variable = expression ] ; [condition] ; [assignment expression])`
- `for ([let] variable in array/object)`
- `for ([let] variable of array/object)`
