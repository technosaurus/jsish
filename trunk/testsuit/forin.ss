/*
=!EXPECTSTART!=
0:1
1:2
2:3
4:5
a:
1
b:
2
c:
3
d:
[ 4, 3, 2, 1 ]
e:
{ x:"x", y:"y", z:"z" }
=!EXPECTEND!=
*/

var x = [1,2,3,4,5, 6,7,8];
for (var i in x) { 
	console.log(i + ":" +x[i]); 
	if (i == 3) continue;
	if (i > 4) break;
}

var obj = { a: 1, b:2, c:3, d:[4,3,2,1], e:{x:"x", y:"y", z:"z"}};
for (i in obj) {
	console.log(i + ":");
	console.log( obj[i]);
}
