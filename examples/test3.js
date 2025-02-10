// test instructions

// let a;
// if (1 > 2)
//     a = 1;

// let b;
// if (1 > 2)
//     b = 1;
// else
//     b = 2;

// let c = 0;
// while (c < 10)
//      c++;

// let d = 0;
// do
//     d++;
// while (d < 10);

// let e = 0, f;
// while (true) {
//     if (e > 5) {
//         break;
//     };
//     e++;
//     f = 0;
//     while (true) {
//         if (f > 10) {
//             break;
//         };
//         f++;
//         continue;
//     }
//     continue;
// }

// let e = 0, f;
// do {
//     if (e > 5) {
//         break;
//     };
//     e++;
//     f = 0;
//     do {
//         if (f > 10) {
//             break;
//         };
//         f++;
//         continue;
//     } while (true);
//     continue;
// } while (true);

// let i;
// for (let i = 0; i < 10; i++) {
//     continue;
//     break;
// }

// let i = [], j = {};
// for (i[0] = 0; i[0] < 10; i[0]++) {
//     j.foo = i[0];
//     continue;
//     break;
// }

// let i;
// for (let j of ["a", "b", null]) {
//     i = j;
//     break;
// }

// let i = [];
// let j;
// for (i[0] of ["a", "b", null]){
//     j = i[0];
// }

// let i = [[]];
// let j = {"a" : true, "b" : false, "c" : null, "d" : 888, "e" : "hi"};
// let k = 0;
// let l = [[]];
// for (i[0][0] in j) {
//     l[0][k] = i[0][0];
//     k++;
//     l[0][k] = j[i[0][0]];
//     k++;
//     continue;
// }

// let loop1 = 0, fibonacci1 = 0;
// while (loop1 < 10) {
//     fibonacci1 = fibonacci1 + loop1;
//     loop1 = loop1 + 1;
// }

// let loop2 = 0, fibonacci2 = 0;
// do {
//     fibonacci2 += loop2;
//     loop2++;
// } while (loop2 < 10);

// let loop5 = 0, fibonacci5 = 0;
// while (true) {
//     loop5 = loop5 + 1;
//     if (loop5 >= 10)
//         break;
//     if (loop5 % 2 == 0)
//         continue;
//     fibonacci5 = fibonacci5 + loop5;
// }

// let loop6 = 0;
// let array6 = [];
// while (true) {
//     loop6++;
//     if (loop6 >= 20) {
//         {
//             {
//                 break;
//                 loop6 = 888;
//             }
//         }
//     }
//     if (loop6 % 2 == 0) {
//         {
//             {
//                 continue;
//                 loop6 = 999;
//             }
//         }
//     }
//     array6[loop6] = loop6;
// }

// let loop7 = 0;
// let array7 = [];
// do {
//     loop7++;
//     if (loop7 >= 20) {
//         {
//             {
//                 break;
//                 loop7 = 888;
//             }
//         }
//     } else {
//         if (loop7 % 2 == 0) {
//             {
//                 {
//                     continue;
//                     loop7 = 999;
//                 }
//             }
//         }
//     }
//     array7[loop7] = loop7;
// } while (true);

// let arr = [];
// let never_reached;
// for (let i = 0; i < 10; i++) {
//     if (i % 2 == 0)
//         continue;
//     if (i > 7) {
//         break;
//         never_reached = 111;
//     }
//     arr[i] = [];
//     for (let j = 0; j < 10; j++) {
//         if (j % 2 == 0) {
//             {
//                 continue;
//                 never_reached = 222;
//             }
//         }
//         if (j > 5) {
//             {
//                 {
//                     break;
//                     never_reached = 333;
//                 }
//             }
//         }
//         arr[i][j] = i * j;
//     }
// }

// let f1 = 0;
// for (let i1 = 0; i1 < 10; i1++)
//     f1 += i1;

// let i2 = [ {} ], f2 = 0;
// for (i2[0].foo = 0; i2[0].foo < 10; i2[0].foo++) {
//     f2 += i2[0].foo;
// }

// let i3 = 0, f3 = 0;
// for (; i3 < 10; i3++) {
//     f3 += i3;
// }

// let i4 = 0, f4 = 0;
// for (;;) {
//     if (i4 < 10) {
//         f4 += i4;
//         i4++;
//     } else {
//         break;
//     }
// }

// while (false) {
// }
// let foo = "foo";
// do {
// } while (false);
// let bar = "bar";
// for (; false;) {
// }
// let baz = "baz";

// let a1 = [ null, false, 3.14, "hello" ];
// let a2 = [];
// for (let i in a1) {
//     a2[i] = a1[i];
// }

// let i = 0;
// let a3 = [];
// for (let v of a1) {
//     a3[i] = v;
//     i++;
// }

// let o1 = {"foo" : null, "bar" : false, "baz" : 3.14, "qux" : "hello"};
// let o2 = {};
// for (let k in o1) {
//     o2[k] = o1[k];
// }

// i = 0;
// let o3 = [];
// for (let v of o1) {
//     o3[i] = v;
//     i++;
// }

// let long_arr = [ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L" ];
// let short_arr = [];
// for (let i in long_arr) {
//     if (i > 7) {
//         break;
//     } else if (i % 2 == 0) {
//         continue;
//     } else {
//         short_arr[i] = long_arr[i];
//     }
// }

// for (let i in [])
//     ;
// ;
// ;
// for (let v of [])
//     ;
// ;
// ;
// for (let k in {})
//     ;
// ;
// ;
// for (let v of {})
//     ;
// ;
// ;

// let hello = "hello";