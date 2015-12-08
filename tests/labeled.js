/*
=!EXPECTSTART!=
0, 1
shot
1, 1
shot
2, 1
shot
3, 1
shot
4, 1
shot
{ a:{ b:{ c:[ 4, 3 ] } } }
=!EXPECTEND!=
*/

var a = {a:1, b:2, c:3, d:4, e:5, f:6, g:7, h:8, i:9, j:10};
var b = {a:{b:{c:1}}};
fock: for (var i = 0; i < 5; ++i) {
    for (var j in a) {
        switch(a[j]) {
            case 2:
                continue;
            case 3:
                try {
                    with (b.a.b) {
                        c = [i, a[j]];
                        continue fock;
                    }
                } finally {
                    puts("shot");
                }
            case 4:
                break;
            case 5:
                break fock;
        }
        puts(i + ", " + a[j]);
    }
}

puts(b);
