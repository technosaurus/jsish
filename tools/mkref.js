#!/usr/bin/env jsish
// Generate documentation for Jsi builtin cmds in fossil wiki format.

function jsi_mkref() {

    function DumpOpts(opts, nam, isconf) {
        //if (cnam.indexOf('.')<0) cnam=cnam+'.conf';
        var rv = '';//, opts = info.cmds(cnam).options;
        rv += '\n\n<a name="'+nam+'Options"></a>\n';
        rv += '<h3>Options for "'+nam+'"</h3>\n';
        rv += "\nThe following options are available for \""+nam+"\"\n\n";
        rv += "<table border='1' class=optstbl>\n";
        rv += "<tr><th>Option</th> <th>Type</th> <th>Description</th> <th>Default</th></tr>\n";
        //puts(opts);
        for (var o in opts) {
            ci = opts[o];
            if (isconf && ci.initOnly) continue;
            var help = (ci.help?ci.help+'.':'');
            if (ci.readOnly) help += " (readonly)";
            if (ci.customArg && ci.customArg.data)
                help += ' '+ ci.customArg.data;
            rv += "<tr><td>"+ci.name+"</td><td>"+ci.type+"</td><td>"+ help +"</td><td>"+ (ci.init?ci.init:'') +' </td><tr>\n';
        }
        rv += "</table>\n";
        return rv;
    }

    function DumpCmd(cinf) {
        var rv = '';
        var cnam = sprintf("%s(%s)", cinf.name, cinf.argStr);
        index += "<li><a href='#"+cinf.name+"'>"+cnam+"</a></li>\n";
        rv += '<a name="'+cinf.name+'"></a>\n';
        rv += '\n<hr>\n';
        rv += '<a href="#TOC">Return to top</a>\n';
        rv += '\n\n<h2>'+cinf.name+'</h2>\n\n';
        rv += sprintf("<font color=red>Synopsis: %s(%s)</font><p>\n\n", cinf.name, cinf.argStr);
        if (cinf.help)
            rv += cinf.help+".\n\n";
        if (cinf.info)
            rv += cinf.info+'\n\n';
        if (cinf.options != NULL) {
            rv += DumpOpts(cinf.options, cinf.name, false);
        }
        rv += '<a name="'+cinf.name+'end"></a>\n';
        return rv+'\n';
    }
    function LinkOpts(astr,name) {
        var ii;
        if ((ii=astr.indexOf('options'))<0)
            return astr;
        var ss = '';
        if (ii>0)
            ss += astr.slice(0, ii);
        ss += "<a href='#"+name+"Options'>options</a>";
        ss += astr.slice(ii+7);
        return ss;
    }

    function DumpObj(tinf) {
        var hasconf, ro = '', rv = '', ci, cnam = tinf.name, cmds = info.cmds(cnam+'.*');
        index += "<li><a href='#"+tinf.name+"'>"+cnam+"</a></li>\n";
        rv += '<a name="'+tinf.name+'"></a>\n';
        rv += '\n<hr>\n';
        rv += '<a href="#TOC">Return to top</a>\n';
        rv += '\n\n<h2>'+cnam+'</h2>\n\n';
        rv += "<font color=red>Synopsis:";
        if (tinf.constructor) {
            rv += 'new '+cnam+"("+tinf.argStr+")\n\n";
        } else {
            rv += cnam+".method(...)\n\n";
        }
        rv += "</font><p>";
        if (tinf.help)
            rv += tinf.help+".\n\n";
        if (tinf.info)
            rv += tinf.info+'\n\n';
        rv += '\n<h3>Methods</h3>\n';
        rv += "\nThe following methods are available in \""+cnam+"\":\n\n";
        rv += "<table border='1' class=cmdstbl>\n";
        rv += '<tr><th>Method</th> <th>Description</th></tr>\n';
        if (tinf.constructor) {
            ci = info.cmds(cnam+'.'+cnam,true);
            var conhelp = (ci.help?ci.help+'.':''), aastr;
            if (ci.info) conhelp += ci.info;
            aastr = tinf.argStr;
            if (tinf.options) {
                aastr = LinkOpts(aastr, "new "+cnam);
            }
            rv += "<tr><td>new "+cnam+"("+aastr+") </td><td>"+conhelp+'</td></tr>\n';
        }
        if (cmds !== undefined) {
            for (var cmd in cmds) {
                var nam = cmds[cmd].split('.')[1];
                if (nam == cnam)
                    continue;
                ci = info.cmds(cnam+'.'+nam);
                var conhelp = (ci.help?ci.help+'.':'');
                if (ci.info) conhelp += ' '+ci.info;
                if (ci.options) {
                    ro += DumpOpts(ci.options, cnam+'.'+nam, (nam === 'conf'));
                    aastr = LinkOpts(ci.argStr, cnam+'.'+nam);
                } else {
                    aastr = ci.argStr;
                }
                //if (nam == 'conf') hasconf = ci.options;
                rv += "<tr><td>"+nam+"("+aastr+") </td><td>"+conhelp+'</td></tr>\n';
            }
        }
        rv += "</table>\n";
        if (tinf.options)
            rv += DumpOpts(tinf.options, 'new '+cnam, (nam === 'conf'));
        rv += ro;
        rv += '<a name="'+tinf.name+'end"></a>\n';
        return rv;
    }
    
    
    //*************** BEGIN MAIN **************
    var rv = '', tinf, lst = info.cmds();
    var vv = info.version(true);
    var ver = vv.major+'.'+vv.minor+'.'+vv.release;

    puts("<title>Reference</title>");
    var index = "<ul>";

    for (var i in lst) {
        tinf = info.cmds(lst[i]);
        switch (tinf.type) {
        case 'object':
            rv += DumpObj(tinf);
            break;
        case 'command':
            rv += DumpCmd(tinf);
            break;
        default:
            throw("bad id");
            continue;
        }
    }
    rv += "</nowiki>";
    index + "</ul>\n";
    return '<a name="TOC"></a><h2>JSI Command Reference: version '+ver+'</h2>\n<nowiki>\n' + index + rv;
}

if (info.isInvoked()) {
    puts(jsi_mkref());
}
