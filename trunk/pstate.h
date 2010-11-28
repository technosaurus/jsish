#ifndef __PSTAT_H__
#define __PSTAT_H__

#include <stdio.h>
#include "code.h"
#include "value.h"

/* Program state(context) */
typedef struct PSTATE {
	int err_count;				/* error count after parse */
	int eval_flag;				/* currently execute in eval function */
 	struct OpCodes *opcodes;	/* opcodes result(parsing result) */
	struct Lexer *lexer;		/* seq provider */

	int _context_id;			/* used in FastVar-locating */
	Value last_exception;		/* exception */
} PSTATE;

PSTATE *pstate_new_from_file(FILE *fp);
PSTATE *pstate_new_from_string(const char *str);
void pstate_free(PSTATE *ps);

#endif
