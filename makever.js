//#!./jsi
var f = new File('jsiRevision.h');
var n = f.gets();
var rev = n.match(/[0-9]+/);
puts('enum { JSI_VERSION = '+rev[0]+' };');
