/*
=!EXPECTSTART!=
4950
=!EXPECTEND!=
*/

function a(n) {
	sum = 0;
	for ( i = 0; i < n; i = i + 1) sum = sum + i;
	console.log(sum);
};

a(100);

