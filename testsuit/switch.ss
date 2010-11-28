/*
=!EXPECTSTART!=
default
1
=!EXPECTEND!=
*/

switch(3) {
    default:   console.log("default");
    case 1:
        console.log("1");
        break;
    case 2:
        console.log("2");
	continue;
}
