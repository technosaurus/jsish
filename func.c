#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "func.h"
#include "error.h"
#include "value.h"
#include "scope.h"
#include "unichar.h"

Func *func_make_static(strs *args, strs *localvar, struct OpCodes *ops)
{
	Func *f = malloc(sizeof(Func));
	memset(f, 0, sizeof(Func));
	f->type = FC_NORMAL;
	f->exec.opcodes = ops;
	f->argnames = args;
	f->localnames = localvar;
	return f;
}

void func_init_localvar(Value *arguments, Func *who)
{
	if (who->localnames) {
		int i;
		for (i = 0; i < who->localnames->count; ++i) {
			const unichar *argkey = strs_get(who->localnames, i);
			if (argkey) {
				ObjKey *strkey = objkey_new(argkey, OM_DONTEMU);
				value_object_insert(arguments, strkey, value_new());
			}
		}
	}
}

static FuncObj *func_make_internal(SSFunc callback)
{
	Func *f = malloc(sizeof(Func));
	memset(f, 0, sizeof(Func));
	f->type = FC_BUILDIN;
	f->exec.callback = callback;
	
	return funcobj_new(f);
}

Value *func_utils_make_func_value(SSFunc callback)
{
	Object *o = object_new();
	o->ot = OT_FUNCTION;
	o->d.fobj = func_make_internal(callback);

	Value *v = value_new();
	value_make_object(*v, o);
	return v;
}

