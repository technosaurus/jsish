/*
=!EXPECTSTART!=
[ undefined ]
[ undefined, 2 ]
5
3
2
1
2
2
undefined
undefined
undefined
0
=!EXPECTEND!=
*/

var a = new Array(1);

console.log(a);
a[1] = 2;
console.log(a);

console.log(a.push(1,2,3));
console.log(a.pop());
console.log(a.pop());
console.log(a.pop());
console.log(a.length);
console.log(a.pop());
console.log(a.pop());
console.log(a.pop());
console.log(a.pop());
console.log(a.length);

