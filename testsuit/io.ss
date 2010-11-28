/*
=!EXPECTSTART!=
Can not open file: 1
=!EXPECTEND!=
*/

if (console.args.length < 1) {
	console.log("Usage: smallscript io.ss <filename>");
	exit(-1);
}

var file = new File(console.args[0], "r");
if (!file) {
	console.log("Can not open file: " + console.args[0]);
	exit(-1);
}

while ((line = file.gets()) != undefined) {
	console.log(line);
}

file.close();
