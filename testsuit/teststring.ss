/*
=!EXPECTSTART!=
the original string
the original string
he original string
g

tri

a
4
-1
6
16
=!EXPECTEND!=
*/

var a = "the original string";

console.log(a.substr());
console.log(a.substr(false));
console.log(a.substr(1));
console.log(a.substr(-1, 1));

console.log(a.substr(2, -1));
console.log(a.substr(-5, 3));
console.log(a.substr(100, 0));
console.log(a.substr(10, 1));

console.log(a.indexOf("ori"));
console.log(a.indexOf("swer"));
console.log(a.indexOf("i"));
console.log(a.indexOf("i", 9));
