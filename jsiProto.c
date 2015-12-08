#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

/* Jsi_Obj constructor */
static int ObjectConstructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    
    if (Jsi_FunctionIsConstructor(funcPtr)) {
        /* new operator will do the rest */
        return JSI_OK;
    }
    
    if (Jsi_ValueGetLength(interp, args) <= 0) {
        Jsi_Obj *o = Jsi_ObjNew(interp);
        o->__proto__ = interp->Object_prototype;
        Jsi_ValueMakeObject(interp,*ret, o);
        return JSI_OK;
    }
    Jsi_Value *v = Jsi_ValueArrayIndex(interp, args, 0);
    if (!v || v->vt == JSI_VT_UNDEF || v->vt == JSI_VT_NULL) {
        Jsi_Obj *o = Jsi_ObjNewType(interp, JSI_OT_OBJECT);
        Jsi_ValueMakeObject(interp,*ret, o);
        return JSI_OK;
    }
    Jsi_ValueCopy(interp,*ret, v);
    Jsi_ValueToObject(interp, *ret);
    return JSI_OK;
}

/* Function.prototype pointed to a empty function */
static int FunctionPrototypeConstructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    return JSI_OK;
}

static int Function_constructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    if (Jsi_FunctionIsConstructor(funcPtr)) {      /* Todo, parse the argument, return the new function obj */
        _this->d.obj->ot = JSI_OT_FUNCTION;
        return JSI_OK;
    }
    Jsi_Obj *o = Jsi_ObjNewType(interp, JSI_OT_FUNCTION);
    Jsi_ValueMakeObject(interp, *ret, o);
    return JSI_OK;
}

/* delete array[0], array[1]->array[0] */
void Jsi_ValueArrayShift(Jsi_Interp *interp, Jsi_Value *v)
{
    if (v->vt != JSI_VT_OBJECT) {
        Jsi_LogBug("Jsi_ValueArrayShift, target is not object");
        return;
    }
    Jsi_Obj *o = v->d.obj;
    if (o->isArray) {
        int i;
        if (!o->arrCnt)
            return;
        if (o->arr[0])
            Jsi_DecrRefCount(interp, o->arr[0]);
        for (i=1; i<o->arrCnt; i++) {
            o->arr[i-1] = o->arr[i];
        }
        o->arr[o->arrCnt--] = NULL;
        return;
    }
    
    int len = Jsi_ObjGetLength(interp, v->d.obj);
    if (len <= 0) return;
    
    Jsi_Value *v0 = Jsi_ValueArrayIndex(interp, v, 0);
    if (!v0) return;
    
    Jsi_ValueReset(interp,v0);
    
    int i;
    Jsi_Value *last = v0;
    for (i = 1; i < len; ++i) {
        Jsi_Value *t = Jsi_ValueArrayIndex(interp, v, i);
        if (!t) return;
        Jsi_ValueCopy(interp,last, t);
        Jsi_ValueReset(interp,t);
        last = t;
    }
    
    Jsi_ObjSetLength(interp, v->d.obj, len - 1);
}

void jsi_SharedArgs(Jsi_Interp *interp, Jsi_Value *args, Jsi_ScopeStrs *argnames, int alloc)
{
    int i;
    if (!argnames) return;
    
    for (i = 0; i < argnames->count; ++i) {
        const char *argkey = jsi_ScopeStrsGet(argnames, i);
        if (!argkey) break;
        
        Jsi_Value *v = Jsi_ValueArrayIndex(interp, args, i);
        if (!alloc) return;
        if (!alloc) {
            if (v)
                Jsi_DecrRefCount(interp, v);
        } else {
            if (v)
                Jsi_IncrRefCount(interp, v);
            else {
                v = Jsi_ValueNew1(interp);
            }
            jsi_ValueObjSet(interp, args, argkey, v, JSI_OM_DONTENUM | (v?JSI_OM_INNERSHARED:0), 1);
        }
    }
}

void jsi_SetCallee(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *tocall)
{
    if (interp->hasCallee) {
        Jsi_Value *callee = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp,callee, tocall);
        Jsi_ValueInsert(interp, args, "\1callee\1", callee, JSI_OM_DONTENUM);
    }
}

int Jsi_FunctionCall(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret)
{
    Jsi_Value *tocall = _this;
    if (!Jsi_ValueIsFunction(interp, tocall)) {
        Jsi_LogError("can not execute expression, expression is not a function\n");
        return -1;
    }

    if (!tocall->d.obj->d.fobj) {   /* empty function */
        return JSI_OK;
    }
    
    /* func to call */
    Jsi_Func *fstatic = tocall->d.obj->d.fobj->func;
    
    /* new this */
    Jsi_Value newthis = VALINIT;
    Jsi_Value *arg1 = NULL;
    if ((arg1 = Jsi_ValueArrayIndex(interp, args, 0)) && !Jsi_ValueIsUndef(interp, arg1)
        && !Jsi_ValueIsNull(interp, arg1)) {
        Jsi_ValueCopy(interp,&newthis, arg1);
        Jsi_ValueToObject(interp, &newthis);
    } else {
        Jsi_ValueCopy(interp,&newthis, interp->Top_object);
    }
    
    /* prepare args */
    Jsi_ValueArrayShift(interp, args);
    jsi_SharedArgs(interp, args, fstatic->argnames, 1);
    
    jsi_InitLocalVar(interp, args, fstatic);
    jsi_SetCallee(interp, args, tocall);
    
    int res = 0;
    if (fstatic->type == FC_NORMAL) {
        res = jsi_evalcode(interp->ps, fstatic->opcodes, tocall->d.obj->d.fobj->scope, 
                   args, &newthis, ret);
    } else {
        res = fstatic->callback(interp, args, &newthis, ret, fstatic);
    }
    jsi_SharedArgs(interp, args, fstatic->argnames, 0);
    Jsi_ValueReset(interp,&newthis);
    return res;
}

static int FunctionCallCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    if (Jsi_FunctionIsConstructor(funcPtr)) {
        Jsi_LogError("Execute call as constructor\n");
        return -1;
    }
    
    return Jsi_FunctionCall(interp, args, _this, ret);
}


static int ObjectKeysCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc = Jsi_ValueGetKeys(interp, _this, *ret);
    if (rc != JSI_OK)
        Jsi_LogWarn("can not call Keys() with non-object");
    return rc;
}

int jsi_ObjectToStringCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{//TODO: support quote arg
    int quote = JSI_OUTPUT_QUOTE;
    Jsi_DString dStr = {};
    Jsi_ValueGetDString(interp, _this, &dStr, quote);
    Jsi_ValueMakeString(interp, *ret, Jsi_Strdup(Jsi_DSValue(&dStr)));
    Jsi_DSFree(&dStr);
    return JSI_OK;
}


int Jsi_FunctionApply(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret)
{
    int isalloc = 0;
    Jsi_Value *tocall = _this;
    if (!Jsi_ValueIsFunction(interp, tocall)) {
        Jsi_LogError("can not execute expression, expression is not a function\n");
        return -1;
    }

    if (!tocall->d.obj->d.fobj) {   /* empty function */
        return 0;
    }
    
    /* func to call */
    Jsi_Func *fstatic = tocall->d.obj->d.fobj->func;
    
    /* new this */
    Jsi_Value newthis = VALINIT;
    Jsi_Value *arg1 = NULL;
    if ((arg1 = Jsi_ValueArrayIndex(interp, args, 0)) && !Jsi_ValueIsUndef(interp, arg1)
        && !Jsi_ValueIsNull(interp, arg1)) {
        Jsi_ValueCopy(interp,&newthis, arg1);
        Jsi_ValueToObject(interp, &newthis);
    } else {
        Jsi_ValueCopy(interp,&newthis, interp->Top_object);
    }
    
    /* prepare args */
    Jsi_Value *newscope = Jsi_ValueArrayIndex(interp, args, 1);
    if (newscope) {
        if (newscope->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, newscope->d.obj)) {
            Jsi_LogError("second argument to Function.prototype.apply must be an array\n");
            return -1;
        }
    } else {
        isalloc = 1;
        newscope = jsi_ObjValueNew(interp);
        Jsi_ObjSetLength(interp, newscope->d.obj, 0);
        Jsi_IncrRefCount(interp, newscope);
    }
    
    jsi_SharedArgs(interp, newscope, fstatic->argnames, 1);
    jsi_InitLocalVar(interp, newscope, fstatic);
    jsi_SetCallee(interp, newscope, tocall);

    int res = 0;
    if (fstatic->type == FC_NORMAL) {
        res = jsi_evalcode(interp->ps, fstatic->opcodes, tocall->d.obj->d.fobj->scope, 
            newscope, &newthis, ret);
    } else {
        res = fstatic->callback(interp, newscope, &newthis, ret, fstatic);
    }
    jsi_SharedArgs(interp, newscope, fstatic->argnames, 0);
    if (isalloc)
        Jsi_DecrRefCount(interp, newscope);
    Jsi_ValueReset(interp,&newthis);
    return res;
}


static int ObjectValueOfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_ValueCopy(interp, *ret, _this);
    //Jsi_ValueToPrimitive(interp, ret);
    return JSI_OK;
}

int jsi_GetPrototypeOfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *v = Jsi_ValueArrayIndex(interp, args,0);
    if (v->vt != JSI_VT_OBJECT)
        return JSI_ERROR;
    Jsi_ValueCopy(interp, *ret, v->d.obj->__proto__);
    return JSI_OK;
}

int jsi_HasOwnPropertyCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *v;
    char *key = Jsi_ValueArrayIndexToStr(interp, args,0, NULL);
    v = Jsi_ValueObjLookup(interp, _this, key, 0);
    Jsi_ValueMakeBool(interp, *ret, (v != NULL));
    return JSI_OK;
}

static int ObjectIsPrototypeOfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *proto, *sproto, *v = Jsi_ValueArrayIndex(interp, args,0);
    int bval = 0;
    if (v->vt != JSI_VT_OBJECT || _this->vt != JSI_VT_OBJECT) {
        goto retval;
    }
    proto = _this->d.obj->__proto__;
    sproto = v->d.obj->__proto__;
    if (!proto)
        goto retval;
    while (sproto) {
        if ((bval=(sproto == proto)))
            break;
        if (sproto->vt != JSI_VT_OBJECT)
            break;
        sproto = sproto->d.obj->__proto__;
    }
retval:
    Jsi_ValueMakeBool(interp, *ret, bval);
    return JSI_OK;
    
}


static int ObjectCreateCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *proto = Jsi_ValueArrayIndex(interp, args,0);
    /* TODO: support properties argument. */
    /*Jsi_Value *props = Jsi_ValueArrayIndex(interp, args,1);*/

    if (proto->vt == JSI_VT_NULL) {
        Jsi_ValueMakeObject(interp, *ret, Jsi_ObjNew(interp));
    }
    if (proto->vt != JSI_VT_OBJECT) {
        Jsi_LogError("expected proto object or null");
        return JSI_ERROR;
    }
    Jsi_ValueCopy(interp, *ret, proto);
    return JSI_OK;
    
}

static int ObjectBindCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Obj *obj = interp->lastBindSubscriptObj;
    if (obj == NULL || obj->ot != JSI_OT_FUNCTION) {
        Jsi_LogError("invalid bind call");
        return JSI_ERROR;
    }
    Jsi_Value *v = Jsi_ValueArrayIndex(interp, args, 0); /*TODO: supposed to use this object in call*/
    if (!((v->vt == JSI_VT_UNDEF) || (v->vt == JSI_VT_OBJECT && 
        (v->d.obj->ot == JSI_OT_OBJECT || v->d.obj->ot == JSI_OT_FUNCTION)))) {
        Jsi_LogError("expected an object or 'undefined'");
        return JSI_ERROR;
    }
    Jsi_Func *func = obj->d.fobj->func;
    Jsi_Obj *o = Jsi_ObjNew(interp);
    Jsi_Func *f = Jsi_Calloc(1,sizeof(*f));
    SIGINIT(f,FUNC);
    *f = *func;
    o->d.fobj = jsi_FuncObjNew(interp, f);
    o->ot = JSI_OT_FUNCTION;
    f->bindArgs = jsi_ValueDup(interp, args);
    Jsi_IncrRefCount(interp, f->bindArgs);
    v = Jsi_ValueMakeObject(interp, NULL, o);
    Jsi_ValueCopy(interp, *ret, v);
    v->d.obj->d.fobj->scope = obj->d.fobj->scope;
    return JSI_OK;
    
}

static int ObjectPropertyIsEnumerableCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *v;
    int b = 0;

    char *key = Jsi_ValueArrayIndexToStr(interp, args,0, NULL);
    v = Jsi_ValueObjLookup(interp, _this, key, 0);
    b = (v && (v->f.bits.dontenum==0));

    Jsi_ValueMakeBool(interp, *ret, b);
    return JSI_OK;
}

static int ObjectToLocaleStringCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    return jsi_ObjectToStringCmd(interp, args, _this, ret, funcPtr);
}

static int FunctionApplyCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    if (Jsi_FunctionIsConstructor(funcPtr)) {
        Jsi_LogError("Execute apply as constructor\n");
        return -1;
    }
    
    return Jsi_FunctionApply(interp, args, _this, ret);
}

Jsi_Value *jsi_ProtoObjValueNew1(Jsi_Interp *interp, const char *name) {
    Jsi_Value *v = jsi_ObjValueNew(interp);
    v->f.bits.isglob = 1;
    if (name != NULL)
        Jsi_PrototypeDefine(interp, name, v);
    Jsi_IncrRefCount(interp, v);
    return v;
}

Jsi_Value *jsi_ProtoValueNew(Jsi_Interp *interp, const char *name, const char *parent)
{
    Jsi_Value *fproto;
    if (parent == NULL)
        parent = "Object";
    fproto = jsi_ProtoObjValueNew1(interp, name);
    Jsi_PrototypeObjSet(interp, parent, Jsi_ValueGetObj(interp, fproto));
    return fproto;
}

static Jsi_CmdSpec functionCmds[] = {
    { "Function",  Function_constructor,   0, 1,  "str", JSI_CMD_IS_CONSTRUCTOR, .help="Function constructor" },
    { "apply",     FunctionApplyCmd,       1, 2,  "thisArg,?argArray?", .help="Call function passing args array"},
    { "call",      FunctionCallCmd,        1, -1, "thisArg?,arg1,arg2,...?", .help="Call function with args" },
    { NULL, .help="Commands for accessing functions" }
};

int jsi_FunctionInit(Jsi_Interp *interp)
{
    Jsi_CommandCreateSpecs(interp, "Function", functionCmds, interp->Function_prototype, JSI_CMDSPEC_PROTO);
    return JSI_OK;
}

/* TODO: defineProperty, defineProperties, */
static Jsi_CmdSpec objectCmds[] = {
    { "Object",         ObjectConstructor,      0,  1,  "str", JSI_CMD_IS_CONSTRUCTOR, .help="Object constructor" },
    { "bind",           ObjectBindCmd,          1, -1, "thisArg?,arg,arg,...?", .help="Creates a new function that when called, has its this keyword set to the provided thisArg" },
    { "create",         ObjectCreateCmd,        1, 2, "proto?,properties?", .help="Create a new object with prototype object and properties" },
    { "getPrototypeOf", jsi_GetPrototypeOfCmd,  1, 1, "name", .help="Return prototype of an object" },
    { "hasOwnProperty", jsi_HasOwnPropertyCmd,  1, 1, "name", .help="Returns a boolean indicating whether the object has the specified property"},
    { "isPrototypeOf",  ObjectIsPrototypeOfCmd, 1, 1, "name", .help="Tests for an object in another object's prototype chain" },
    { "keys",           ObjectKeysCmd,          0, 0, "", .help="Return the keys of an object or array" },
    { "propertyIsEnumerable", ObjectPropertyIsEnumerableCmd,1, 1, "name", .help="Determine if a property is enumerable" },
    { "toLocaleString", ObjectToLocaleStringCmd,0, 1, "?quoteflag?", .help="Convert to string" },
    { "toString",       jsi_ObjectToStringCmd,  0, 1, "?quoteflag?", .help="Convert to string" }, 
    { "valueOf",        ObjectValueOfCmd,       0, 0, "", .help="Returns primitive value" },
    { NULL, .help="Commands for accessing Objects" }
};

int jsi_ObjectInit(Jsi_Interp *interp)
{
    Jsi_CommandCreateSpecs(interp, "Object", objectCmds, interp->Object_prototype, JSI_CMDSPEC_PROTO);
    return JSI_OK;
}

int jsi_ObjectDone(Jsi_Interp *interp)
{
    return JSI_OK;
}

int jsi_ProtoInit(Jsi_Interp *interp)
{
     /* Function and Object are created together. */
    Jsi_Value *global = interp->csc;
    /* Top, the default "this" value, pointed to global, is an object */
    interp->Top_object = global;

    /* object_prototype the start of protochain */
    interp->Object_prototype = jsi_ProtoObjValueNew1(interp, "Object");
    interp->Top_object->d.obj->__proto__ = interp->Object_prototype;
        
    /* Function.prototype.prototype is a common object */
    interp->Function_prototype_prototype = jsi_ProtoObjValueNew1(interp, "Function.prototype");
    interp->Function_prototype_prototype->d.obj->__proto__ = interp->Object_prototype;
    
    /* Function.prototype.__proto__ pointed to Jsi_Obj.prototype */
    interp->Function_prototype = jsi_MakeFuncValue(interp, FunctionPrototypeConstructor, "prototype");
    //Jsi_IncrRefCount(interp, interp->Function_prototype);
    Jsi_ValueInsertFixed(interp, interp->Function_prototype, "prototype", 
                              interp->Function_prototype_prototype);
    interp->Function_prototype->d.obj->__proto__ = interp->Object_prototype;
    
    /* Jsi_Obj.__proto__ pointed to Function.prototype */
    Jsi_Value *_Object = jsi_MakeFuncValue(interp, ObjectConstructor, "prototype");
    //Jsi_IncrRefCount(interp, _Object);
    Jsi_ValueInsertFixed(interp, _Object, "prototype", interp->Object_prototype);
    _Object->d.obj->__proto__ = interp->Function_prototype;

    /* both Function.prototype,__proto__ pointed to Function.prototype */
    Jsi_Value *_Function = jsi_MakeFuncValue(interp, Function_constructor, "prototype");
    //Jsi_IncrRefCount(interp, _Function);
    Jsi_ValueInsertFixed(interp, _Function, "prototype", interp->Function_prototype);
    _Function->d.obj->__proto__ = interp->Function_prototype;

    Jsi_ValueInsert(interp, global, "Object", _Object, JSI_OM_DONTENUM);
    Jsi_ValueInsert(interp, global, "Function", _Function, JSI_OM_DONTENUM);

    //Jsi_HashSet(interp->genValueTbl, _Object, _Object);
    //Jsi_HashSet(interp->genValueTbl, _Function, _Function);
    //Jsi_HashSet(interp->genValueTbl, interp->Object_prototype , interp->Object_prototype);
    //Jsi_HashSet(interp->genValueTbl, interp->Function_prototype, interp->Function_prototype);
    jsi_ObjectInit(interp);
    jsi_FunctionInit(interp);
    
    jsi_StringInit(interp);
    jsi_BooleanInit(interp);
    jsi_NumberInit(interp);
    jsi_ArrayInit(interp);
    jsi_RegexpInit(interp);
    jsi_MathInit(interp);
    jsi_TreeInit(interp);
    return JSI_OK;
}

int jsi_ProtoDone(Jsi_Interp *interp)
{
    jsi_ObjectDone(interp);
 /*   jsi_FunctionDone(interp);
    
    jsi_StringDone(interp);
    jsi_BooleanDone(interp);
    jsi_NumberDone(interp);
    jsi_ArrayDone(interp);
    jsi_RegexpDone(interp);
    jsi_MathDone(interp);
    jsi_TreeDone(interp);*/
    return JSI_OK;
}
#endif
