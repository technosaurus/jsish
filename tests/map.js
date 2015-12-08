/*
=!EXPECTSTART!=
EE: foot
EE: goose
EE: moose
EE: kangaroo
[ "<foot>", "<goose>", "<moose>", "<kangaroose>" ]
=!EXPECTEND!=
*/

function fuzzyPlural(single) {
puts("EE: "+single);
  //var result = single.replace(/o/g, 'e');  
  var result = single;
  if( single === 'kangaroo'){
    result += 'se';
  }
  return '<'+result+'>'; 
}

var words = ["foot", "goose", "moose", "kangaroo"];
puts(words.map(fuzzyPlural));

// ["feet", "geese", "meese", "kangareese"]

