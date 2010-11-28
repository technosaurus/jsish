#ifndef __FUNC_H__
#define __FUNC_H__

#include "scope.h"

struct Value;
struct OpCodes;
struct PSTATE;

/* System func callback type */
typedef int (*SSFunc)(struct PSTATE *ps, struct Value *args, 
					  struct Value *_this, struct Value *ret, int asconstructor);

/* raw function data, with script function or system SSFunc */
typedef struct Func {
	enum {
		FC_NORMAL,
		FC_BUILDIN
	} type;							/* type */
	union {
		struct OpCodes *opcodes;	/* FC_NORMAL, codes of this function */
		SSFunc callback;			/* FC_BUILDIN, callback */
	} exec;
	strs *argnames;					/* FC_NORMAL, argument names */
	strs *localnames;				/* FC_NORMAL, local var names */
} Func;

Func *func_make_static(strs *args, strs *localvar, struct OpCodes *ops);

/* Make a function value from SSFunc */
struct Value *func_utils_make_func_value(SSFunc callback);
void func_init_localvar(struct Value *arguments, Func *who);

#endif
