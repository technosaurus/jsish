#ifndef __CODE_H__
#define __CODE_H__

#include <regex.h>

#include "unichar.h"

/* stack change */
/* 0  nothing change */
/* +1 push */
/* -1 pop */
typedef enum {		/* SC 	type of data	comment 							*/
	OP_NOP,			/* 0 */
	OP_PUSHNUM,		/* +1 	*double			number 								*/
	OP_PUSHSTR,		/* +1 	*unichar		string 								*/
	OP_PUSHVAR,		/* +1 	*FastVar		variable name	 					*/
	OP_PUSHUND,		/* +1 	-				undefined 							*/
	OP_PUSHBOO,		/* +1 	int				bool 								*/
	OP_PUSHFUN,		/* +1 	*Func			function 							*/
	OP_PUSHREG,		/* +1 	*regex_t		regex 								*/
	OP_PUSHARG,		/* +1	-				push arguments(cur scope)			*/
	OP_PUSHTHS,		/* +1 	-				push this 							*/
	OP_PUSHTOP,		/* +1 	-				duplicate top 						*/
	OP_PUSHTOP2,	/* +2	-				duplicate toq and top				*/
	OP_UNREF,		/* 0	-				make top be right value				*/
	OP_POP,			/* -n 	int				pop n elements 						*/
	OP_LOCAL,		/* 0  	*unichar		add a var to current scope 			*/
	OP_NEG,			/* 0 	-				make top = - top 					*/
	OP_POS,			/* 0	-				make top = + top, (conv to number)	*/
	OP_NOT,			/* 0	-				reserve top 						*/
	OP_BNOT,		/* 0	-				bitwise not							*/
	OP_ADD,			/* -1	-				all math opr pop 2 elem from stack,	*/
	OP_SUB,			/* -1	-				 calc and push back in to the stack */
	OP_MUL,			/* -1	-													*/
	OP_DIV,			/* -1	-													*/
	OP_MOD,			/* -1	-													*/
	OP_LESS,		/* -1	-				logical opr, same as math opr 		*/
	OP_GREATER,		/* -1	-													*/
	OP_LESSEQU,		/* -1	-													*/
	OP_GREATEREQU,	/* -1	-													*/
	OP_EQUAL,		/* -1	-													*/
	OP_NOTEQUAL,	/* -1	-													*/
	OP_STRICTEQU,	/* -1	-													*/
	OP_STRICTNEQ,	/* -1	-													*/
	OP_BAND,		/* -1	-				bitwise and							*/
	OP_BOR,			/* -1	-				bitwise or							*/
	OP_BXOR,		/* -1	-				bitwise xor							*/
	OP_SHF,			/* -1	int(right)		signed shift left or shift right	*/
	
	OP_ASSIGN,		/* -n	int				if n = 1, assign to lval,			*/
					/*						n = 2, assign to object member 		*/
	OP_SUBSCRIPT,	/* -1	-				do subscript TOQ[TOP]				*/
	OP_INC,			/* 0	int				data indicate prefix/postfix inc/dec 				*/
	OP_DEC,			/* 0	int 																*/
	OP_KEY,			/* +1	-				push an iter object that contain all key in top 	*/
	OP_NEXT,		/* -1	-				assign next key to top, make top be res of this opr */
	OP_JTRUE,		/* -1	int				jmp to offset if top is true, 						*/
	OP_JFALSE,		/* -1	int				jmp to offset if top is false,						*/
	OP_JTRUE_NP,	/* 0	int				jtrue no pop version 								*/
	OP_JFALSE_NP,	/* 0	int				jfalse no pop version 								*/
	OP_JMP,			/* 0	int				jmp to offset 										*/
	OP_JMPPOP,		/* -n	*JmpPopInfo		jmp to offset with pop n 							*/
	OP_FCALL,		/* -n+1	int				call func with n args, pop then, make ret to be top */
	OP_NEWFCALL,	/* -n+1	int				same as fcall, call as a constructor 				*/
	OP_RET,			/* -n	int				n = 0|1, return with arg 							*/
	OP_DELETE,		/* -n 	int				n = 1, delete var, n = 2, delete object member 		*/
	OP_CHTHIS,		/* 0,	-				make toq as new 'this'								*/
	OP_OBJECT,		/* -n*2+1	int			create object from stack, and push back in 			*/
	OP_ARRAY,		/* -n+1	int				create array object from stack, and push back in 	*/
	OP_EVAL,		/* -n+1	int				eval can not be assign to other var 				*/
	OP_STRY,		/* 0	*TryInfo		push try statment poses info to trylist 			*/
	OP_ETRY,		/* 0	-				end of try block, jmp to finally 					*/
	OP_SCATCH,		/* 0	*unichar		create new scope, assign to current excption 		*/
	OP_ECATCH,		/* 0	-				jmp to finally 										*/
	OP_SFINAL,		/* 0	-				restore scope chain create by Scatch 				*/
	OP_EFINAL,		/* 0	-				end of finally, any unfinish code in catch, do it 	*/
	OP_THROW,		/* 0	-				make top be last exception, pop trylist till catched*/
	OP_WITH,		/* -1	-				make top be top of scopechain, add to trylist 		*/
	OP_EWITH,		/* 0	-				pop trylist 										*/
	OP_RESERVED,	/* 0	ReservedInfo*	reserved, be replaced by iterstat by jmp/jmppop 	*/
	OP_DEBUG,		/* 0	-				DEBUG OPCODE, output top 							*/
	OP_LASTOP		/* 0	-				END OF OPCODE 										*/
} Eopcode;

#define RES_CONTINUE	1
#define RES_BREAK		2

extern const char *op_names[OP_LASTOP];

typedef struct {
	Eopcode op;
	void *data;
} OpCode;

typedef struct OpCodes {
	OpCode *codes;
	int code_len;
	int code_size;
	
	int expr_counter;			/* context related expr count */
	int lvalue_flag;			/* left value count/flag */
	const unichar *lvalue_name;	/* left value name */
} OpCodes;

struct Func;
struct Value;

typedef struct FastVar {
	int context_id;
	struct {
		unichar *varname;
		struct Value *lval;
	} var;
} FastVar;

typedef struct TryInfo {
	int trylen;
	int catchlen;
	int finallen;
} TryInfo;

typedef struct ReservedInfo {
	int type;
	const unichar *label;
	int topop;
} ReservedInfo;

typedef struct JmpPopInfo {
	int off;
	int topop;
} JmpPopInfo;

OpCodes *code_push_undef();
OpCodes *code_push_bool(int v);
OpCodes *code_push_num(double *v);
OpCodes *code_push_string(const unichar *str);
OpCodes *code_push_index(unichar *varname);
OpCodes *code_push_args();
OpCodes *code_push_this();
OpCodes *code_push_func(struct Func *fun);
OpCodes *code_push_regex(regex_t *reg);
OpCodes *code_push_top();
OpCodes *code_push_top2();
OpCodes *code_unref();
OpCodes *code_local(const unichar *varname);

OpCodes *code_nop();
OpCodes *code_neg();
OpCodes *code_pos();
OpCodes *code_bnot();
OpCodes *code_not();
OpCodes *code_mul();
OpCodes *code_div();
OpCodes *code_mod();
OpCodes *code_add();
OpCodes *code_sub();
OpCodes *code_less();
OpCodes *code_greater();
OpCodes *code_lessequ();
OpCodes *code_greaterequ();
OpCodes *code_equal(); 
OpCodes *code_notequal();
OpCodes *code_eequ();
OpCodes *code_nneq();
OpCodes *code_band();
OpCodes *code_bor();
OpCodes *code_bxor();
OpCodes *code_shf(int right);

OpCodes *code_assign(int h);
OpCodes *code_subscript(int right_val);
OpCodes *code_inc(int e);
OpCodes *code_dec(int e);

OpCodes *code_fcall(int argc);
OpCodes *code_newfcall(int argc);
OpCodes *code_pop(int n);
OpCodes *code_ret(int n);
OpCodes *code_object(int c);
OpCodes *code_array(int c);
OpCodes *code_key();
OpCodes *code_next();
OpCodes *code_delete(int n);
OpCodes *code_chthis(int n);

OpCodes *code_jfalse(int off);
OpCodes *code_jtrue(int off);
OpCodes *code_jfalse_np(int off);
OpCodes *code_jtrue_np(int off);
OpCodes *code_jmp(int off);
OpCodes *code_eval(int argc);

OpCodes *code_throw();
OpCodes *code_stry(int trylen, int catchlen, int finlen);
OpCodes *code_etry();
OpCodes *code_scatch(const unichar *var);
OpCodes *code_ecatch();
OpCodes *code_sfinal();
OpCodes *code_efinal();
OpCodes *code_throw();
OpCodes *code_with(int withlen);
OpCodes *code_ewith();

OpCodes *code_debug();
OpCodes *code_reserved(int type, unichar *id);

OpCodes *codes_join(OpCodes *a, OpCodes *b);
OpCodes *codes_join3(OpCodes *a, OpCodes *b, OpCodes *c);
OpCodes *codes_join4(OpCodes *a, OpCodes *b, OpCodes *c, OpCodes *d);

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
 * 1. break_only used only in swith
 * 2. desire_label, only replace if current iter statement has the same label with opcode
 * 3. topop, if not replace in current iter statment, make sure when jmp out of this loop/switch
 *	  corrent stack elems poped(for in always has 2 elem, while switch has 1)
 */
void code_reserved_replace(OpCodes *ops, int step_len, int break_only,
						   const unichar *desire_label, int topop);

void code_decode(OpCode *op, int currentip);
void codes_print(OpCodes *ops);
OpCodes *codes_new(int size);
void codes_free(OpCodes *ops);
#endif
