/*
=!EXPECTSTART!=
y=2
x=2
z=function () {...}
CALLED Z
=!EXPECTEND!=
*/
var a = { x:1, y:0};
a.z = function() { puts('CALLED Z'); };
var b = { y:2 };
b.__proto__ = a;
a.x++;
for (var i in b) { puts(i+'='+b[i]); }
b.z();


