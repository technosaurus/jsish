/*
=!EXPECTSTART!=
B=1, A=99
B=2, A=95
B=3, A=91
[ { a:99, b:1 }, { a:95, b:2 }, { a:91, b:3 } ]
CONF: { bindWarn:false, debug:0, errorCnt:0, execOpts:{ callback:null, cdata:null, headers:false, limit:0, mapundef:false, mode:"rows", nocache:false, nullvalue:null, separator:null, table:null, width:undefined }, forceInt:false, maxStmts:1000, mutex:"default", name:"", nocreate:false, readonly:false, trace:[  ], vfs:null }
95
EXECING: select * from foo;
[ { a:99, b:1 }, { a:95, b:2 }, { a:91, b:3 } ]
EXECING: select * from foo;
99|1
95|2
91|3
EXECING: select * from foo;
a|b
99|1
95|2
91|3
EXECING: select * from foo;
<TR><TD>99</TD><TD>1</TD></TR>
<TR><TD>95</TD><TD>2</TD></TR>
<TR><TD>91</TD><TD>3</TD></TR>
EXECING: select * from foo;
<TR><TH>a</TH><TH>b</TH></TR>
<TR><TD>99</TD><TD>1</TD></TR>
<TR><TD>95</TD><TD>2</TD></TR>
<TR><TD>91</TD><TD>3</TD></TR>
EXECING: select * from foo;
99,1
95,2
91,3
EXECING: select * from foo;
a,b
99,1
95,2
91,3
EXECING: select * from foo;
a          b          99         1         
95         2         
91         3         
EXECING: select * from foo;
a          b           
---------- ----------
99         1         
95         2         
91         3         
EXECING: select * from foo;
[ {"a":99, "b":1}, {"a":95, "b":2}, {"a":91, "b":3} ]
EXECING: select * from foo;
[ ["a", "b"], [99, 1], [95, 2], [91, 3] ]
EXECING: select * from foo;
{ "names": [ "a", "b" ], "values": [ [99, 1 ], [95, 2 ], [91, 3 ] ] } 
EXECING: select * from foo;
99	1
95	2
91	3
EXECING: select * from foo;
a	b
99	1
95	2
91	3
EXECING: select * from foo;
    a = 99    b = 1
    a = 95
    b = 2
    a = 91
    b = 3
EXECING: select * from foo;
INSERT INTO table VALUES(99,NULL);
INSERT INTO table VALUES(95,NULL);
INSERT INTO table VALUES(91,NULL);
TRACING: select bar(a) from foo where b == 2;
95.000
function (sql) {...}
=!EXPECTEND!=
*/


var db = new Sqlite('/tmp/testsql.db',{maxStmts:1000, readonly:false});
//var db = new Sqlite('/tmp/testsql.db');

db.evaluate('drop table IF EXISTS foo;');
try {
  db.exec('drop table foo;', null);
} catch(e) {
  //puts("OK");
};

db.evaluate('drop table IF EXISTS foo; create table foo(a,b);');
//db.evaluate('drop table IF EXISTS foo; create table foo(a,b);');
var n = 0;
var x = 99;
while (n++ < 3) {
  db.exec('insert into foo values(@x,@n);');
  x -= 4;

  //db.exec('insert into foo values("x",' + n + ');');
}
db.exec('select * from foo;', function (n) {
    puts("B="+n.b + ", A="+n.a);
});

puts(db.exec('select * from foo;').toString());

db.exec('select * from foo;', null);

puts("CONF: "+db.conf().toString());
db.profile(function(sql,time) { puts("EXECING: "+sql); });

puts(db.onecolumn('select a from foo where b == 2;'));
var s = {};
s.b = 2;

puts(db.exec('select * from foo;'));
puts(db.exec('select * from foo;',{mode:'list'}));
puts(db.exec('select * from foo;',{mode:'list',headers:true}));
puts(db.exec('select * from foo;',{mode:'html'}));
puts(db.exec('select * from foo;',{mode:'html',headers:true}));
puts(db.exec('select * from foo;',{mode:'csv'}));
puts(db.exec('select * from foo;',{mode:'csv',headers:true}));
puts(db.exec('select * from foo;',{mode:'column'}));
puts(db.exec('select * from foo;',{mode:'column',headers:true}));
puts(x=db.exec('select * from foo;',{mode:'json'}));
JSON.parse(x);
puts(x=db.exec('select * from foo;',{mode:'json',headers:true}));
JSON.parse(x);
puts(x=db.exec('select * from foo;',{mode:'json2'}));
JSON.parse(x);
puts(db.exec('select * from foo;',{mode:'tabs'}));
puts(db.exec('select * from foo;',{mode:'tabs',headers:true}));
puts(db.exec('select * from foo;',{mode:'line'}));
puts(db.exec('select * from foo;',{mode:'insert'}));

db.func('bar',function(n) { return n+'.000'; });


db.trace(function(sql) { puts("TRACING: "+sql); });
puts(db.onecolumn('select bar(a) from foo where b == 2;'));
puts(db.trace());

//puts(db.version());
delete db;


/*
var res1 = db.exec('select * from table foo(a,b);');
for (i in res1) {
  puts("CONS: "+i.toString()); 
}


var curs, n;
for (curs = db.exec('select * from table foo(a,b);'),
    (n = db.getresult(curs))!=undefined,
    db.nextresult(curs)) {
    puts(n.toString());
}
db.endresult(curs);


*/
