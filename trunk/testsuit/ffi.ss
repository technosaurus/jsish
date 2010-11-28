/*
=!EXPECTSTART!=
undefined
================
a:1
b:2
c:3
d:4
================
a:1
b:2
c:3
d:4
shit:fuck
================
a:1
b:2
c:3
d:4
fuck:shit
=!EXPECTEND!=
*/

a = {a: 1,b:2,c:3,d:4};
console.log(a.e);

console.log("================");
for(var s in a) { console.log(s + ":" + a[s]); }

a.shit = "fuck";

console.log("================");
for(var s in a) { a.fuck = "shit"; console.log(s + ":" + a[s]); }

console.log("================");
for(var s in a) { 
	console.log(s + ":" + a[s]);
	delete a.shit; 
}