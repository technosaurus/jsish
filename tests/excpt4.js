/*
=!EXPECTSTART!=
catch
final
shot
2
2th final
ff
fock
=!EXPECTEND!=
*/

var a;
try {
for (a = 0; a < 100; ++a) {
    try {
        try {
            throw(1);
        } catch(e) {
            puts("catch");
            continue;
        } finally {
            puts("final");
            throw(2);
        }
    } catch(f) {
        puts("shot");
        puts(f);
        throw('ff');
    } finally {
        puts("2th final");
    }
}
} catch(x) {
    puts(x);
}
puts("fock");

