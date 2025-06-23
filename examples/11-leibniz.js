// https://niklas-heer.github.io/speed-comparison/
// https://github.com/niklas-heer/speed-comparison/blob/master/src/leibniz.js

let start_time = clock();
for (let loop = 1;; loop++) {
    let x = 1.0;
    let pi = 1.0;
    let rounds = 1000000;
    let i = 2;
    while (i < rounds + 2) {
        x *= -1;
        pi += x / (2 * i - 1);
        i++;
    }
    pi *= 4;
    gc();
    print((clock() - start_time) / loop, "secs, pi is", pi);
}