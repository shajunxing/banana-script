let empty_array = [];
let empty_object = {};

let a = 1, b = -2, c = (1 + -2) * 3;
let complex_expr = 1 + 2 * 3 < -(-((1 + 2) * 3)) && (!false);
let complex_obj = {"foo" : [ null, true, false, {"bar" : [ a, -2, c * 1.1 * 3, -4.4, 5e2, -6e-2, 7.7e3, -8.8e-4, "hel" +
                                                                                                                     "lo",
                                                           !false ]} ]};
let obj_member = complex_obj["fo" +
                             "o"][2 + 1]["bar"][(1 + 1) * 2];

let complex_obj_1 = complex_obj;
let eq_same_obj = complex_obj_1 == complex_obj;
let eq_null = null == null;
let ne_bool = false != true;
let eq_num = 1.23e2 == 123e0;
let ne_str = "hello" != "world";
let ne_arr = [] != [];
let ne_obj = {} != {};

let build_obj = {};
build_obj["foo"] = [ 1, (2 + 4) * 6, 2, 3 ];
build_obj["f" +
          "o" +
          "o"][(1 + 1) * 2 - 1] = 10;
build_obj["bar"] = {};
let build_obj_bar = build_obj["bar"];
build_obj_bar["hello"] = {};
build_obj_bar["hello"]["world"] = true;

let member_access = build_obj_bar["he" + ("l" +
                                          "lo")]
                        ?.world;
let optional_chaining = member_access?.foo?.bar?.baz?.qux;

let loop1 = 0, fibonacci1 = 0;
while (loop1 < 10) {
    fibonacci1 = fibonacci1 + loop1;
    loop1 = loop1 + 1;
}

let loop2 = 0, fibonacci2 = 0;
do {
    fibonacci2 += loop2;
    loop2++;
} while (loop2 < 10);

let loop5 = 0, fibonacci5 = 0;
while (true) {
    loop5 = loop5 + 1;
    if (loop5 >= 10)
        break;
    if (loop5 % 2 == 0)
        continue;
    fibonacci5 = fibonacci5 + loop5;
}

let loop6 = 0;
let array6 = [];
while (true) {
    loop6++;
    if (loop6 >= 20) {
        {
            {
                break;
                loop6 = 888;
            }
        }
    }
    if (loop6 % 2 == 0) {
        {
            {
                continue;
                loop6 = 999;
            }
        }
    }
    array6[loop6] = loop6;
}

let loop7 = 0;
let array7 = [];
do {
    loop7++;
    if (loop7 >= 20) {
        {
            {
                break;
                loop7 = 888;
            }
        }
    } else {
        if (loop7 % 2 == 0) {
            {
                {
                    continue;
                    loop7 = 999;
                }
            }
        }
    }
    array7[loop7] = loop7;
} while (true);

let arr = [];
let never_reached;
for (let i = 0; i < 10; i++) {
    if (i % 2 == 0)
        continue;
    if (i > 7) {
        break;
        never_reached = 111;
    }
    arr[i] = [];
    for (let j = 0; j < 10; j++) {
        if (j % 2 == 0) {
            {
                continue;
                never_reached = 222;
            }
        }
        if (j > 5) {
            {
                {
                    break;
                    never_reached = 333;
                }
            }
        }
        arr[i][j] = i * j;
    }
}