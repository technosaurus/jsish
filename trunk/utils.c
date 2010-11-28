#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "pstate.h"
#include "error.h"
#include "value.h"
#include "func.h"
#include "proto.h"
#include "eval.h"

static void print_value(Value *v, int quote);
static int _object_print_callback(void *key, void *value, void *userdata)
{
	ObjKey *ok = key;
	Value *v = value;
	int *first = userdata;
	int flag = KEYFLAG(ok);
	if (bits_get(flag, OM_DONTEMU)) return 0;
	
	if (*first) {
		*first = 0;
	} else printf(", ");
	
	printf("%s:", tochars((unichar *)ok));

	print_value(v, 1);
	
	return 0;
}

static void print_value(Value *v, int quote)
{
	switch(v->vt) {
		case VT_UNDEF:
			printf("undefined");
			break;
		case VT_NULL:
			printf("null");
			break;
		case VT_BOOL:
			printf("%s", v->d.val ? "true":"false");
			break;
		case VT_NUMBER:
			if (is_integer(v->d.num)) {
				printf("%d", (int)v->d.num);
			} else if (ieee_isnormal(v->d.num)) {
				printf("%g", v->d.num);
			} else if (ieee_isnan(v->d.num)) {
				printf("NaN");
			} else {
				int s = ieee_infinity(v->d.num);
				if (s > 0) printf("+Infinity");
				else if (s < 0) printf("-Infinity");
				else bug("Ieee function got problem");
			}
			break;
		case VT_STRING:
			if (quote) printf("\"%s\"", tochars(v->d.str));
			else printf("%s", tochars(v->d.str));
			break;
		case VT_OBJECT: {
			Object *o = v->d.obj;
			switch(o->ot) {
				case OT_BOOL:
					printf("%s ", o->d.val ? "true":"false");
					break;
				case OT_NUMBER:
					if (is_integer(o->d.num)) {
						printf("%d ", (int)o->d.num);
					} else {
						printf("%g ", o->d.num);
					}
					break;
				case OT_STRING:
					printf("%s", tochars(o->d.str));
					break;
				case OT_FUNCTION: {
					Func *f = o->d.fobj->func;
					if (f->type == FC_NORMAL) {
						printf("function (");
						int i;
						for (i = 0; i < f->argnames->count; ++i) {
							printf("%s ", tochars(strs_get(f->argnames, i)));
						}
						printf(") {\n");
						codes_print(f->exec.opcodes);
						printf("}");
					} else {
						printf("function () { [Native code] }");
					}
					break;
				}
				case OT_REGEXP:
					printf("/regex/ ");
					break;
				case OT_USERDEF:
					printf("#UserData%d ", o->d.uobj->id);
					break;
				default:
					break;
			}
			
			int len = object_get_length(o);
			
			if (len > 0) {
				int i;
				Value *nv;
				printf("[ ");

				if (len > 0) {
					nv = value_object_lookup_array(v, 0, NULL);
					if (nv) print_value(nv, 1);
					else printf("undefined");
				}
				for (i = 1; i < len; ++i) {
					nv = value_object_lookup_array(v, i, NULL);
					printf(", ");
					if (nv) print_value(nv, 1);
					else printf("undefined");
				}
				printf(" ]");
			} else {
				int first = 1;
				printf("{ ");
				rbtree_walk(o->tree, &first, _object_print_callback);
				printf(" }");
			}
			break;
		}
		default:
			bug("Unexpected value type: %d\n", v->vt);
	}
}

static int console_input(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	char buf[1024];
	char *p = buf;
	if (asc) die("Can not call console.input as a constructor\n");
	
	if (fgets(buf, 1024, stdin) == NULL) {
		value_make_undef(*ret);
		return 0;
	}
	if ((p = strchr(buf, '\r'))) *p = 0;
	if ((p = strchr(buf, '\n'))) *p = 0;
	
	value_make_string(*ret, unistrdup(tounichars(buf)));
	return 0;
}

static int global_exit(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	int err = 0;
	if (asc) die("Can not call exit as a constructor\n");
	if (value_get_length(args) > 0) {
		Value *v = value_object_lookup_array(args, 0, NULL);
		if (v && is_number(v)) err = (int)v->d.num;
	}
	exit(err);
	return 0;
}

extern int yyparse(PSTATE *ps);

/* eval here is diff from SSFunc, current scope info should be past to eval */
/* make evaling script execute in the same context */
int utils_global_eval(PSTATE *ps, const char *program,
					   ScopeChain *scope, Value *currentScope, Value *_this, Value *ret)
{
	PSTATE *newps = pstate_new_from_string(program);
	newps->eval_flag = 1;
	
	yyparse(newps);
	
	if (!newps->err_count) {
		int r = eval(newps, newps->opcodes, scope, currentScope, _this, ret);
		if (r) {
			value_copy(ps->last_exception, newps->last_exception);
		}
		pstate_free(newps);
		return r;
	} else {
		/* todo, parse error */
		value_make_string(ps->last_exception, unistrdup_str("Syntax error"));
		return -1;
	}
}

/* here demo how to build console.log */
/* first: define console.log function */
static int console_log(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Can not call console.log as a constructor\n");
	int argc = value_get_length(args);
	int i;
	for (i = 0; i < argc; ++i) {
		Value *v = value_object_lookup_array(args, i, NULL);
		if (v) print_value(v, 0);
	}
	printf("\n");
	return 0;
}

Value *init_program_args(int argc, char **argv)
{
	Value *ret = value_new();
	Object *obj = object_new();
	obj->__proto__ = Array_prototype;
	value_make_object(*ret, obj);
	object_set_length(obj, 0);
	
	int i;
	for (i = 0; i < argc; ++i) {
		Value *val = value_new();
		value_make_string(*val, unistrdup_str(argv[i]));
		value_object_utils_insert_array(ret, i, val, 1, 1, 1);
	}
	return ret;
}

void utils_init(Value *global, int argc, char **argv)
{
	/* second, build console object */
	Value *console = value_object_utils_new_object();
	/* no __proto__, console is not an Object */
	
	/* third, make console.log object */
	Value *conlog = func_utils_make_func_value(console_log);
	conlog->d.obj->__proto__ = Function_prototype;

	/* forth, insert console.log value into console object */
	value_object_utils_insert(console, tounichars("log"), conlog, 1, 1, 0);
	value_object_utils_insert(console, tounichars("print"), value_dup(conlog), 1, 1, 0);
	value_object_utils_insert(console, tounichars("output"), value_dup(conlog), 1, 1, 0);
	value_object_utils_insert(console, tounichars("input"), 
							  func_utils_make_func_value(console_input),
							  1, 1, 0);
	value_object_utils_insert(console, tounichars("args"),
							  init_program_args(argc, argv), 1, 1, 0);
	
	/* last, insert console to global naming space */
	value_object_utils_insert(global, tounichars("console"), console, 0, 0, 0);
	value_object_utils_insert(global, tounichars("exit"),
							  func_utils_make_func_value(global_exit),
							  0, 0, 0);
}
