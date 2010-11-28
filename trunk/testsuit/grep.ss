/*
=!EXPECTSTART!=
wenxichang@163.com
=!EXPECTEND!=
*/


if (console.args.length < 1) {
	console.log("Usage: smallscript grep.ss <PATTERN>");
	exit(-1);
}

var reg = new RegExp(console.args[0]);
line = "";
while (1) {
	line = console.input();
	if (line == undefined) break;
	if (line.match(reg)) {
		console.log(line);
	}
}

