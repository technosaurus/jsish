/*
=!EXPECTSTART!=
[ 0 ]
UPDATE
UPDATE
UPDATE
UPDATE
FOO CALLED: 6
=!EXPECTEND!=
*/

var i=0, j = 0, k = 0;
function foo() {
   i++,j++;
}
var id = setInterval(foo,100);
puts(info.event());
while (true) {
   if (k++<=3)
     puts("UPDATE");
   sys.update(50);
   if (i>5) { clearInterval(id); break; }
}
puts("FOO CALLED: "+j);
sys.update();


