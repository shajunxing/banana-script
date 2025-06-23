// test op_variable_declare
let a, b = true;
let c = false, d = 31.41592654e-1, e = "hello";
let f = [], g = {};
// test op_variable_delete
delete c;
// test op_variable_put op_variable_get
let h;
h = g;

// [];
// {}; // will be treated as call stack, not empty object, because call stack parsing first
// let h = [ a, b, c, d, e, {"array" : f, "object" : g, "another array" : [ 2, 3, 3 ]} ];
// delete a;
// delete b;