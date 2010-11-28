function song() {
        bottlesOfBeer = function(i) { return i+' bottles of beer'; };
        bottlesOfBeerOnTheWall = function(i) { return this.bottlesOfBeer(i)+' on the wall'; };
        takeOneDown = function() { return 'Take one down and pass it around, '; };

        createVerse= function(first,second) {
				console.log(first);
				console.log(second);
        };
        getNormalVerseFunction = function(i) {
                return function() {
                        createVerse(
                                bottlesOfBeerOnTheWall(i)+', '+bottlesOfBeer(i),
                                takeOneDown()+bottlesOfBeerOnTheWall(i-1)+'.'
                        );
                };
        };

        verse = new Array();

        for( var i = 3; i < 100; i++ )
                verse[i] = getNormalVerseFunction(i);
        verse[2] = function() {
                createVerse(
                        bottlesOfBeerOnTheWall(2)+', '+bottlesOfBeer(2),
                        takeOneDown()+'1 bottle of beer.'
                );
        };
        verse[1] = function() {
                createVerse(
                        '1 bottle of beer on the wall, 1 bottle of beer.',
                        takeOneDown()+bottlesOfBeerOnTheWall('no more')+'.'
                );
        };
        verse[0] = function() {
                createVerse(
                        bottlesOfBeerOnTheWall('No more')+', '+bottlesOfBeer('no more'),
                        'Go to the store and buy some more, '+bottlesOfBeerOnTheWall(99)+'.'
                );
        };

        this.getDom = function() {
                for( var i = 99; i >= 0 ; i-- ) {
					verse[i]();
				}
        };
};

new song().getDom();