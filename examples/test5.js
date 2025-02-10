function foo(a) {
    return function(b) {
        return function(c) {
            return a + b + c;
        };
    };
}
let bar = foo(1);
let baz = bar(2);
let qux = baz(3);

let a = [ "A", null, "B", null ];
let b = [ "C", ...a, null, "D" ];
let c = [];
c[100] = "E";
let d = [ "F", ...c, "G" ];

function closure(a, ...args1) {
    return function(b, ...args2) {
        return function(c, ...args3) {
            let ret = a + b + c;
            for (let args of [args1, args2, args3]) {
                for (let arg of args) {
                    ret += arg;
                }
            }
            return ret;
        };
    };
}
let result = closure("All ", "This ", "is ")("attention ", "your ", "mother ")("please. ", "fxxker ", "speaking.");

let type1 = typeof null;
let type2 = typeof true;
let type3 = typeof false;
let type4 = typeof 3.14;
let type5 = typeof "hello";
let type6 = typeof[];
let type7 = typeof{};
let type8 = typeof function() {};
// let type9 = typeof gc;
