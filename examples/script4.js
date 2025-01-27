let nr_add, nr_sub, nr_mul, nr_div;
function add(a = 1, b = 2) {
    return a + b;
    nr_add = "never reached";
}
let sub = function(a, b = 3) {
    return a - b;
    nr_add = "never reached";
};
let add_sub = [ add, sub ];
let mul_div = {
    mul : function(a = 4, b) {
        return a * b;
        nr_add = "never reached";
    },
    div : function(a, b) {
        return a / b;
        nr_add = "never reached";
    }
};
let default_result = add();
let add_result = add(5, 6);
let sub_result = sub(7);
let mul_result = mul_div.mul(null, 10, 11, 12);
let div_result = mul_div["div"](13, 14, 15);
[false, {"foo" : dump}][1].foo();

function add_many(a, b = 2, ...rest) {
    let ret = a + b;
    for (let arg of rest) {
        ret += arg;
    }
    return ret;
}
let result = add_many(1, null, 3, 4, null, 5);

let ret = {
    mul : function(a = 4, b) {
        return a * b;
        nr_add = "never reached";
    },
    div : function(a, b) {
        return a / b;
        nr_add = "never reached";
    }
}["mul"](3, 2);

function recur(param, i) {
    if (i > 10) {
        return "done";
    }
    param[i] = i % 2 == 0;
    return recur(param, i + 1);
}
let recur_param = [];
let recur_result = recur(recur_param, 1);
