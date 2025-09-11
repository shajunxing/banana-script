// test std functions
// print(length("foo"));
// print(length([ 1, 2, 3 ]));
// try {
//     print(length());
// } catch (err) {
//     print(tojson(err));
// }
// try {
//     print(length(null, true, false));
// } catch (err) {
//     print(tojson(err));
// }
// try {
//     print(length(1.2));
// } catch (err) {
//     print(tojson(err));
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
//     print(tojson(err));
// }
// try {
//     print(input(123));
// } catch(err) {
//     print(tojson(err));
// }
// try {
//     print(input("prompt: "));
// } catch(err) {
//     print(tojson(err));
// }

// print(argc, length(argv), tojson(argv));

// let wd = cwd();
// print(wd);
// for (let dir of ["d:\\", "~!@#$%^&*()", "z:\\"]) {
//     try {
//         cd(dir);
//         print(cwd());
//     } catch (err) {
//         print(tojson(err));
//     }
// }
// cd(wd);
// print(cwd());

// for (let dir of ["abcdefg1234567", "~!@#$%^&*()", "z:\\asdasd"]) {
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
//         print(tojson(err));
//     }
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
//     print(tojson(err));
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
//         print(tojson(err));
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
// print(tojson({"list" : [ null, true, false, 3.14, "hello", os, [], {}, function() {}, print ]}));
// for (;;) {
//     try {
//         print(tonumber(input("Enter a number: ")));
//         break;
//     } catch (err) {
//         print(tojson(err));
//     }
// }

// try {
//     exit("foo");
// } catch (err) {
//     print(tojson(err));
// }
// exit(12.3); // to show exit code, in posix "echo $?" and in windows "echo %errorlevel%"

// regular expression
// print(format("${0}\n${1}\n${2}\n${3}", ...match("Unknown-14886@noemail.invalid", "^([\\w\\.-]+)\\@([\\w-]+)\\.([a-zA-Z\\w]+)$")));
// "filename.ext"::match("[^\\.]+$")::tojson()::print();
// "filename.ext"::match("[^\.]+$")::tojson()::print();
// "filename.ext"::match("[^.]+$")::tojson()::print();

// system("exit 123")::print();

// whoami()::print();

// exec("cl.exe");
// exec("ping.exe", "www.baidu.com", "-t");

// (whoami() == "root" || exec("su", "-c", argv::join(" ")));
// whoami()::print();

// if (os == "windows") {
//     spawn("cmd", "/c", "start", "js", "examples/8-print-args.js", "foo bar", "baz qux")::print();
//     spawn("notepad", "make.bat")::print();
// } else {
//     spawn("xterm", "-e", "bin/js", "examples/8-print-args.js", "foo bar", "baz qux")::print();
//     spawn("xmessage", "-font", "variable", "-title", "Greetings", "Hello, World!")::print();
//     spawn("xclock", "-update", "1")::print();
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

// sleep(3.6, function(remained){
//     write(stdout, format("\rTime remained: ${0}", tostring(remained)));
// }, 0.3);
// print();

