// test assignment expression without assignment
// let id = 33;
// id;
// 66;
// [ "a", null, ...["b", null, "c"], null, "d" ];
// // object { may conflict with block statement, must surround with parentheses
// ({"foo" : {bar: {baz: ["a", "b"]}}})["foo"].bar?.baz[1];

// test inscription
// let aa = 1;
// aa = 1;
// let bb = aa;
// bb = aa;
// let aab = 1;
// let aabb = 1;
// let abbb = 1;

// // test operators
// let a = 10, b = 10, c = 10, d = 10, e = 10, f = 10, g = 10;
// a += 3;
// b -= 3;
// c *= 3;
// d /= 3;
// e %= 3;
// f **= 3;
// g++;
// g--;
// let h = f == g;
// let i = f != g;
// let j = "hello";
// let k = "world";
// j += k;
// let l = j == "helloworld";
// let m = j != "helloworld";
// let o = null == null;
// let p = true == true;
// let q = 0 == -0;
// let r = 1 < 2;
// let s = "aaa" < "bbb";
// let t = false && false || true && true;
// let u = false && (false || true) && true;
// let v = !true;
// let w = typeof null;
// let x = typeof w; // vt_scripture
// let y = typeof k; // vt_inscription
// let z = typeof j; // vt_string
// let complex_expr1 = 1 + 2 * 3 < -(-((1 + 2) * 3)) && (!false);
// let complex_expr2 = 1 + 2 * 3 ** 2 ** 2;
// let ternary_result = true ? "hello" : "world";

// test array and object
// let k1 = "hel", k2 = "lo", k3 = 2;
// let obj = {"foo" : true && false, bar : 3 * 3};
// let arr = [ "a", null, ...["b", null, "c"], null, "d" ];
// let complex_obj = {"foo" : [ null, true, false, {hello : [ obj, -2, k1 + k2, k3 * 1.1 * 3, -4.4, 5e2, -6e-2, 7.7e3, -8.8e-4, !false ]} ]};

// test accessor
// let acc_1 = [ {foo : {"bar" : "hello"}} ];
// let acc_2 = [ {foo : {}}, 3 ];
// acc_2[0]["foo"].bar = acc_1[0]["foo"]?.bar;
// acc_2[1] **= 3;
