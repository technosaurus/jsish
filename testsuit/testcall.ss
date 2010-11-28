/*
=!EXPECTSTART!=
{ a:1 }
{ a:1 }
1
2
3
3
{ fuck:32 }
4
5
6
9
1
3
{ a:1 }
4
6
{ a:1 }
1
3
{ fn:4 }
4
6
{ fm:8 }
=!EXPECTEND!=
*/

this.a = 1;
console.log(this);

function f(x, y, z) {
	var a = arguments[0] + arguments[1];
	console.log(this);
	console.log(x);
	console.log(y);
	console.log(z);
	console.log(a);
	var ff = function (a) {
		console.log(x);
		console.log(z);
		console.log(this);
	};
	return ff;
};

var fn = f(1, 2,3);

fm = f.call({fuck:32}, 4,5,6);

fn(456);
fm(789);

fn.call({fn:4}, 456);
fm.call({fm:8}, 55667);

