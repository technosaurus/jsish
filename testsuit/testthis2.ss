/*
=!EXPECTSTART!=
TOP
n
=!EXPECTEND!=
*/

this.name = 'TOP';

function a() {
	console.log(this.name);
};

function b(x, y) {
	a();
	console.log(this.name);
};

var n = { name: 'n', test: b };

n.test(1, 2);
