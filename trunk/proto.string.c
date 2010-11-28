#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstate.h"
#include "error.h"
#include "value.h"
#include "proto.h"
#include "regexp.h"

#define MAX_SUBREGEX	256

/* substr */
static int strpto_substr(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute String.prototype.substr as constructor\n");
	if (_this->vt != VT_OBJECT || _this->d.obj->ot != OT_STRING) {
		die("apply String.prototype.substr to a non-string object\n");
	}

	unichar *v = _this->d.obj->d.str;
	Value *start = value_object_lookup_array(args, 0, NULL);
	Value *len = value_object_lookup_array(args, 1, NULL);
	
	if (!start || !is_number(start)) {
		value_make_string(*ret, unistrdup(v));
		return 0;
	}
	int istart = (int) start->d.num;
	
	if (!len || !is_number(len)) {
		value_make_string(*ret, unisubstrdup(v, istart, -1));
		return 0;
	}
	int ilen = (int) len->d.num;
	if (ilen <= 0) {
		value_make_string(*ret, unistrdup_str(""));
	} else {
		value_make_string(*ret, unisubstrdup(v, istart, ilen));
	}
	return 0;
}

/* indexOf */
/* todo regex */
static int strpto_indexOf(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute String.prototype.indexOf as constructor\n");
	if (_this->vt != VT_OBJECT || _this->d.obj->ot != OT_STRING) {
		die("apply String.prototype.indexOf to a non-string object\n");
	}

	unichar *v = _this->d.obj->d.str;
	Value *seq = value_object_lookup_array(args, 0, NULL);
	Value *start = value_object_lookup_array(args, 1, NULL);

	if (!seq) {
		value_make_number(*ret, -1);
		return 0;
	}

	value_tostring(seq);
	unichar *vseq = seq->d.str;
	
	int istart = 0;
	if (start && is_number(start)) {
		istart = (int) start->d.num;
		if (istart < 0) istart = 0;
	}

	int r = unistrpos(v, istart, vseq);
	value_make_number(*ret, r);

	return 0;
}

static UNISTR(5) INDEX = { 5, { 'i', 'n', 'd', 'e', 'x' } };
static UNISTR(5) INPUT = { 5, { 'i', 'n', 'p', 'u', 't' } };


/* match */
static int strpto_match(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute String.prototype.match as constructor\n");
	if (_this->vt != VT_OBJECT || _this->d.obj->ot != OT_STRING) {
		die("apply String.prototype.match to a non-string object\n");
	}

	unichar *v = _this->d.obj->d.str;
	Value *seq = value_object_lookup_array(args, 0, NULL);

	if (!seq || seq->vt != VT_OBJECT || seq->d.obj->ot != OT_REGEXP) {
		value_make_null(*ret);
		return 0;
	}

	regex_t *reg = seq->d.obj->d.robj;
	
	regmatch_t pos[MAX_SUBREGEX];
	memset(&pos, 0, MAX_SUBREGEX * sizeof(regmatch_t));
	int r;
	if ((r = regexec(reg, tochars(v), MAX_SUBREGEX, pos, 0)) != 0) {
		if (r == REG_NOMATCH) {
			value_make_null(*ret);
			return 0;
		} else die("Out of memory\n");
	}

	Object *obj = object_new();
	obj->__proto__ = Array_prototype;
	value_make_object(*ret, obj);
	object_set_length(ret->d.obj, 0);
	
	int i;
	for (i = 0; i < MAX_SUBREGEX; ++i) {
		if (pos[i].rm_so <= 0 && pos[i].rm_eo <= 0) break;

		Value *val = value_new();
		value_make_string(*val, 
			unisubstrdup(v, pos[i].rm_so, pos[i].rm_eo - pos[i].rm_so));
		value_object_utils_insert_array(ret, i, val, 1, 1, 1);
	}

	Value *vind = value_new();
	value_make_number(*vind, pos[0].rm_so);
	value_object_utils_insert(ret, INDEX.unistr, vind, 1, 1, 1);
	Value *vinput = value_new();
	value_make_string(*vinput, unistrdup(v));
	value_object_utils_insert(ret, INPUT.unistr, vinput, 1, 1, 1);
	
	return 0;
}

/* charCodeAt */
static int strpto_charCodeAt(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Execute String.prototype.charCodeAt as constructor\n");

	Value target = { 0 };
	value_copy(target, *_this);
	value_tostring(&target);
	
	int slen = unistrlen(target.d.str);
	int pos = 0;
	Value *vpos;
	if ((vpos = value_object_lookup_array(args, 0, NULL))) {
		value_toint32(vpos);
		pos = (int)vpos->d.num;
	}

	if (pos < 0 || pos >= slen) {
		value_make_number(*ret, ieee_makenan());
	} else {
		value_make_number(*ret, target.d.str[pos]);
	}
	value_erase(target);
	return 0;
}

static struct st_strpro_tab {
	const char *name;
	SSFunc func;
} strpro_funcs[] = {
	{ "substr", strpto_substr },
	{ "indexOf", strpto_indexOf },
	{ "match", strpto_match },
	{ "charCodeAt", strpto_charCodeAt }
};

void proto_string_init(Value *global)
{
	if (!String_prototype) bug("proto init failed?");
	int i;
	for (i = 0; i < sizeof(strpro_funcs) / sizeof(struct st_strpro_tab); ++i) {
		Value *n = func_utils_make_func_value(strpro_funcs[i].func);
		n->d.obj->__proto__ = Function_prototype;
		value_object_utils_insert(String_prototype, tounichars(strpro_funcs[i].name), n, 0, 0, 0);
	}
}

