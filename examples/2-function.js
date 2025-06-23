// test op_parameter_get_next ...
// let params;
// function foo(a, b = 2, c, d = "?", e = 5, f, ...g) {
//     params = {"a" : a, "b" : b, "c" : c, "d" : d, "e" : e, "f" : f, "g" : g};
//     return "Nice to meet you.";
// }
// let arr = [ 3, 4, null, 6, null, 7 ];
// let bar = foo(null, null, ...arr);
// return 3;

// test null parameter and spread caused undefined
// print(null, 1, ...[null, 2, null, 3, null], 4, null, 5, null);

// test undefined array hole correctly converted to null
// function foo(a, b, c) {
//     dump();
// }
// foo(...[null, null, 3]);

// function foo() {
//     let bar = function(a) {
//         let a = 1;
//     };
//     return bar;
// }

// foo()(2);

// test closure
// function foo(a) {
//     let d = "foo1";
//     {
//         let d = "foo2";
//         return function (b) {
//             let d = "bar1";
//             {
//                 let d = "bar2";
//                 return function (c) {
//                     // console.log(a, b, c, d);
//                     dump();
//                 };
//             }
//         };
//     }
// }
// foo(1)(2)(3);

// test no adding self to closure
// function foo(a) {
//     let b = 2;
//     let bar = function(c) {
//         return a + b + c;
//     };
//     return bar;
// }
// let qux = foo(1)(3);

// test outside function returns inside another function
// let d = 4;
// function bar(c) {
//     console.log(a, b, c, d);
// }
// function foo(a) {
//     let b = 2;
//     return bar;
// }
// foo(1)(3);

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
// function foo() {
//     let a;
//     let b = [];
//     function bar(i, j) {
//         a = i;
//         b[0] = j;
//     }
//     function qux() {
//         return [a, b];
//     }
//     return [bar, qux];
// }
// let barqux = foo();
// barqux[0](1, 1);
// console.log(barqux[1]());
// barqux[0](2, 2);
// console.log(barqux[1]());

// check closure is correct
// let a = 1;
// function foo() {}
// let bar = (function(a = 1) {let b = 2; return function(){}; })();
// let baz = (function(a = 1) {let b = 2; let anony = function(){}; return anony; })();
// let qux = (function(a = 1) {let b = 2; return foo; })();
// let quxx = (function(a = 1) {let b = 2; return bar; })();

// test builtin functions
print(length("foo"));
print(length([ 1, 2, 3 ]));
try {
    print(length());
} catch (err) {
    print(err);
}
try {
    print(length(null, true, false));
} catch (err) {
    print(err);
}
try {
    print(length(1.2));
} catch (err) {
    print(err);
}
let obj = {"foo": true, "bar": false};
print(length(obj));
obj.qux = 1;
print(length(obj));
obj.foo = null;
print(length(obj));
obj.bar = null;
print(length(obj));
obj.qux = null;
print(length(obj));
try {
    print(input("a", "b", "c", "d"));
} catch(err) {
    print(err);
}
try {
    print(input(123));
} catch(err) {
    print(err);
}
try {
    print(input("prompt: "));
} catch(err) {
    print(err);
}
