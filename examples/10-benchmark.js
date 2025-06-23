function bench_1() {
    let start_time = clock();
    let round = 1000000;
    for (let i = 1;; i++) {
        gc();
        let arr = [];
        for (let j = 0; j < round; j++) {
            arr[j] = j * j;
        }
        print((clock() - start_time) / round / i);
    }
}
function bench_2() {
    let start_time = clock();
    let round = 1000000;
    for (let i = 1;; i++) {
        gc();
        let obj = {"foo" : [ {}, [] ]};
        for (let j = 0; j < round; j++) {
            obj.foo[0].bar = j;
            obj.foo[1][j] = "Hello," + "World!";
        }
        print((clock() - start_time) / round / i);
    }
}
bench_2();

