let h = 1;
try {
    let i = 1;throw "something wrong";let j = 2;
} catch (exception) {
    let ex = exception;
    throw ex; // test 'throw' outside 'try' block
}
let greetings = "hello"; // test egress of 'catch'
throw greetings;