%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "pstate.h"
#include "func.h"
#include "scope.h"
#include "lexer.h"
#include "parser.h"
#include "unichar.h"

typedef struct ArgList {
	unichar *argname;
	struct ArgList *tail;
	struct ArgList *next;
} ArgList;

static ArgList *arglist_new(const unichar *name)
{
	ArgList *a = malloc(sizeof(ArgList));
	memset(a, 0, sizeof(ArgList));
	a->argname = unistrdup(name);
	a->tail = a;
	return a;
}

static ArgList *arglist_insert(ArgList *a, const unichar *name)
{
	ArgList *b = malloc(sizeof(ArgList));
	memset(b, 0, sizeof(ArgList));
	b->argname = unistrdup(name);
	a->tail->next = b;
	a->tail = b;
	return a;
}

typedef struct ForinVar {
	unichar *varname;
	OpCodes *local;
	OpCodes *lval;
} ForinVar;

ForinVar *forinvar_new(unichar *varname, OpCodes *local, OpCodes *lval)
{
	ForinVar *r = malloc(sizeof(ForinVar));
	r->varname = varname;
	r->local = local;
	r->lval = lval;
	return r;
}

static OpCodes *make_forin(OpCodes *lval, OpCodes *expr, OpCodes *stat, const unichar *label)
{
	OpCodes *init = codes_join(expr, code_key());
	OpCodes *cond = codes_join3(lval, code_next(),
								   code_jfalse(stat->code_len + 2));
	OpCodes *stat_jmp = code_jmp(-(cond->code_len + stat->code_len));
	code_reserved_replace(stat, 1, 0, label, 2);
	return codes_join3(codes_join(init, cond), 
						  codes_join(stat, stat_jmp), code_pop(2));
}

typedef struct CaseExprStat {
	OpCodes *expr;
	OpCodes *stat;
	int isdefault;
} CaseExprStat;

CaseExprStat *exprstat_new(OpCodes *expr, OpCodes *stat, int isdef)
{
	CaseExprStat *r = malloc(sizeof(CaseExprStat));
	r->expr = expr;
	r->stat = stat;
	r->isdefault = isdef;
	return r;
}

typedef struct CaseList {
	CaseExprStat *es;
	int off;
	struct CaseList *tail;
	struct CaseList *next;
} CaseList;

static CaseList *caselist_new(CaseExprStat *es)
{
	CaseList *a = malloc(sizeof(CaseList));
	memset(a, 0, sizeof(CaseList));
	a->es = es;
	a->tail = a;
	return a;
}

static CaseList *caselist_insert(CaseList *a, CaseExprStat *es)
{
	CaseList *b = malloc(sizeof(CaseList));
	memset(b, 0, sizeof(CaseList));
	b->es = es;
	a->tail->next = b;
	a->tail = b;
	return a;
}

static OpCodes *opassign(OpCodes *lval, OpCodes *oprand, OpCodes *op)
{
	OpCodes *ret;
	if (((OpCodes *)lval)->lvalue_flag == 1) {
		ret = codes_join3(lval, 
							 codes_join3(code_push_top(), oprand, op),
							 code_assign(1));
	} else {
		ret = codes_join3(lval,
							 codes_join4(code_push_top2(), code_subscript(1), oprand, op),
							 code_assign(2));
	}
	return ret;
}

%}

%locations			/* location proccess */
%pure-parser		/* re-entence */
%parse-param	{PSTATE *pstate}
%lex-param		{PSTATE *pstate}
%error-verbose
%expect 6			/* if-else shift/reduce
					   lvalue shift/reduce 
					   ',' shift/reduce
					   empty statement '{''}' empty object shift/reduct */

%token STRING
%token IDENTIFIER
%token IF
%token ELSE
%token FOR
%token IN
%token WHILE
%token DO
%token CONTINUE
%token SWITCH
%token CASE
%token DEFAULT
%token BREAK
%token FUNC
%token RETURN
%token LOCAL
%token NEW
%token DELETE
%token TRY
%token CATCH
%token FINALLY
%token THROW
%token WITH
%token UNDEF
%token _TRUE
%token _FALSE
%token _THIS
%token ARGUMENTS
%token FNUMBER
%token REGEXP
%token __DEBUG

%left MIN_PRI
%left ','
%left ARGCOMMA						/* comma in argument list */
%right '=' ADDAS MNSAS MULAS MODAS LSHFAS RSHFAS URSHFAS BANDAS BORAS BXORAS DIVAS
/*           +=    -=    *=    %=   <<=     >>=   >>>=     &=     |=    ^=    /= */
%left '?' ':'
%left OR							/* || */
%left AND							/* && */
%left '|'							/* | */
%left '^'							/* ^ */
%left '&'							/* & */
%left EQU NEQ EEQU NNEQ				/* == != === !== */
%left '>' '<' LEQ GEQ INSTANCEOF	/* <= >= instanceof */
%left LSHF RSHF URSHF				/* << >> >>> */
%left '+' '-'
%left '*' '/' '%'
%left NEG '!' INC DEC '~' TYPEOF VOID	/* - ++ -- typeof */
%left NEW								/* new */
%left '.' '[' '('
%left MAX_PRI

%%

file:	{ pstate->opcodes = code_nop(); }
	| statements {
		pstate->opcodes = $1;
	}
	| statements expr {
		pstate->opcodes = codes_join3($1, $2, code_ret(1));
	}
	| expr {	/* for json */
		pstate->opcodes = codes_join($1, code_ret(1));
	}
;

statements:	statement		{ $$ = $1; }
	| statements statement	{ $$ = codes_join($1, $2); }
;

/* todo, ';' auto gen */
statement: 
	iterstatement		{ $$ = $1; }
	| comonstatement	{ $$ = $1; }
	| IDENTIFIER ':' comonstatement	{ $$ = $3; }
;

comonstatement:
	expr ';' { $$ = codes_join($1, code_pop(1)); }
	| if_statement 	{ $$ = $1; }
	| delete_statement 	{ $$ = $1; }
	| BREAK identifier_opt ';'		{ $$ = code_reserved(RES_BREAK, $2); }
	| CONTINUE identifier_opt ';'	{ $$ = code_reserved(RES_CONTINUE, $2); }
	| RETURN expr ';'   { $$ = codes_join($2, code_ret(1)); }
	| RETURN ';'		{ $$ = code_ret(0); }
	| LOCAL vardecs ';' { $$ = $2; }
	| THROW expr ';'	{ $$ = codes_join($2, code_throw()); }
	| try_statement		{ $$ = $1; }
	| with_statement	{ $$ = $1; }
	| ';'					{ $$ = code_nop(); }
	| '{' statements '}'	{ $$ = $2; }
	| func_statement 		{ $$ = $1; }
;
func_statement:
	func_prefix '(' args_opt ')' func_statement_block {
		OpCodes *ret = codes_join4(code_push_index($1),
									  code_push_func(func_make_static($3, scope_get_varlist(), $5)),
									  code_assign(1), code_pop(1));
		if (pstate->eval_flag) ret = codes_join(code_local($1), ret);
		scope_pop();
		$$ = ret;
	}
;

func_prefix:
	FUNC IDENTIFIER %prec MAX_PRI {
		if (!pstate->eval_flag) {
			scope_add_var($2);
		}
		$$ = $2;
	}
;

iterstatement:
	for_statement	{ $$ = $1; }
	| while_statement	{ $$ = $1; }
	| do_statement		{ $$ = $1; }
	| switch_statement	{ $$ = $1; }
;

identifier_opt:	{ $$ = NULL; }
	| IDENTIFIER { $$ = $1; }
;

label_opt: { $$ = NULL; }
	| IDENTIFIER ':' {
		$$ = $1;
	}
;

statement_or_empty:
	statement	{ $$ = $1; }
	| '{' '}'	{ $$ = code_nop(); }
;

with_statement:
	WITH '(' expr ')' statement_or_empty { 
		$$ = codes_join4($3, code_with(((OpCodes *)$5)->code_len + 1), $5, code_ewith());
	}
;

switch_statement: 
	label_opt SWITCH '(' expr ')' '{' '}' { $$ = codes_join($4, code_pop(1)); }
	| label_opt SWITCH '(' expr ')' '{' cases '}'	{
		CaseList *cl = $7;
		OpCodes *allstats = codes_new(3);
		CaseList *cldefault = NULL;
		CaseList *head = NULL;
		
		while (cl) {
			cl->off = allstats->code_len;
			allstats = codes_join(allstats, cl->es->stat);

			CaseList *t = cl;
			cl = cl->next;
			
			if (t->es->isdefault) {
				if (cldefault) yyerror(&@8, pstate, "More then one switch default\n");
				cldefault = t;
			} else {
				t->next = head;
				head = t;
			}
		}
		code_reserved_replace(allstats, 0, 1, $1, 1);
		
		OpCodes *ophead = code_jmp(allstats->code_len + 1);
		if (cldefault) {
			ophead = codes_join(code_jmp(ophead->code_len + cldefault->off + 1), ophead);
			free(cldefault);
		}
		while (head) {
			OpCodes *e = codes_join4(code_push_top(), head->es->expr, 
										code_equal(), code_jtrue(ophead->code_len + head->off + 1));
			ophead = codes_join(e, ophead);
			CaseList *t = head;
			head = head->next;
			free(t);
		}
		$$ = codes_join4(codes_join($4, code_unref()), ophead, allstats, code_pop(1));
	}
;

cases:
	case			{ $$ = caselist_new($1); }
	| cases case	{ $$ = caselist_insert($1, $2); }
;

case:
	CASE expr ':' statements	{ $$ = exprstat_new($2, $4, 0); }
	| DEFAULT ':' statements	{ $$ = exprstat_new(NULL, $3, 1); }
;

try_statement:
	TRY func_statement_block CATCH '(' IDENTIFIER ')' func_statement_block {
		OpCodes *catchblock = codes_join3(code_scatch($5), $7, code_ecatch());
		OpCodes *finallyblock = codes_join(code_sfinal(), code_efinal());
		OpCodes *tryblock = codes_join($2, code_etry());
		$$ = codes_join4(code_stry(tryblock->code_len, catchblock->code_len, finallyblock->code_len),
							tryblock, catchblock, finallyblock);
	}
	| TRY func_statement_block FINALLY func_statement_block {
		OpCodes *catchblock = codes_join(code_scatch(NULL), code_ecatch());
		OpCodes *finallyblock = codes_join3(code_sfinal(), $4, code_efinal());
		OpCodes *tryblock = codes_join($2, code_etry());
		$$ = codes_join4(code_stry(tryblock->code_len, catchblock->code_len, finallyblock->code_len),
							tryblock, catchblock, finallyblock);
	}
	| TRY func_statement_block CATCH '(' IDENTIFIER ')' func_statement_block 
		FINALLY func_statement_block {
		OpCodes *catchblock = codes_join3(code_scatch($5), $7, code_ecatch());
		OpCodes *finallyblock = codes_join3(code_sfinal(), $9, code_efinal());
		OpCodes *tryblock = codes_join($2, code_etry());
		$$ = codes_join4(code_stry(tryblock->code_len, catchblock->code_len, finallyblock->code_len),
							tryblock, catchblock, finallyblock);
	}
;
vardecs:
	vardec					{ $$ = $1; }
	| vardecs ',' vardec 	{ $$ = codes_join($1, $3); }
;

vardec:
	IDENTIFIER				{
		OpCodes *ret = codes_join4(code_push_index($1),
							code_push_undef(),
							code_assign(1),
							code_pop(1));
		if (!pstate->eval_flag)	scope_add_var($1);
		else ret = codes_join(code_local($1), ret);
		$$ = ret;
	}
	| IDENTIFIER '=' expr	{
		OpCodes *ret = codes_join4(code_push_index($1),
							$3,
							code_assign(1),
							code_pop(1));
		if (!pstate->eval_flag) scope_add_var($1);
		else ret = codes_join(code_local($1), ret);
		$$ = ret;
	}
;

delete_statement:
	DELETE lvalue ';'			{
		if (((OpCodes *)$2)->lvalue_flag == 2) {
			$$ = codes_join($2, code_delete(2));
		} else $$ = code_delete(1);
	}
;

if_statement:
	IF '(' expr ')' statement_or_empty {
		int offset = ((OpCodes *)$5)->code_len;
		$$ = codes_join3($3, code_jfalse(offset + 1), $5);
	}
	| IF '(' expr ')' statement_or_empty ELSE statement_or_empty {
		int len_block2 = ((OpCodes *)$7)->code_len;
		OpCodes *block1 = codes_join($5, code_jmp(len_block2 + 1));
		OpCodes *condi = codes_join($3, code_jfalse(block1->code_len + 1));
		$$ = codes_join3(condi, block1, $7);
	}
;

for_statement:
	label_opt FOR '(' for_init for_cond ';' expr_opt ')' statement_or_empty {
		OpCodes *init = $4;
		OpCodes *cond = $5;
		OpCodes *step = ($7 ? codes_join($7, code_pop(1)) : code_nop());
		OpCodes *stat = $9;
		OpCodes *cont_jmp = code_jfalse(step->code_len + stat->code_len + 2);
		OpCodes *step_jmp = code_jmp(-(cond->code_len + step->code_len + stat->code_len + 1));
		code_reserved_replace(stat, step->code_len + 1, 0, $1, 0);
		$$ = codes_join(codes_join3(init, cond, cont_jmp),
						   codes_join3(stat, step, step_jmp));
	}
	| label_opt FOR '(' for_var IN expr ')' statement_or_empty {
		ForinVar *fv = $4;
		OpCodes *lval;
		if (fv->varname) lval = code_push_index(fv->varname);
		else lval = fv->lval;
		
		OpCodes *ret = make_forin(lval, $6, $8, $1);
		if (fv->varname && fv->local) {
			if (!pstate->eval_flag) {
				scope_add_var(fv->varname);
				codes_free(fv->local);
			} else ret = codes_join(fv->local, ret);
		}
		$$ = ret;
	}
;

for_var:
	| LOCAL IDENTIFIER {
		$$ = forinvar_new($2, code_local($2), NULL);
	}
	| lvalue {
		if (((OpCodes *)$1)->lvalue_flag == 2) 
			$$ = forinvar_new(NULL, NULL, codes_join($1, code_subscript(0)));
		else $$ = forinvar_new(NULL, NULL, $1);
	}
;

for_init:
	';'					{ $$ = code_nop(); }
	| expr ';'			{ $$ = codes_join($1, code_pop(1)); }
	| LOCAL vardecs ';' { $$ = $2; }
;

for_cond:				{ $$ = code_push_bool(1); }
	| expr				{ $$ = $1; }
;

expr_opt:				{ $$ = NULL; }
	| expr				{ $$ = $1; }
;

while_statement:
	label_opt WHILE '(' expr ')' statement_or_empty {
		OpCodes *cond = $4;
		OpCodes *stat = $6;
		code_reserved_replace(stat, 1, 0, $1, 0);
		$$ = codes_join4(cond, code_jfalse(stat->code_len + 2), stat,
						   code_jmp(-(stat->code_len + cond->code_len + 1)));
	}
;

do_statement:
	label_opt DO statement_or_empty WHILE '(' expr ')' {
		OpCodes *stat = $3;
		OpCodes *cond = $6;
		code_reserved_replace(stat, cond->code_len + 1, 0, $1, 0);
		$$ = codes_join3(stat, cond,
							code_jtrue(-(stat->code_len + cond->code_len)));
	}
;

func_expr:
	FUNC '(' args_opt ')' func_statement_block {
		$$ = code_push_func(func_make_static($3, scope_get_varlist(), $5));
		scope_pop();
	}
	| FUNC IDENTIFIER '(' args_opt ')' func_statement_block {
		$$ = code_push_func(func_make_static($4, scope_get_varlist(), $6));
		scope_pop();
	}
;

args_opt: { scope_push(); $$ = strs_new(); }
	| args {
		scope_push();
		ArgList *a = $1;
		strs *s = strs_new();
		while (a) {
			strs_push(s, a->argname);
			a = a->next;
		}
		$$ = s;
	}
;

args:
	IDENTIFIER	{ $$ = arglist_new($1); }
	| args ',' IDENTIFIER { $$ = arglist_insert($1, $3); }
;

func_statement_block: '{' statements '}'	{ $$ = $2; }
	| '{' '}'								{ $$ = code_nop(); }
;

expr:
	value					{ $$ = $1; }
	| func_expr				{ $$ = $1; }
	| lvalue				{ 
		if (((OpCodes *)$1)->lvalue_flag == 2) $$ = codes_join($1, code_subscript(1)); 
		else $$ = $1;
	}
	| expr ',' expr			{ $$ = codes_join3($1, code_pop(1), $3); }
	| expr '[' expr ']'		{ $$ = codes_join3($1, $3, code_subscript(1)); }
	| expr '.' IDENTIFIER	{ $$ = codes_join3($1, code_push_string($3), code_subscript(1)); }
	| '-' expr %prec NEG 	{ $$ = codes_join($2, code_neg()); }
	| '+' expr %prec NEG 	{ $$ = codes_join($2, code_pos()); }
	| '~' expr				{ $$ = codes_join($2, code_bnot()); }
	| '!' expr				{ $$ = codes_join($2, code_not()); }
	| VOID expr				{ $$ = codes_join3($2, code_pop(1), code_push_undef()); }
	| expr '*' expr 		{ $$ = codes_join3($1, $3, code_mul()); }
	| expr '/' expr 		{ $$ = codes_join3($1, $3, code_div()); }
	| expr '%' expr 		{ $$ = codes_join3($1, $3, code_mod()); }
	| expr '+' expr 		{ $$ = codes_join3($1, $3, code_add()); }
	| expr '-' expr 		{ $$ = codes_join3($1, $3, code_sub()); }
	| lvalue INC			{
 		if (((OpCodes *)$1)->lvalue_flag == 2) $$ = codes_join3($1, code_subscript(0), code_inc(1));
 		else $$ = codes_join($1, code_inc(1));
 	}
	| lvalue DEC			{ 
		if (((OpCodes *)$1)->lvalue_flag == 2) $$ = codes_join3($1, code_subscript(0), code_dec(1));
		else $$ = codes_join($1, code_dec(1)); 
	}
	| INC lvalue			{
		if (((OpCodes *)$2)->lvalue_flag == 2) $$ = codes_join3($2, code_subscript(0), code_inc(0));
		else $$ = codes_join($2, code_inc(0));
	}
	| DEC lvalue 			{ 
		if (((OpCodes *)$2)->lvalue_flag == 2) $$ = codes_join3($2, code_subscript(0), code_dec(0));
		else $$ = codes_join($2, code_dec(0));
	}
	| '(' expr ')'			{ $$ = $2; }
	| expr AND expr			{
		OpCodes *expr2 = codes_join(code_pop(1), $3);
		$$ = codes_join3($1, code_jfalse_np(expr2->code_len + 1), expr2);
	}
	| expr OR expr			{
		OpCodes *expr2 = codes_join(code_pop(1), $3);
		$$ = codes_join3($1, code_jtrue_np(expr2->code_len + 1), expr2);
	}
	| expr '<' expr			{ $$ = codes_join3($1, $3, code_less()); }
	| expr '>' expr			{ $$ = codes_join3($1, $3, code_greater()); }
	| expr LEQ expr			{ $$ = codes_join3($1, $3, code_lessequ()); }
	| expr GEQ expr			{ $$ = codes_join3($1, $3, code_greaterequ()); }
	| expr EQU expr			{ $$ = codes_join3($1, $3, code_equal()); }
	| expr NEQ expr			{ $$ = codes_join3($1, $3, code_notequal()); }
	| expr EEQU expr		{ $$ = codes_join3($1, $3, code_eequ());	}
	| expr NNEQ expr		{ $$ = codes_join3($1, $3, code_nneq()); }
	| expr '&' expr			{ $$ = codes_join3($1, $3, code_band()); }
	| expr '|' expr			{ $$ = codes_join3($1, $3, code_bor()); }
	| expr '^' expr			{ $$ = codes_join3($1, $3, code_bxor()); }
	| expr LSHF expr		{ $$ = codes_join3($1, $3, code_shf(0)); }
	| expr RSHF expr		{ $$ = codes_join3($1, $3, code_shf(1)); }
	| expr URSHF expr		{ $$ = codes_join3($1, $3, code_shf(2)); }
	| lvalue '=' expr 		{ $$ = codes_join3($1, $3, code_assign(((OpCodes *)$1)->lvalue_flag)); }
	| lvalue ADDAS expr		{ $$ = opassign($1, $3, code_add()); }
	| lvalue MNSAS expr		{ $$ = opassign($1, $3, code_sub()); }
	| lvalue MULAS expr		{ $$ = opassign($1, $3, code_mul()); }
	| lvalue MODAS expr		{ $$ = opassign($1, $3, code_mod()); }
	| lvalue LSHFAS expr	{ $$ = opassign($1, $3, code_shf(0)); }
	| lvalue RSHFAS expr	{ $$ = opassign($1, $3, code_shf(1)); }
	| lvalue URSHFAS expr	{ $$ = opassign($1, $3, code_shf(2)); }
	| lvalue BANDAS expr	{ $$ = opassign($1, $3, code_band()); }
	| lvalue BORAS expr		{ $$ = opassign($1, $3, code_bor()); }
	| lvalue BXORAS expr	{ $$ = opassign($1, $3, code_bxor()); }
	| lvalue DIVAS expr		{ $$ = opassign($1, $3, code_div()); }
	| fcall_exprs			{ $$ = $1; }
	
	| NEW value				{ $$ = codes_join($2, code_newfcall(0)); }
	| NEW lvalue			{ 
		if (((OpCodes *)$2)->lvalue_flag == 2) $$ = codes_join3($2, code_subscript(1), code_newfcall(0));
 		else $$ = codes_join($2, code_newfcall(0));}
	| NEW '(' expr ')'		{ $$ = codes_join($3, code_newfcall(0)); }
	| NEW func_expr			{ $$ = codes_join($2, code_newfcall(0)); }
	| NEW value '(' exprlist_opt ')'		{ 
		OpCodes *opl = $4;
		int expr_cnt = opl ? opl->expr_counter:0;
 		$$ = codes_join3($2, (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	}
	| NEW lvalue '(' exprlist_opt ')'		{
		OpCodes *opl = $4;
		int expr_cnt = opl ? opl->expr_counter:0;
		OpCodes *lv = NULL;
		if (((OpCodes *)$2)->lvalue_flag == 2) lv = codes_join($2, code_subscript(1));
		else lv = $2;
		$$ = codes_join3(lv, (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	}
	| NEW '(' expr ')' '(' exprlist_opt ')'	{ 
		OpCodes *opl = $6;
		int expr_cnt = opl ? opl->expr_counter:0;
		$$ = codes_join3($3, (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	}
	| NEW func_expr '(' exprlist_opt ')'	{
		OpCodes *opl = $4;
		int expr_cnt = opl ? opl->expr_counter:0;
 		$$ = codes_join3($2, (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	}
	| expr '?' expr ':' expr {
		OpCodes *expr2 = codes_join($3, code_jmp(((OpCodes *)$5)->code_len + 1));
		$$ = codes_join4($1, code_jfalse(expr2->code_len + 1), expr2, $5);
	}
	| __DEBUG '(' expr ')' { $$ = codes_join($3, code_debug()); }
;

fcall_exprs:
	expr '.' IDENTIFIER '(' exprlist_opt ')' {
		OpCodes *ff = codes_join4($1, code_push_string($3), code_chthis(1), code_subscript(1));
		OpCodes *opl = $5;
		int expr_cnt = opl ? opl->expr_counter:0;
 		$$ = codes_join3(ff, (opl ? opl : code_nop()), code_fcall(expr_cnt));
	}
	| expr '[' expr ']' '(' exprlist_opt ')' {
		OpCodes *ff = codes_join4($1, $3, code_chthis(1), code_subscript(1));
		OpCodes *opl = $6;
		int expr_cnt = opl ? opl->expr_counter:0;
 		$$ = codes_join3(ff, (opl ? opl : code_nop()), code_fcall(expr_cnt));
	}
	| '(' expr ')' '(' exprlist_opt ')' {
		OpCodes *opl = $5;
		int expr_cnt = opl ? opl->expr_counter:0;
 		$$ = codes_join4($2, code_chthis(0), (opl ? opl : code_nop()), code_fcall(expr_cnt));
	}
	| lvalue '(' exprlist_opt ')' {
		OpCodes *opl = $3;
		int expr_cnt = opl ? opl->expr_counter:0;
		OpCodes *pref;
		OpCodes *lval = $1;
		if (lval->lvalue_flag == 2) {
			pref = codes_join3($1, code_chthis(1), code_subscript(1));
 			$$ = codes_join3(pref, (opl ? opl : code_nop()), code_fcall(expr_cnt));
		} else {
			if (lval->lvalue_name && unistrcmp(lval->lvalue_name, tounichars("eval")) == 0) {
				$$ = codes_join((opl ? opl : code_nop()), code_eval(expr_cnt));
			} else {
				pref = codes_join($1, code_chthis(0));
				$$ = codes_join3(pref, (opl ? opl : code_nop()), code_fcall(expr_cnt));
			}
		}
	}
;

lvalue:
	IDENTIFIER 				{
		$$ = code_push_index($1); 
		((OpCodes *)$$)->lvalue_flag = 1; 
		((OpCodes *)$$)->lvalue_name = $1; 
	}
	| ARGUMENTS 			{ $$ = code_push_args(); ((OpCodes *)$$)->lvalue_flag = 1; }
	| _THIS					{ $$ = code_push_this(); ((OpCodes *)$$)->lvalue_flag = 1; }
	| lvalue '[' expr ']'	{
		if (((OpCodes *)$1)->lvalue_flag == 2) $$ = codes_join3($1, code_subscript(1), $3); 
		else $$ = codes_join($1, $3); 
		((OpCodes *)$$)->lvalue_flag = 2;
	}
	| lvalue '.' IDENTIFIER	{
		if (((OpCodes *)$1)->lvalue_flag == 2) $$ = codes_join3($1, code_subscript(1), code_push_string($3)); 
		else $$ = codes_join($1, code_push_string($3));
		((OpCodes *)$$)->lvalue_flag = 2;
	}
;

exprlist_opt: 	{ $$ = NULL; }
	| exprlist 	{ $$ = $1; }
;

exprlist:
	expr %prec ARGCOMMA { $$ = $1; ((OpCodes *)$$)->expr_counter = 1; }
	| exprlist ',' expr %prec ARGCOMMA { 
		int exprcnt = ((OpCodes *)$1)->expr_counter + 1;
		$$ = codes_join($1, $3);
		((OpCodes *)$$)->expr_counter = exprcnt;
	}
;

value: STRING { $$ = code_push_string($1); }
	| UNDEF { $$ = code_push_undef(); }
	| _TRUE { $$ = code_push_bool(1); }
	| _FALSE { $$ = code_push_bool(0); }
	| FNUMBER { $$ = code_push_num($1); }
	| REGEXP { $$ = code_push_regex($1); }
	| object { $$ = $1; }
	| array { $$ = $1; }
;

object:
	'{' items '}' 	{ $$ = codes_join($2, code_object(((OpCodes *)$2)->expr_counter)); }

;

items:		{ $$ = code_nop(); ((OpCodes *)$$)->expr_counter = 0; }
	| item 	{ $$ = $1; ((OpCodes *)$$)->expr_counter = 1; }
	| items ',' item {
		int cnt = ((OpCodes *)$1)->expr_counter + 1;
		$$ = codes_join($1, $3);
		((OpCodes *)$$)->expr_counter = cnt;
	}
;

item:
	IDENTIFIER ':' expr	{ $$ = codes_join(code_push_string($1), $3); }
	| STRING ':' expr	{ $$ = codes_join(code_push_string($1), $3); }
;

array:
	'[' exprlist ']' { $$ = codes_join($2, code_array(((OpCodes *)$2)->expr_counter)); }
	| '[' ']' { $$ = code_array(0); }
;


%%

