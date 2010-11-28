function a() {
    console.log('in a');
};

function b() {
    console.log('in b');
};

var c = a;

c((c = b), 2);
