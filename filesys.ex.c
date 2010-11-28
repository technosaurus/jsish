/*
 * file system extern.
 * File - Constructor of File Object
 *   .prototype       - prototype of File
 *   .prototype.open  - open file
 *   .prototype.close - close file
 *   .prototype.eof   - end of file
 *   .prototype.gets  - read line
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "pstate.h"
#include "error.h"
#include "value.h"
#include "func.h"
#include "proto.h"
#include "unichar.h"

#ifdef USE_FILESYS_EX

/* global fileoject id, used to access userdata struct from Object */
static udid global_fileobject_udid;
static Value *File_prototype;

/* here demo how to store user-defined data into Object */
/* first, defined what will be store */
typedef struct UdFileobj {
	FILE *fp;
	unichar *filename;
	unichar *mode;
} UdFileobj;

static void fileobject_erase(UdFileobj *fo)
{
	if (fo->filename) {
		fclose(fo->fp);
		unifree(fo->filename);
		unifree(fo->mode);
	}
	fo->filename = NULL;
}

static void fileobject_free(void *data)
{
	UdFileobj *fo = data;
	fileobject_erase(fo);
	free(fo);
}

static int fileobject_istrue(void *data)
{
	UdFileobj *fo = data;
	if (!fo->filename) return 0;
	else return 1;
}

static int fileobject_equ(void *data1, void *data2)
{
	return (data1 == data2);
}

/* second, defined a UserDataReg with how to proccess userdefined data */
static UserDataReg fileobject = {
	"fileobject",
	fileobject_free,
	fileobject_istrue,
	fileobject_equ
};

static int try_open_file(UdFileobj *udf, Value *args)
{
	int ret = 0;
	fileobject_erase(udf);
	
	Value *fname = value_object_lookup_array(args, 0, NULL);
	if (fname && fname->vt == VT_STRING) {
		Value *vmode = value_object_lookup_array(args, 1, NULL);
		const char *mode = NULL;
		if (vmode && vmode->vt == VT_STRING) {
			mode = tochars(vmode->d.str);
		}
		char *rmode = c_strdup(mode ? mode : "r");
		FILE *fp = fopen(tochars(fname->d.str), rmode);
		if (fp) {
			udf->fp = fp;
			udf->filename = unistrdup(fname->d.str);
			udf->mode = unistrdup_str(rmode);
		} else ret = -1;
		c_strfree(rmode);
	}
	return ret;
}

static int File_constructor(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	Value *toacc = NULL;
	if (asc) {
		toacc = _this;
	} else {
		Object *o = object_new();
		o->__proto__ = File_prototype;
		value_make_object(*ret, o);
		toacc = ret;
	}

	UdFileobj *udf = malloc(sizeof(UdFileobj));
	memset(udf, 0, sizeof(UdFileobj));
	
	try_open_file(udf, args);
	
	userdata_set(toacc->d.obj, global_fileobject_udid, udf);
	return 0;
}

static int File_prototype_open(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Can not call File.open as constructor\n");
	if (_this->vt != VT_OBJECT) bug("this is not object\n");

	UdFileobj *udf = userdata_get(_this->d.obj, global_fileobject_udid);
	if (!udf) die("Apply File.open in a non-file object\n");

	if (try_open_file(udf, args)) {
		value_make_bool(*ret, 0);
	}
	value_make_bool(*ret, 1);
	return 0;
}

static int File_prototype_close(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Can not call File.close as constructor\n");
	if (_this->vt != VT_OBJECT) bug("this is not object\n");
	
	UdFileobj *udf = userdata_get(_this->d.obj, global_fileobject_udid);
	if (!udf) die("Apply File.close in a non-file object\n");

	fileobject_erase(udf);
	value_make_bool(*ret, 1);
	return 0;
}

static int File_prototype_gets(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Can not call File.gets as constructor\n");
	if (_this->vt != VT_OBJECT) bug("this is not object\n");
	
	UdFileobj *udf = userdata_get(_this->d.obj, global_fileobject_udid);
	if (!udf) die("Apply File.gets in a non-file object\n");

	if (!udf->filename) {
		value_make_undef(*ret);
		return 0;
	}
	char buf[2048];
	if (!fgets(buf, 2046, udf->fp)) {
		value_make_undef(*ret);
		return 0;
	}
	buf[2046] = 0;
	char *r = strchr(buf, '\r');
	if (r) *r = 0;
	else {
		r = strchr(buf, '\n');
		if (r) *r = 0;
	}
	value_make_string(*ret, unistrdup_str(buf));
	return 0;
}

static int File_prototype_puts(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Can not call File.puts as constructor\n");
	if (_this->vt != VT_OBJECT) bug("this is not object\n");
	
	UdFileobj *udf = userdata_get(_this->d.obj, global_fileobject_udid);
	if (!udf) die("Apply File.puts in a non-file object\n");

	if (!udf->filename) {
		value_make_bool(*ret, 0);
		return 0;
	}
	Value *toput = value_object_lookup_array(args, 0, NULL);
	if (!toput) {
		value_make_bool(*ret, 0);
		return 0;
	}
	value_tostring(toput);

	if (fprintf(udf->fp, "%s\n", tochars(toput->d.str)) < 0) {
		value_make_bool(*ret, 0);
		return 0;
	}
	value_make_bool(*ret, 1);
	return 0;
}

static int File_prototype_eof(PSTATE *ps, Value *args, Value *_this, Value *ret, int asc)
{
	if (asc) die("Can not call File.eof as constructor\n");
	if (_this->vt != VT_OBJECT) bug("this is not object\n");
	
	UdFileobj *udf = userdata_get(_this->d.obj, global_fileobject_udid);
	if (!udf) die("Apply File.eof in a non-file object\n");

	value_make_bool(*ret, feof(udf->fp));
	return 0;
}

void filesys_init(Value *global)
{
	/* third, register userdata */
	global_fileobject_udid = userdata_register(&fileobject);
	if (global_fileobject_udid < 0) die("Can not init file system\n");

	/* File.prototype */
	File_prototype = value_object_utils_new_object();
	File_prototype->d.obj->__proto__ = Object_prototype;
	value_object_utils_insert(File_prototype, tounichars("open"),
			func_utils_make_func_value(File_prototype_open), 0, 0, 0);
	value_object_utils_insert(File_prototype, tounichars("close"),
			func_utils_make_func_value(File_prototype_close), 0, 0, 0);
	value_object_utils_insert(File_prototype, tounichars("gets"),
			func_utils_make_func_value(File_prototype_gets), 0, 0, 0);
	value_object_utils_insert(File_prototype, tounichars("puts"),
			func_utils_make_func_value(File_prototype_puts), 0, 0, 0);
	value_object_utils_insert(File_prototype, tounichars("eof"),
			func_utils_make_func_value(File_prototype_eof), 0, 0, 0);
	
	Value *_File = func_utils_make_func_value(File_constructor);
	value_object_utils_insert(_File, tounichars("prototype"), 
			File_prototype, 0, 0, 0);
	_File->d.obj->__proto__ = Function_prototype;

	value_object_utils_insert(global, tounichars("File"), _File, 1, 1, 0);
}

#else

/* no filesystem extern, simply empty init */
void filesys_init(Value *global) {}

#endif

