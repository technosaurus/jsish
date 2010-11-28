/*
=!EXPECTSTART!=
undefined
=!EXPECTEND!=
*/

//test void expr
console.log(void 1);

//test - expr
console.log(-1);
console.log(-0);
console.log(-NaN);
console.log(-Infinity);
console.log(- true);
console.log(- "123" );

//wrong: NaN, not 0
console.log(- {a:1});

//test + expr
console.log(1 + 2);
console.log(1.3 + 2.3);
console.log(1.2 + "12.3");
console.log(4 + true);
console.log("2342" + true);
console.log({} + 12);
console.log(NaN + NaN);
console.log(NaN + "NaN");
console.log(Infinity - Infinity);
console.log(NaN + 3);

console.log("================");
console.log(1<<4);
console.log(1<<344.3);
console.log(2>>4);
console.log(-200000 >> -4);

console.log("===============");
console.log(1.0 < 2.3);
console.log(NaN < NaN);
console.log(Infinity < -Infinity);
console.log(Infinity > -Infinity);
console.log(10000.456 < 10000.456);
console.log(10000.456 > 10000.456);
console.log(10000.456 <= 10000.456);
console.log(10000.456 >= 10000.456);
console.log("10000.456" < "10000.456");
console.log("10000.456" > "10000.456");
console.log("10000.456" <= "10000.456");
console.log("10000.456" >= "10000.456");
console.log("a" > "b");
console.log("a" >= "a");
console.log("a" < "aa");
console.log("a" < "b");
console.log("===============");
console.log(1 == 1);
console.log(2 == 1);
console.log(NaN == NaN);
console.log("2" == 2);
console.log(true == null);
console.log("234234" == "234234");
console.log(true == 1);

console.log("===================");
console.log(1 === true);
console.log(1 === "1");
console.log(NaN === NaN);
console.log("abc" === "abc");
console.log(3.1415926 === 3.1415926);

console.log("============");
console.log(1 | 2 | 4 | 8 | 16);
console.log(123 & 234);
console.log(3456 ^ 2342);

console.log("===============");
var a = 12;
a += 4;
console.log(a);
a -= 4;
console.log(a);
a /= 4;
console.log(a);
a *= 4;
console.log(a);
a %= 4;
console.log(a);
a <<= 4;
console.log(a);
a >>= 3;
console.log(a);
a += true;
console.log(a);
a += "fuck";
console.log(a);

a = { a: 120 };
a.a += 4;
console.log(a);
a.a -= 4;
console.log(a);
a.a /= 4;
console.log(a);
a.a *= 4;
console.log(a);
a.a %= 4;
console.log(a);
a.a <<= 4;
console.log(a);
a.a >>= 3;
console.log(a);
a.a += true;
console.log(a);
a.a += "fuck";
console.log(a);
