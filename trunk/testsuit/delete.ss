/*
=!EXPECTSTART!=
shit
{ x:1, y:2, z:"fuck" }
{ x:1, y:2, z:"fuck" }
1
2
=!EXPECTEND!=
*/

a = { a:"shit", b:{x:1,y:2,z:"fuck"}};
for (x in a) console.log(a[x]);
delete a.a;
for (x in a) console.log(a[x]);
delete a.b.z;

for (x in a.b) console.log(a.b[x]);
