/*
=!EXPECTSTART!=
catch:
abc
final
shit
2
2th final
ff
fuck
undefined
=!EXPECTEND!=
*/

function test() {
	try {
		throw('abc');
	} finally {
	
	}
	return 0;
};

var a, n;
try {
	for (a = 0; a < 100; ++a) {
		try {
			try {
				n = test();
			} catch(e) {
				console.log("catch:");
				console.log(e);
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
console.log(n);
