/*
=!EXPECTSTART!=
abc
finally
=!EXPECTEND!=
*/

try {
    eval("throw('abc');");
} catch(e) {
    console.log(e);
} finally {
	console.log("finally");
}

