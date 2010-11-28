/*
=!EXPECTSTART!=
1
2
3
4
xfsldafkjasldf
asdfsadfsadf
6
=!EXPECTEND!=
*/

function a() {
	for (i = 0; i < arguments.length; ++i) {
		console.log(arguments[i]);
	}
};

a(1,2,3,4,"xfsldafkjasldf","asdfsadfsadf");
console.log(i);