/*
=!EXPECTSTART!=
0
0
1970-01-01
=!EXPECTEND!=
*/
var s = sys.strftime(0);
puts(sys.strptime(s));
var s = sys.strftime(0,{utc:true});
puts(sys.strptime(s,{utc:true}));
var t = sys.strptime(s,{utc:true,fmt:"%c"});
puts(sys.strftime(t,{utc:true,fmt:"%Y-%m-%d"}));

