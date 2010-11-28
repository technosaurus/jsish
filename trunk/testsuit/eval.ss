/*
=!EXPECTSTART!=
{ test:1 }
{ a:1, b:2, c:3, d:4 }
undefined
{ a:1, b:2, c:3, d:4 }
undefined
=!EXPECTEND!=
*/

a = eval("{ test:1}");
console.log(a);

b = eval("console.log({a:1,b:2,c:3,d:4});");
console.log(b);
b = eval("console.log({a:1,b:2,c:3,d:4})", x, n, g, k);
console.log(b);
