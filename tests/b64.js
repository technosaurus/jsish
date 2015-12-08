/*
=!EXPECTSTART!=
1234 ---> MTIzNA==
=!EXPECTEND!=
*/
var j = sys.b64encode('1234');
puts(sys.b64decode(j) + ' ---> ' + j);
