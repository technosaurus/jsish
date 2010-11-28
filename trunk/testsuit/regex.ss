/*
=!EXPECTSTART!=
input the email
invalid email format
input the email
invalid email format
input the email
match at: wenxichang@163.com
name: wenxichang
domain: 163
input the email
invalid email format
input the email
=!EXPECTEND!=
*/


while (1) {
	console.log("input the email");
	email = console.input();
	if (email == undefined) break;
	if ((res = email.match(/([a-zA-Z0-9]+)@([a-zA-Z0-9]+)\.com/))) {
		console.log("match at: " + res[0]);
		console.log("name: " + res[1]);
		console.log("domain: " + res[2]);
	} else {
		console.log("invalid email format");
	}
}

