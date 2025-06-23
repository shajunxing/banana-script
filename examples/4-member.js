let arr1 = [ null, false, true, 3.14, "hello" ];
let mbr1 = arr1[3];
// let mbr2 = arr1[-1];
// let mbr3 = arr1["foo"];
let obj1 = {"name" : "tom", "age" : 31};
let mbr4 = obj1.name;
let mbr5 = obj1["age"];
let mbr6 = obj1.foo;
// let mbr7 = obj1[1234];
let mbr8 = obj1?.foo?.bar;
arr1[100] = "bingo";
let obj2 = {"arr2" : [ obj1, [...arr1 ] ]};
console.log(arr1); // test if node.js skip null