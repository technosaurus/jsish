#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "pstate.h"

PSTATE *pstate_new_from_file(FILE *fp)
{
	PSTATE *ps = malloc(sizeof(PSTATE));
	memset(ps, 0, sizeof(PSTATE));
	Lexer *l = malloc(sizeof(Lexer));
	memset(l, 0, sizeof(Lexer));
	
	ps->lexer = l;
	l->ltype = LT_FILE;
	l->d.fp = fp;
	rewind(fp);
	l->cur_line = 1;
	return ps;
}

PSTATE *pstate_new_from_string(const char *str)
{
	PSTATE *ps = malloc(sizeof(PSTATE));
	memset(ps, 0, sizeof(PSTATE));
	Lexer *l = malloc(sizeof(Lexer));
	memset(l, 0, sizeof(Lexer));
	
	ps->lexer = l;
	l->ltype = LT_STRING;
	l->d.str = str;
	l->cur_line = 1;
	return ps;
}

void pstate_free(PSTATE *ps)
{
	/* todo: free opcodes */
	free(ps->lexer);
	free(ps);
}

