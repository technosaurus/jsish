/*
=!EXPECTSTART!=
BAR
=!EXPECTEND!=
*/
var b = { a:1, b:2};
b.foo = function () {};
function c() {}
c.prototype.bar = function () {puts('BAR');};
var C = new c();
C.bar();
return;
c.xx();
var a = [1,2,3];
a.xx();

