/*
=!EXPECTSTART!=
{ type:"number" }
[ "xx", "x" ]
[ "xx", "x", "y" ]
[ "a" ]
[ "XX", "X" ]
[ "XX", "X", "Y" ]
F
[ "z" ]
[ "f", "g" ]
info.js
info.js
info.js
[ "load" ]
{ argStr:"str?,str,...?", help:"Append one or more strings", maxArgs:-1, minArgs:1, name:"String.concat", type:"command" }
[ "P", "Q" ]
=!EXPECTEND!=
*/
var x = 1;
var xx = 2;
var y = 2;

puts(info.vars('x'));
puts(info.vars(/x/));
puts(info.vars());

function X(a) {var jj=1, kk; return jj++;}
function XX(a) {}
function Y(a) {}

puts(info.funcs('X').args);
puts(info.funcs(/X/));
puts(info.funcs());

var K = {};
K.f = function(z) { puts("F"); };
K.g = function(z) { puts("G"); };
K.f();
puts(info.funcs(K.f).args);
puts(info.funcs(K));

puts(file.tail(info.script()));
puts(file.tail(info.script(XX)));
puts(file.tail(info.script(/.*/)[0]));

puts(info.cmds(/^loa./));
puts(info.cmds('String.concat'));

X.prototype.P = function(M) { return M; };
X.prototype.Q = function(M) { return M; };
puts(info.funcs(X.prototype));

