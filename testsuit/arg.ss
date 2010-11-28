/*
=!EXPECTSTART!=
3
[ "1", "2", "3" ]
argv[0] = 1
argv[1] = 2
argv[2] = 3
=!EXPECTEND!=
*/
console.log(console.args.length);
console.log(console.args);

for (i = 0; i < console.args.length; ++i) {
	console.log ("argv[" + i + "] = " + console.args[i]);
}



