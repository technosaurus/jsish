/*
=!EXPECTSTART!=
Have native helper: false
1999 2999 4999 8999
=!EXPECTEND!=
*/

// prime.js

try { load("prime.so"); } catch(e) { };

// Pure Ecmascript version of low level helper
function primeCheckEcmascript(val, limit) {
    for (var i = 2; i <= limit; i++) {
        if ((val % i) == 0) { return false; }
    }
    return true;
}

// Select available helper at load time
var primeCheckHelper = (this.primeCheckNative || primeCheckEcmascript);

// Check 'val' for primality
function primeCheck(val) {
    if (val == 1 || val == 2) { return true; }
    var limit = Math.ceil(Math.sqrt(val));
    while (limit * limit < val) { limit += 1; }
    return primeCheckHelper(val, limit);
}

// Find primes below one million ending in '9999'.
function primeTest() {
    var res = [];

    puts('Have native helper: ' + (primeCheckHelper !== primeCheckEcmascript));
    for (var i = 1; i < 10000; i++) {
        if (primeCheck(i) && (i % 1000) == 999) { res.push(i); }
    } 
    puts(res.join(' '));
}

primeTest();

