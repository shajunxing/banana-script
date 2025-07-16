function createFib() {
    return function(n, g) {
        if (n <= 2)
            return 1;
        return g(n - 1, g) + g(n - 2, g);
    };
}
let fib = createFib();
console.log(fib(36, fib));

