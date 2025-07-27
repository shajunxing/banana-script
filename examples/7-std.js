// test std functions
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

// print(argc, length(argv), argv);
// let cwd = getcwd();
// print(cwd);
// for (let dir of ["d:\\", "~!@#$%^&*()", "z:\\"]) {
//     try {
//         chdir(dir);
//         print(getcwd());
//     } catch (ex) {
//         print(ex);
//     }
// }
// chdir(cwd);
// print(getcwd());

// for (let dir of ["abcdefg1234567", "~!@#$%^&*()", "z:\\asdasd"]) {
//     try {
//         let cwd = getcwd();
//         mkdir(dir);
//         chdir(dir);
//         print(getcwd());
//         chdir(cwd);
//         print(getcwd());
//         print(dir, exists(dir) ? "is exist" : "not exist");
//         rmdir(dir);
//         print(dir, exists(dir) ? "is exist" : "not exist");
//     } catch (ex) {
//         print(ex);
//     }
// }

// print(pathsep);
// print(dirname("foo"));
// print(dirname(""));
// print(dirname(argv[0]));

// listdir("c:", print);
// listdir("c:\\", print);
// listdir("c:/", print);
// listdir("c:\\windows", function(...args) {
//     print(...args);
//     throw "Boom!";
// });
// gc();
// dump();

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

// print(format("-${1}-${0}-", "world", "hello"));

// print(startswith("123456", "123"));
// print(startswith("123456", "456"));
// print(startswith("123456", "123456789"));
// print(endswith("123456", "123"));
// print(endswith("123456", "456"));
// print(endswith("123456", "123456789"));

// let fname = "foo.txt";
// write(fname, "Foo\n");
// write(fname, true, "Bar\n");
// print(read(fname));
// try {
//     let fp = open(fname);
//     print(read(fp));
//     close(fp);
// } catch (err) {
//     print(tojson(err));
// }
// try {
//     open(fname, "a", function(fp) {
//         write(fp, "Baz\n");
//         write(fp, "Qux\n");
//         throw "Boom!";
//     });
// } catch (err) {
//     print(tojson(err));
// }
// open(fname, function(fp) { read(fp, print); });
// remove(fname);
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
// } catch (ex) {
//     print(ex);
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
//         pop(arr);
//         pop(arr);
//     } catch (ex) {
//         ;
//     }
//     gc();
// }

// let str = "--hello--world--";
// try {
//     print(split(str, ""));
// } catch (ex) {
//     print(ex);
// }
// print(split(str, "-"));
// print(split(str, "--"));
// print(split(str, "---"));
// str = "hello--world";
// print(split(str, "-"));
// print(split(str, "--"));
// print(split(str, "---"));

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
// print(arr);
// arr = [ 3, null, 5, "foo", 1, true, false, 2, null, "bar" ];
// sort(arr, function(lhs, rhs) { return lhs - rhs; });
// print(arr);

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
// print(list);
// sort(list, function(lhs, rhs) {
//     return natural_compare(rhs, lhs);
// });
// print(list);

// print(null, true, false, 3.14, "hello", os, [], {}, function() {}, tostring(print));
// print(tostring(null), tostring(true), tostring(false), tostring(3.14),
//     tostring("hello"), tostring(os), tostring([]), tostring({}),
//     tostring(function() {}), tostring(print));
// print(tojson({"list" : [ null, true, false, 3.14, "hello", os, [], {}, function() {}, print ]}));
// print(tonumber(input("Enter a number: ")));

// try {
//     exit("foo");
// } catch (err) {
//     print(tojson(err));
// }
// exit(12.3); // to show exit code, in posix "echo $?" and in windows "echo %errorlevel%"

// regular expression
// print(format("${0}\n${1}\n${2}\n${3}", ...match("Unknown-14886@noemail.invalid", "^([\\w\\.-]+)\\@([\\w-]+)\\.([a-zA-Z\\w]+)$")));