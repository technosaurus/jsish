#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"

const char *op_names[OP_LASTOP] = {
	"NOP",
	"PUSHNUM",
	"PUSHSTR",
	"PUSHVAR",
	"PUSHUND",
	"PUSHBOO",
	"PUSHFUN",
	"PUSHREG",
	"PUSHARG",
	"PUSHTHS",
	"PUSHTOP",
	"PUSHTOP2",
	"UNREF",
	"POP",
	"LOCAL",
	"NEG",
	"POS",
	"NOT",
	"BNOT",
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"MOD",
	"LESS",
	"GREATER",
	"LESSEQU",
	"GREATEREQU",
	"EQUAL",
	"NOTEQUAL",
	"STRICTEQU",
	"STRICTNEQ",
	"BAND",
	"BOR",
	"BXOR",
	"SHF",
	"ASSIGN",
	"SUBSCRIPT",
	"INC",
	"DEC",
	"KEY",
	"NEXT",
	"JTRUE",
	"JFALSE",
	"JTRUE_NP",
	"JFALSE_NP",
	"JMP",
	"JMPPOP",
	"FCALL",
	"NEWFCALL",
	"RET",
	"DELETE",
	"CHTHIS",
	"OBJECT",
	"ARRAY",
	"EVAL",
	"STRY",
	"ETRY",
	"SCATCH",
	"ECATCH",
	"SFINAL",
	"EFINAL",
	"THROW",
	"WITH",
	"EWITH",
	"RESERVED",
	"DEBUG",
};

OpCodes *codes_new(int size)
{
	OpCodes *ret = malloc(sizeof(OpCodes));
	memset(ret, 0, sizeof(OpCodes));
	ret->codes = malloc(sizeof(OpCode) * size);
	ret->code_size = size;
	return ret;
}

int codes_insert(OpCodes *c, Eopcode code, void *extra)
{
	if (c->code_size - c->code_len <= 0) {
		c->code_size += 100;
		c->codes = realloc(c->codes, c->code_size * sizeof(OpCode));
	}
	c->codes[c->code_len].op = code;
	c->codes[c->code_len].data = extra;
	c->code_len ++;
	return 0;
}

OpCodes *codes_join(OpCodes *a, OpCodes *b)
{
	OpCodes *ret = codes_new(a->code_len + b->code_len);
	memcpy(ret->codes, a->codes, a->code_len * sizeof(OpCode));
	memcpy(&ret->codes[a->code_len], b->codes, b->code_len * sizeof(OpCode));
	ret->code_size = a->code_len + b->code_len;
	ret->code_len = ret->code_size;
	ret->expr_counter = a->expr_counter + b->expr_counter;
	free(a->codes);
	free(b->codes);
	free(a);
	free(b);
	return ret;
}

OpCodes *codes_join3(OpCodes *a, OpCodes *b, OpCodes *c)
{
	return codes_join(codes_join(a, b), c);
}

OpCodes *codes_join4(OpCodes *a, OpCodes *b, OpCodes *c, OpCodes *d)
{
	return codes_join(codes_join(a, b), codes_join(c, d));
}

#define NEW_CODES(code, extra) do {					\
		OpCodes *r = codes_new(3);					\
		codes_insert(r, (code), (void *)(extra));	\
		return r;									\
	} while(0)

OpCodes *code_push_undef() { NEW_CODES(OP_PUSHUND, 0); }
OpCodes *code_push_bool(int v) { NEW_CODES(OP_PUSHBOO, v); }
OpCodes *code_push_num(double *v) { NEW_CODES(OP_PUSHNUM, v); }
OpCodes *code_push_string(const unichar *str) { NEW_CODES(OP_PUSHSTR, str); }
OpCodes *code_push_index(unichar *varname)
{
	FastVar *n = malloc(sizeof(FastVar));
	memset(n, 0, sizeof(FastVar));
	n->context_id = -1;
	n->var.varname = varname;
	NEW_CODES(OP_PUSHVAR, n);
}
OpCodes *code_push_this() { NEW_CODES(OP_PUSHTHS, 0); }
OpCodes *code_push_top() { NEW_CODES(OP_PUSHTOP, 0); }
OpCodes *code_push_top2() { NEW_CODES(OP_PUSHTOP2, 0); }
OpCodes *code_unref() { NEW_CODES(OP_UNREF, 0); }
OpCodes *code_push_args() { NEW_CODES(OP_PUSHARG, 0); }
OpCodes *code_push_func(struct Func *fun) { NEW_CODES(OP_PUSHFUN, fun); }
OpCodes *code_push_regex(regex_t *reg) { NEW_CODES(OP_PUSHREG, reg); }

OpCodes *code_local(const unichar *varname) { NEW_CODES(OP_LOCAL, varname); }

OpCodes *code_nop() { NEW_CODES(OP_NOP, 0); }
OpCodes *code_neg() { NEW_CODES(OP_NEG, 0); }
OpCodes *code_pos() { NEW_CODES(OP_POS, 0); }
OpCodes *code_bnot() { NEW_CODES(OP_BNOT, 0); }
OpCodes *code_not() { NEW_CODES(OP_NOT, 0); }
OpCodes *code_mul() { NEW_CODES(OP_MUL, 0); }
OpCodes *code_div() { NEW_CODES(OP_DIV, 0); }
OpCodes *code_mod() { NEW_CODES(OP_MOD, 0); }
OpCodes *code_add() { NEW_CODES(OP_ADD, 0); }
OpCodes *code_sub() { NEW_CODES(OP_SUB, 0); }
OpCodes *code_less() { NEW_CODES(OP_LESS, 0); }
OpCodes *code_greater() { NEW_CODES(OP_GREATER, 0); }
OpCodes *code_lessequ() { NEW_CODES(OP_LESSEQU, 0); }
OpCodes *code_greaterequ() { NEW_CODES(OP_GREATEREQU, 0); }
OpCodes *code_equal() { NEW_CODES(OP_EQUAL, 0); } 
OpCodes *code_notequal() { NEW_CODES(OP_NOTEQUAL, 0); }
OpCodes *code_eequ() { NEW_CODES(OP_STRICTEQU, 0); }
OpCodes *code_nneq() { NEW_CODES(OP_STRICTNEQ, 0); }
OpCodes *code_band() { NEW_CODES(OP_BAND, 0); }
OpCodes *code_bor() { NEW_CODES(OP_BOR, 0); }
OpCodes *code_bxor() { NEW_CODES(OP_BXOR, 0); }
OpCodes *code_shf(int right) { NEW_CODES(OP_SHF, right); }
OpCodes *code_assign(int h) { NEW_CODES(OP_ASSIGN, h); }
OpCodes *code_subscript(int right_val) { NEW_CODES(OP_SUBSCRIPT, right_val); }
OpCodes *code_inc(int e) { NEW_CODES(OP_INC, e); }
OpCodes *code_dec(int e) { NEW_CODES(OP_DEC, e); }

OpCodes *code_fcall(int argc) { NEW_CODES(OP_FCALL, argc); }
OpCodes *code_newfcall(int argc) { NEW_CODES(OP_NEWFCALL, argc); }
OpCodes *code_ret(int n) { NEW_CODES(OP_RET, n); }
OpCodes *code_delete(int n) { NEW_CODES(OP_DELETE, n); }
OpCodes *code_chthis(int n) { NEW_CODES(OP_CHTHIS, n); }
OpCodes *code_pop(int n) { NEW_CODES(OP_POP, n); }
OpCodes *code_jfalse(int off) { NEW_CODES(OP_JFALSE, off); }
OpCodes *code_jtrue(int off) { NEW_CODES(OP_JTRUE, off); }
OpCodes *code_jfalse_np(int off) { NEW_CODES(OP_JFALSE_NP, off); }
OpCodes *code_jtrue_np(int off) { NEW_CODES(OP_JTRUE_NP, off); }
OpCodes *code_jmp(int off) { NEW_CODES(OP_JMP, off); }
OpCodes *code_object(int c) { NEW_CODES(OP_OBJECT, c); }
OpCodes *code_array(int c) { NEW_CODES(OP_ARRAY, c); }
OpCodes *code_key() { NEW_CODES(OP_KEY, 0); }
OpCodes *code_next() { NEW_CODES(OP_NEXT, 0); }

OpCodes *code_eval(int argc) { NEW_CODES(OP_EVAL, argc); }

OpCodes *code_stry(int trylen, int catchlen, int finlen)
{ 
	TryInfo *ti = malloc(sizeof(TryInfo));
	ti->trylen = trylen;
	ti->catchlen = catchlen;
	ti->finallen = finlen;
	NEW_CODES(OP_STRY, ti); 
}
OpCodes *code_etry() { NEW_CODES(OP_ETRY, 0); }
OpCodes *code_scatch(const unichar *var) { NEW_CODES(OP_SCATCH, var); }
OpCodes *code_ecatch() { NEW_CODES(OP_ECATCH, 0); }
OpCodes *code_sfinal() { NEW_CODES(OP_SFINAL, 0); }
OpCodes *code_efinal() { NEW_CODES(OP_EFINAL, 0); }
OpCodes *code_throw() { NEW_CODES(OP_THROW, 0); }
OpCodes *code_with(int withlen) { NEW_CODES(OP_WITH, withlen); }
OpCodes *code_ewith() { NEW_CODES(OP_EWITH, 0); }

OpCodes *code_debug() { NEW_CODES(OP_DEBUG, 0); }
OpCodes *code_reserved(int type, unichar *id)
{
	ReservedInfo *ri = malloc(sizeof(ReservedInfo));
	ri->type = type;
	ri->label = id;
	ri->topop = 0;
	NEW_CODES(OP_RESERVED, ri);
}

JmpPopInfo *jpinfo_new(int off, int topop)
{
	JmpPopInfo *r = malloc(sizeof(JmpPopInfo));
	r->off = off;
	r->topop = topop;
	return r;
}

void code_reserved_replace(OpCodes *ops, int step_len, int break_only,
						   const unichar *desire_label, int topop)
{
	int i;
	for (i = 0; i < ops->code_len; ++i) {
		if (ops->codes[i].op != OP_RESERVED) continue;
		ReservedInfo *ri = ops->codes[i].data;

		if (ri->label) {
			if (!desire_label || unistrcmp(ri->label, desire_label) != 0) {
				ri->topop += topop;
				continue;
			}
		}
		
		if (ri->type == RES_CONTINUE) {
			if (break_only) {
				ri->topop += topop;
				continue;
			} else {
				int topop = ri->topop;
				free(ri);		/* kill reserved info, replace with other opcode */
				if (topop) {
					ops->codes[i].data = jpinfo_new(ops->code_len - i, topop);
					ops->codes[i].op = OP_JMPPOP;
				} else {
					ops->codes[i].data = (void *)(ops->code_len - i);
					ops->codes[i].op = OP_JMP;
				}
			}
		} else if (ri->type == RES_BREAK) {
			int topop = ri->topop;
			free(ri);
			if (topop) {
				ops->codes[i].data = jpinfo_new(step_len + ops->code_len - i, topop);
				ops->codes[i].op = OP_JMPPOP;
			} else {
				ops->codes[i].data = (void *)(step_len + ops->code_len - i);
				ops->codes[i].op = OP_JMP;
			}
		}
	}
}

void code_decode(OpCode *op, int currentip)
{
	if (op->op < 0 || op->op >= OP_LASTOP) {
		printf("Bad opcode[%d] at %d\n", op->op, currentip);
	}
	printf("%d:\t%s", currentip, op_names[op->op]);
	if (op->op == OP_PUSHBOO || op->op == OP_FCALL || op->op == OP_EVAL ||
		op->op == OP_POP || op->op == OP_ASSIGN ||
		op->op == OP_RET || op->op == OP_NEWFCALL ||
		op->op == OP_DELETE || op->op == OP_CHTHIS ||
 		op->op == OP_OBJECT || op->op == OP_ARRAY ||
 		op->op == OP_SHF ||
		op->op == OP_INC || op->op == OP_DEC) printf("\t%d\n", (int)op->data);
	else if (op->op == OP_PUSHNUM) printf("\t%g\n", *((double *)op->data));
	else if (op->op == OP_PUSHSTR || op->op == OP_LOCAL ||
			 op->op == OP_SCATCH) printf("\t\"%s\"\n", tochars(op->data ? op->data:"(NoCatch)"));
	else if (op->op == OP_PUSHVAR) printf("\tvar: \"%s\"\n", tochars(((FastVar *)op->data)->var.varname));
	else if (op->op == OP_PUSHFUN) printf("\tfunc: 0x%x\n", (int)op->data);
 	else if (op->op == OP_JTRUE || op->op == OP_JFALSE ||
			 op->op == OP_JTRUE_NP || op->op == OP_JFALSE_NP ||
			 op->op == OP_JMP) printf("\t{%d}\t#%d\n", (int)op->data, currentip + (int)op->data);
	else if (op->op == OP_JMPPOP) {
		JmpPopInfo *jp = op->data;
		printf("\t{%d},%d\t#%d\n", jp->off, jp->topop, currentip + jp->off);
	}
	else if (op->op == OP_STRY) {
		TryInfo *t = (TryInfo *)op->data;
		printf("\t{try:%d, catch:%d, final:%d}\n", t->trylen, t->catchlen, t->finallen);
	} else printf("\n");
}

void codes_free(OpCodes *ops)
{
	/* TODO*/
	free(ops->codes);
	free(ops);
}

void codes_print(OpCodes *ops)
{
	int i = 0;
	OpCode *opcodes = ops->codes;
	int opcodesi = ops->code_len;
	
	printf("opcodes count = %d\n", opcodesi);
	
	while(i < opcodesi) {
		code_decode(&opcodes[i], i);
		i++;
	}
}
