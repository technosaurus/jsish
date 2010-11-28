/*
=!EXPECTSTART!=
try1
finally2
fuck
finally1
{ b:{ c:0 }, x:0 }
=!EXPECTEND!=
*/

var a = {
	b: {
		c: 0
	},
	x: 0
};

for (var i = 0; i < 10; ++i) {
	try {
		console.log("try1");

		with(a.b) {
			c = i;
			try {
				if (i == 5) throw("shit");
			} catch (e) {
				console.log(e);
				with (a) {
					x = 'shit';
					throw("sadf");
				}
			} finally {
				console.log("finally2");
				throw("fuck");
			}
		}
	} catch(e) {
		console.log(e);
		break;
	} finally {
		console.log("finally1");
	}
}

console.log(a);
