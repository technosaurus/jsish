/*
=!EXPECTSTART!=
2400
2400
1.212.3
NaN
99
-9223372036854775808
99
99
=!EXPECTEND!=
*/
puts(parseFloat(2400));
puts(parseFloat('2400'));
puts(1.2 + "12.3");
puts(parseFloat('xx'));
puts(parseFloat('99'));
puts(parseInt('xx'));
puts(parseInt('99'));
puts(parseInt('99.9'));

