/*
=!EXPECTSTART!=
Can not open file: g
=!EXPECTEND!=
*/

if (console.args.length < 1) {
    puts("Usage: jsi io.ss <filename>");
    exit(0);
}

var fp;
try {
  fp = new File(console.args[0], "r");
}
catch (e) {
    puts("Can not open file: " + console.args[0]);
    exit(0);
}

while ((line = fp.gets()) != undefined) {
    puts(line);
}

fp.close();
