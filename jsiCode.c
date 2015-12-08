#ifndef JSI_LITE_ONLY
#ifndef _JSI_CODE_C_
#define _JSI_CODE_C_
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

/* replace continue/break(coded as OP_RESERVED) jmp
 * |------------------| \
 * |                  | \\ where 'continue' jmp (jmp to step code)
 * |       ops        |  / 
 * |                  | / \
 * |------------------|    \ where 'break' jmp (jmp after step code)
 * |                  |    /
 * |       step       |   /
 * |                  |  /
 * |------------------| /
 * 1. break_only used only in switch
 * 2. desire_label, only replace if current iter statement has the same label with opcode
 * 3. topop, if not replace in current iter statment, make sure when jmp out of this loop/switch
 *    corrent stack elems poped(for in always has 2 elem, while switch has 1)
 */

static const char *op_names[OP_LASTOP] = {
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
    "INSTANCEOF",
    "ASSIGN",
    "SUBSCRIPT",
    "INC",
    "TYPEOF",
    "DEC",
    "IN",
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
    "DEBUG"
};

static OpCodes *codes_new(int size)
{
    OpCodes *ret = Jsi_Calloc(1,sizeof(*ret));
    ret->codes = Jsi_Calloc(size, sizeof(OpCode));
    ret->code_size = size;
    return ret;
}

static int codes_insert(OpCodes *c, Eopcode code, void *extra, int doalloc)
{
    if (c->code_size - c->code_len <= 0) {
        c->code_size += 100;
        c->codes = Jsi_Realloc(c->codes, c->code_size * sizeof(OpCode));
    }
    c->codes[c->code_len].op = code;
    c->codes[c->code_len].data = extra;
    c->codes[c->code_len].alloc = doalloc;
    c->code_len ++;
    return 0;
}

static int codes_insertln(OpCodes *c, Eopcode code, void *extra, jsi_Pstate *pstate, jsi_Pline *line, int doalloc)
{
    if (c->code_size - c->code_len <= 0) {
        c->code_size += 100;
        c->codes = Jsi_Realloc(c->codes, c->code_size * sizeof(OpCode));
    }
    c->codes[c->code_len].op = code;
    c->codes[c->code_len].data = extra;
    c->codes[c->code_len].line = (code == OP_FCALL ? line->first_line-1:line->first_line);
    c->codes[c->code_len].fname = jsi_PstateGetFilename(pstate);
    c->codes[c->code_len].alloc = doalloc;
    c->code_len ++;
    return 0;
}


static OpCodes *codes_join(OpCodes *a, OpCodes *b)
{
    OpCodes *ret = codes_new(a->code_len + b->code_len);
    memcpy(ret->codes, a->codes, a->code_len * sizeof(OpCode));
    memcpy(&ret->codes[a->code_len], b->codes, b->code_len * sizeof(OpCode));
    ret->code_size = a->code_len + b->code_len;
    ret->code_len = ret->code_size;
    ret->expr_counter = a->expr_counter + b->expr_counter;
    Jsi_Free(a->codes);
    Jsi_Free(b->codes);
    Jsi_Free(a);
    Jsi_Free(b);
    return ret;
}

static OpCodes *codes_join3(OpCodes *a, OpCodes *b, OpCodes *c)
{
    return codes_join(codes_join(a, b), c);
}

static OpCodes *codes_join4(OpCodes *a, OpCodes *b, OpCodes *c, OpCodes *d)
{
    return codes_join(codes_join(a, b), codes_join(c, d));
}

#define NEW_CODES(doalloc,code, extra) do {                 \
        OpCodes *r = codes_new(3);                  \
        codes_insert(r, (code), (void *)(extra), doalloc);   \
        return r;                                   \
    } while(0)

#define NEW_CODESLN(doalloc,code, extra) do {                 \
        OpCodes *r = codes_new(3);                  \
        codes_insertln(r, (code), (void *)(extra), p, line, doalloc);   \
        return r;                                   \
    } while(0)

static OpCodes *code_push_undef() { NEW_CODES(0,OP_PUSHUND, 0); }
static OpCodes *code_push_bool(int v) { NEW_CODES(0,OP_PUSHBOO, v); }
static OpCodes *code_push_num(Jsi_Number *v) { NEW_CODES(0,OP_PUSHNUM, v); }
static OpCodes *code_push_string(jsi_Pstate *p, jsi_Pline *line, const char *str) {
    if (*str == 'c' && !strcmp(str,"callee"))
        p->interp->hasCallee = 1;
    NEW_CODESLN(0,OP_PUSHSTR, str);
}

static OpCodes *code_push_index(jsi_Pstate *p, jsi_Pline *line, char *varname)
{
    FastVar *n = Jsi_Calloc(1,sizeof(*n)); /* TODO: free when opcodes are freed. */
    n->sig = JSI_SIG_FASTVAR;
    n->ps = p;
    n->context_id = -1;
    n->var.varname = (char*)Jsi_KeyAdd(p->interp, varname);
    Jsi_HashSet(p->fastVarTbl, n, n);
    NEW_CODESLN(1,OP_PUSHVAR, n);
}

static OpCodes *code_push_this(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_PUSHTHS, 0); }
static OpCodes *code_push_top() { NEW_CODES(0,OP_PUSHTOP, 0); }
static OpCodes *code_push_top2() { NEW_CODES(0,OP_PUSHTOP2, 0); }
static OpCodes *code_unref() { NEW_CODES(0,OP_UNREF, 0); }
static OpCodes *code_push_args() { NEW_CODES(0,OP_PUSHARG, 0); }
static OpCodes *code_push_func(jsi_Pstate *p, jsi_Pline *line, struct Jsi_Func *fun) { p->funcDefs++; NEW_CODESLN(0,OP_PUSHFUN, fun); }
static OpCodes *code_push_regex(jsi_Pstate *p, jsi_Pline *line, Jsi_Regex *reg) { NEW_CODESLN(0,OP_PUSHREG, reg); }

static OpCodes *code_local(jsi_Pstate *p, jsi_Pline *line, const char *varname) { NEW_CODESLN(0,OP_LOCAL, varname); }

static OpCodes *code_nop() { NEW_CODES(0,OP_NOP, 0); }
static OpCodes *code_neg() { NEW_CODES(0,OP_NEG, 0); }
static OpCodes *code_pos() { NEW_CODES(0,OP_POS, 0); }
static OpCodes *code_bnot() { NEW_CODES(0,OP_BNOT, 0); }
static OpCodes *code_not() { NEW_CODES(0,OP_NOT, 0); }
static OpCodes *code_mul() { NEW_CODES(0,OP_MUL, 0); }
static OpCodes *code_div() { NEW_CODES(0,OP_DIV, 0); }
static OpCodes *code_mod() { NEW_CODES(0,OP_MOD, 0); }
static OpCodes *code_add() { NEW_CODES(0,OP_ADD, 0); }
static OpCodes *code_sub() { NEW_CODES(0,OP_SUB, 0); }
static OpCodes *code_in() { NEW_CODES(0,OP_IN, 0); }
static OpCodes *code_less() { NEW_CODES(0,OP_LESS, 0); }
static OpCodes *code_greater() { NEW_CODES(0,OP_GREATER, 0); }
static OpCodes *code_lessequ() { NEW_CODES(0,OP_LESSEQU, 0); }
static OpCodes *code_greaterequ() { NEW_CODES(0,OP_GREATEREQU, 0); }
static OpCodes *code_equal() { NEW_CODES(0,OP_EQUAL, 0); } 
static OpCodes *code_notequal() { NEW_CODES(0,OP_NOTEQUAL, 0); }
static OpCodes *code_eequ() { NEW_CODES(0,OP_STRICTEQU, 0); }
static OpCodes *code_nneq() { NEW_CODES(0,OP_STRICTNEQ, 0); }
static OpCodes *code_band() { NEW_CODES(0,OP_BAND, 0); }
static OpCodes *code_bor() { NEW_CODES(0,OP_BOR, 0); }
static OpCodes *code_bxor() { NEW_CODES(0,OP_BXOR, 0); }
static OpCodes *code_shf(int right) { NEW_CODES(0,OP_SHF, right); }
static OpCodes *code_instanceof() { NEW_CODES(0,OP_INSTANCEOF, 0); }
static OpCodes *code_assign(jsi_Pstate *p, jsi_Pline *line, int h) { NEW_CODESLN(0,OP_ASSIGN, h); }
static OpCodes *code_subscript(jsi_Pstate *p, jsi_Pline *line, int right_val) { NEW_CODESLN(0,OP_SUBSCRIPT, right_val); }
static OpCodes *code_inc(jsi_Pstate *p, jsi_Pline *line, int e) { NEW_CODESLN(0,OP_INC, e); }
static OpCodes *code_dec(jsi_Pstate *p, jsi_Pline *line, int e) { NEW_CODESLN(0,OP_DEC, e); }
static OpCodes *code_typeof(jsi_Pstate *p, jsi_Pline *line, int e) { NEW_CODESLN(0,OP_TYPEOF, e); }

static OpCodes *code_fcall(jsi_Pstate *p, jsi_Pline *line, int argc) { NEW_CODESLN(0,OP_FCALL, argc); }
static OpCodes *code_newfcall(jsi_Pstate *p, jsi_Pline *line, int argc) { NEW_CODESLN(0,OP_NEWFCALL, argc); }
static OpCodes *code_ret(int n) { NEW_CODES(0,OP_RET, n); }
static OpCodes *code_delete(int n) { NEW_CODES(0,OP_DELETE, n); }
static OpCodes *code_chthis(int n) { NEW_CODES(0,OP_CHTHIS, n); }
static OpCodes *code_pop(int n) { NEW_CODES(0,OP_POP, n); }
static OpCodes *code_jfalse(int off) { NEW_CODES(0,OP_JFALSE, off); }
static OpCodes *code_jtrue(int off) { NEW_CODES(0,OP_JTRUE, off); }
static OpCodes *code_jfalse_np(int off) { NEW_CODES(0,OP_JFALSE_NP, off); }
static OpCodes *code_jtrue_np(int off) { NEW_CODES(0,OP_JTRUE_NP, off); }
static OpCodes *code_jmp(int off) { NEW_CODES(0,OP_JMP, off); }
static OpCodes *code_object(jsi_Pstate *p, jsi_Pline *line, int c) { NEW_CODESLN(0,OP_OBJECT, c); }
static OpCodes *code_array(jsi_Pstate *p, jsi_Pline *line, int c) { NEW_CODESLN(0,OP_ARRAY, c); }
static OpCodes *code_key() { NEW_CODES(0,OP_KEY, 0); }
static OpCodes *code_next() { NEW_CODES(0,OP_NEXT, 0); }

static OpCodes *code_eval(jsi_Pstate *p, jsi_Pline *line, int argc) { NEW_CODESLN(0,OP_EVAL, argc); }

static OpCodes *code_stry(jsi_Pstate *p, jsi_Pline *line, int trylen, int catchlen, int finlen)
{ 
    TryInfo *ti = Jsi_Calloc(1,sizeof(*ti));
    ti->trylen = trylen;
    ti->catchlen = catchlen;
    ti->finallen = finlen;
    NEW_CODESLN(1,OP_STRY, ti); 
}
static OpCodes *code_etry(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_ETRY, 0); }
static OpCodes *code_scatch(jsi_Pstate *p, jsi_Pline *line, const char *var) { NEW_CODESLN(0,OP_SCATCH, var); }
static OpCodes *code_ecatch(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_ECATCH, 0); }
static OpCodes *code_sfinal(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_SFINAL, 0); }
static OpCodes *code_efinal(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_EFINAL, 0); }
static OpCodes *code_throw(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_THROW, 0); }
static OpCodes *code_with(jsi_Pstate *p, jsi_Pline *line, int withlen) { NEW_CODESLN(0,OP_WITH, withlen); }
static OpCodes *code_ewith(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_EWITH, 0); }

static OpCodes *code_debug(jsi_Pstate *p, jsi_Pline *line) { NEW_CODESLN(0,OP_DEBUG, 0); }
static OpCodes *code_reserved(jsi_Pstate *p, jsi_Pline *line, int type, char *id)
{
    ReservedInfo *ri = Jsi_Calloc(1, sizeof(*ri));
    ri->type = type;
    ri->label = id;
    ri->topop = 0;
    NEW_CODESLN(1,OP_RESERVED, ri);
}

static JmpPopInfo *jpinfo_new(int off, int topop)
{
    JmpPopInfo *r = Jsi_Calloc(1, sizeof(*r));
    r->off = off;
    r->topop = topop;
    return r;
}

static void code_reserved_replace(OpCodes *ops, int step_len, int break_only,
                           const char *desire_label, int topop)
{
    int i;
    for (i = 0; i < ops->code_len; ++i) {
        if (ops->codes[i].op != OP_RESERVED) continue;
        ReservedInfo *ri = ops->codes[i].data;

        if (ri->label) {
            if (!desire_label || Jsi_Strcmp(ri->label, desire_label) != 0) {
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
                Jsi_Free(ri);       /* kill reserved Warn, replace with other opcode */
 /*               if (ops->codes[i].data && ops->codes[i].alloc) //TODO: memory leak?
                    Jsi_Free(ops->codes[i].data);*/
                if (topop) {
                    ops->codes[i].data = jpinfo_new(ops->code_len - i, topop);
                    ops->codes[i].op = OP_JMPPOP;
                    ops->codes[i].alloc = 1;
                } else {
                    ops->codes[i].data = (void *)(ops->code_len - i);
                    ops->codes[i].op = OP_JMP;
                    ops->codes[i].alloc = 0;
                }
            }
        } else if (ri->type == RES_BREAK) {
            int topop = ri->topop;
            Jsi_Free(ri);
/*           if (ops->codes[i].data && ops->codes[i].alloc)
                Jsi_Free(ops->codes[i].data); */
            if (topop) {
                ops->codes[i].data = jpinfo_new(step_len + ops->code_len - i, topop);
                ops->codes[i].op = OP_JMPPOP;
                ops->codes[i].alloc = 1;
            } else {
                ops->codes[i].data = (void *)(step_len + ops->code_len - i);
                ops->codes[i].op = OP_JMP;
                ops->codes[i].alloc = 0;
            }
        }
    }
}

void jsi_code_decode(OpCode *op, int currentip)
{
    if (op->op < 0 || op->op >= OP_LASTOP) {
        printf("Bad opcode[%d] at %d\n", op->op, currentip);
    }
    if (op->line)
        printf("LINE:%d: %d:\t%s", op->line, currentip, op_names[op->op]);
    else
        printf("%d:\t%s", currentip, op_names[op->op]);
    if (op->op == OP_PUSHBOO || op->op == OP_FCALL || op->op == OP_EVAL ||
        op->op == OP_POP || op->op == OP_ASSIGN ||
        op->op == OP_RET || op->op == OP_NEWFCALL ||
        op->op == OP_DELETE || op->op == OP_CHTHIS ||
        op->op == OP_OBJECT || op->op == OP_ARRAY ||
        op->op == OP_SHF ||
        op->op == OP_INC || op->op == OP_DEC) printf("\t%d\n", (int)op->data);
    else if (op->op == OP_PUSHNUM) printf("\t%" JSI_NUMGFMT "\n", *((Jsi_Number *)op->data));
    else if (op->op == OP_PUSHSTR || op->op == OP_LOCAL ||
             op->op == OP_SCATCH) printf("\t\"%s\"\n", op->data ? (char*)op->data:"(NoCatch)");
    else if (op->op == OP_PUSHVAR) printf("\tvar: \"%s\"\n", ((FastVar *)op->data)->var.varname);
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

static void codes_free(OpCodes *ops)
{
    /* TODO*/
    int i;
    for (i=0; i<ops->code_len; i++) {
        /*OpCode *op = ops->codes+i;
        if (op->data && op->alloc)
            Jsi_Free(op->data);*/
    }
    Jsi_Free(ops->codes);
    Jsi_Free(ops);
}

void jsi_FreeOpcodes(OpCodes *ops) {
    codes_free(ops);
}

void jsi_codes_print(OpCodes *ops)
{
    int i = 0;
    OpCode *opcodes = ops->codes;
    int opcodesi = ops->code_len;
    
    printf("opcodes count = %d\n", opcodesi);
    
    while(i < opcodesi) {
        jsi_code_decode(&opcodes[i], i);
        i++;
    }
}
#endif
#endif
