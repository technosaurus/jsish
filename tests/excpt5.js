/*
=!EXPECTSTART!=
catch:
abc
final
shot
2
2th final
ff
fock
undefined
=!EXPECTEND!=
*/

function test() {
    try {
        throw('abc');
    } finally {
    
    }
    return 0;
};

var a, n;
try {
    for (a = 0; a < 100; ++a) {
        try {
            try {
                n = test();
            } catch(e) {
                puts("catch:");
                puts(e);
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
puts(n);
