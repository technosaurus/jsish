/*
=!EXPECTSTART!=
1
2
3
3000000
1000
1000
=!EXPECTEND!=
*/

i = 0;
result = 0; 
while (i<3) {
    j = 0; 
    while (j<1000) {
        k = 0; 
        while (k<1000) {
            ++k; 
            ++result; 
        }
        ++j; 
    }
    ++i;
    puts(i);
}
puts(result);
puts(j);
puts(k);

