// test op_argument_get_next ...
let params;
function foo(a, b = 2, c, d = "?", e = 5, f, ...g) {
    params = {"a" : a, "b" : b, "c" : c, "d" : d, "e" : e, "f" : f, "g" : g};
    return "Nice to meet you.";
}
let arr = [ 3, 4, null, 6, null, 7 ];
let bar = foo(null, null, ...arr);
print(params);
return 3;

// test null argument and spread caused undefined
// print(null, 1, ...[null, 2, null, 3, null], 4, null, 5, null);

// test undefined array hole correctly converted to null
// function foo(a, b, c) {
//     dump_vm();
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
//                     dump_vm();
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

// print(function(a){return a;}(10));

// let b = function(a){return a;};
// print(b(10));

// https://zhuanlan.zhihu.com/p/1920601495604278753
// function createFib() {
//     return function(n, g) {
//         if (n <= 2)
//             return 1;
//         return g(n - 1, g) + g(n - 2, g);
//     };
// }
// let fib = createFib();
// print(fib(10, fib));

// Variable "fibB" not found
// function createFib() {
//     function fibA(n) {
//         if (n <= 2)
//             return 1;
//         return fibB(n - 1) + fibB(n - 2);
//     }
//     function fibB(n) {
//         if (n <= 2)
//             return 1;
//         return fibA(n - 1) + fibA(n - 2);
//     }
//     return fibA;
// }
// let fib = createFib();
// print(fib(36));

// function createFib() {
//     let fibs = [];
//     fibs[0] = function(n) {
//         if (n <= 2)
//             return 1;
//         return fibs[1](n - 1) + fibs[1](n - 2);
//     };
//     fibs[1] = function(n) {
//         if (n <= 2)
//             return 1;
//         return fibs[0](n - 1) + fibs[0](n - 2);
//     };
//     return fibs[0];
// }
// let fib = createFib();
// console.log(fib(36));

// test whether anonymous function will be gced while in use
// function foo(f) {
//     f();
// }
// foo(function(){
//     dump_vm();
//     gc();
//     dump_vm();
// });
// gc();
// dump_vm();

// forward(function(...args) {
//     forward(function(...args) {
//         forward(function(...args) {
//             gc();
//             print(...args);
//         }, ...args);
//     }, ...args);
// }, null, true, 3.14, "hello");


/*
*/