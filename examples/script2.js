// member access
let a = [[[1,2,3]]];
let b = a[0][0][2];
let c = a[0][0][3];
let d = {foo:true, bar:{"baz":a[0][0][0],"qux":a[0][0][1]+a[0][0][2]}};
let e = d["foo"];
let f = d["baz"];
let g = d.foo;
let h = d.baz;
let i = d?.baz;
let j = d?.baz?.qux;
let k = [];
k[(1+2)*3] = a[((1+2)*3)];
k[0] = a[0];
k[0][0] = a[0][0];
let l = {};
l["f"+"o"+"o"] = d["f"+"o"+"o"];
l.bar = d?.bar;
l["bar"].baz = d?.bar["qux"];

// ternary operator 
let m = a[0][0][1] != l.bar.baz ? a : b;

// assignment expressions
let n = 1;
n += 2;
let o = "hello";
o += "world";
let p = 10;
p -= 5;
let q = 3;
q *= 3;
let r = 999;
r /= 3;
let s = 10000.333;
s %= 10;
let t = 999;
t++;
let u = 999;
u--;