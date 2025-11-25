// print(length("foo"));
// print(length([ 1, 2, 3 ]));
// try {
//     print(length());
// } catch (err) {
//     print(err);
// }
// try {
//     print(length(null, true, false));
// } catch (err) {
//     print(err);
// }
// try {
//     print(length(1.2));
// } catch (err) {
//     print(err);
// }

// let obj = {"foo": true, "bar": false};
// print(length(obj));
// obj.qux = 1;
// print(length(obj));
// obj.foo = null;
// print(length(obj));
// obj.bar = null;
// print(length(obj));
// obj.qux = null;
// print(length(obj));

// try {
//     print(input("a", "b", "c", "d"));
// } catch(err) {
//     print(err);
// }
// try {
//     print(input(123));
// } catch(err) {
//     print(err);
// }
// try {
//     print(input("prompt: "));
// } catch(err) {
//     print(err);
// }

// print(argc, length(argv), tojson(argv));

// let wd = cwd();
// print(wd);
// for (let dir of ["d:\\", "~!@#$%^&*()", "z:\\"]) {
//     try {
//         cd(dir);
//         print(cwd());
//     } catch (err) {
//         print(err);
//     }
// }
// cd(wd);
// print(cwd());

// for (let dir of ["abcdefg1234567", "~!@#$%^&*()", "z:\\asdasd", "你好世界"]) {
//     try {
//         let wd = cwd();
//         md(dir);
//         cd(dir);
//         print(cwd());
//         cd(wd);
//         print(cwd());
//         print(dir, exists(dir) ? "is exist" : "not exist");
//         rd(dir);
//         print(dir, exists(dir) ? "is exist" : "not exist");
//     } catch (err) {
//         print(err);
//     }
// }

// for (;;) {
//     print(exists("examples/当我们将文明之梦、科技之梦、生态之梦赠予青年，便是赋予他们力量与使命，激励他们在时代的浪潮中奋勇前行/わたしの趣味はたくさんあります。でも、一番好きな事は写真をとることです。15歳のときに始めてから、もう二年になります/마음이괴로울때가있다.그럴때면나는다른누군가에게의지하기보다혼자극복하려고노력하는편이었다.연인이나친구혹은술에의지하지않고오로지나스스로굳게서려고애"));
//     print(exists("examples/当我们将文明之梦、科技之梦、生态之梦赠予青年，便是赋予他们力量与使命，激励他们在时代的浪潮中奋勇前行/わたしの趣味はたくさんあります。でも、一番好きな事は写真をとることです。15歳のときに始めてから、もう二年になります/마음이괴로울때가있다.그럴때면나는다른누군가에게의지하기보다혼자극복하려고노력하는편이었다.연인이나친구혹은술에의지하지않고오로지나스스로굳게서려고애/"));
//     gc();
// }

// print(pathsep);
// print(dirname("foo"));
// print(dirname(""));
// print(dirname(argv[0]));
// https://www.man7.org/linux/man-pages/man3/dirname.3p.html
// https://www.man7.org/linux/man-pages/man3/basename.3p.html
// if (os == "posix") {
//     for (let path of ["usr", "usr/", "", "/", "//", "///", "/usr/", "/usr/lib", "//usr//lib//", "/home//dwc//test"]) {
//         print(basename(path), dirname(path));
//     }
// } else {
//     for (let path of ["usr", "usr\\", "", "\\", "\\\\", "\\\\\\", "\\usr\\", "\\usr\\lib", "\\\\usr\\\\lib\\\\", "c:\\home\\\\dwc\\\\test"]) {
//         print(basename(path), dirname(path));
//     }
// }

// ls("c:", print);
// ls("c:\\", print);
// ls("c:/", print);
// ls("c:\\windows", function(...args) {
//     print(...args);
//     throw "Boom!";
// });
// gc();
// dump_vm();

// test SetConsoleCP SetConsoleOutputCP utf-8 works good
// for (;;) {
//     ls("C:\\Users\\shajunxing\\Documents", print);
//     gc();
// }

// for (;;) {
//     try {
//         let hello = "Hello";
//         let world = "World";
//         let formatted = format("-${hello}-${world}-");
//         print(formatted);
//     } catch (error) {
//         print(error);
//     }
//     gc();
// }

// for(;;) {
//     print(format("-${1}-${0}-${2}-${3}-${4}-${5}-${6}-", "world", "hello", true, 3.14, {}, []));
//     gc();
// }

// print("123456"::startswith("123456789"));
// print("123456"::startswith("123456789", "456"));
// print("123456"::startswith("123456789", "456", "123"));
// print("123456"::endswith("123456789"));
// print("123456"::endswith("123456789", "123"));
// print("123456"::endswith("123456789", "123", "456"));

// let fname = "foo.txt";
// write(fname, "Foo\n");
// write(fname, true, "Bar\n");
// print(read(fname));
// try {
//     let fp = open(fname);
//     print(read(fp));
//     close(fp);
// } catch (err) {
//     print(err);
// }
// try {
//     open(fname, "a", function(fp) {
//         write(fp, "Baz\n");
//         write(fp, "Qux\n");
//         throw "Boom!";
//     });
// } catch (err) {
//     print(err);
// }
// open(fname, function(fp) { read(fp, print); });
// rm(fname);
// write(stdout, "Hello\n");

// print(read("make.bat"));
// let line_no = 0;
// read("make.sh", function(line) {
//     print(line_no, ":", line);
//     line_no++;
// });
// try {
//     read("make.c", function() {
//         throw "Boom!";
//     });
// } catch (err) {
//     print(err);
// }
// let cmd = os == "windows" ? "dir /b" : "ls -l";
// print(read(cmd, true));
// line_no = 0;
// read(cmd, true, function(line) {
//     print(line_no, ":", line);
//     line_no++;
// });
// print(stdin, stdout, stderr);
// print("print(read(stdin)), ctrl-z (windows) or ctrl-d (posix) to end, you must press at line beginning:");
// print(read(stdin));
// print("read(stdin, print), ctrl-z (windows) or ctrl-d (posix) to end, you must press at line beginning:");
// read(stdin, print);

// let arr = [];
// for (;;) {
//     try {
//         push(arr, "hello");
//         push(arr, "world");
//         print(tojson(arr));
//         print(pop(arr));
//         print(pop(arr));
//         print(pop(arr));
//         print(pop(arr));
//         print(tojson(arr));
//     } catch (err) {
//         print(err);
//     }
//     gc();
// }

// let str = "--hello--world--";
// split(str)::tojson()::print();
// split(str, "")::tojson()::print();
// split(str, "-")::tojson()::print();
// split(str, "--")::tojson()::print();
// split(str, "---")::tojson()::print();
// str = "hello--world";
// split(str)::tojson()::print();
// split(str, "")::tojson()::print();
// split(str, "-")::tojson()::print();
// split(str, "--")::tojson()::print();
// split(str, "---")::tojson()::print();

// print(join(split("--hello--world--", "-"), "-"));
// let arr = [];
// print(join(arr, ""));
// print(join(arr, "--"));
// push(arr, "hello");
// print(join(arr, ""));
// print(join(arr, "--"));
// push(arr, "world");
// print(join(arr, ""));
// print(join(arr, "--"));
// arr[1000] = "folks";
// print(join(arr, ""));
// print(join(arr, "--"));

// let arr = [ 5, 3, 9, 2, 3, 8, 2, 5, 9, 7 ];
// sort(arr, function(lhs, rhs) { return lhs - rhs; });
// arr::tojson()::print();
// arr = [ 3, null, 5, "foo", 1, true, false, 2, null, "bar" ];
// sort(arr, function(lhs, rhs) { return lhs - rhs; });
// arr::tojson()::print();

// let list = [
//     "1000X Radonius Maximus",
//     "10X Radonius",
//     "200X Radonius",
//     "20X Radonius",
//     "20X Radonius Prime",
//     "30X Radonius",
//     "40X Radonius",
//     "Allegia 50 Clasteron",
//     "Allegia 500 Clasteron",
//     "Allegia 50B Clasteron",
//     "Allegia 51 Clasteron",
//     "Allegia 6R Clasteron",
//     "Alpha 100",
//     "Alpha 2",
//     "Alpha 200",
//     "Alpha 2A",
//     "Alpha 2A-8000",
//     "Alpha 2A-900",
//     "Callisto Morphamax",
//     "Callisto Morphamax 500",
//     "Callisto Morphamax 5000",
//     "Callisto Morphamax 600",
//     "Callisto Morphamax 6000 SE",
//     "Callisto Morphamax 6000 SE2",
//     "Callisto Morphamax 700",
//     "Callisto Morphamax 7000",
//     "Xiph Xlater 10000",
//     "Xiph Xlater 2000",
//     "Xiph Xlater 300",
//     "Xiph Xlater 40",
//     "Xiph Xlater 5",
//     "Xiph Xlater 50",
//     "Xiph Xlater 500",
//     "Xiph Xlater 5000",
//     "Xiph Xlater 58"
// ];
// sort(list, natural_compare);
// for (let elem of list) {
//     print(elem);
// }
// print();
// sort(list, function(lhs, rhs) {
//     return natural_compare(rhs, lhs);
// });
// for (let elem of list) {
//     print(elem);
// }
// print();

// print(null, true, false, 3.14, "hello", os, [], {}, function() {}, print);
// print(tostring(null), tostring(true), tostring(false), tostring(3.14), tostring("hello"), tostring(os), tostring([]), tostring({}), tostring(function() {}), tostring(print));
// print(todump(null), todump(true), todump(false), todump(3.14), todump("hello"), todump(os), todump([]), todump({}), todump(function() {}), todump(print));
// print(tojson({"list" : [ null, true, false, 3.14, "hello", os, [], {}, function() {}, print ], "map" : {"a" : function() {}, "b" : print}}));
// print("hello\n\tworld", [ "hello\n\tworld", [ "hello\n\tworld" ] ], {"foo" : "hello\n\tworld", "bar" : {"qux" : "hello\n\tworld"}});
// (function(a, b) {
//     let c = 3;
//     dump_vm();
// })(1, 2);
// gc();
// dump_vm();

// for (;;) {
//     try {
//         print(tonumber(input("Enter a number: ")));
//         break;
//     } catch (err) {
//         print(err);
//     }
// }

// try {
//     exit("foo");
// } catch (err) {
//     print(err);
// }
// exit(12.3); // to show exit code, in posix "echo $?" and in windows "echo %errorlevel%"

// regular expression
// print(format("${0}\n${1}\n${2}\n${3}", ...match("Unknown-14886@noemail.invalid", "^([\\w\\.-]+)\\@([\\w-]+)\\.([a-zA-Z\\w]+)$")));
// "filename.ext"::match("[^\\.]+$")::tojson()::print();
// "filename.ext"::match("[^\.]+$")::tojson()::print();
// "filename.ext"::match("[^.]+$")::tojson()::print();
// "lK98hBgmK*tNNkYt5E3fv"::match("[0-9a-zA-Z]+")::tojson()::print();
// "lK98hBgmK*tNNkYt5E3fv"::match("^[0-9a-zA-Z]+")::tojson()::print();
// "lK98hBgmK*tNNkYt5E3fv"::match("[0-9a-zA-Z]+$")::tojson()::print();

// let pairs = [
//     [ "aaabbbccc", "a*" ],
//     [ "aaabbbccc", "b*" ]
// ];
// for (let pair of pairs) {
//     print(match(pair[0], pair[1])::tojson());
// }

// system("exit 123")::print();

// (whoami() == "root" || exec("su", "-c", argv::join(" ")));

// for (;;) {
//     whoami()::print();
//     gc();
// }

// if (os == "windows") {
//     spawn("cmd", "/c", "start", "js", "examples/8-print-args.js", "foo bar", "baz qux")::print();
//     spawn("notepad", "make.bat")::print();
// } else {
//     spawn("xterm", "-e", "bin/js", "examples/8-print-args.js", "foo bar", "baz qux")::print();
//     spawn("xmessage", "-font", "variable", "-title", "Greetings", "Hello, World!")::print();
//     spawn("xclock", "-update", "1")::print();
// }

// if (os == "windows") {
//     for (;;) {
//         spawn("cmd", "/c", "type", "examples\\当我们将文明之梦、科技之梦、生态之梦赠予青年，便是赋予他们力量与使命，激励他们在时代的浪潮中奋勇前行\\わたしの趣味はたくさんあります。でも、一番好きな事は写真をとることです。15歳のときに始めてから、もう二年になります\\마음이괴로울때가있다.그럴때면나는다른누군가에게의지하기보다혼자극복하려고노력하는편이었다.연인이나친구혹은술에의지하지않고오로지나스스로굳게서려고애");
//         sleep(0.1);
//         gc();
//     }
// }

// print(time());
// print(ctime(0));
// print(ctime(time()));

// ls(cwd(), function(filename, isdir) {
//     if (isdir) {
//         return;
//     }
//     print(filename);
//     let st = stat(filename);
//     print("   ", "size:", st.size);
//     print("   ", "atime:", ctime(st.atime));
//     print("   ", "ctime:", ctime(st.ctime));
//     print("   ", "mtime:", ctime(st.mtime));
//     print("   ", "uid:", st.uid);
//     print("   ", "gid:", st.gid);
// });
// print(stat("lkasdjhasdkjh"));

// let begin = time();
// sleep(1.5);
// let end = time();
// print(end - begin);

// let period = 10;
// let interval = 0.2;
// let period_str = period::trunc()::tostring();
// sleep(period, function(remained){
//     let msg = format("${0} / ${1}", remained::trunc()::tostring(), period_str);
//     title(msg);
// }, interval);
// print();

// for (;;) {
//     title("你好世界");
//     gc();
// }

// let s = "`abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`1234567890-=~!@#$%^&*()_+[]\\{}|;':\",./<>?你好，世界。";
// let sl = tolower(s);
// let su = toupper(s);
// print(s);
// print(sl);
// print(su);

// function replace(str, pat, rep) {
//     let m = str::match(pat);
//     print(m::tojson());
//     return format(rep, ...m);
// }
// print(replace("Hello World", "([^\\s]+)\\s([^\\s]+)", "~~${1}~~${2}~~"));

// print("ceil");
// print(ceil(+2.4)); // = +3.0
// print(ceil(-2.4)); // = -2.0
// print(ceil(-0.0)); // = -0.0
// print("floor");
// print(floor(+2.7)); // = +2.0
// print(floor(-2.7)); // = -3.0
// print(floor(-0.0)); // = -0.0
// print("trunc");
// print(trunc(+2.7)); // = +2.0
// print(trunc(-2.7)); // = -2.0
// print(trunc(-0.0)); // = -0.0
// print("round");
// print(round(+2.3)); // = +2.0
// print(round(+2.5)); // = +3.0
// print(round(+2.7)); // = +3.0
// print(round(-2.3)); // = -2.0
// print(round(-2.5)); // = -3.0
// print(round(-2.7)); // = -3.0
// print(round(-0.0)); // = -0.0
// print("modf");
// modf(123.45)::tojson()::print();

// let pid = fork();
// print(pid);
// if (pid == 0) {
//     exec("xclock");
// } else {
//     print("parent process exit");
// }

// let arr = [ null, 1.2, null, true, null, "Hello", null, {"Name" : "John"}, null ];
// try {
//     arr::filter(function(elem) { throw "Boom!"; });
// } catch (err) {
//     err::todump()::print();
// }
// try {
//     arr::filter(function(elem) { return null; });
// } catch (err) {
//     err::todump()::print();
// }
// try {
//     arr::map(function(elem) { throw "Boom!"; });
// } catch (err) {
//     err::todump()::print();
// }
// try {
//     arr::reduce(function(elem) { throw "Boom!"; });
// } catch (err) {
//     err::todump()::print();
// }
// [] ::filter(function(elem) { return elem != null; })::print();
// [] ::map(tostring)::print();
// function add(a, b) {
//     return a + b;
// }
// [] ::reduce(add)::print();
// [1] ::reduce(add)::print();
// [1, 2] ::reduce(add)::print();
// [1, 2, 3] ::reduce(add)::print();
// arr::filter(function(elem) { return elem != null; })::map(tostring)::reduce(add)::print();

for (;;) {
    play("C:\\Windows\\Media\\Alarm01.wav");
    gc();
}