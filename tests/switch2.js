/*
=!EXPECTSTART!=
10
20
30
20
30
30
7
=!EXPECTEND!=
*/

for(var i = 0; i < 100; ++i) {
    switch(i) {
        case 10:
            puts("10");
        case 20:
            puts("20");
        case 30:
            puts("30");
            break;
        case 40: {
            continue;
            puts("40");
        }
        break;
        case 41: break;
        case 50:
            for (var j = 0; j < 10; ++j) {
                if (j < 5) continue;
                if (j > 6) break;
            }
            puts(j);
            break;
        default:
            break;
    }
    if (i == 40) puts("hehe");
}
