/*
=!EXPECTSTART!=
TOP
n
=!EXPECTEND!=
*/

this.name = 'TOP';

function a() {
    puts(this.name);
};

function b(x, y) {
    a();
    puts(this.name);
};

var n = { name: 'n', test: b };

n.test(1, 2);
