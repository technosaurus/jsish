/*
=!EXPECTSTART!=
catch: b:[object Object]
finally: b:1
{ b:2 }
1
=!EXPECTEND!=
*/

var i = 0, b = 0;
i++;
try {
	try {
		b++;
	    throw({a:1});
	} catch (b) {
        console.log("catch: b:" + b);
	} finally {
		console.log("finally: b:" + b);
		throw({b:2});
	}
} catch (b) {
	console.log(b);
} finally {
	console.log(b);
}
