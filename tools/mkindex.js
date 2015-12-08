#!/usr/bin/env jsish
// Create the Master index file for the wiki.

var flst = [], clst = [], jlst = [], lst = file.glob('*.wiki');

for (var i in lst) {
  if (lst[i].match('^c-')) clst.push(lst[i]);
  else if (lst[i].match('^js-')) jlst.push(lst[i]);
  else flst.push(lst[i]);
}
puts("<title>Master Index</title><table><tr><td valign=top>");
puts("\n<h4>General");
var dat,title;
for (var i in flst.sort()) {
  if (flst[i] == "jsindex.wiki") continue;
  if (flst[i] == "index.wiki") continue;
  dat = file.read(flst[i]);
  title = dat.match('<title>([^>]+)</title>');
  if (!title)
    console.log("failed on "+flst[i]);
  puts('  *  <a href='+flst[i]+'>'+title[1]+'</a>');
}
puts("</td><td valign=top>");
puts("\n<h4>Script");
var dat,title, ttl;
for (var i in jlst.sort()) {
    dat = file.read(jlst[i]);
    title = dat.match('<title>([^>]+)</title>');
    if (!title)
    console.log("failed on "+flst[i]);
    puts('  *  <a href='+jlst[i]+'>'+title[1]+'</a>');
}
puts("</td><td valign=top>");
puts("\n<h4>C-API");
for (var i in clst.sort()) {
    dat = file.read(clst[i]);
    title = dat.match('<title>([^>]+)</title>');
    if (!title)
        console.log("failed on "+flst[i]);
    ttl = title[1]; 
    if (ttl.substr(0,6) == 'C-API:')
        ttl = ttl.substr(6);
    puts('  *  <a href='+clst[i]+'>'+ttl+'</a>');
}
puts("</td></tr></table>");
