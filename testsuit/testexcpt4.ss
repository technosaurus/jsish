/*
=!EXPECTSTART!=
catch
final
shit
2
2th final
ff
fuck
=!EXPECTEND!=
*/

var a;
try {
for (a = 0; a < 100; ++a) {
	try {
		try {
			throw(1);
		} catch(e) {
			console.log("catch");
			continue;
		} finally {
			console.log("final");
			throw(2);
		}
	} catch(f) {
		console.log("shit");
		console.log(f);
		throw('ff');
	} finally {
		console.log("2th final");
	}
}
} catch(x) {
	console.log(x);
}
console.log("fuck");

