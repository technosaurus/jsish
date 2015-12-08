/*
=!EXPECTSTART!=
extra args, expected "cmds(?string|pattern ?,all??)" 
=!EXPECTEND!=
*/
function foo() {
  info.cmds(1,2,3,4);
}
try { foo(); } catch(e) { puts(e); }
