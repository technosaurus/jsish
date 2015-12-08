JSIDIR=..
CFLAGS= -g -Wall -I$(JSIDIR)
LDFLAGS=-shared -fpic 

all: simple dyn.so prime.so dbdemo

simple: simple.c
	make -C$(JSIDIR) jsi.c
	$(CC) $(CFLAGS) -DHAVE_SQLITE=1 simple.c -o simple -lz -lm -ldl -lpthread -lsqlite3

dbdemo: dbdemo.c
	make -C$(JSIDIR) jsi.c
	$(CC) $(CFLAGS) -DHAVE_SQLITE=1 dbdemo.c -o dbdemo -lz -lm -ldl -lpthread -lsqlite3

dyn.so: dyn.c
	$(CC) $(CFLAGS) $(LDFLAGS) dyn.c -o dyn.so

prime.so: prime.c
	$(CC) $(CFLAGS) $(LDFLAGS) prime.c -o prime.so

clean:
	rm -rf *.so *.o simple

