if (console.args.length < 2) {
	console.log("Usage: smallscript contchkcmp.ss <contchk file 1> <contchk file 2>");
	exit(-1);
}

function parsefile(filename)
{
	var file = new File(filename, "r");
	var obj = {};
	
	if (!file) {
		console.log("Can not open file: " + filename);
		exit(-1);
	}
	
	var curname = "";
	var cursetion = {};
	while ((line = file.gets()) != undefined) {
		var arr;
		if (line.match(/\[.*\]/)) {
			if (curname) {
				obj[curname] = cursetion;
				curname = "";
				cursetion = {};
			}
		} else if ((arr = line.match(/([a-zA-Z0-9]+)\ *=\ *(.*)/))) {
			cursetion[arr[1]] = arr[2];
			if (arr[1] == 'Name') {
				curname = arr[2];
			}
		}
	}
	if (curname) obj[curname] = cursetion;
	file.close();
	return obj;
};

var file1 = parsefile(console.args[0]);
var file2 = parsefile(console.args[1]);
var outfile = new File("report.txt", "w");

for (var sess1 in file1) {
	var se = file1[sess1];
	if (!file2[sess1]) {
		outfile.puts("Rule missing: " + sess1 + " Not found at: " + console.args[1]);
		continue;
	}
	var te = file2[sess1];
	
	for (var li in se) {
		if (se[li] != te[li]) {
			outfile.puts("Key diff: " + li + " at Rule: " + sess1);
			outfile.puts("       " + console.args[0] + ": " + se[li]);
			outfile.puts("       " + console.args[1] + ": " + te[li]);
		}
		delete te[li];
	}
	for (var li in te) {
		if (se[li] != te[li]) {
			outfile.puts("Key diff: " + li + " at Rule: " + sess1);
			outfile.puts("       " + console.args[0] + ": " + se[li]);
			outfile.puts("       " + console.args[1] + ": " + te[li]);
		}
	}
	delete file2[sess1];
}

for (var sess1 in file2) {
	if (!file1[sess1]) {
		outfile.puts("Rule missing: " + sess1 + " Not found at: " + console.args[0]);
	}
}
