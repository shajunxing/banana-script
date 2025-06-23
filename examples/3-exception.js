try {
    throw "something wrong";
} catch (exception) {
    let ex = exception;
    throw ex; // test 'throw' outside 'try' block
}
let greetings = "hello"; // test egress of 'catch'
