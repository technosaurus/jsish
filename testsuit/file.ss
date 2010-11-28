/*
=!EXPECTSTART!=
=!EXPECTEND!=
*/

var f = new File('proto.h');
if (f) {
	while((n = f.gets())!=undefined) {
        	console.log(n);
	}
} else console.log('Can not open proto.h');
