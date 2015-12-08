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
        puts("catch: b:" + b);
    } finally {
        puts("finally: b:" + b);
        throw({b:2});
    }
} catch (b) {
    puts(b);
} finally {
    puts(b);
}
