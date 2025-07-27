// test op_variable_declare
// let a, b = true;
// let c = false, d = 31.41592654e-1, e = "hello";
// let f = [], g = {};

// test op_variable_delete
// delete c;

// test op_variable_put op_variable_get
// let h;
// h = g;

// [];
// {}; // will be treated as call stack, not empty object, because call stack parsing first
// let h = [ a, b, c, d, e, {"array" : f, "object" : g, "another array" : [ 2, 3, 3 ]} ];
// delete a;
// delete b;

// test unexisted identifier
// leti;

// test variable scope
// {
//     if (true)
//         let i = 10;
//     console.log(i);
// }

// test string escape
// print("\'\"\?\\\a\b\f\n\r\t\v\u1234");
// let o = {"\'\"\?\\\a\b\f\n\r\t\v\u1234": "Hi"};
// for (let k in o) {
//     print(k, o[k]);
// }