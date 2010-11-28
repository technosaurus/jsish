ACFILES	= parser.c
CFILES = $(ACFILES) lexer.c code.c eval.c func.c value.c regexp.c main.c pstate.c \
		rbtree.c scope.c utils.c proto.c filesys.ex.c unichar.c proto.string.c \
		number.c proto.number.c proto.array.c mempool.c
OBJS    = $(CFILES:.c=.o)
DEFIN	= -DUSE_FILESYS_EX
CFLAGS	= -O2 -Wall -Werror -ansi $(DEFIN)
YACC	= bison -v
TARGET	= quadwheel

.PHONY: all clean cleanall

all: $(OBJS)
	gcc $(CFLAGS) $(OBJS) -o $(TARGET) -lm

debug: $(OBJS)
	gcc -g -Wall -DDONT_USE_POOL $(DEFIN) $(CFILES) -o $(TARGET) -lm

stepdebug: $(OBJS)
	gcc -g -Wall -DDEBUG $(DEFIN) $(CFILES) -o $(TARGET) -lm

parser.c: parser.y
	$(YACC) -oparser.c -d parser.y
	

clean:
	rm -f *.o *.output *.stackdump $(TARGET)

cleanall:
	rm -f $(ACFILES) *.o parser.h *.output *.stackdump $(TARGET)
