#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "pstate.h"
#include "parser.h"
#include "regexp.h"
#include "code.h"
#include "value.h"
#include "eval.h"
#include "func.h"
#include "utils.h"
#include "proto.h"
#include "filesys.ex.h"
#include "error.h"
#include "mempool.h"

extern int yyparse(PSTATE *ps);

int Usage()
{
	fprintf(stderr, "Usage: smallscript [input file] [arguments]\n");
	return -1;
}

int main(int argc, char **argv)
{
	FILE *input = stdin;
	
	argv++;
	argc--;
	
	if (argc > 0) {
		input = fopen(argv[0], "r");
		if (!input) {
			fprintf(stderr, "Can not open '%s'\n", argv[0]);
			return Usage();
		}
		argv++;
		argc--;
	}

	/* subsystem init */
	mpool_init();			/* general mempool */
	objects_init();
	
	PSTATE *ps = pstate_new_from_file(input);
	yyparse(ps);
	
	if (!ps->err_count) {
		Value ret;

		/* current scope, also global */
		Value *csc = value_new();
		value_make_object(*csc, object_new());

		/* top this and prototype chain */
		proto_init(csc);

		/* global funtion, debugger, etc */
		utils_init(csc, argc, argv);

		/* file system extern init */
		filesys_init(csc);

		/* initial scope chain, nothing */
		ScopeChain *gsc = scope_chain_new(0);
		
#ifdef DEBUG
		codes_print(ps->opcodes);
		printf("------------------------\n");
#endif
		if (eval(ps, ps->opcodes, gsc, csc, csc, &ret)) {
			die("Uncatched error");
		} else {
			extern int sp;
			if (sp != 0) {
				bug("Stack not ballence after execute script\n");
			}
		}
		scope_chain_free(gsc);
		value_free(csc);
	}
	fclose(input);
	pstate_free(ps);
	return 0;
}

