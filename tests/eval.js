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
puts(a);

b = eval("puts({a:1,b:2,c:3,d:4});");
puts(b);
b = eval("puts({a:1,b:2,c:3,d:4})", x, n, g, k);
puts(b);
