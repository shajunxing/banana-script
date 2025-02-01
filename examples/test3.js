let f1 = 0;
for (let i1 = 0; i1 < 10; i1++)
    f1 += i1;

let i2 = [ {} ], f2 = 0;
for (i2[0].foo = 0; i2[0].foo < 10; i2[0].foo++) {
    f2 += i2[0].foo;
}

let i3 = 0, f3 = 0;
for (; i3 < 10; i3++) {
    f3 += i3;
}

let i4 = 0, f4 = 0;
for (;;) {
    if (i4 < 10) {
        f4 += i4;
        i4++;
    } else {
        break;
    }
}

while (false) {
}
let foo = "foo";
do {
} while (false);
let bar = "bar";
for (; false;) {
}
let baz = "baz";

let a1 = [ null, false, 3.14, "hello" ];
let a2 = [];
for (let i in a1) {
    a2[i] = a1[i];
}
let i = 0;
let a3 = [];
for (let v of a1) {
    a3[i] = v;
    i++;
}
let o1 = {"foo" : null, "bar" : false, "baz" : 3.14, "qux" : "hello"};
let o2 = {};
for (let k in o1) {
    o2[k] = o1[k];
}
i = 0;
let o3 = [];
for (let v of o1) {
    o3[i] = v;
    i++;
}
let long_arr = [ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L" ];
let short_arr = [];
for (let i in long_arr) {
    if (i > 7) {
        break;
    } else if (i % 2 == 0) {
        continue;
    } else {
        short_arr[i] = long_arr[i];
    }
}

for (let i in [])
    ;
;
;
for (let v of [])
    ;
;
;
for (let k in {})
    ;
;
;
for (let v of {})
    ;
;
;

let hello = "hello";