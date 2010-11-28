/*
=!EXPECTSTART!=
at global
predefine must be in global
predefine in global after eval
predefine in global after eval
global predefine: predefine in global after eval
predefine in global after eval
100
predefine in global after eval
=!EXPECTEND!=
*/

var PREDEFINE = 'at global';

function haha() {
	return {
		getpredefine: function() { console.log(PREDEFINE); },
		setpredefine: function(a) { PREDEFINE = a; },
		eval: function(n) { eval(n); }
	};
};

var a = haha();

a.getpredefine();
a.setpredefine('predefine must be in global');
a.getpredefine();
a.eval("PREDEFINE = 'predefine in global after eval';");
a.getpredefine();
a.eval("var PREDEFINE = 'predefine now in local';");
a.getpredefine();
console.log("global predefine: " + PREDEFINE);

function hehe(n)
{
	if (n) eval(n);
	console.log(PREDEFINE);
};

hehe();
hehe("var PREDEFINE = 100;");
hehe();