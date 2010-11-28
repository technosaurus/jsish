/*
=!EXPECTSTART!=
100
=!EXPECTEND!=
*/

function a(x) {
	arguments[0] = 100; 
	console.log(x);
};

a(1);