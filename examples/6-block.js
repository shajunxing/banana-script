// let a;
// if (1 > 2)
//     a = 1;

// let b;
// if (1 > 2)
//     b = 1;
// else
//     b = 2;

// let i = 4, j;
// if (i == 1) {
//     j = 1;
// } else if (i == 2) {
//     j = 2;
// } else if (i == 3) {
//     j = 3;
// } else {
//     j = 4;
// }

// let c = 0;
// while (true) {
//     {
//         if (c >= 10) {
//             {
//                 break;
//             }
//         } else {
//             c++;
//             {
//                 continue;
//             }
//         }
//     }
// }

// let c = 0;
// try {
//     {
//         do {
//             {
//                 if (c >= 10) {
//                     {
//                         throw "Boom!";
//                     }
//                 } else {
//                     c++;
//                     {
//                         continue;
//                     }
//                 }
//             }
//         } while (true);
//     }
// } catch (ex) {
//     {
//         dump_vm();
//     }
// }

// let c = 0;
// for (let i = 0; i < 10; i++) {
//     c += i;
//     if (i >= 10) {
//      break;
//     }
//     i++;
//     continue;
// }

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

// let arr = [ "a", "b", "c" ];
// let obj = {};
// let a = 0;
// for (obj.foo of arr) {
//     obj[obj.foo] = a;
//     a++;
// }

// test garbage collector
for (;;) {
    for (let i = 0; i < 100; i++) {
        function foo(a = [ 1, 2, 3 ], b = {"k" : "v"}) {
            for (let j = 0; j < 20; j++) {
                if (j > 10) {
                    break;
                }
                if (j % 2 == 0) {
                    continue;
                }
                {
                    let c = [ 4, 5, 6 ];
                    let d = {"k" : "v"};
                    let f = function(e = [ 7, 8, 9 ], f = {"k" : "v"}) {
                        {
                            return {"foo" : [ null, true, false, {hello : [ {"foo" : true && false, bar : 3 * 3}, -2, "hel" +
                                                                                                                          "lo",
                                                                            2 * 1.1 * 3, -4.4, 5e2, -6e-2, 7.7e3, -8.8e-4, !false ]} ]};
                        }
                    };
                    {
                        if (i % 3 == 0) {
                            {
                                throw "Boom!";
                            }
                        }
                    }
                    return f;
                }
            }
        }
        try {
            console.log(foo()());
        } catch (ex) {
            console.log(i, ex);
        }
        gc();
    }
}