#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

#define JSIVAL_GET_STR(pv) (pv->vt == JSI_VT_STRING ? &pv->d.s : \
  ((pv->vt == JSI_VT_OBJECT && pv->d.obj->ot == JSI_OT_STRING) ? &pv->d.obj->d.s : NULL))

#define bits_set(who, mask)     ((who) |= (mask))
#define bits_unset(who, mask)   ((who) &= (~(mask)))
#define bits_get(who, mask)     ((who) & (mask))

static void IterGetKeys(Jsi_Interp *interp, Jsi_Value *target, Jsi_IterObj *iterobj, int depth);

#ifdef  JSI_DEBUG_MEMORY
void jsi_VALCHK(Jsi_Value *val) {
    SIGASSERT(val,VALUE);
    assert(val->vt >= 0 && val->vt <= JSI_VT__MAX);
    if (val->vt == JSI_VT_OBJECT)
        OBJCHK(val->d.obj);
}
void jsi_OBJCHK(Jsi_Obj *obj) {
    SIGASSERT(obj,OBJ);
    assert(obj->ot >= 0 && obj->ot <= JSI_OT__MAX);
}
#endif

/*********************************************/

int Jsi_IsShared(Jsi_Interp* interp, Jsi_Value *v)
{
    SIGASSERT(v,VALUE);
    return (v->refCnt > 1);
}

int Jsi_IncrRefCount(Jsi_Interp* interp, Jsi_Value *v)
{
    SIGASSERT(v,VALUE);
    assert(v->refCnt>=0);
    return ++(v->refCnt);
}

int Jsi_DecrRefCount(Jsi_Interp* interp, Jsi_Value *v) {
    SIGASSERT(v,VALUE);
#ifndef HIDE_MEMLEAKS
    assert(v->refCnt>0);
#endif
    int ref = 1;
    if ((ref = --(v->refCnt)) == 0) {
        if (v->vt == JSI_VT_OBJECT) {
             Jsi_ObjDecrRefCount(interp, v->d.obj);
             v->vt = JSI_VT_UNDEF;
        }
        Jsi_ValueFree(interp, v);
    } else if (ref == 1 && v->f.bits.onstack) {
        v->f.bits.onstack = 0;
        Jsi_DecrRefCount(interp, v);
    }
    
    return ref;
}

static Jsi_Value *ValueNew(Jsi_Interp *interp)
{
    interp->valueCnt++;
    Jsi_Value *v = Jsi_Calloc(1,sizeof(*v));
    SIGINIT(v,VALUE)
    v->vt = JSI_VT_UNDEF;
    return v;
}

static Jsi_Value *ValueNew1(Jsi_Interp *interp)
{
    Jsi_Value *v = Jsi_ValueNew(interp);
    Jsi_IncrRefCount(interp, v);
    return v;
}

static Jsi_Value *ValueDup(Jsi_Interp *interp, Jsi_Value *v)
{
    Jsi_Value *r = Jsi_ValueNew(interp);
    Jsi_ValueCopy(interp,r, v);
    if (Jsi_IsShared(interp,v)) {
        r->refCnt = 1;
    }
    return r;
}
#ifndef VALUE_DEBUG
Jsi_Value* Jsi_ValueNew(Jsi_Interp *interp) {
    return ValueNew(interp);
}
Jsi_Value* Jsi_ValueNew1(Jsi_Interp *interp) {
    return ValueNew1(interp);
}
Jsi_Value *jsi_ValueDup(Jsi_Interp *interp, Jsi_Value *v) {
    return ValueDup(interp, v);
}
#else
#define UpdateValueDebug(interp, v) \
    v->fname = fname; \
    v->line = line; \
    v->func = func; \
    v->hPtr = Jsi_HashSet(interp->valueDebugTbl, v, 0); printf("VALDEF: %p\n", v)

Jsi_Value * jsi_ValueNew(Jsi_Interp *interp, const char *fname, int line, const char *func) {
    Jsi_Value *v = ValueNew(interp);
    UpdateValueDebug(interp, v);
    return v;
}

Jsi_Value * jsi_ValueNew1(Jsi_Interp *interp, const char *fname, int line, const char *func) {
    Jsi_Value *v = ValueNew1(interp);
    UpdateValueDebug(interp, v);
    return v;
}
Jsi_Value * jsi_ValueDup(Jsi_Interp *interp, Jsi_Value *ov, const char *fname, int line, const char *func) {
    Jsi_Value *v = ValueDup(interp, ov);
    UpdateValueDebug(interp, v);
    return v;
}
#endif

/* Reset a value back to undefined, releasing string/obj if necessary. */
void Jsi_ValueReset(Jsi_Interp *interp, Jsi_Value *v) {
    SIGASSERT(v,VALUE);
    Assert(v->vt >= 0 && v->vt <= JSI_VT__MAX);
    if(v->refCnt<=0)
        return;
    if (v->vt == JSI_VT_UNDEF)
        return;
    if (v->vt == JSI_VT_STRING) {
        if (v->d.s.str && !v->f.bits.isstrkey)
            Jsi_Free(v->d.s.str);
        v->f.bits.isstrkey = 0;
        v->d.s.str = NULL;
    } else if (v->vt == JSI_VT_OBJECT) {
        Jsi_ObjDecrRefCount(interp,v->d.obj);
    } else if (v->vt == JSI_VT_VARIABLE) {
       //Jsi_DecrRefCount(interp, v->d.lval);
    }
#ifdef VALUE_DEBUG
    v->hPtr = NULL;
#endif
    v->vt = JSI_VT_UNDEF;
    v->f.flag = 0;
    //v->refCnt = 1;
}

void Jsi_ValueFree(Jsi_Interp *interp, Jsi_Value* v)
{
    interp->valueCnt--;
#ifdef VALUE_DEBUG
    if (v->hPtr)
        Jsi_HashEntryDelete(v->hPtr);
#endif
    SIGASSERT(v,VALUE);
    Jsi_ValueReset(interp, v);
    MEMCLEAR(v);
    Jsi_Free(v);
}

void Jsi_ValueCopy(Jsi_Interp *interp, Jsi_Value *to, Jsi_Value *from )  {
    if (!from) {
        if (to->refCnt>1)
            Jsi_DecrRefCount(interp, to);
        to->refCnt = 1;
        to->vt = JSI_VT_UNDEF;
        return;
    }
    VALCHK(from);
    if( to == from) return;
    Jsi_ValueMakeUndef(interp, to);
    *to = *from;
    if (to->refCnt) {
        if (to->vt == JSI_VT_STRING) {
            if (!to->f.bits.isstrkey)
                to->d.s.str = Jsi_Strdup(to->d.s.str);
        } else if (to->vt == JSI_VT_OBJECT)
            Jsi_ObjIncrRefCount(interp,to->d.obj);
    }
    to->refCnt = 1;
#ifdef VALUE_DEBUG
    to->hPtr = NULL;
#endif
}

void Jsi_ValueReplace(Jsi_Interp *interp, Jsi_Value **to, Jsi_Value *from )  {
    VALCHK(from);
    if( *to == from) return;
    if (*to)
        Jsi_DecrRefCount(interp, *to);
    *to = from;
    Jsi_IncrRefCount(interp, from);
}

Jsi_Value *Jsi_ValueObjLookup(Jsi_Interp *interp, Jsi_Value *target, char *key, int isstrkey)
{
    Jsi_Obj *obj;
    Jsi_Value *v = NULL;
    if (target->vt != JSI_VT_OBJECT) {
        Jsi_LogWarn("Target is not object: %d\n", target->vt);
        return NULL;
    }
    obj = target->d.obj;
    if (*key == '_' && strcmp(key, "__proto__")==0)
        return obj->__proto__;
    if (*key == 't' && strcmp(key, "this")==0)
        return interp->inthis;
    if (obj->arr)
        v = jsi_ObjArrayLookup(interp, obj, key);
    if (!v)
        v= Jsi_TreeObjGetValue(obj, key, isstrkey);
    return v;  /* TODO: redo with copy */
}

Jsi_Value *Jsi_ValueArrayIndex(Jsi_Interp *interp, Jsi_Value *args, int index)
{
    char unibuf[12];
    Jsi_Obj *obj = args->d.obj;
    Jsi_Value *v;
    assert(args->vt == JSI_VT_OBJECT);
    if (obj->isArray && obj->arr) {
        if (index < 0 || index >= obj->arrCnt) {
            return NULL;
        }
        v = obj->arr[index];
        assert(v != args);
        return v;
    } else {
        jsi_num_itoa10(index, unibuf);
        v = Jsi_TreeObjGetValue(args->d.obj, unibuf, 0);
    } /* TODO: redo with copy */
    return v;
}


/**************************************************/

Jsi_Value* jsi_ValuesAlloc(Jsi_Interp *interp, int cnt, Jsi_Value*old, int oldsz) {
    Jsi_Value *v, vs = VALINIT;
    int i;
    v = Jsi_Realloc(old, cnt* sizeof(Jsi_Value));
    for (i=oldsz; i<cnt; i++)
        v[i] = vs;
    return v;
}

int Jsi_ValueGetBoolean(Jsi_Interp *interp, Jsi_Value *pv, int *val)
{
    if (!pv) return JSI_ERROR;
    if (pv->vt == JSI_VT_BOOL)
        *val = pv->d.val;
    else if (pv->vt == JSI_VT_OBJECT && pv->d.obj->ot == JSI_OT_BOOL)
        *val = pv->d.obj->d.val;
    else 
        return JSI_ERROR;
    return JSI_OK;
}

int Jsi_ValueIsArray(Jsi_Interp *interp, Jsi_Value *v)  {
    return (v->vt == JSI_VT_OBJECT && v->d.obj->ot == JSI_OT_OBJECT && v->d.obj->isArray);
}

int Jsi_ValueIsBoolean(Jsi_Interp *interp, Jsi_Value *pv)
{
    return (pv->vt == JSI_VT_BOOL || (pv->vt == JSI_VT_OBJECT && pv->d.obj->ot == JSI_OT_BOOL));
}

int Jsi_ValueIsNull(Jsi_Interp *interp, Jsi_Value *pv)
{
    return (pv->vt == JSI_VT_NULL);
}

int Jsi_ValueIsUndef(Jsi_Interp *interp, Jsi_Value *pv)
{
    return (pv->vt == JSI_VT_UNDEF);
}

int Jsi_ValueGetNumber(Jsi_Interp *interp, Jsi_Value *pv, Jsi_Number *val)
{
    if (!pv) return JSI_ERROR;
    if (pv->vt == JSI_VT_NUMBER)
        *val = pv->d.num;
    else if (pv->vt == JSI_VT_OBJECT && pv->d.obj->ot == JSI_OT_NUMBER)
        *val = pv->d.obj->d.num;
    else 
        return JSI_ERROR;
    return JSI_OK;
}
int Jsi_ValueIsNumber(Jsi_Interp *interp, Jsi_Value *pv)
{
    return (pv->vt == JSI_VT_NUMBER || (pv->vt == JSI_VT_OBJECT && pv->d.obj->ot == JSI_OT_NUMBER));
}

int Jsi_ValueIsString(Jsi_Interp *interp, Jsi_Value *pv)
{
    return (pv->vt == JSI_VT_STRING || (pv->vt == JSI_VT_OBJECT && pv->d.obj->ot == JSI_OT_STRING));
}

int Jsi_ValueIsFunction(Jsi_Interp *interp, Jsi_Value *v)
{
    return (v!=NULL && v->vt == JSI_VT_OBJECT && v->d.obj->ot == JSI_OT_FUNCTION && interp == v->d.obj->d.fobj->interp);
}

int Jsi_ValueIsType(Jsi_Interp *interp, Jsi_Value *pv, Jsi_vtype vtype) {
    if (!pv) return 0;
    return pv->vt == vtype;
}

Jsi_vtype Jsi_ValueTypeGet(Jsi_Value *pv) { return pv->vt; }


int Jsi_ValueIsObjType(Jsi_Interp *interp, Jsi_Value *v, Jsi_otype otype)
{
    return (v && v->vt == JSI_VT_OBJECT && v->d.obj->ot == otype);
}

/* TODO: resolve multiple toString() like functions. */
void Jsi_ValueToPrimitive(Jsi_Interp *interp, Jsi_Value *v)
{
    if (v->vt == JSI_VT_OBJECT) {
        Jsi_Value res = VALINIT;
        Jsi_Obj *obj = v->d.obj;
        switch(obj->ot) {
            case JSI_OT_BOOL:
                Jsi_ValueMakeBool(interp,&res, obj->d.val);
                break;
            case JSI_OT_NUMBER:
                Jsi_ValueMakeNumber(interp,&res, obj->d.num);
                break;
            case JSI_OT_STRING:
                if (obj->isstrkey) {
                    res.d.s = obj->d.s;
                    res.f.bits.isstrkey = 1;
                    obj->d.s.str = NULL;
                } else {
                    if (obj->d.s.len >= 0) 
                    {
                        Jsi_ValueMakeBlob(interp,&res, (void*)obj->d.s.str, obj->d.s.len);
#ifdef USE_VALCOPY
                    } else if (obj->refcnt==1) {
                        Jsi_ValueMakeString(interp, &res, obj->d.s.str);
                        res.d.s = obj->d.s;
                        obj->d.s.str = NULL;
#endif
                    } else
                        Jsi_ValueMakeString(interp, &res, Jsi_Strdup(obj->d.s.str));
                }
                break;
            case JSI_OT_FUNCTION: {
                Jsi_DString dStr;
                Jsi_DSInit(&dStr);
                Jsi_FuncObjToString(interp, obj, &dStr);
                Jsi_ValueMakeString(interp, &res, Jsi_Strdup(Jsi_DSValue(&dStr)));
                Jsi_DSFree(&dStr);
                break;
            }
            case JSI_OT_USEROBJ: {
                Jsi_DString dStr;
                Jsi_DSInit(&dStr);
                jsi_UserObjToName(interp, obj->d.uobj, &dStr);
                Jsi_ValueMakeString(interp, &res, Jsi_Strdup(Jsi_DSValue(&dStr)));
                Jsi_DSFree(&dStr);
                break;
            }
            default:
                Jsi_ValueMakeStringKey(interp,&res, "[object Object]");
                break;
        }
        Jsi_ValueReset(interp,v);
        *v = res;
    }
}

/* Return the string value.  Coerce value to a string type. */
const char* Jsi_ValueToString(Jsi_Interp *interp, Jsi_Value *v)
{
    Jsi_Number d;
    const char *ntxt = "undefined";
    if (!v) return ntxt;
    char unibuf[BUFSIZ];
    int kflag = 1;
    switch(v->vt) {
        case JSI_VT_STRING:
            return v->d.s.str;
        case JSI_VT_UNDEF:
            break;
        case JSI_VT_BOOL:
            ntxt = v->d.val ? "true":"false";
            break;
        case JSI_VT_NULL:
            ntxt = "null";
            break;
        case JSI_VT_NUMBER: {
            d = v->d.num;
fmtnum:
            if (jsi_is_integer(d)) {
                jsi_num_itoa10((int)d, unibuf);
                kflag = 0;
                ntxt = unibuf;
            } else if (jsi_ieee_isnormal(d)) {
                jsi_num_dtoa2(d, unibuf, 10);
                kflag = 0;
                ntxt = unibuf;
            } else if (jsi_ieee_isnan(v->d.num)) {
                ntxt = "NaN";
            } else {
                int s = jsi_ieee_infinity(d);
                if (s > 0) ntxt = "Infinity";
                else if (s < 0) ntxt = "-Infinity";
                else Jsi_LogBug("Ieee function got problem");
            }
            break;
        }
        case JSI_VT_OBJECT: {
            Jsi_Obj *obj = v->d.obj;
            switch(obj->ot) {
                case JSI_OT_STRING:
                    return obj->d.s.str;
                    break;
                case JSI_OT_BOOL:
                    ntxt = v->d.val ? "true":"false";
                    break;
                case JSI_OT_NUMBER:
                    d = obj->d.num;
                    goto fmtnum;
                    break;
                default:
                    ntxt = "[object Object]";
                    break;
            }
            break;
        }
        default:
            Jsi_LogBug("Convert a unknown type: 0x%x to string\n", v->vt);
            break;
    }
    Jsi_ValueReset(interp,v);
    if (kflag) {
        int isKey = 0;
        char *key = jsi_KeyFind(interp, ntxt, kflag, &isKey);
        if (isKey) {
            Jsi_ValueMakeStringKey(interp,v, key);
            return v->d.s.str;
        }
    }
    Jsi_ValueMakeString(interp,v, Jsi_Strdup(ntxt));
    return v->d.s.str;
}

Jsi_Number Jsi_ValueToNumberInt(Jsi_Interp *interp, Jsi_Value *v, int isInt)
{
    char *endPtr = NULL;
    Jsi_Number a = 0;
    switch(v->vt) {
        case JSI_VT_BOOL:
            a = (Jsi_Number)(v->d.val ? 1.0: 0);
            break;
        case JSI_VT_NULL:
            a = 0;
            break;
        case JSI_VT_OBJECT: {
            Jsi_Obj *obj = v->d.obj;
            switch(obj->ot) {
                case JSI_OT_BOOL:
                    a = (Jsi_Number)(obj->d.val ? 1.0: 0);
                    break;
                case JSI_OT_NUMBER:
                    a = obj->d.num;
                    break;
                case JSI_OT_STRING:
                    if (!isInt)
                        a = strtod(v->d.s.str, &endPtr);
                    else
                        a = (Jsi_Number)strtol(v->d.s.str, &endPtr, 0);
                    if (endPtr && *endPtr)
                        a = interp->NaNValue->d.num;
                    break;
                default:
                    a = 0;
                break;
            }
            break;
        }
        case JSI_VT_UNDEF:
            a = jsi_ieee_makenan();
            break;
        case JSI_VT_NUMBER:
            a = v->d.num;
            break;
        case JSI_VT_STRING:
            if (!isInt)
                a = strtod(v->d.s.str, &endPtr);
            else
                a = (Jsi_Number)strtol(v->d.s.str, &endPtr, 0);
            if (endPtr && *endPtr)
                a = interp->NaNValue->d.num;
            break;
        default:
            Jsi_LogBug("Convert a unknown type: 0x%x to number\n", v->vt);
            break;
    }
    return a;
}

Jsi_Number Jsi_ValueToNumber(Jsi_Interp *interp, Jsi_Value *v)
{
    if (v->vt == JSI_VT_NUMBER) return v->d.num;
    Jsi_Number a = Jsi_ValueToNumberInt(interp, v, 0);
    Jsi_ValueReset(interp,v);
    Jsi_ValueMakeNumber(interp,v, a);
    return a;
}

int Jsi_ValueToBool(Jsi_Interp *interp, Jsi_Value *v)
{
    int a = 0;
    switch(v->vt) {
        case JSI_VT_BOOL:
            return v->d.val;
            break;
        case JSI_VT_NULL:
            a = 0;
            break;
        case JSI_VT_UNDEF:
            a = 0;
            break;
        case JSI_VT_NUMBER:
            a = (v->d.num ? 1: 0);
            break;
        case JSI_VT_STRING:     /* TODO: NaN */
            a = atoi(v->d.s.str);
            a = (a ? 1 : 0);
            break;
        case JSI_VT_OBJECT: {
            Jsi_Obj *obj = v->d.obj;
            switch(obj->ot) {
                case JSI_OT_BOOL:
                    a = (obj->d.val ? 1.0: 0);
                    break;
                case JSI_OT_NUMBER:
                    a = obj->d.num;
                    a = (a ? 1 : 0);
                    break;
                case JSI_OT_STRING:
                    a = atoi(obj->d.s.str);
                    a = (a ? 1 : 0);
                    break;
                default:
                    a = 0;
                break;
            }
            break;
        }
        default:
            Jsi_LogBug("Convert a unknown type: 0x%x to number\n", v->vt);
            break;
    }
    Jsi_ValueReset(interp,v);
    Jsi_ValueMakeBool(interp,v, a);
    return a;
}

int jsi_ValueToOInt32(Jsi_Interp *interp, Jsi_Value *v)
{
    Jsi_Number a = Jsi_ValueToNumberInt(interp, v, 1);
    Jsi_ValueReset(interp,v);
    Jsi_ValueMakeNumber(interp,v, a);
    return (int)a;
}

void Jsi_ValueToObject(Jsi_Interp *interp, Jsi_Value *v)
{
    if (v->vt == JSI_VT_OBJECT) return;
    Jsi_Obj *o = Jsi_ObjNew(interp);
    switch(v->vt) {
        case JSI_VT_UNDEF:
        case JSI_VT_NULL:
            Jsi_LogError("Can not convert a undefined/null value to object\n");
            o->d.num = 0;
            o->ot = JSI_OT_NUMBER;
            o->__proto__ = interp->Number_prototype;
            break;
        case JSI_VT_BOOL: {
            o->d.val = v->d.val;
            o->ot = JSI_OT_BOOL;
            o->__proto__ = interp->Boolean_prototype;
            break;
        }
        case JSI_VT_NUMBER: {
            o->d.num = v->d.num;
            o->ot = JSI_OT_NUMBER;
            o->__proto__ = interp->Number_prototype;
            break;
        }
        case JSI_VT_STRING: {
            o->d.s = v->d.s;
            if (!v->f.bits.isstrkey)
                o->d.s.str = (char*)Jsi_KeyAdd(interp, v->d.s.str);
            o->isstrkey = 1;
            o->ot = JSI_OT_STRING;
            o->__proto__ = interp->String_prototype;
            break;
        }
        default:
            Jsi_LogBug("toobject, not suppose to reach here\n");
    }
    Jsi_ValueReset(interp,v);
    Jsi_ValueMakeObject(interp,v, o);
}

/* also toBoolean here, in ecma */
int Jsi_ValueIsTrue(Jsi_Interp *interp, Jsi_Value *v)
{
    switch(v->vt) {
        case JSI_VT_UNDEF:
        case JSI_VT_NULL:   return 0;
        case JSI_VT_BOOL:   return v->d.val ? 1:0;
        case JSI_VT_NUMBER: 
            if (v->d.num == 0.0 || jsi_ieee_isnan(v->d.num)) return 0;
            return 1;
        case JSI_VT_STRING: return Jsi_ValueStrlen(v) ? 1 : 0;
        case JSI_VT_OBJECT: {
            Jsi_Obj *o = v->d.obj;
            if (o->ot == JSI_OT_USEROBJ && o->d.uobj->interp == interp) {
                return jsi_UserObjIsTrue(interp, o->d.uobj);
            }
            return 1;
        }
        default: Jsi_LogBug("TOP is type incorrect: %d\n", v->vt);
    }
    return 0;
}

int Jsi_ValueIsFalse(Jsi_Interp *interp, Jsi_Value *v)
{
    if (v->vt == JSI_VT_BOOL)  return v->d.val ? 0:1;
    return 0;
}

int Jsi_ValueCmp(Jsi_Interp *interp, Jsi_Value *v1, Jsi_Value* v2, int flags)
{
    int r = 1;
    int nocase = (flags&JSI_SORT_NOCASE), dict = ((flags & JSI_SORT_ASCII)==0);
    if (v1->vt != v2->vt) {
        Jsi_ValueToPrimitive(interp, v1);
        Jsi_ValueToPrimitive(interp, v2);
    }
    if (v1->vt != v2->vt) {
        if ((flags&JSI_CMP_EXACT))
            return 1;
        if ((v1->vt == JSI_VT_UNDEF || v1->vt == JSI_VT_NULL) && 
            (v2->vt == JSI_VT_UNDEF || v2->vt == JSI_VT_NULL)) {
            r = 0;
        } else {
            Jsi_Value vs1=VALINIT, vs2=VALINIT;
            Jsi_ValueCopy(interp, &vs1, v1);
            Jsi_ValueCopy(interp, &vs2, v2);
            Jsi_ValueToNumber(interp, &vs1);
            Jsi_ValueToNumber(interp, &vs2);
            r = (vs2.d.num - vs1.d.num);
            Jsi_ValueMakeUndef(interp, &vs1);
            Jsi_ValueMakeUndef(interp, &vs2);
        }
    } else {
        switch (v1->vt) {
            case JSI_VT_NUMBER:
                r = (v2->d.num - v1->d.num);
                break;
            case JSI_VT_BOOL:
                r = (v2->d.val - v1->d.val);
                break;
            case JSI_VT_STRING:
                r = (Jsi_StrcmpDict(v2->d.s.str, v1->d.s.str, nocase, dict));
                break;
            case JSI_VT_OBJECT:
                /* TODO: refer to objects joined to each other */
                
                r = (v2->d.obj - v1->d.obj);
                break;
            case JSI_VT_UNDEF:
            case JSI_VT_NULL:
                r = 0;
                break;
            default:
                Jsi_LogBug("Unexpected value type\n");
        }
    }
    return r;
}

/**
 * @brief Split a string.
 * @param interp 
 * @param str - input string to split
 * @param split - to split on
 * @returns an array of string values
 * 
 * 
 */
Jsi_Value *Jsi_StringSplit(Jsi_Interp *interp, char *str, char *spliton)
{
    char **argv; int argc;
    Jsi_DString dStr;
    Jsi_DSInit(&dStr);
    Jsi_SplitStr(str, &argc, &argv, spliton, &dStr);
    Jsi_Value *nret = Jsi_ValueNewArray(interp, NULL, 0);
    Jsi_Obj *obj = nret->d.obj;
    int i;
    for (i = 0; i < argc; ++i) {
        Jsi_ObjArraySet(interp, obj, Jsi_ValueNewStringDup(interp, argv[i]), i);
    }
    Jsi_ObjSetLength(interp, obj, argc);
    Jsi_ValueMakeArrayObject(interp, nret, obj);
    Jsi_DSFree(&dStr);
    return nret;
}

void jsi_ValueObjSet(Jsi_Interp *interp, Jsi_Value *target, const char *key, Jsi_Value *value, int flags, int isstrkey)
{
    Jsi_TreeEntry *hPtr;
    if (target->vt != JSI_VT_OBJECT) {
        Jsi_LogWarn("Target is not object: %d\n", target->vt);
        return;
    }
    hPtr = Jsi_ObjInsert(interp, target->d.obj, key, value, isstrkey);
    if (!hPtr)
        return;
    hPtr->f.flags |= (flags&0x7f); /* TODO:  Define a standard mask??? */
}

Jsi_Value *jsi_ValueObjKeyAssign(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *keyval, Jsi_Value *value, int flag)
{
    int arrayindex = -1;

    if (keyval->vt == JSI_VT_NUMBER && jsi_is_integer(keyval->d.num) && keyval->d.num >= 0) {
        arrayindex = (int)keyval->d.num;
    }
    /* TODO: array["1"] also extern the length of array */
    
    if (arrayindex >= 0 && arrayindex < MAX_ARRAY_LIST &&
        target->vt == JSI_VT_OBJECT && target->d.obj->arr) {
        return jsi_ObjArraySetDup(interp, target->d.obj, value, arrayindex);
    }
    Jsi_ValueToString(interp, keyval);

    if (strcmp(keyval->d.s.str, "__proto__")==0) {
        Jsi_Obj *obj = target->d.obj;
        Jsi_ValueCopy(interp,obj->__proto__, value);
        return obj->__proto__;
    }
    Jsi_Value *v;
    if (value)
        Jsi_ValueCopy(interp,v = Jsi_ValueNew(interp), value);
    else
        value = (v = Jsi_ValueNew1(interp));

    jsi_ValueObjSet(interp, target, keyval->d.s.str, v, flag, (keyval->f.bits.isstrkey? JSI_OM_ISSTRKEY:0));
    //Jsi_DecrRefCount(interp, v);
    return v;
}

void jsi_ValueSubscript(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *key, Jsi_Value *ret, int right_val)
{
    if (!target) {
        Jsi_ValueMakeUndef(interp,ret);
        return;
    }
    
    if (target->vt != JSI_VT_OBJECT) {
        Jsi_LogBug("subscript operand is not object\n");
    }
    Jsi_ValueToString(interp, key);
    
    Jsi_Value *r = Jsi_ValueObjLookup(interp, target, key->d.s.str, (key->f.bits.isstrkey? 1 : 0));
    if (!r) {
        /* query from prototype, always no right_val */
        if (target->d.obj->__proto__) {
            jsi_ValueSubscript(interp, target->d.obj->__proto__, key, ret, 1);
        }
        if (right_val == 0) {       /* need a lvalue */
            Jsi_Value *n = Jsi_ValueNew(interp);
            Jsi_ValueCopy(interp,n, ret);   /* copy from prototype */
            
            jsi_ValueObjKeyAssign(interp, target, key, n, 0);

            Jsi_ValueReset(interp,ret);
            ret->vt = JSI_VT_VARIABLE;
            ret->d.lval = n;
            Jsi_IncrRefCount(interp, n);
            SIGASSERT(n, VALUE);
        }
    } else {
        if (right_val || r->f.bits.readonly) {
            Jsi_ValueCopy(interp,ret, r);
        } else {
            ret->vt = JSI_VT_VARIABLE;
            ret->d.lval = r;
            Jsi_IncrRefCount(interp, r);
            SIGASSERT(r, VALUE);
        }
    }
}

/* Like above, but handle "length", "constructor", "String", etc specially. */
void jsi_ValueSubscriptLen(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *key,
                           Jsi_Value *ret, int right_val)
{
    int len;
    jsi_ValueSubscript(interp, target, key, ret, right_val);
    if (ret->vt != JSI_VT_UNDEF || key->vt != JSI_VT_STRING) {
        /* TODO: generalize this so that eval knows who is the parent object of a lookup. */
        if (Jsi_ValueIsFunction(interp, target) && key->vt == JSI_VT_STRING && !strcmp(key->d.s.str, "bind"))
            interp->lastBindSubscriptObj = target->d.obj;
        return;
    }
    if (Jsi_Strcmp(key->d.s.str,"length")==0) {
        if (Jsi_ValueIsString(interp, target)) {
            len = Jsi_ValueStrlen(target);
        } else if (target->vt == JSI_VT_OBJECT && target->d.obj->isArray /*&& target->d.obj->arr*/) {
            /* && target->d.obj->ot == JSI_OT_ARRAY   ??? */
            len = target->d.obj->arrCnt;
        } else if (target->vt == JSI_VT_OBJECT && target->d.obj->ot == JSI_OT_FUNCTION) {
            if (target->d.obj->d.fobj->func->type == FC_NORMAL)
                len = target->d.obj->d.fobj->func->argnames->count;
            else
                len = target->d.obj->d.fobj->func->cmdSpec->maxArgs;
        } else {
            return;
        }
        ret->vt = JSI_VT_NUMBER;
        ret->d.num = (Jsi_Number)len;
        return;
    }

    if (target->vt == JSI_VT_OBJECT && Jsi_Strcmp(key->d.s.str,"constructor")==0) {
        char *cp;
        Jsi_Obj *o = target->d.obj->constructor;
        if (o) {
            if (o->ot == JSI_OT_FUNCTION) {
                Jsi_Value *proto = Jsi_TreeObjGetValue(o, "prototype", 0);
                if (proto && proto->vt == JSI_VT_OBJECT && proto->d.obj->constructor) {
                    o = proto->d.obj->constructor;
                }
            }
        } else {
            switch(target->d.obj->ot) {
            case JSI_OT_NUMBER:
                cp = "Number";
                break;
            case JSI_OT_BOOL:
                cp = "Boolean";
                break;
            case JSI_OT_STRING:
                cp = "String";
                break;
            default:
                Jsi_ValueMakeUndef(interp, ret);
                return;
            }
            Jsi_Value *v = Jsi_ValueObjLookup(interp, interp->csc, cp, 0);
            if (v==NULL || v->vt != JSI_VT_OBJECT)
                return;
            o = target->d.obj->constructor = v->d.obj;
        }
        Jsi_ValueMakeObject(interp, ret, o);
        return;
    }

    if (target->vt == JSI_VT_OBJECT && target->d.obj->ot == JSI_OT_FUNCTION) {
        /* Looking up something like "String.charFromCode()" */
        /* Need to do this as JSI doesn't insert cmd keys into proto to reduce overhead . */
        Jsi_Func* func = target->d.obj->d.fobj->func;
        int isInterp = 0;
        if (func->f.bits.iscons && func->name && (strcmp(func->name,"String")==0 ||
                strcmp(func->name,"Number")==0 || strcmp(func->name,"Regexp")==0 || (isInterp=(strcmp(func->name,"Interp")==0)) )) {
            Jsi_Value *v = Jsi_ValueObjLookup(interp, interp->csc, (char*)func->name, 0);
            if (v && ((v = Jsi_ValueObjLookup(interp, v, "prototype", 0)))
                    && ((v = Jsi_ValueObjLookup(interp, v, key->d.s.str, 0)))) {
                if (v->vt == JSI_VT_OBJECT && v->d.obj->ot == JSI_OT_FUNCTION && !isInterp) {
                    Jsi_Func* sfunc = v->d.obj->d.fobj->func;
                    sfunc->callflags.bits.addargs = 1;
                }
                Jsi_ValueCopy(interp, ret, v);
                return;
            }
        }
        if (func->type == FC_BUILDIN) {
            if (Jsi_ValueIsString(interp, key)) {
                char *kstr = Jsi_ValueString(interp, key, NULL);
                if (0==strcmp(kstr,"call") || 0==strcmp(kstr,"apply")) {
                    char fbuf[100];
                    strcpy(fbuf, "Function.");
                    strcat(fbuf,kstr);
                    Jsi_Value *vv = Jsi_NameLookup(interp, fbuf);
                    if (vv) {
                        Jsi_ValueCopy(interp, ret, vv);
                        Jsi_ValueInsertFixed(interp, target, kstr, vv);
                        return;
                    }
                }
            }
            /* Lookup failed, save cmdSpec in case a function call */
            interp->lastFuncIndex = func;
        }
        return;
    }

}

int Jsi_ValueKeyPresent(Jsi_Interp *interp, Jsi_Value *target, const char *key, int isstrkey)
{
    SIGASSERT(interp,INTERP);
    //SIGASSERT(target,VALUE);
    if (Jsi_TreeObjGetValue(target->d.obj, key, isstrkey))
        return 1;
    if (target->d.obj->__proto__ == NULL || target->d.obj->__proto__ == target)
        return 0;
    return Jsi_ValueKeyPresent(interp, target->d.obj->__proto__, key, isstrkey);
}

void jsi_ValueObjGetKeys(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *ret)
{
    Jsi_IterObj *io = jsi_IterObjNew(interp, NULL);
    Jsi_Obj *to = target->d.obj;
    
    if (target->vt != JSI_VT_UNDEF && target->vt != JSI_VT_NULL) {

        if (target->vt == JSI_VT_OBJECT && to->arr) {
            io->isArrayList = 1;
            io->count = to->arrCnt;
        } else {
            IterGetKeys(interp, target, io, 0);
        }
    }
    io->obj = to;
    Jsi_Obj *r = Jsi_ObjNew(interp);
    r->ot = JSI_OT_ITER;
    r->d.iobj = io;
    Jsi_ValueMakeObject(interp,ret, r);
}

int Jsi_ValueGetKeys(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *ret)
{
    int i, n = 0;
    Jsi_IterObj *io;
    if (target->vt != JSI_VT_OBJECT)
        return JSI_ERROR;
    Jsi_Obj *to = target->d.obj;
    Jsi_Obj *r = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    Jsi_ValueMakeArrayObject(interp, ret, r);
    if (to->arr) {
        for (i=0; i<to->arrCnt; i++)
            if (to->arr[i]) n++;
        if (jsi_ArraySizer(interp, r, n) <= 0) {
            Jsi_LogError("too long");
            Jsi_ValueMakeUndef(interp, ret);
            return JSI_ERROR;
        }
        for (i=0, n=0; i<to->arrCnt; i++) {
            if (to->arr[i]) {
                r->arr[n] = Jsi_ValueNewNumber(interp, (Jsi_Number)i);
                Jsi_IncrRefCount(interp, r->arr[n]);
                n++;
            }
        }
        r->arrCnt = n;
        return JSI_OK;
    }
    io = jsi_IterObjNew(interp, NULL);
    IterGetKeys(interp, target, io, 0);
    if (jsi_ArraySizer(interp, r, io->count) <= 0) {
        Jsi_LogError("too long");
        Jsi_ValueMakeUndef(interp, ret);
        return JSI_ERROR;
    }
    for (i=0; i<io->count; i++) {
        r->arr[i] = (io->keys[i] ? Jsi_ValueNewStringKey(interp, io->keys[i]) : NULL);
        Jsi_IncrRefCount(interp, r->arr[i]);
    }
    io->count = 0;
    r->arrCnt = i;
    jsi_IterObjFree(io);
    return JSI_OK;
}

jsi_ScopeChain *jsi_ScopeChainNew(Jsi_Interp *interp, int cnt)
{
    jsi_ScopeChain *r = Jsi_Calloc(1, sizeof(*r));
    r->interp = interp;
    SIGINIT(r,SCOPE);
    r->chains = Jsi_Calloc(cnt, sizeof(r->chains[0]));
    r->chains_cnt = cnt;
    return r;
}

Jsi_Value *jsi_ScopeChainObjLookupUni(jsi_ScopeChain *sc, char *key)
{
    int i;
    Jsi_Value *ret;
    for (i = sc->chains_cnt - 1; i >= 0; --i) {
        if ((ret = Jsi_ValueObjLookup(sc->interp, sc->chains[i], key, 0))) {
            return ret;
        }
    }
    return NULL;
}

jsi_ScopeChain *jsi_ScopeChainDupNext(Jsi_Interp *interp, jsi_ScopeChain *sc, Jsi_Value *next)
{
    if (!sc) {
        jsi_ScopeChain *nr = jsi_ScopeChainNew(interp, 1);
        nr->chains[0] = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp,(nr->chains[0]), next);
        nr->chains_cnt = 1;
        return nr;
    }
    jsi_ScopeChain *r = jsi_ScopeChainNew(interp, sc->chains_cnt + 1);
    int i;
    for (i = 0; i < sc->chains_cnt; ++i) {
        r->chains[i] = Jsi_ValueNew1(interp);
        Jsi_ValueCopy(interp,(r->chains[i]), sc->chains[i]);
    }
    r->chains[i] = Jsi_ValueNew1(interp);
    Jsi_ValueCopy(interp,(r->chains[i]), next);
    r->chains_cnt = i + 1;
    return r;
}

void jsi_ScopeChainFree(Jsi_Interp *interp, jsi_ScopeChain *sc)
{
    int i;
    for (i = 0; i < sc->chains_cnt; ++i) {
        Jsi_DecrRefCount(interp, sc->chains[i]);
    }
    Jsi_Free(sc->chains);
    Jsi_Free(sc);
}

int Jsi_ValueGetLength(Jsi_Interp *interp, Jsi_Value *v)
{
    if (!v) return JSI_ERROR;
    if (v->vt != JSI_VT_OBJECT)
        return 0;
    
    return Jsi_ObjGetLength(interp, v->d.obj);
}

char *Jsi_ValueArrayIndexToStr(Jsi_Interp *interp, Jsi_Value *args, int index, int *lenPtr)
{
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, index);
    if (!arg)
        return NULL;
    char *res = Jsi_ValueString(interp, arg, lenPtr);
    if (res)
        return res;
    res = (char*)Jsi_ValueToString(interp, arg);
    if (res && lenPtr)
        *lenPtr = Jsi_Strlen(res);
    return res;
}

void Jsi_ValueInsert(Jsi_Interp *interp, Jsi_Value *target, const char *key, Jsi_Value *val, int flags)
{
    if (target == NULL)
        target = interp->csc;
    if (target->vt != JSI_VT_OBJECT) {
        Jsi_LogWarn("Target is not object\n");
        return ;
    }
    target->f.flag |= flags;
    Jsi_ObjInsert(interp, target->d.obj, key, val, flags);
}

void jsi_ValueInsertArray(Jsi_Interp *interp, Jsi_Value *target, int key, Jsi_Value *val, int flags)
{
    if (target->vt != JSI_VT_OBJECT) {
        Jsi_LogWarn("Target is not object\n");
        return;
    }
    Jsi_Obj *obj = target->d.obj;
    
    if (obj->isArray) {
        if (key >= 0 && key < interp->maxArrayList) {
            jsi_ObjArraySetDup(interp, obj, val, key);
            return;
        }
    }
    char unibuf[12];
    jsi_num_itoa10(key, unibuf);
    Jsi_ObjInsert(interp, obj, unibuf, val, flags);
}

/* OBJ INTERFACE TO BTREE */

static void IterObjInsert(Jsi_IterObj *io, Jsi_TreeEntry *hPtr)
{
    const char *key = Jsi_TreeKeyGet(hPtr);
    assert(!io->isArrayList);
    if (io->depth) {
        int i;
        for (i=0; i<io->count; i++) {
            if (!strcmp(key, io->keys[i]))
                return;
        }
    }

    if (io->count >= io->size) {
        io->size += 10;
        io->keys = Jsi_Realloc(io->keys, io->size * sizeof(io->keys[0]));
    }
    io->keys[io->count] = Jsi_TreeKeyGet(hPtr);
    io->count++;
}

Jsi_TreeEntry * Jsi_ObjInsert(Jsi_Interp *interp, Jsi_Obj *obj, const char *key, Jsi_Value *val, int flags)
{
    Jsi_TreeEntry *hPtr;
    if (val)
        Jsi_IncrRefCount(interp, val);
    hPtr = Jsi_TreeObjSetValue(obj, key, val, (flags&JSI_OM_ISSTRKEY));
    if ((flags&JSI_OM_DONTDEL)) hPtr->f.bits.dontdel = 1;
    if ((flags&JSI_OM_READONLY))  hPtr->f.bits.readonly = 1;
    if ((flags&JSI_OM_DONTENUM))  hPtr->f.bits.dontenum = 1;
    return hPtr;
}

static int IterGetKeysCallback(Jsi_Tree* tree, Jsi_TreeEntry *hPtr, void *data)
{
    Jsi_IterObj *io = data;
    if (!hPtr->f.bits.dontenum) {
        IterObjInsert(io, hPtr);
    }
    return JSI_OK;
}

static void IterGetKeys(Jsi_Interp *interp, Jsi_Value *target, Jsi_IterObj *iterobj, int depth)
{
    if (!target) return;
    if (target->vt != JSI_VT_OBJECT) {
        Jsi_LogWarn("operand is not a object\n");
        return;
    }
    iterobj->depth = depth;
    Jsi_TreeWalk(target->d.obj->tree, IterGetKeysCallback, iterobj, 0);
    if (target->d.obj->__proto__ && target != target->d.obj->__proto__)
        IterGetKeys(interp, target->d.obj->__proto__, iterobj, depth+1);
    iterobj->depth = depth;
}

Jsi_Value* Jsi_ValueMakeDStringObject(Jsi_Interp *interp, Jsi_Value *v, Jsi_DString *dsPtr)  {
    Jsi_Obj *obj;
    if (!v)
        v = Jsi_ValueNew(interp);
    else {
        assert(v->vt >= 0 && v->vt <= JSI_VT__MAX);
        if (v->vt == JSI_VT_OBJECT && v->d.obj->ot == JSI_OT_STRING
#ifdef USE_VALCOPY
            && v->d.obj->refcnt == 1
#endif
        ) {
            Jsi_ObjFromDS(dsPtr, v->d.obj);
            return v;
        }
        Jsi_ValueReset(interp, v);
    }
    obj = Jsi_ObjNewType(interp, JSI_OT_STRING);
    Jsi_ObjFromDS(dsPtr, obj);
    Jsi_ValueMakeObject(interp, v, obj);
    return v;
}

Jsi_Value* Jsi_ValueMakeObject(Jsi_Interp *interp, Jsi_Value *v, Jsi_Obj *o)  {
    if (v && v->vt == JSI_VT_OBJECT && o == v->d.obj)
        return v;
    if (v)
        Jsi_ValueReset(interp, v);
    else
        v = Jsi_ValueNew(interp);
    //Jsi_IncrRefCount(interp, v);
    if (!o)
        o = Jsi_ObjNewType(interp, JSI_VT_OBJECT);
    v->vt = JSI_VT_OBJECT;
    v->d.obj = o;
    Jsi_ObjIncrRefCount(interp,v->d.obj);
    return v;
}

Jsi_Value* Jsi_ValueMakeArrayObject(Jsi_Interp *interp, Jsi_Value *v, Jsi_Obj *o)  {
    if (!o)
        o = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    if (!v)
        v = Jsi_ValueNew(interp);
    else {
       if (v->vt == JSI_VT_OBJECT && o == v->d.obj) {
            if (!o->isArray) {
                if (o->tree)
                    Jsi_TreeDelete( o->tree);
                o->tree = NULL;
                o->__proto__ = interp->Array_prototype;
                o->isArray = 1;
            }
            return v;
        }
        Jsi_ValueReset(interp, v);
    }
    v->vt = JSI_VT_OBJECT;
    v->d.obj = o;
    o->ot = JSI_OT_OBJECT;
    o->__proto__ = interp->Array_prototype;
    o->isArray = 1;
    jsi_ArraySizer(interp, o, 0);
    Jsi_ObjIncrRefCount(interp,v->d.obj);
    return v;
}

Jsi_Value* Jsi_ValueMakeNumber(Jsi_Interp *interp, Jsi_Value *v, Jsi_Number n) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    v->vt = JSI_VT_NUMBER;
    v->d.num = n;
    return v;
}

Jsi_Value* Jsi_ValueMakeBool(Jsi_Interp *interp, Jsi_Value *v, int b) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    v->vt = JSI_VT_BOOL;
    v->d.val = b;
    return v;
}

Jsi_Value* Jsi_ValueMakeBlob(Jsi_Interp *interp, Jsi_Value *v, unsigned char *s, int len) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    Jsi_ValueMakeString(interp, v, (char*)s);
    if (len<0) return v;
    char *cp = Jsi_Malloc(len+1);
    memcpy(cp, s, len);
    cp[len] = 0;
    v->d.s.str = cp;
    v->d.s.len = len;
    return v;
}

Jsi_Value* Jsi_ValueMakeString(Jsi_Interp *interp, Jsi_Value *v, const char *s) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    v->vt = JSI_VT_STRING;
    v->d.s.str = (char*)s;
    v->d.s.len = -1;
    return v;
}

Jsi_Value* Jsi_ValueMakeStringDup(Jsi_Interp *interp, Jsi_Value *v, const char *s) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    if (*s) {
        v->vt = JSI_VT_STRING;
        v->d.s.str = Jsi_Strdup(s);
        v->d.s.len = -1;
        return v;
    }
    v->vt = JSI_VT_STRING;
    v->d.s.str = (char*)Jsi_KeyAdd(interp,s);
    v->d.s.len = -1;
    v->f.bits.isstrkey = 1;
    return v;
}

Jsi_Value* Jsi_ValueMakeStringKey(Jsi_Interp *interp, Jsi_Value *v, const char *s) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    v->vt = JSI_VT_STRING;
    v->d.s.str = (char*)Jsi_KeyAdd(interp,s);
    v->d.s.len = -1;
    v->f.bits.isstrkey = 1;
    return v;
}

Jsi_Value* Jsi_ValueMakeNull(Jsi_Interp *interp, Jsi_Value *v) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else
        Jsi_ValueReset(interp, v);
    v->vt = JSI_VT_NULL;
    return v;
}

Jsi_Value* Jsi_ValueMakeUndef(Jsi_Interp *interp, Jsi_Value *v) {
    if (!v)
        v = Jsi_ValueNew(interp);
    else {
        if (v->vt == JSI_VT_UNDEF) return v;
        Jsi_ValueReset(interp, v);
    }
    return v;
}

Jsi_Value* Jsi_ValueNewNumber(Jsi_Interp *interp, Jsi_Number n) {
    Jsi_Value *v = Jsi_ValueNew(interp);
    v->vt = JSI_VT_NUMBER;
    v->d.num = n;
    return v;
}


Jsi_Value* Jsi_ValueNewString(Jsi_Interp *interp, const char *s) {
    Jsi_Value *v = Jsi_ValueNew(interp);
    v->vt = JSI_VT_STRING;
    v->d.s.str = (char*)s;
    v->d.s.len = -1;
    return v;
}

Jsi_Value* Jsi_ValueNewStringDup(Jsi_Interp *interp, const char *s) {
    return Jsi_ValueNewString(interp, Jsi_Strdup(s));
}

Jsi_Value* Jsi_ValueNewStringKey(Jsi_Interp *interp, const char *s) {
    Jsi_Value *v = Jsi_ValueNew(interp);
    v->vt = JSI_VT_STRING;
    v->d.s.str = (char*)Jsi_KeyAdd(interp,s);
    v->d.s.len = -1;
    v->f.bits.isstrkey = 1;
    return v;
}

Jsi_Value* Jsi_ValueNewBlob(Jsi_Interp *interp, unsigned char *s, int len) {
    Jsi_Value *v = Jsi_ValueNew(interp);
    v->vt = JSI_VT_STRING;
    v->d.s.str = Jsi_Malloc(len+1);
    memcpy(v->d.s.str, (char*)s, len);
    v->d.s.str[len] = 0;
    v->d.s.len = len;
    return v;
}

Jsi_Value* Jsi_ValueNewBoolean(Jsi_Interp *interp, int bval) {
    Jsi_Value *v = Jsi_ValueNew(interp);
    v->vt = JSI_VT_BOOL;
    v->d.val = bval;
    return v;
}

Jsi_Value* Jsi_ValueNewNull(Jsi_Interp *interp) {
    Jsi_Value *v = Jsi_ValueNew(interp);
    v->vt = JSI_VT_NULL;
    return v;
}

Jsi_Value *Jsi_ValueNewArray(Jsi_Interp *interp, char **items, int count)
{
    Jsi_Obj *obj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    int i = 0;
    if (count<0) {
        count = 0;
        while (items[count])
            count++;
    }
    if (jsi_ArraySizer(interp, obj, count) <= 0) {
        Jsi_ObjFree(interp, obj);
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        obj->arr[i] = Jsi_ValueNewStringDup(interp, items[i]);
        Jsi_IncrRefCount(interp, obj->arr[i]);
    }
    obj->arrCnt = count;
    assert(obj->arrCnt<=obj->arrMaxSize);
    return Jsi_ValueMakeArrayObject(interp, NULL, obj);
}

Jsi_Obj *Jsi_ValueGetObj(Jsi_Interp *interp, Jsi_Value* v)
{
    if (v->vt == JSI_VT_OBJECT) {
        return v->d.obj;
    }
    return NULL;
}

int Jsi_ValueStrlen(Jsi_Value* v) {
    Jsi_String *s = JSIVAL_GET_STR(v);
    if (s == 0 || s->str == 0)
        return 0;
    return (s->len>=0 ? s->len : Jsi_Strlen(s->str));
}

char *Jsi_ValueString(Jsi_Interp *interp, Jsi_Value* v, int *lenPtr)
{
    if (!v) return NULL;
    Jsi_String *s = JSIVAL_GET_STR(v);
    if (s) {
        if (lenPtr)
            *lenPtr = (s->len<0 ? Jsi_Strlen(s->str) : s->len);
        return s->str;
    }
    if (lenPtr)
        *lenPtr = 0;
    return NULL;
}

char* Jsi_ValueGetStringLen(Jsi_Interp *interp, Jsi_Value *pv, int *lenPtr)
{
    if (!pv)
        return NULL;
    Jsi_String *s = JSIVAL_GET_STR(pv);
    if (!s)
        return NULL;
    if (lenPtr)
        *lenPtr = (s->len<0 ? Jsi_Strlen(s->str) : s->len);
    return s->str;
}

int Jsi_ValueInstanceOf( Jsi_Interp *interp, Jsi_Value* v1, Jsi_Value* v2)
{
    Jsi_Value *proto, *sproto;
    if (v1->vt != JSI_VT_OBJECT || v2->vt != JSI_VT_OBJECT  || v2->d.obj->ot != JSI_OT_FUNCTION)
        return 0;
    proto = Jsi_ValueObjLookup(interp, v2, "prototype", 0);
    if (!proto)
        return 0;
    sproto = v1->d.obj->__proto__ ;
    while (sproto) {
        if (sproto == proto)
            return 1;
        if (sproto->vt != JSI_VT_OBJECT)
            return 0;
        sproto = sproto->d.obj->__proto__;
    }
    return 0;
}


int jsi_ValueInit(Jsi_Interp *interp)
{
    return JSI_OK;
}


/* Return null terminated string, duplicated if still static.  Resets to empty/init (free). */
static char *DSDupReset(Jsi_DString *dsPtr)
{
    char *cp;
    Jsi_DSInit(dsPtr);
    if (dsPtr->str != dsPtr->staticSpace) {
        cp = dsPtr->str;
        dsPtr->str = dsPtr->staticSpace;
    } else {
        cp = Jsi_Strdup(dsPtr->str);
    }
    dsPtr->len = 0;
    dsPtr->spaceAvl = dsPtr->staticSize;
    dsPtr->staticSpace[0] = 0;
    return cp;
}

void  Jsi_ValueFromDS(Jsi_Interp *interp, Jsi_DString *dsPtr, Jsi_Value *ret)
{
    char *cp;
    cp = DSDupReset(dsPtr);
    if (cp)
        Jsi_ValueMakeString(interp, ret, cp);
}

#endif
