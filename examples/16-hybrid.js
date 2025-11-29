forward(function(...args) {
    forward(function(...args) {
        forward(function(...args) {
            print(...args);
        }, ...args);
    }, ...args);
}, null, true, 3.14, "hello");
