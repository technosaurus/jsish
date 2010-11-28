#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstate.h"
#include "error.h"
#include "value.h"
#include "proto.h"
#include "regexp.h"
#include "eval.h"

#include "proto.string.h"
#include "proto.number.h"
#include "proto.array.h"

Value *Object_prototype;
static Value *Function_prototype_prototype;
Value *Function_prototype;
Value *String_prototype;
Value *Number_prototype;
Value *Boolean_prototype;
Value *Array_prototype;
Value *RegExp_prototype;
Value *Top_object;

/* Object constructor */
static int Object_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) {
		/* new oprator will do the rest */
		return 0;
	}
	
	if (value_get_length(args) <= 0) {
		Object *o = object_new();
		o->__proto__ = Object_prototype;
		value_make_object(*ret, o);
		return 0;
	}
	Value *v = value_object_lookup_array(args, 0, NULL);
	if (!v || v->vt == VT_UNDEF || v->vt == VT_NULL) {
		Object *o = object_new();
		o->__proto__ = Object_prototype;
		value_make_object(*ret, o);
		return 0;
	}
	value_copy(*ret, *v);
	value_toobject(ret);
	return 0;
}

/* Function.prototype pointed to a empty function */
static int Function_prototype_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	return 0;
}

static int Function_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) {		/* todo, parse the argument, return the new function obj */
		_this->d.obj->ot = OT_FUNCTION;
		return 0;
	}
	Object *o = object_new();
	o->ot = OT_FUNCTION;
	o->__proto__ = Function_prototype;
	value_make_object(*ret, o);
	return 0;
}

/* delete array[0], array[1]->array[0] */
static void value_array_shift(Value *v)
{
	if (v->vt != VT_OBJECT) bug("value_array_shift, target is not object\n");
	
	int len = object_get_length(v->d.obj);
	if (len <= 0) return;
	
	Value *v0 = value_object_lookup_array(v, 0, NULL);
	if (!v0) return;
	
	value_erase(*v0);
	
	int i;
	Value *last = v0;
	for (i = 1; i < len; ++i) {
		Value *t = value_object_lookup_array(v, i, NULL);
		if (!t) return;
		value_copy(*last, *t);
		value_erase(*t);
		last = t;
	}
	
	object_set_length(v->d.obj, len - 1);
}

void fcall_shared_arguments(Value *args, strs *argnames)
{
	int i;
	if (!argnames) return;
	
	for (i = 0; i < argnames->count; ++i) {
		const unichar *argkey = strs_get(argnames, i);
		if (!argkey) break;
		
		Value *v = value_object_lookup_array(args, i, NULL);
		if (v) {
			ObjKey *strkey = objkey_new(argkey, OM_DONTEMU | OM_INNERSHARED);
			value_object_insert(args, strkey, v);
		} else {
			ObjKey *strkey = objkey_new(argkey, OM_DONTEMU);
			value_object_insert(args, strkey, value_new());
		}
	}
}

static UNISTR(8) _CALLEE_ = { 8, { 1, 'c', 'a', 'l', 'l', 'e', 'e', 1 } };
static UNISTR(0) EMPTY = { 0, { 0 } };

void fcall_set_callee(Value *args, Value *tocall)
{
	Value *callee = value_new();
	value_copy(*callee, *tocall);
	value_object_utils_insert(args, _CALLEE_.unistr, callee, 0, 0, 0);
}


/* Function.prototype.call */
static int Function_prototype_call(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute call as constructor\n");
	
	Value *tocall = _this;
	if (tocall->vt != VT_OBJECT || tocall->d.obj->ot != OT_FUNCTION) {
		die("can not execute expression, expression is not a function\n");
	}

	if (!tocall->d.obj->d.fobj) {	/* empty function */
		return 0;
	}
	
	/* func to call */
	Func *fstatic = tocall->d.obj->d.fobj->func;
	
	/* new this */
	Value newthis = {0};
	Value *arg1 = NULL;
	if ((arg1 = value_object_lookup_array(args, 0, NULL))) {
		value_copy(newthis, *arg1);
		value_toobject(&newthis);
	} else {
		value_copy(newthis, *Top_object);
	}
	
	/* prepare args */
	value_array_shift(args);
	fcall_shared_arguments(args, fstatic->argnames);
	
	func_init_localvar(args, fstatic);
	fcall_set_callee(args, tocall);
	
	int res = 0;
	if (fstatic->type == FC_NORMAL) {
		res = eval(ps, fstatic->exec.opcodes, tocall->d.obj->d.fobj->scope, 
				   args, &newthis, ret);
	} else {
		res = fstatic->exec.callback(ps, args, &newthis, ret, 0);
	}

	value_erase(newthis);
	return res;
}

/* Function.prototype.apply */
static int Function_prototype_apply(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute apply as constructor\n");
	
	Value *tocall = _this;
	if (tocall->vt != VT_OBJECT || tocall->d.obj->ot != OT_FUNCTION) {
		die("can not execute expression, expression is not a function\n");
	}

	if (!tocall->d.obj->d.fobj) {	/* empty function */
		return 0;
	}
	
	/* func to call */
	Func *fstatic = tocall->d.obj->d.fobj->func;
	
	/* new this */
	Value newthis = {0};
	Value *arg1 = NULL;
	if ((arg1 = value_object_lookup_array(args, 0, NULL))) {
		value_copy(newthis, *arg1);
		value_toobject(&newthis);
	} else {
		value_copy(newthis, *Top_object);
	}
	
	/* prepare args */
	Value *newscope = value_object_lookup_array(args, 1, NULL);
	if (newscope) {
		if (newscope->vt != VT_OBJECT || !obj_isarray(newscope->d.obj)) {
			die("second argument to Function.prototype.apply must be an array\n");
		}
	} else {
		newscope = value_object_utils_new_object();
		object_set_length(newscope->d.obj, 0);
	}
	
	fcall_shared_arguments(newscope, fstatic->argnames);
	func_init_localvar(newscope, fstatic);
	fcall_set_callee(newscope, tocall);

	int res = 0;
	if (fstatic->type == FC_NORMAL) {
		res = eval(ps, fstatic->exec.opcodes, tocall->d.obj->d.fobj->scope, 
			newscope, &newthis, ret);
	} else {
		res = fstatic->exec.callback(ps, newscope, &newthis, ret, 0);
	}
	value_erase(newthis);
	return res;
}

static int String_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) {
		const unichar *nv = EMPTY.unistr;
		if (value_get_length(args) > 0) {
			Value *v = value_object_lookup_array(args, 0, NULL);
			if (v) {
				value_tostring(v);
				nv = v->d.str;
			}
		}
		_this->d.obj->ot = OT_STRING;
		_this->d.obj->d.str = unistrdup(nv);
		object_set_length(_this->d.obj, 0);
		
		int i;
		int len = unistrlen(nv);
		for (i = 0; i < len; ++i) {
			Value *v = value_new();
			value_make_string(*v, unisubstrdup(nv, i, 1));

			object_utils_insert_array(_this->d.obj, i, v, 0, 0, 1);
		}
		
		return 0;
	}
	if (value_get_length(args) > 0) {
		Value *v = value_object_lookup_array(args, 0, NULL);
		if (v) {
			value_copy(*ret, *v);
			value_tostring(ret);
			return 0;
		}
	}
	value_make_string(*ret, unistrdup(EMPTY.unistr));
	return 0;
}

static int String_fromCharCode(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	UNISTR(4096) unibuf;
	
	if (asc) die("Can not execute String.fromCharCode as a constructor\n");

	int len = value_get_length(args);
	int i;

	if (len > 4096) len = 4096;
	
	unibuf.len = len;
	unichar *u = unibuf.unistr;

	for (i = 0; i < len; ++i) {
		Value *v = value_object_lookup_array(args, i, NULL);
		if (!v) bug("Arguments error\n");

		value_tonumber(v);
		u[i] = (unichar) v->d.num;
	}
	value_make_string(*ret, unistrdup(unibuf.unistr));
	return 0;
}

static int Number_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) {
		double nv = 0.0;
		if (value_get_length(args) > 0) {
			Value *v = value_object_lookup_array(args, 0, NULL);
			if (v) {
				value_tonumber(v);
				nv = v->d.num;
			}
		}
		_this->d.obj->ot = OT_NUMBER;
		_this->d.obj->d.num = nv;
		return 0;
	}
	if (value_get_length(args) > 0) {
		Value *v = value_object_lookup_array(args, 0, NULL);
		if (v) {
			value_copy(*ret, *v);
			value_tonumber(ret);
			return 0;
		}
	}
	value_make_number(*ret, 0.0);
	return 0;
}

static int Array_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	int argc = value_get_length(args);
	Value *target;
	
	if (asc) target = _this;
	else {
		Object *o = object_new();
		o->__proto__ = Array_prototype;
		value_make_object(*ret, o);
		target = ret;
	}
	
	if (argc == 1) {
		Value *v = value_object_lookup_array(args, 0, NULL);
		if (v && is_number(v)) {
			if (!is_integer(v->d.num) || v->d.num < 0) die("Invalid array length\n");
			object_set_length(target->d.obj, (int)v->d.num);
			return 0;
		}
	}

	int i;
	object_set_length(target->d.obj, 0);
	
	for (i = 0; i < argc; ++i) {
		Value *v = value_new();
		Value *argv = value_object_lookup_array(args, i, NULL);
		
		value_copy(*v, *argv);

		value_object_utils_insert_array(_this, i, v, 1, 1, 1);
	}
	return 0;
}

static int Boolean_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) {
		int nv = 0;
		if (value_get_length(args) > 0) {
			Value *v = value_object_lookup_array(args, 0, NULL);
			if (v) {
				nv = value_istrue(v);
			}
		}
		_this->d.obj->ot = OT_BOOL;
		_this->d.obj->d.val = nv;
		return 0;
	}
	if (value_get_length(args) > 0) {
		Value *v = value_object_lookup_array(args, 0, NULL);
		if (v) {
			value_make_bool(*ret, value_istrue(v));
			return 0;
		}
	}
	value_make_bool(*ret, 0);
	return 0;
}

static int RegExp_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	Value *target;
	
	if (asc) target = _this;
	else {
		Object *o = object_new();
		o->__proto__ = RegExp_prototype;
		value_make_object(*ret, o);
		target = ret;
	}
	
	Value *v = value_object_lookup_array(args, 0, NULL);
	const unichar *regtxt = EMPTY.unistr;
	if (v) {
		if (v->vt == VT_OBJECT && v->d.obj->ot == OT_REGEXP) {
			value_copy(*target, *v);
			return 0;
		} else if (v->vt == VT_STRING) {
			regtxt = v->d.str;
		}	/* todo tostring */
	}
	
	int flag = REG_EXTENDED;
	Value *f = value_object_lookup_array(args, 1, NULL);
	if (f && f->vt == VT_STRING) {
		if (unistrchr(f->d.str, 'i')) flag |= REG_ICASE;
	}

	target->d.obj->d.robj = regex_new(tochars(regtxt), flag);;
	target->d.obj->ot = OT_REGEXP;
	return 0;
}

void proto_init(Value *global)
{
	/* object_prototype the start of protochain */
	Object_prototype = value_object_utils_new_object();	
	
	/* Top, the default "this" value, pointed to global, is an object */
	Top_object = global;
	Top_object->d.obj->__proto__ = Object_prototype;
	
	/* Function.prototype.prototype is a common object */
	Function_prototype_prototype = value_object_utils_new_object();
	Function_prototype_prototype->d.obj->__proto__ = Object_prototype;

	/* Function.prototype.__proto__ pointed to Object.prototype */
	Function_prototype = func_utils_make_func_value(Function_prototype_constructor);
	value_object_utils_insert(Function_prototype, tounichars("prototype"), 
							  Function_prototype_prototype, 0, 0, 0);
	Function_prototype->d.obj->__proto__ = Object_prototype;
	
	/* Function prototype.call */
	Value *_Function_p_call = func_utils_make_func_value(Function_prototype_call);
	value_object_utils_insert(Function_prototype, tounichars("call"), 
							  _Function_p_call, 0, 0, 0);
	_Function_p_call->d.obj->__proto__ = Function_prototype;
	
	/* Function prototype.apply */
	Value *_Function_p_apply = func_utils_make_func_value(Function_prototype_apply);
	value_object_utils_insert(Function_prototype, tounichars("apply"), 
							  _Function_p_apply, 0, 0, 0);
	_Function_p_apply->d.obj->__proto__ = Function_prototype;
	
	/* Object.__proto__ pointed to Function.prototype */
	Value *_Object = func_utils_make_func_value(Object_constructor);
	value_object_utils_insert(_Object, tounichars("prototype"), Object_prototype, 0, 0, 0);
	_Object->d.obj->__proto__ = Function_prototype;

	/* both Function.prototype,__proto__ pointed to Function.prototype */
	Value *_Function = func_utils_make_func_value(Function_constructor);
	value_object_utils_insert(_Function, tounichars("prototype"), Function_prototype, 0, 0, 0);
	_Function->d.obj->__proto__ = Function_prototype;

	/* String.prototype is a common object */
	String_prototype = value_object_utils_new_object();
	String_prototype->d.obj->__proto__ = Object_prototype;
	
	Value *_String = func_utils_make_func_value(String_constructor);
	value_object_utils_insert(_String, tounichars("prototype"), String_prototype, 0, 0, 0);
	_String->d.obj->__proto__ = Function_prototype;
	Value *_String_fcc = func_utils_make_func_value(String_fromCharCode);
	_String_fcc->d.obj->__proto__ = Function_prototype;
	value_object_utils_insert(_String, tounichars("fromCharCode"), _String_fcc, 0, 0, 0);
	
	Number_prototype= value_object_utils_new_object();
	Number_prototype->d.obj->__proto__ = Object_prototype;
	
	Value *_Number = func_utils_make_func_value(Number_constructor);
	value_object_utils_insert(_Number, tounichars("prototype"), Number_prototype, 0, 0, 0);
	_Number->d.obj->__proto__ = Function_prototype;

	Boolean_prototype= value_object_utils_new_object();
	Boolean_prototype->d.obj->__proto__ = Object_prototype;
	
	Value *_Boolean = func_utils_make_func_value(Boolean_constructor);
	value_object_utils_insert(_Boolean, tounichars("prototype"), Boolean_prototype, 0, 0, 0);
	_Boolean->d.obj->__proto__ = Function_prototype;

	Array_prototype= value_object_utils_new_object();
	Array_prototype->d.obj->__proto__ = Object_prototype;
	object_set_length(Array_prototype->d.obj, 0);
	
	Value *_Array = func_utils_make_func_value(Array_constructor);
	value_object_utils_insert(_Array, tounichars("prototype"), Array_prototype, 0, 0, 0);
	_Array->d.obj->__proto__ = Function_prototype;

	RegExp_prototype = value_object_utils_new_object();
	RegExp_prototype->d.obj->__proto__ = Object_prototype;
	
	Value *_RegExp = func_utils_make_func_value(RegExp_constructor);
	value_object_utils_insert(_RegExp, tounichars("prototype"), RegExp_prototype, 0, 0, 0);
	_RegExp->d.obj->__proto__ = Function_prototype;
	
	value_object_utils_insert(global, tounichars("Object"), _Object, 1, 1, 0);
	value_object_utils_insert(global, tounichars("Function"), _Function, 1, 1, 0);
	value_object_utils_insert(global, tounichars("String"), _String, 1, 1, 0);
	value_object_utils_insert(global, tounichars("Number"), _Number, 1, 1, 0);
	value_object_utils_insert(global, tounichars("Boolean"), _Boolean, 1, 1, 0);
	value_object_utils_insert(global, tounichars("Array"), _Array, 1, 1, 0);
	value_object_utils_insert(global, tounichars("RegExp"), _RegExp, 1, 1, 0);

	proto_string_init(global);
	proto_number_init(global);
	proto_array_init(global);
}

