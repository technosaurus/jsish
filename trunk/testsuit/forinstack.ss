/*
=!EXPECTSTART!=
A
fin
fuck
=!EXPECTEND!=
*/

var a = {a:1, b:2};

try {
for (var n in a) {
	try {
		switch(n) {
			case "a":
				console.log("A");
				continue;
			case "b":
				console.log("B");
				throw("ex");
		}
	} catch(e) {
		console.log(e);
		throw(e);
	} finally {
		console.log("fin");
		throw("fuck");
	}
}

} catch (e) {
	console.log(e);
}