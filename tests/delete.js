/*
=!EXPECTSTART!=
shot
{ x:1, y:2, z:"fock" }
{ x:1, y:2, z:"fock" }
1
2
=!EXPECTEND!=
*/

a = { a:"shot", b:{x:1,y:2,z:"fock"}};
for (x in a) puts(a[x]);
delete a.a;
for (x in a) puts(a[x]);
delete a.b.z;

for (x in a.b) puts(a.b[x]);
