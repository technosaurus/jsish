/*
=!EXPECTSTART!=
a: 1
this.a: a in x
=!EXPECTEND!=
*/

function fuck(a,b){
	console.log("a: " + a);
	console.log("this.a: " + this.a);
};

x = { test: fuck, a: 'a in x', b: 'b in x' };

{ test: fuck, a: 'a in x', b: 'b in x' }.test(1, 2);
