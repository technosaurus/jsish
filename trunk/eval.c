#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "code.h"
#include "pstate.h"
#include "eval.h"
#include "func.h"
#include "error.h"
#include "value.h"
#include "proto.h"
#include "utils.h"
#include "unichar.h"
#include "mempool.h"

/* todo list:
 * ecma, instanceof, in opr
 */

/* stacks */
Value stack[65536];

/* with a function call, opcode chthis will make obj_this assign to new this */
Value obj_this[65536];

int sp = 0;

#define TOP	(stack[sp-1])
#define TOQ (stack[sp-2])

/* pop n value from stack */
#define pop_n(n) do {										\
	int t = (int)(n);										\
	while (t > 0) {											\
		value_erase(obj_this[sp-t]);						\
		value_erase(stack[sp-t]);							\
		--t;												\
	}														\
	sp -= (int)(n);											\
} while(0)

#define pop() do {										\
	value_erase(obj_this[sp-1]);						\
	value_erase(TOP);									\
	--sp;												\
} while(0)

/* make top to be a right value */
#define topeval1() do {							\
	if (stack[sp - 1].vt == VT_VARIABLE) {		\
		Value *v = stack[sp - 1].d.lval;		\
		value_copy(stack[sp - 1], *v);			\
	}											\
} while(0)

/* make top&toq to be right values */
#define topeval2() do {							\
	if (stack[sp - 1].vt == VT_VARIABLE) {		\
		Value *v = stack[sp - 1].d.lval;		\
		value_copy(stack[sp - 1], *v);			\
	}											\
	if (stack[sp - 2].vt == VT_VARIABLE) {		\
		Value *v = stack[sp - 2].d.lval;		\
		value_copy(stack[sp - 2], *v);			\
	}											\
} while(0)

/* make top n to be right values */
#define topevaln(n) do {						\
	int a = (n); 								\
	int c; 										\
	for (c = 1; c <= a; ++c) { 					\
		if (stack[sp - c].vt == VT_VARIABLE) {	\
			Value *v = stack[sp - c].d.lval;	\
			value_copy(stack[sp - c], *v);		\
		}										\
	}											\
} while(0)

#define common_math_opr(opr) {						\
	topeval2();										\
	if (!is_number(&TOP)) value_tonumber(&TOP);		\
	if (!is_number(&TOQ)) value_tonumber(&TOQ);		\
	TOQ.d.num = TOQ.d.num opr TOP.d.num;			\
	pop();											\
}

#define common_bitwise_opr(opr) {						\
	int a, b;										\
	topeval2();										\
	if (!is_number(&TOP)) value_tonumber(&TOP);		\
	if (!is_number(&TOQ)) value_tonumber(&TOQ);		\
	a = TOQ.d.num; b = TOP.d.num;					\
	TOQ.d.num = (double)(a opr b);					\
	pop();											\
}

/* todo: use memcmp may cause bug, use a strncmp competible unistrncmp */
#define logic_less(v1, v2, res) do {								\
	int val = 0;													\
	value_toprimitive(&v1);											\
	value_toprimitive(&v2);											\
	if (is_string(&v1) && is_string(&v2)) {							\
		int v1len = unistrlen(v1.d.str);							\
		int v2len = unistrlen(v2.d.str);							\
		int mlen = v1len < v2len ? v1len : v2len;					\
		val = memcmp(v1.d.str, v2.d.str, mlen * sizeof(unichar));	\
		if (val > 0) val = 0;										\
		else if (val < 0) val = 1;									\
		else val = (v1len < v2len);									\
		value_erase(res);											\
		value_make_bool(res, val);									\
	} else {														\
		value_tonumber(&v1);										\
		value_tonumber(&v2);										\
		if (ieee_isnan(v1.d.num) || ieee_isnan(v2.d.num)) {			\
			value_erase(res);										\
			value_make_undef(res);									\
		} else {													\
			val = (v1.d.num < v2.d.num);							\
			value_erase(res);										\
			value_make_bool(res, val);								\
		}															\
	}																\
} while(0);

#ifdef DEBUG

static const char *vprint(Value *v)
{
	static char buf[100];
	if (is_number(v)) {
		snprintf(buf, 100, "NUM:%g ", v->d.num);
	} else if (v->vt == VT_BOOL) {
		snprintf(buf, 100, "BOO:%d ", v->d.val);
	} else if (v->vt == VT_STRING) {
		snprintf(buf, 100, "STR:%s ", tochars(v->d.str));
	} else if (v->vt == VT_VARIABLE) {
		snprintf(buf, 100, "VAR:%x ", (int)v->d.lval);
	} else if (v->vt == VT_NULL) {
		snprintf(buf, 100, "NUL:null ");
	} else if (v->vt == VT_OBJECT) {
		snprintf(buf, 100, "OBJ:%x", (int)v->d.obj);
	} else if (v->vt == VT_UNDEF) {
		snprintf(buf, 100, "UND:undefined");
	}
	return buf;
}
#endif

typedef enum {
		TL_TRY,
		TL_WITH,
} ttype;							/* type of try */

typedef struct TryList {
	ttype type;
	union {
		struct {					/* try data */
			OpCode *tstart;			/* try start ip */
			OpCode *tend;			/* try end ip */
			OpCode *cstart;			/* ...*/
			OpCode *cend;
			OpCode *fstart;
			OpCode *fend;
			int sp;
			enum {
				LOP_NOOP,
				LOP_THROW,
				LOP_JMP
			} last_op;				/* what to do after finally block */
									/* depend on last jmp code in catch block */
			union {
				OpCode *tojmp;
			} ld;					/* jmp out of catch (target)*/
		} td;
		struct {					/* with data */
			OpCode *wstart;			/* with start */
			OpCode *wend;			/* with end */
		} wd;
	} d;
	
	ScopeChain *scope_save;			/* saved scope (used in catch block/with block)*/
	Value *curscope_save;			/* saved current scope */
	struct TryList *next;
} TryList;

/* restore scope chain */
#define restore_scope(scope_save, curscope_save) do {	\
	if (scope != (scope_save)) {						\
		scope_chain_free(scope);						\
		scope = (scope_save);							\
	}													\
	if (currentScope != (curscope_save)) {				\
		value_free(currentScope);						\
		currentScope = (curscope_save);					\
	}													\
	context_id = ps->_context_id++;						\
} while(0)

/* destory top of trylist */
#define pop_try(head) do {				\
	TryList *t = (head)->next;			\
	mm_free((head));					\
	(head) = t;							\
} while(0)

#define push_try(head, n) do {			\
	(n)->next = (head);					\
	(head) = (n);						\
} while(0)
				
/* do_throw return fail, or make IP to the right place.*/
/* so make sure the next opration of do_throw is ip++*/
#define do_throw() while (1) {						\
	if (trylist == NULL) return -1;					\
	if (trylist->type == TL_TRY) {					\
		int n = sp - trylist->d.td.sp;				\
		pop_n(n);									\
		if (ip >= trylist->d.td.tstart && ip < trylist->d.td.tend) {		\
			ip = trylist->d.td.cstart - 1;									\
			break;															\
		} else if (ip >= trylist->d.td.cstart && ip < trylist->d.td.cend) {	\
			trylist->d.td.last_op = LOP_THROW;								\
			ip = trylist->d.td.fstart - 1;									\
			break;															\
		} else if (ip >= trylist->d.td.fstart && ip < trylist->d.td.fend) {	\
			pop_try(trylist);												\
		} else bug("Throw within a try, but not in its scope?");			\
	} else {																\
		restore_scope(trylist->scope_save, trylist->curscope_save);			\
		pop_try(trylist);													\
	}																		\
}

TryList *trylist_new(ttype t, ScopeChain *scope_save, Value *curscope_save)
{
	TryList *n = mm_alloc(sizeof(TryList));
	memset(n, 0, sizeof(TryList));
	
	n->type = t;
	n->curscope_save = curscope_save;
	n->scope_save = scope_save;
	
	return n;
}

static UNISTR(9) PROTOTYPE = { 9, { 'p','r','o','t','o','t','y','p','e' } };
static UNISTR(6) CALLEE = { 6, { 'c','a','l','l','e','e' } };
static UNISTR(8) _CALLEE_ = { 8, { 1,'c','a','l','l','e','e', 1 } };

int eval(PSTATE *ps, OpCodes *opcodes, 
		 ScopeChain *scope, Value *currentScope, 	/* scope chain */
		 Value *_this,
		 Value *vret)
{
	int context_id = ps->_context_id++;
	OpCode *ip = &opcodes->codes[0];
	OpCode *end = &opcodes->codes[opcodes->code_len];
	TryList *trylist = NULL;
	
	if (currentScope->vt != VT_OBJECT) {
		bug("Eval: current scope is not a object\n");
	}
	
	while(ip < end) {
#ifdef DEBUG
		int i;
		printf("STACK%d: ", sp);
		for (i = 0; i < sp; ++i) {
			printf("%s ", vprint(&stack[i]));
		}
		printf("\tthis: %s ", vprint(_this));
		TryList *tlt = trylist;
		for (i = 0; tlt; tlt = tlt->next) i++;
		printf("TL: %d, excpt: %s\n", i, vprint(&ps->last_exception));
		code_decode(ip, ip - opcodes->codes);

		
#endif
		switch(ip->op) {
			case OP_NOP:
			case OP_LASTOP:
				break;
			case OP_PUSHNUM:
				value_make_number(stack[sp], (*((double *)ip->data)));
				sp++;
				break;
			case OP_PUSHSTR:
				value_make_string(stack[sp], unistrdup(ip->data));
				sp++;
				break;
			case OP_PUSHVAR: {
				FastVar *n = ip->data;
				Value *v = NULL;
				if (n->context_id == context_id) {
					v = n->var.lval;
				} else {
					unichar *varname = n->var.varname;
					v = value_object_lookup(currentScope, (ObjKey *)varname, NULL);
					if (!v) v = scope_chain_object_lookup(scope, (ObjKey *)varname);

					if (!v) {	/* add to global scope */
						Value *global_scope = scope->chains_cnt > 0 ? scope->chains[0]:currentScope;
						Value key;
						value_make_string(key, varname);	/* varname is not dupped, do not erase*/
						Value val;
						value_make_undef(val);
						v = value_object_key_assign(global_scope, &key, &val, OM_DONTEMU);
						
						/* key assign dup key and insert into object, so release ourself */
					}
					n->context_id = context_id;
					n->var.lval = v;
				}
				stack[sp].vt = VT_VARIABLE;
				stack[sp].d.lval = v;
				sp++;
				break;
			}
			case OP_PUSHUND:
				value_make_undef(stack[sp]);
				sp++;
				break;
			case OP_PUSHBOO:
				value_make_bool(stack[sp], (int)ip->data);
				sp++;
				break;
			case OP_PUSHFUN: {
				FuncObj *fo = funcobj_new((Func *)ip->data);
				fo->scope =	scope_chain_dup_next(scope, currentScope);
				
				Object *obj = object_new();
				obj->ot = OT_FUNCTION;
				obj->d.fobj = fo;
				obj->__proto__ = Function_prototype;
				
				Value *fun_prototype = value_object_utils_new_object();
				fun_prototype->d.obj->__proto__ = Object_prototype;
				
				value_make_object(stack[sp], obj);
				
				value_object_utils_insert(&stack[sp], PROTOTYPE.unistr, fun_prototype, 0, 1, 0);
				/* todo: make own prototype and prototype.constructor */
				sp++;
				break;
			}
			case OP_PUSHREG: {
				Object *obj = object_new();
				obj->ot = OT_REGEXP;
				obj->d.robj = (regex_t *)ip->data;
				obj->__proto__ = RegExp_prototype;

				value_make_object(stack[sp], obj);
				sp++;
				break;
			}
			case OP_PUSHARG:
				value_copy(stack[sp], *currentScope);
				sp++;
				break;
			case OP_PUSHTHS:
				value_copy(stack[sp], *_this);
				sp++;
				break;
			case OP_PUSHTOP:
				value_copy(stack[sp], TOP);
				sp++;
				break;
			case OP_UNREF:
				topeval1();
				break;
			case OP_PUSHTOP2:
				value_copy(stack[sp], TOQ);
				value_copy(stack[sp+1], TOP);
				sp += 2;
				break;
			case OP_CHTHIS: {
				int t = sp - 2;
				if (ip->data) {
					value_erase(obj_this[t]);
					value_copy(obj_this[t], TOQ);
					if (obj_this[t].vt == VT_VARIABLE) {
						Value *v = obj_this[t].d.lval;
						value_copy(obj_this[t], *v);
					}
					value_toobject(&obj_this[t]);
				}
				break;
			}
			case OP_LOCAL: {
				ObjKey *strkey = objkey_new((const unichar *)ip->data, OM_DONTEMU);
				value_object_insert(currentScope, strkey, value_new());
				
				/* make all FastVar to be relocated */
				context_id = ps->_context_id++;
				break;
			}
			case OP_POP:
				pop_n(ip->data);
				break;
			case OP_NEG:
				topeval1();
				value_tonumber(&TOP);
				TOP.d.num = -(TOP.d.num);
				break;
			case OP_POS:
				topeval1();
				value_tonumber(&TOP);
				break;
			case OP_NOT: {
				int val = 0;
				topeval1();
				
				val = value_istrue(&TOP);
				
				value_erase(TOP);
				value_make_bool(TOP, !val);
				break;
			}
			case OP_BNOT: {
				topeval1();
				value_toint32(&TOP);
				TOP.d.num = (double)(~((int)TOP.d.num));
				break;
			}
			case OP_ADD: {
				topeval2();
				value_toprimitive(&TOP);
				value_toprimitive(&TOQ);
				
				if (TOP.vt == VT_STRING || TOQ.vt == VT_STRING) {
					value_tostring(&TOP);
					value_tostring(&TOQ);
					
					unichar *v = unistrcat(TOQ.d.str, TOP.d.str);
					
					value_erase(TOQ);
					value_make_string(TOQ, v);
				} else {
					value_tonumber(&TOP);
					value_tonumber(&TOQ);
					double n = TOP.d.num + TOQ.d.num;
					value_erase(TOQ);
					value_make_number(TOQ, n);
				}
				pop();
				break;
			}
			case OP_SUB:	/* god, the notes in ecma is so long, pray to run correctly */
				common_math_opr(-); break;
			case OP_MUL:
				common_math_opr(*); break;
			case OP_DIV:
				common_math_opr(/); break;
			case OP_MOD: {
				topeval2();
				if (!is_number(&TOP)) value_tonumber(&TOP);
				if (!is_number(&TOQ)) value_tonumber(&TOQ);

				TOQ.d.num = fmod(TOQ.d.num, TOP.d.num);
				pop();
				break;
			}
			case OP_LESS:
				topeval2();
				logic_less(TOQ, TOP, TOQ);
				pop();
				break;
			case OP_GREATER:
				topeval2();
				logic_less(TOP, TOQ, TOQ);
				pop();
				break;
			case OP_LESSEQU:
				topeval2();
				logic_less(TOP, TOQ, TOQ);
				TOQ.d.val = !TOQ.d.val;
				pop();
				break;
			case OP_GREATEREQU:
				topeval2();
				logic_less(TOQ, TOP, TOQ);
				TOQ.d.val = !TOQ.d.val;
				pop();
				break;
			case OP_EQUAL:
			case OP_NOTEQUAL: {		/* awful, equal opration */
				int r = 0;
				topeval2();
				if (TOP.vt != TOQ.vt) {
					value_toprimitive(&TOP);
					value_toprimitive(&TOQ);
				}
				if (TOP.vt != TOQ.vt) {
					if ((is_undef(&TOP) || is_null(&TOP)) && 
						(is_undef(&TOQ) || is_null(&TOQ))) {
						r = 1;
					} else {
						value_tonumber(&TOP);
						value_tonumber(&TOQ);
						r = (TOP.d.num == TOQ.d.num);
					}
				} else {
					switch (TOP.vt) {
						case VT_NUMBER:
							r = (TOP.d.num == TOQ.d.num);
							break;
						case VT_BOOL:
							r = (TOP.d.val == TOQ.d.val);
							break;
						case VT_STRING:
							r = (unistrcmp(TOQ.d.str, TOP.d.str) == 0);
							break;
						case VT_OBJECT:
							/* todo: refer to objects joined to each other */
							r = (TOP.d.obj == TOQ.d.obj);
							break;
						case VT_UNDEF:
						case VT_NULL:
							r = 1;
							break;
						default:
							bug("Unexpected value type\n");
					}
				}
				r = (ip->op == OP_EQUAL ? r : !r);
				value_erase(TOQ);
				value_make_bool(TOQ, r);
				pop();
				break;
			}
			case OP_STRICTEQU:
			case OP_STRICTNEQ: {
				int r = 0;
				topeval2();
				if (TOP.vt == TOQ.vt) {
					switch (TOP.vt) {
						case VT_NUMBER:
							r = (TOP.d.num == TOQ.d.num);
							break;
						case VT_BOOL:
							r = (TOP.d.val == TOQ.d.val);
							break;
						case VT_STRING:
							r = (unistrcmp(TOQ.d.str, TOP.d.str) == 0);
							break;
						case VT_OBJECT:
							/* todo: refer to objects joined to each other */
							r = (TOP.d.obj == TOQ.d.obj);
							break;
						case VT_UNDEF:
						case VT_NULL:
							r = 1;
							break;
						default:
							bug("Unexpected value type\n");
					}
				}
				r = (ip->op == OP_STRICTEQU ? r : !r);
				value_erase(TOQ);
				value_make_bool(TOQ, r);
				pop();
				break;
			}
			case OP_BAND: 
				common_bitwise_opr(&); break;
			case OP_BOR:
				common_bitwise_opr(|); break;
			case OP_BXOR:
				common_bitwise_opr(^); break;
			case OP_SHF: {
				topeval2();
				value_toint32(&TOQ);
				value_toint32(&TOP);
				int t1 = (int)TOQ.d.num;
				int t2 = ((unsigned int)TOP.d.num) & 0x1f;
				if (ip->data) {					/* thift right */
					if ((int)ip->data == 2) {	/* unsigned shift */
						unsigned int t3 = (unsigned int)t1;
						t3 >>= t2;
						value_make_number(TOQ, t3);
					} else {
						t1 >>= t2;
						value_make_number(TOQ, t1);
					}
				} else {
					t1 <<= t2;
					value_make_number(TOQ, t1);
				}
				pop();
				break;
			}
			case OP_ASSIGN: {
				if ((int)ip->data == 1) {
					topeval1();
					if (TOQ.vt != VT_VARIABLE) die("operand not a left value\n");

					Value *v = TOQ.d.lval;
					value_erase(*v);
					value_copy(*v, TOP);
					
					value_erase(TOQ);
					value_copy(TOQ, TOP);
					pop();
				} else {
					topevaln(3);
					if (stack[sp-3].vt == VT_OBJECT) {
						value_object_key_assign(&stack[sp-3], &TOQ, &TOP, 0);
 					} else warn("assign to a non-esist object\n");
					value_erase(stack[sp-3]);
					value_copy(stack[sp-3], TOP);
					pop_n(2);
				}
				break;
			}
			case OP_SUBSCRIPT: {
				topeval2();
				Value res = {0};
				value_toobject(&TOQ);
				if (TOQ.d.obj == currentScope->d.obj) {	/* callee hack, fuck ecma */
					if (TOP.vt == VT_STRING && unistrcmp(TOP.d.str, CALLEE.unistr) == 0) {
						value_erase(TOP);
						value_make_string(TOP, unistrdup(_CALLEE_.unistr));
					}
				}
				value_subscript(&TOQ, &TOP, &res, (int)ip->data);
				value_erase(TOQ);
				TOQ = res;		/* don't need to erase */
				pop();
				break;
			}
			case OP_KEY: {
				topeval1();
				value_toobject(&TOP);
				Value spret = {0};
				value_object_getkeys(&TOP, &spret);

				stack[sp] = spret;
				sp++;
				break;
			}
			case OP_NEXT: {
				if (TOQ.vt != VT_OBJECT || TOQ.d.obj->ot != OT_ITER) bug("next: TOQ not a iter\n");
				if (TOP.vt != VT_VARIABLE) die("invalid for/in left hand-side\n");
				
				IterObj *io = TOQ.d.obj->d.iobj;

				while (io->iter < io->count) {
					if (value_key_present(&stack[sp-3], io->keys[io->iter])) break;
					io->iter++;
				}
				if (io->iter >= io->count) {
					value_erase(TOP);
					value_make_number(TOP, 0);
				} else {
					Value *v = TOP.d.lval;
					value_erase(*v);
					
					value_make_string(*v, unistrdup(io->keys[io->iter]));
					io->iter++;
					
					value_erase(TOP);
					value_make_number(TOP, 1);
				}
				break;
			}
			case OP_INC:
			case OP_DEC: {
				int inc = ip->op == OP_INC ? 1 : -1;
				
				if (TOP.vt != VT_VARIABLE) die("operand not left value\n");
				Value *v = TOP.d.lval;
				value_tonumber(v);
				
				v->d.num += inc;
					
				topeval1();
				if (ip->data) {
					TOP.d.num -= inc;
				}
				break;
			}
			case OP_JTRUE:
			case OP_JFALSE: 
			case OP_JTRUE_NP:
			case OP_JFALSE_NP: {
				topeval1();
				int off = (int)ip->data - 1; 
				int r = value_istrue(&TOP);
				
				if (ip->op == OP_JTRUE || ip->op == OP_JFALSE) pop();
				ip += ((ip->op == OP_JTRUE || ip->op == OP_JTRUE_NP) ^ r) ? 0 : off;
				break;
			}
			case OP_JMPPOP: 
				pop_n(((JmpPopInfo *)ip->data)->topop);
			case OP_JMP: {
				int off = ip->op == OP_JMP ? (int)ip->data - 1
							: ((JmpPopInfo *)ip->data)->off - 1;

				while (1) {
					if (trylist == NULL) break;
					OpCode *tojmp = ip + off;

					/* jmp out of a try block, should execute the finally block */
					/* while jmp out a 'with' block, restore the scope */

					if (trylist->type == TL_TRY) { 
						if (tojmp >= trylist->d.td.tstart && tojmp < trylist->d.td.fend) break;
						
						if (ip >= trylist->d.td.tstart && ip < trylist->d.td.cend) {
							trylist->d.td.last_op = LOP_JMP;
							trylist->d.td.ld.tojmp = tojmp;
							
							ip = trylist->d.td.fstart - 1;
							off = 0;
							break;
						} else if (ip >= trylist->d.td.fstart && ip < trylist->d.td.fend) {
							pop_try(trylist);
						} else bug("jmp within a try, but not in its scope?");
					} else {
						/* with block */
						
						if (tojmp >= trylist->d.wd.wstart && tojmp < trylist->d.wd.wend) break;
						
						restore_scope(trylist->scope_save, trylist->curscope_save);
						pop_try(trylist);
					}
				}
				
				ip += off;
				break;
			}
			case OP_FCALL: 
			case OP_NEWFCALL: {
				int as_constructor = (ip->op == OP_NEWFCALL);
				int stackargc = (int)ip->data;
				topevaln(stackargc + 1);

				int tocall_index = sp - stackargc - 1;
				Value *tocall = &stack[tocall_index];
				if (tocall->vt != VT_OBJECT || tocall->d.obj->ot != OT_FUNCTION) {
					die("can not execute expression, expression is not a function\n");
				}

				if (!tocall->d.obj->d.fobj) {	/* empty function */
					pop_n(stackargc);
					value_erase(TOP);
					value_make_undef(TOP);
					break;
				}
				Func *fstatic = tocall->d.obj->d.fobj->func;

				/* create new scope, prepare arguments */
				/* here we shared scope and 'arguments' with the same object */
				/* so that arguments[0] is easier to shared space with first local variable */
				
				Value *newscope = value_new();
				value_make_object(*newscope, object_make_array(&stack[sp - stackargc], stackargc));
				newscope->d.obj->__proto__ = Object_prototype;			/* ecma */
				
				fcall_shared_arguments(newscope, fstatic->argnames);	/* make arg vars to shared arguments */
				func_init_localvar(newscope, fstatic);					/* init local vars */
				fcall_set_callee(newscope, tocall);
				
				pop_n(stackargc);

				Value newthis = {0};
				if (obj_this[tocall_index].vt == VT_OBJECT) {
					value_copy(newthis, obj_this[tocall_index]);
					value_erase(obj_this[tocall_index]);
				} else {
					value_copy(newthis, *Top_object);
				}
				
				if (as_constructor) {						/* new Constructor */
					Object *newobj = object_new();
					newobj->__proto__ = Object_prototype;
					
					Value *proto = value_object_lookup(tocall, (ObjKey *)PROTOTYPE.unistr, NULL);
					if (proto && proto->vt == VT_OBJECT) newobj->__proto__ = proto;

					value_erase(newthis);
					value_make_object(newthis, newobj);
					
					/* todo, constructor */
				}
				
				Value spret = {0};
				int excpt_ret = 0;
				if (fstatic->type == FC_NORMAL) {
					excpt_ret = eval(ps, fstatic->exec.opcodes, tocall->d.obj->d.fobj->scope, 
						newscope, &newthis, &spret);
				} else {
					excpt_ret = fstatic->exec.callback(ps, newscope, &newthis, &spret, as_constructor);
				}
				
				if (as_constructor) {
					if (spret.vt != VT_OBJECT) {
						value_erase(spret);
						value_copy(spret, newthis);
					}
				}
				
				value_erase(newthis);
				value_erase(TOP);
				TOP = spret;
				
				value_free(newscope);
				
				if (excpt_ret) {		/* throw an execption */
					do_throw();
				}
				/* todo: new Function return a function without scopechain, add here */
				break;
			}
			case OP_EVAL: {
				int stackargc = (int)ip->data;
				topevaln(stackargc);

				Value spret = {0};
				int r = 0;
				if (stackargc > 0) {
					if (stack[sp - stackargc].vt == VT_UNDEF) die("eval undefined value\n");
					if (stack[sp - stackargc].vt == VT_STRING) {
						char *pro = c_strdup(tochars(stack[sp - stackargc].d.str));
						r = utils_global_eval(ps, pro, scope, currentScope, _this, &spret);
						c_strfree(pro);
					} else {
						value_copy(spret, stack[sp - stackargc]);
					}
				}
				pop_n(stackargc);
				stack[sp] = spret;
				sp++;

				if (r) {
					do_throw();
				}
				break;
			}
			case OP_RET: {
				topeval1();
				value_copy(*vret, TOP);
				pop_n(ip->data);
				return 0;
			}
			case OP_DELETE: {
				int count = (int)ip->data;
				if (count == 1) {
					if (TOP.vt != VT_VARIABLE) die("delete a right value\n");
					Value *v = TOP.d.lval;
					if (v != currentScope) value_erase(*v);		/* not allow to delete arguments */
					else warn("Delete arguments\n");
					pop();
				} else if (count == 2) {
					topeval2();
					if (TOQ.vt != VT_OBJECT) warn("delete non-object key, ignore\n");
					if (TOQ.d.obj == currentScope->d.obj) warn("Delete arguments\n");
					
					value_object_delete(&TOQ, &TOP);
					
					pop(); pop();
				} else bug("delete");
				break;
			}
			case OP_OBJECT: {
				int itemcount = (int)ip->data;
				topevaln(itemcount * 2);
				Object *obj = object_make(&stack[sp-itemcount*2], itemcount*2);
				pop_n(itemcount * 2 - 1);		/* one left */
				value_erase(TOP);
				value_make_object(TOP, obj);
				break;
			}
			case OP_ARRAY: {
				int itemcount = (int)ip->data;
				topevaln(itemcount);
				Object *obj = object_make_array(&stack[sp-itemcount], itemcount);
				pop_n(itemcount - 1);
				value_erase(TOP);
				value_make_object(TOP, obj);
				break;
			}
			case OP_STRY: {
				TryInfo *ti = (TryInfo *)ip->data;
				TryList *n = trylist_new(TL_TRY, scope, currentScope);
				
				n->d.td.tstart = ip;							/* make every thing pointed to right pos */
				n->d.td.tend = n->d.td.tstart + ti->trylen;
				n->d.td.cstart = n->d.td.tend + 1;
				n->d.td.cend = n->d.td.tend + ti->catchlen;
				n->d.td.fstart = n->d.td.cend + 1;
				n->d.td.fend = n->d.td.cend + ti->finallen;
				n->d.td.sp = sp;

				push_try(trylist, n);
				break;
			}
			case OP_ETRY: {				/* means nothing happen go to final */
				if (trylist == NULL || trylist->type != TL_TRY)
					bug("Unexpected ETRY opcode??");

				ip = trylist->d.td.fstart - 1;
				break;
			}
			case OP_SCATCH: {
				if (trylist == NULL || trylist->type != TL_TRY)
					bug("Unexpected SCATCH opcode??");

				if (!ip->data) {
					do_throw();
				} else {
					/* new scope and make var */
					scope = scope_chain_dup_next(scope, currentScope);
					currentScope = value_object_utils_new_object();
					Value *excpt = value_new();
					value_copy(*excpt, ps->last_exception);
					value_erase(ps->last_exception);
					
					value_object_utils_insert(currentScope, ip->data, excpt, 1, 1, 0);

					context_id = ps->_context_id++;
				}
				break;
			}
			case OP_ECATCH: {
				if (trylist == NULL || trylist->type != TL_TRY)
					bug("Unexpected ECATCH opcode??");

				ip = trylist->d.td.fstart - 1;
				break;
			}
			case OP_SFINAL: {
				if (trylist == NULL || trylist->type != TL_TRY)
					bug("Unexpected SFINAL opcode??");

				/* restore scatch scope chain */
				restore_scope(trylist->scope_save, trylist->curscope_save);
				break;
			}
			case OP_EFINAL: {
				if (trylist == NULL || trylist->type != TL_TRY)
					bug("Unexpected EFINAL opcode??");

				int last_op = trylist->d.td.last_op;
				OpCode *tojmp = (last_op == LOP_JMP ? trylist->d.td.ld.tojmp : 0);
				
				pop_try(trylist);

				if (last_op == LOP_THROW) {
					do_throw();
				} else if (last_op == LOP_JMP) {
					while (1) {
						if (trylist == NULL) {
							ip = tojmp;
							break;
						}
						/* same as jmp opcode, see above */
						if (trylist->type == TL_TRY) {
							if (tojmp >= trylist->d.td.tstart && tojmp < trylist->d.td.fend) {
								ip = tojmp;
								break;
							}
							
							if (ip >= trylist->d.td.tstart && ip < trylist->d.td.cend) {
								trylist->d.td.last_op = LOP_JMP;
								trylist->d.td.ld.tojmp = tojmp;
								
								ip = trylist->d.td.fstart - 1;
								break;
							} else if (ip >= trylist->d.td.fstart && ip < trylist->d.td.fend) {
								pop_try(trylist);
							} else bug("jmp within a try, but not in its scope?");
						} else {		/* 'with' block */
							if (tojmp >= trylist->d.wd.wstart && tojmp < trylist->d.wd.wend) {
								ip = tojmp;
								break;
							}
							restore_scope(trylist->scope_save, trylist->curscope_save);
							pop_try(trylist);
						}
					}
				}
				break;
			}
			case OP_THROW: {
				topeval1();
				value_copy(ps->last_exception, TOP);

				do_throw();
				break;
			}
			case OP_WITH: {
				topeval1();
				value_toobject(&TOP);
				
				TryList *n = trylist_new(TL_WITH, scope, currentScope);
				
				n->d.wd.wstart = ip;
				n->d.wd.wend = n->d.wd.wstart + (int)ip->data;

				push_try(trylist, n);
				
				/* make expr to top of scope chain */
				scope = scope_chain_dup_next(scope, currentScope);
				currentScope = value_new();
				value_copy(*currentScope, TOP);
				pop();
				
				context_id = ps->_context_id++;
				break;
			}
			case OP_EWITH: {
				if (trylist == NULL || trylist->type != TL_WITH)
					bug("Unexpected EWITH opcode??");

				restore_scope(trylist->scope_save, trylist->curscope_save);
				
				pop_try(trylist);
				break;
			}
			case OP_DEBUG: {
				topeval1();
				if (TOP.vt == VT_OBJECT) {
					printf("R%d:", TOP.d.obj->__refcnt);
				}
				value_tostring(&TOP);
				printf("%s\n", tochars(TOP.d.str));
				break;
			}
			case OP_RESERVED: {
				ReservedInfo *ri = ip->data;
				const char *cmd = ri->type == RES_CONTINUE ? "continue" : "break";

				if (ri->label) {
					die("%s: label(%s) not found\n", cmd, tochars(ri->label));
				} else {
					die("%s must be inside loop(or switch)\n", cmd);
				}
				break;
			}
		}
		ip++;
	}
	return 0;
}

