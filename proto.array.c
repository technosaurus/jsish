#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstate.h"
#include "error.h"
#include "value.h"
#include "proto.h"

/* push */
static int arrpto_push(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute Array.prototype.push as constructor\n");

	if (_this->vt != VT_OBJECT || !obj_isarray(_this->d.obj)) {
		value_make_number(*ret, 0);
		return 0;
	}

	int argc = value_get_length(args);
	int curlen = object_get_length(_this->d.obj);
	if (curlen < 0) {
		object_set_length(_this->d.obj, 0);
	}
	
	int i;
	for (i = 0; i < argc; ++i) {
		Value *v = value_new();
		Value *ov = value_object_lookup_array(args, i, NULL);
		if (!ov) bug("Arguments error\n");

		value_copy(*v, *ov);

		value_object_utils_insert_array(_this, curlen + i, v, 1, 1, 1);
	}
	
	value_make_number(*ret, object_get_length(_this->d.obj));
	return 0;
}

/* pop */
static int arrpto_pop(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute Array.prototype.pop as constructor\n");

	if (_this->vt != VT_OBJECT || !obj_isarray(_this->d.obj)) {
		value_make_number(*ret, 0);
		return 0;
	}

	int i = object_get_length(_this->d.obj) - 1;

	if (i >= 0) {
		Value *v = value_object_lookup_array(_this, i, NULL);
		if (v) {
			value_copy(*ret, *v);
			value_erase(*v);	/* diff from ecma, not actually delete the key */
		}
		object_set_length(_this->d.obj, i);
		return 0;
	}
	
	value_make_undef(*ret);
	return 0;
}

static struct st_arrpro_tab {
	const char *name;
	SSFunc func;
} arrpro_funcs[] = {
	{ "push", arrpto_push },
	{ "pop", arrpto_pop }
};

void proto_array_init(Value *global)
{
	if (!Array_prototype) bug("proto init failed?");
	int i;
	for (i = 0; i < sizeof(arrpro_funcs) / sizeof(struct st_arrpro_tab); ++i) {
		Value *n = func_utils_make_func_value(arrpro_funcs[i].func);
		n->d.obj->__proto__ = Function_prototype;
		value_object_utils_insert(Array_prototype, tounichars(arrpro_funcs[i].name), n, 0, 0, 0);
	}
}

