/*
=!EXPECTSTART!=
11
30
130
1
2
7
12
112
130
=!EXPECTEND!=
*/

var a = 1;
var b = 2;

function abc(a,b) {
	var c = a+b;
	var d = a * b;
	console.log(c);
	console.log(d);
	return d;
};

console.log(x = abc(5,6) + 100);

console.log(a);
console.log(b);
console.log(abc(3,4)+100);

console.log(x);
