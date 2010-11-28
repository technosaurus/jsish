#ifndef __LEXER_H__
#define __LEXER_H__

#define YYSTYPE void *

#include "parser.h"
#include "pstate.h"

/* Lexer, where input seq provided */
typedef struct Lexer {
	enum {
		LT_FILE,			/* read from file */
		LT_STRING			/* read from a string */
	} ltype;
	union {
		FILE *fp;			/* LT_FILE, where to read */
		const char *str;	/* LT_STRING */
	} d;
	int last_token;			/* last token returned */
	int cur;				/* LT_STRING, current char */
	int cur_line;			/* current line no. */
	int cur_char;			/* current column no. */
} Lexer;

int yylex (YYSTYPE *yylvalp, YYLTYPE *yyllocp, PSTATE *pstate);
void yyerror(YYLTYPE *yylloc, PSTATE *ps, char *msg);

#endif

