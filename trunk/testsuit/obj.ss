/*
=!EXPECTSTART!=
{ a:[ { b:1, c:[ 2, 3, 5, 6 ], d:{ x:1, y:2 } }, 2, 3, 4 ], x:"yz" }
{ x:{ a:1, b:2 }, y:{ a:3, b:4 }, abc:1, cde:"123123" }
=!EXPECTEND!=
*/

var x = {a:[{b:1,c:[2,3,5,6],d:{x:1,y:2}},2,3,4],x:"yz"};
console.log (x);

var a = { abc:1,cde:"123123",x:{a:1,b:2},y:{a:3,b:4}};
console.log(a);

