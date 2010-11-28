#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

#include "lexer.h"
#include "parser.h"
#include "error.h"
#include "regexp.h"

#define COMMENT (-128)

static int lexer_getchar(Lexer *lex)
{
	int c = 0;
	if (!lex) bug("No lexer init");
	if (lex->ltype == LT_FILE) {
		c = fgetc(lex->d.fp);
		if (c == EOF) c = 0;
	} else {
		c = lex->d.str[lex->cur];
		if (c != 0) lex->cur++;
	}
	if (c == '\n') {
		lex->cur_line++;
		lex->cur_char = 0;
	}
	lex->cur_char++;
	return c;
}

static void lexer_ungetc(int c, Lexer *lex)
{
	if (!lex) bug("No lexer init");
	if (!c) return;
	
	if (lex->ltype == LT_FILE) {
		ungetc(c, lex->d.fp);
	} else {
		lex->cur--;
	}
}

static int iskey(const char *word)
{
	static struct st_kw {
		const char *name;
		int value;
	} keywords[] = {
		{ "if", IF },
		{ "else", ELSE },
		{ "for", FOR },
		{ "in", IN },
		{ "while", WHILE },
		{ "do", DO },
		{ "continue", CONTINUE },
		{ "switch", SWITCH },
		{ "case", CASE },
		{ "default", DEFAULT },
		{ "break", BREAK },
		{ "function", FUNC },
		{ "return", RETURN },
		{ "var", LOCAL },
		{ "new", NEW },
		{ "delete", DELETE },
		{ "try", TRY },
		{ "catch", CATCH },
		{ "throw", THROW },
		{ "finally", FINALLY },
		{ "with", WITH },
		{ "undefined", UNDEF },
		{ "true", _TRUE },
		{ "false", _FALSE },
		{ "this", _THIS },
		{ "arguments", ARGUMENTS },
		{ "void", VOID },
		{ "__debug", __DEBUG }
	};
	int i;
	for (i = 0; i < sizeof(keywords) / sizeof(struct st_kw); ++i) {
		if (strcmp(word, keywords[i].name) == 0)
			return keywords[i].value;
	}
	return 0;
}

unichar *do_string(Lexer *lex)
{
	int c = lexer_getchar(lex);
	int endchar = c;
	
	UNISTR(65536) unibuf;

	unichar *buf = unibuf.unistr;
	int bufi = 0;
	
	while (bufi < 65530) {
		c = lexer_getchar(lex);
		if (c == EOF || c == 0) {
			die("Unexpected EOF parsing string.\n");
		}
		if (c == '\\') {
			int n = lexer_getchar(lex);
			switch(n) {
				case 'b': buf[bufi++] = '\b'; break;
				case 'f': buf[bufi++] = '\f'; break;
				case 'n': buf[bufi++] = '\n'; break;
				case 'r': buf[bufi++] = '\r'; break;
				case 't': buf[bufi++] = '\t'; break;
				case EOF: 
				case 0:
					die("Unexpected EOF parsing string.\n");
				default: buf[bufi++] = n;
			}
		} else {
			buf[bufi++] = c;
		}
		if (c == endchar) {
			bufi --;
			break;
		}
	}
	buf[bufi] = 0;
	unibuf.len = bufi;
	return unistrdup(buf);
}

char *do_regex(Lexer *lex, int *flag)
{
	char buf[65536];
	int bufi = 0;
	char *ret;
	
	lexer_getchar(lex);		/* first '/'*/
	while (bufi < 65530) {
		int c = lexer_getchar(lex);
		if (c == EOF || c == 0) {
			die("Unexpected EOF parsing regular expression.\n");
		}
		if (c == '\\') {
			int n = lexer_getchar(lex);
			if (n == EOF || c == 0) die("Unexpected EOF parsing regular expression.\n");
			
			buf[bufi++] = c;
			buf[bufi++] = n;
		} else if (c == '/') {
			buf[bufi] = 0;
			while (1) {
				c = lexer_getchar(lex);
				if (!isalnum(c)) break;
				if (c == 'i') *flag |= REG_ICASE;
			}
			lexer_ungetc(c, lex);
			break;
		} else {
			buf[bufi++] = c;
		}
	}
	ret = c_strdup(buf);
	return ret;
}

int do_sign(Lexer *lex)
{
	static struct st_sn {
		const char *name;
		int len;
		int value;
	} signs[] = {
		{ ">>>=", 4, URSHFAS },
		{ "<<=", 3, LSHFAS },
		{ ">>=", 3, RSHFAS },
		{ "===", 3, EEQU },
		{ "!==", 3, NNEQ },
		{ ">>>", 3, URSHF },
		{ "==", 2, EQU },
		{ "!=", 2, NEQ },
		{ "<=", 2, LEQ },
		{ ">=", 2, GEQ },
		{ "++", 2, INC },
		{ "--", 2, DEC },
		{ "&&", 2, AND },
		{ "||", 2, OR },
		{ "+=", 2, ADDAS },
		{ "-=", 2, MNSAS },
		{ "*=", 2, MULAS },
		{ "/=", 2, DIVAS },
		{ "%=", 2, MODAS },
		{ "&=", 2, BANDAS },
		{ "|=", 2, BORAS },
		{ "^=", 2, BXORAS },
		{ "<<", 2, LSHF },
		{ ">>", 2, RSHF }
	};

	int bufi;
	char buf[4];
	int i;
	for (bufi = 0; bufi < 4; ++bufi) {
		int c = lexer_getchar(lex);
		if (c == 0 || c == '\n') break;
		buf[bufi] = c;
	}
	if (!bufi) return 0;
	
	for (i = 0; i < sizeof(signs)/sizeof(struct st_sn); ++i) {
		if (bufi < signs[i].len) continue;
		if (strncmp(buf, signs[i].name, signs[i].len) == 0) {
			int j;
			for (j = bufi - 1; j >= signs[i].len; --j)
				lexer_ungetc(buf[j], lex);

			return signs[i].value;
		}
	}
	
	for (i = bufi - 1; i >= 1; --i)
		lexer_ungetc(buf[i], lex);
	
	return buf[0];
}

#define LOCATION_START(loc, lex) do { 		\
	(loc)->first_line = (lex)->cur_line;	\
	(loc)->first_column = (lex)->cur_char;	\
	} while(0)
#define LOCATION_END(loc, lex) do {			\
	(loc)->last_line = (lex)->cur_line;		\
	(loc)->last_column = (lex)->cur_char;	\
	} while(0)

static void eat_comment(Lexer *lex)
{
	int c;
	while((c = lexer_getchar(lex))) {
		if (c == '*') {
			c = lexer_getchar(lex);
			if (c == '/') return;
			lexer_ungetc(c, lex);
		}
	}
	die("Comment reach end of file\n");
}

static int _yylex (YYSTYPE *yylvalp, YYLTYPE *yyllocp, Lexer *lex)
{
	int c;
	
	UNISTR(1024) unibuf;

	unichar *word = unibuf.unistr;
	int wi = 0;
		
	LOCATION_START(yyllocp, lex);
	while ((c = lexer_getchar(lex)) == ' ' || c == '\t' || c == '\n' || c == '\r');
	
	if (isdigit(c)) {
		int fnum = 0;
		word[wi++] = c;
		while (wi < 1020) {
			c = lexer_getchar(lex);
			if (isdigit(c)) word[wi++] = c;
			else if (c == '.') {
				if (fnum) die("Number format error");
				fnum = 1;
				word[wi++] = c;
			} else {
				lexer_ungetc(c, lex);
				break;
			}
		}
		LOCATION_END(yyllocp, lex);
		word[wi] = 0;
		unibuf.len = wi;
		double *db = malloc(sizeof(double));
		sscanf(tochars(word), "%lf", db);
		*yylvalp = db;
		return FNUMBER;
	} else if (c == '"' || c == '\'') {
		lexer_ungetc(c, lex);
		*yylvalp = do_string(lex);
		LOCATION_END(yyllocp, lex);
		return STRING;
	} else if (isalpha(c) || c == '_' || c == '$') {
		lexer_ungetc(c, lex);
		while (wi < 1020) {
			c = lexer_getchar(lex);
			if (!isalnum(c) && c != '_' && c != '$') break;
			word[wi++] = c;
		}
		lexer_ungetc(c, lex);
		
		word[wi] = 0;
		unibuf.len = wi;
		int r = iskey(tochars(word));
		if (r) return r;
		*yylvalp = unistrdup(word);
		LOCATION_END(yyllocp, lex);
		return IDENTIFIER;
	} else if (c == '/') {
		int d = lexer_getchar(lex);
		if (d == '/') {
			while ((d = lexer_getchar(lex)) != '\r' && d != '\n' && d != 0);
			return COMMENT;
		} else if (d == '*') {
			eat_comment(lex);
			return COMMENT;
		} else lexer_ungetc(d, lex);
		
		if (lex->last_token != FNUMBER && lex->last_token != STRING &&
			lex->last_token != REGEXP && lex->last_token != UNDEF &&
			lex->last_token != _TRUE && lex->last_token != _FALSE &&
			lex->last_token != ARGUMENTS && lex->last_token != _THIS &&
			lex->last_token != IDENTIFIER) {
			lexer_ungetc(c, lex);
			int flag = REG_EXTENDED;
			char *regtxt = do_regex(lex, &flag);
			*yylvalp = regex_new(regtxt, flag);
			c_strfree(regtxt);
			return REGEXP;
		}
	}
	
	lexer_ungetc(c, lex);
	
	int r = do_sign(lex);
	LOCATION_END(yyllocp, lex);
	return r;
}

int yylex (YYSTYPE *yylvalp, YYLTYPE *yyllocp, PSTATE *pstate)
{
	int ret;
	do {
		ret = _yylex(yylvalp, yyllocp, pstate->lexer);
	} while (ret == COMMENT);
/*
	if (ret < 128 && ret > 0) printf("%c\n", ret);
	else printf("%d\n", ret);
*/
	pstate->lexer->last_token = ret;
	return ret;
}

void yyerror(YYLTYPE *yylloc, PSTATE *ps, char *msg)
{
	fprintf(stderr, "%d[%d-%d]:%s\n", yylloc->first_line, 
			yylloc->first_column, yylloc->last_column, msg);
	ps->err_count++;
}
