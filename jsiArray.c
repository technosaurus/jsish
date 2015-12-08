#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

#ifdef HAVE_MUSL
#define NO_QSORT_R 1
#endif

#define STHIS() \
    Jsi_Value *sthis = Jsi_ValueArrayIndex(interp, args, 1); \
    Jsi_Value nthis = VALINIT;\
    if (!sthis) sthis = &nthis

static int ArrayPushCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *v;
    Jsi_Obj *obj;
    
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    obj = _this->d.obj;
    
    int argc = Jsi_ValueGetLength(interp, args);
    int curlen = Jsi_ObjGetLength(interp, obj);
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    
    int i;
    for (i = 0; i < argc; ++i) {
        v = Jsi_ValueNew(interp);
        Jsi_Value *ov = Jsi_ValueArrayIndex(interp, args, i);
        if (!ov) Jsi_LogBug("Arguments Jsi_LogError\n");

        Jsi_ValueCopy(interp,v, ov);

        jsi_ValueInsertArray(interp, _this, curlen + i, v, 0);
    }
    
    Jsi_ValueMakeNumber(interp, *ret, Jsi_ObjGetLength(interp, obj));
    return JSI_OK;
}

static int ArrayPopCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *v;
    Jsi_Obj *obj;
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    obj = _this->d.obj;
    int i = Jsi_ObjGetLength(interp, obj) - 1;

    if (i < 0) {
        Jsi_ValueMakeUndef(interp,*ret);
        return JSI_OK;
    }
    
    if (obj->arr) {
        if ((v = obj->arr[i])) {
            obj->arr[i] = NULL;
            obj->arrCnt--;
        }
    } else {
        v = Jsi_ValueArrayIndex(interp, _this, i);
    }
    if (v) {
        Jsi_ValueCopy(interp, *ret, v);
        Jsi_ValueReset(interp,v);    /* diff from ecma, not actually delete the key */
    }
    Jsi_ObjSetLength(interp, obj, i);
    return JSI_OK;
}


static int ArrayJoinCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    const char *jstr = "";
    int argc;
     if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        goto bail;
    }
    int curlen = Jsi_ObjGetLength(interp, _this->d.obj);
    if (curlen == 0) {
        goto bail;
    }

    if (Jsi_ValueGetLength(interp, args) >= 1) {
        Jsi_Value *sc = Jsi_ValueArrayIndex(interp, args, 0);
        if (sc != NULL)
            jstr = Jsi_ValueToString(interp, sc);
    }
    
    if (0 == (argc=Jsi_ObjGetLength(interp, _this->d.obj))) {
        goto bail;
    }
    Jsi_DString dStr = {};
    int i;
    for (i = 0; i < argc; ++i) {
        const char *cp;
        Jsi_Value *ov = Jsi_ValueArrayIndex(interp, _this, i);
        if (!ov) {
            /* TODO: are NULL args ok? */ 
            continue;
            cp = "";
        } else
            cp = Jsi_ValueToString(interp, ov);
        if (i && jstr[0])
            Jsi_DSAppend(&dStr, jstr, NULL);
        Jsi_DSAppend(&dStr, cp, NULL);
    }
    
    Jsi_ValueMakeString(interp, *ret, Jsi_Strdup(Jsi_DSValue(&dStr)));
    Jsi_DSFree(&dStr);
    return JSI_OK;
bail:
    Jsi_ValueMakeString(interp, *ret, Jsi_Strdup(""));
    return JSI_OK;        
}


Jsi_Value* Jsi_ValueArrayConcat(Jsi_Interp *interp, Jsi_Value *arg1, Jsi_Value *arg2) {
    Jsi_Value *va;
    Jsi_Obj *obj;
    if (arg1->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, arg1->d.obj)) {
        return NULL;
    }
    if (arg2->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, arg2->d.obj)) {
        return NULL;
    }
    int len1 = arg1->d.obj->arrCnt;
    int len2 = arg2->d.obj->arrCnt;
    Jsi_Obj *nobj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    jsi_ArraySizer(interp, nobj, len1+len2);

    int i, j = 0;
    obj = arg1->d.obj;
    for (i = 0; i<len1; i++, j++)
    {
        if (!obj->arr[i]) continue;
        nobj->arr[j] = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp, nobj->arr[j], obj->arr[i]);
    }
    obj = arg2->d.obj;
    for (i = 0; i<len2; i++, j++)
    {
        if (!obj->arr[i]) continue;
        nobj->arr[j] = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp, nobj->arr[j], obj->arr[i]);
    }
    Jsi_ObjSetLength(interp, nobj, len1+len2);
    va = Jsi_ValueNew(interp);
    Jsi_ValueMakeArrayObject(interp, va, nobj);
    return va;
}

static int ArrayConcatCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
   Jsi_Value *va;
    Jsi_Obj *obj;
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        goto bail;
    }
    obj = _this->d.obj;
    
    int argc = Jsi_ValueGetLength(interp, args);
    int curlen = Jsi_ObjGetLength(interp, obj);
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    Jsi_ObjListifyArray(interp, obj);
   
    Jsi_Obj *nobj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    int nsiz = obj->arrMaxSize;
    if (nsiz<=0) nsiz = 100;
    nobj->arr = Jsi_Calloc(nsiz, sizeof(Jsi_Value*));
    nobj->arrMaxSize = nsiz;
    
    int i, j, m;
    for (i = 0; i<curlen; i++)
    {
        if (!obj->arr[i]) continue;
        nobj->arr[i] = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp, nobj->arr[i], obj->arr[i]);
    }
    m = i;
    for (i = 0; i < argc; i++) {
         va = Jsi_ValueArrayIndex(interp, args, i);
         if (va->vt == JSI_VT_OBJECT && Jsi_ObjIsArray(interp, va->d.obj)) {
            int margc = Jsi_ValueGetLength(interp, va);
            Jsi_Obj *mobj = va->d.obj;
            Jsi_ObjListifyArray(interp, mobj);
            jsi_ArraySizer(interp, nobj, curlen += margc);
            for (j = 0; j<margc; j++, m++)
            {
                if (!mobj->arr[j]) continue;
                nobj->arr[m] = Jsi_ValueNew(interp);
                Jsi_ValueCopy(interp, nobj->arr[m], mobj->arr[j]);
            }
        } else {
            if (jsi_ArraySizer(interp, nobj, ++curlen) <= 0)
                break;
            nobj->arr[m] = Jsi_ValueNew(interp);
            Jsi_ValueCopy(interp, nobj->arr[m++], va);
       }
    }
    Jsi_ObjSetLength(interp, nobj, curlen);
    Jsi_ValueMakeArrayObject(interp, *ret, nobj);
    return JSI_OK;
        
bail:
        Jsi_ValueMakeNull(interp, *ret);
        return JSI_OK;
}

static int ArrayMapCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    Jsi_Obj *obj, *nobj;
    int curlen, nsiz;
    int i, rc;
    Jsi_Value *func, *vpargs;
    STHIS();
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        goto bail;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (!Jsi_ValueIsFunction(interp, func)) {
        Jsi_LogError("expected function");
        return JSI_ERROR;
    }
    obj = _this->d.obj;
    curlen = Jsi_ObjGetLength(interp, obj);    
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    Jsi_ObjListifyArray(interp, obj);
    nobj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    nsiz = obj->arrCnt;
    if (nsiz<=0) nsiz = 1;
    nobj->arr = Jsi_Calloc(nsiz, sizeof(Jsi_Value*));
    nobj->arrMaxSize = nsiz;
    Jsi_ValueMakeArrayObject(interp, *ret, nobj);

    for (i = 0; i < curlen; i++) {
        vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, obj->arr+i, 1));
        Jsi_IncrRefCount(interp, vpargs);
        nobj->arr[i] = Jsi_ValueNew1(interp);
        rc = Jsi_FunctionInvoke(interp, func, vpargs, nobj->arr+i, sthis);
        Jsi_DecrRefCount(interp, vpargs);
        if( JSI_OK!=rc ) {
            goto bail;
        }
    }
    Jsi_ObjSetLength(interp, nobj, curlen);
    return JSI_OK;
        
bail:
    Jsi_ValueMakeNull(interp, *ret);
    return JSI_OK;
}

static int ArrayFilterCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    Jsi_Obj *obj, *nobj;
    int curlen, nsiz;
    int i, rc;
    Jsi_Value *func, *vpargs;
    STHIS();
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        goto bail;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (!Jsi_ValueIsFunction(interp, func)) {
        Jsi_LogError("expected function");
        return JSI_ERROR;
    }
    obj = _this->d.obj;
    curlen = Jsi_ObjGetLength(interp, obj);    
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    Jsi_ObjListifyArray(interp, obj);
    nobj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    nsiz = obj->arrCnt;
    if (nsiz<=0) nsiz = 1;
    nobj->arr = Jsi_Calloc(nsiz, sizeof(Jsi_Value*));
    nobj->arrMaxSize = nsiz;
    Jsi_ValueMakeArrayObject(interp, *ret, nobj);
    Jsi_Value nret = VALINIT, *nrPtr = &nret;
    int fval, n = 0;
    for (i = 0; i < curlen; i++) {
        if (!obj->arr[i]) continue;
        vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, obj->arr+i, 1));
        Jsi_IncrRefCount(interp, vpargs);
        rc = Jsi_FunctionInvoke(interp, func, vpargs, &nrPtr, sthis);
        Jsi_DecrRefCount(interp, vpargs);
        fval = Jsi_ValueIsTrue(interp, &nret);
        Jsi_ValueMakeUndef(interp, &nret);
        if( JSI_OK!=rc ) {
            goto bail;
        }
        if (fval) {
            nobj->arr[n++] = obj->arr[i];
            Jsi_IncrRefCount(interp, obj->arr[i]);
        }
    }
    Jsi_ObjSetLength(interp, nobj, n);
    return JSI_OK;
        
bail:
        Jsi_ValueMakeNull(interp, *ret);
        return JSI_OK;
}


static int ArrayReverseCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    Jsi_Obj *obj;
    int i, n, m;
    Jsi_Value *tval;
    STHIS();

    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_LogError("expected array");
        return JSI_ERROR;
    }
    obj = _this->d.obj;
    Jsi_ObjListifyArray(interp, obj);
    m = obj->arrCnt/2;
    for (i = 0, n=obj->arrCnt-1; i < m; i++, n--) {
        tval = obj->arr[i];
        obj->arr[i] = obj->arr[n];
        obj->arr[n] = tval;
    }
    Jsi_ValueCopy(interp, *ret, _this);
    return JSI_OK;
}

static int ArrayForeachCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) 
{
    Jsi_Obj *obj;
    int curlen;
    int i, rc;
    Jsi_Value *func, *vpargs;
    STHIS();

    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_LogError("expected array");
        return JSI_ERROR;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (!Jsi_ValueIsFunction(interp, func)) {
        Jsi_LogError("expected function");
        return JSI_ERROR;
    }
    obj = _this->d.obj;
    curlen = Jsi_ObjGetLength(interp, obj);    
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    Jsi_ObjListifyArray(interp, obj);

    for (i = 0; i < obj->arrCnt; i++) {
        vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, obj->arr+i, 1));
        Jsi_IncrRefCount(interp, vpargs);
        rc = Jsi_FunctionInvoke(interp, func, vpargs, ret, sthis);
        Jsi_DecrRefCount(interp, vpargs);
        if( JSI_OK!=rc ) {
            return rc;
        }
    }
    return JSI_OK;
}

static int ArraySomeCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    Jsi_Obj *obj;
    int curlen;
    int i, rc;
    Jsi_Value *func, *vpargs;
    STHIS();

    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_LogError("expected array");
        return JSI_ERROR;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (!Jsi_ValueIsFunction(interp, func)) {
        Jsi_LogError("expected function");
        return JSI_ERROR;
    }
    obj = _this->d.obj;
    curlen = Jsi_ObjGetLength(interp, obj);    
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    Jsi_ObjListifyArray(interp, obj);
    int fval = 0;
    Jsi_Value nret = VALINIT, *nrPtr = &nret;
    for (i = 0; i < obj->arrCnt; i++) {
        vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, obj->arr+i, 1));
        Jsi_IncrRefCount(interp, vpargs);
        rc = Jsi_FunctionInvoke(interp, func, vpargs, &nrPtr, sthis);
        Jsi_DecrRefCount(interp, vpargs);
        fval = Jsi_ValueIsTrue(interp, &nret);
        Jsi_ValueMakeUndef(interp, &nret);
        if( JSI_OK!=rc ) {
            return rc;
        }
        if (fval) {
            break;
        }
    }
    Jsi_ValueMakeBool(interp, *ret, fval);
    return JSI_OK;

}

static int ArrayIsarrayCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    int isa = (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj));
    Jsi_ValueMakeBool(interp, *ret, isa);
    return JSI_OK;
}

static int ArrayIndexOfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return 0;
    }
    Jsi_Value *seq = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_Value *start = Jsi_ValueArrayIndex(interp, args, 1);

    if (!seq) {
        goto bail;
    }
    Jsi_Obj *obj = _this->d.obj;
    
    int n = Jsi_ObjGetLength(interp, obj);    
    if (n == 0) {
        goto bail;
    }
    int istart = 0;
    if (start && Jsi_ValueIsType(interp,start, JSI_VT_NUMBER)) {
        istart = (int) start->d.num;
        if (istart >= n)
            goto bail;
        if (istart < 0)
            istart = (n+istart);
        if (istart<0)
            istart = 0;
    }
    Jsi_ObjListifyArray(interp, obj);
    int i;
    for (i = istart; i < n; i++)
    {
        if (obj->arr[i] && Jsi_ValueCmp(interp, obj->arr[i], seq, JSI_CMP_EXACT)==0) {
            Jsi_ValueMakeNumber(interp, *ret, i);
            return JSI_OK;
        }
    }
bail:
    Jsi_ValueMakeNumber(interp, *ret, -1);
    return JSI_OK;
}

static int ArrayLastindexOfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    Jsi_Value *seq = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_Value *start = Jsi_ValueArrayIndex(interp, args, 1);

    if (!seq) {
        goto bail;
    }
    Jsi_Obj *obj = _this->d.obj;
    
    int n = Jsi_ObjGetLength(interp, obj);    
    if (n == 0) {
        goto bail;
    }
    int istart = n-1;
    if (start && Jsi_ValueIsType(interp,start, JSI_VT_NUMBER)) {
        istart = (int) start->d.num;
        if (istart >= n)
            goto bail;
        if (istart < 0)
            istart = (n+istart);
        if (istart<0)
            goto bail;
    }
    Jsi_ObjListifyArray(interp, obj);
    int i;
    for (i = istart; i >= 0; i--)
    {
        if (obj->arr[i] && Jsi_ValueCmp(interp, obj->arr[i], seq, JSI_CMP_EXACT)==0) {
            Jsi_ValueMakeNumber(interp, *ret, i);
            return JSI_OK;
        }
    }
bail:
    Jsi_ValueMakeNumber(interp, *ret, -1);
    return JSI_OK;
}


static int ArraySizeOfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    int i;
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj))
        i = 0;
    else
        i = Jsi_ObjGetLength(interp, _this->d.obj);
    Jsi_ValueMakeNumber(interp, *ret, i);
    return JSI_OK;
}

static int ArrayShiftCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    Jsi_Obj *obj;
    Jsi_Value *v;
    int n;
    
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    obj = _this->d.obj;
    Jsi_ObjListifyArray(interp, obj);
    n = Jsi_ObjGetLength(interp, obj);
    assert(n <= obj->arrCnt);
    if (n<=0) {
        Jsi_ValueMakeUndef(interp, *ret);
    } else {
        n--;
        v = obj->arr[0];
        memmove(obj->arr, obj->arr+1, n*sizeof(Jsi_Value*));
        obj->arr[n] = NULL;
        Jsi_ValueCopy(interp, *ret, v);
        Jsi_ObjSetLength(interp, obj, n);
    }
    return JSI_OK;
}

static int ArrayUnshiftCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
    Jsi_Value *v;
    Jsi_Obj *obj;
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    obj = _this->d.obj;
    
    int argc = Jsi_ValueGetLength(interp, args);
    int curlen = Jsi_ObjGetLength(interp, obj);
    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    if (argc <= 0) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    Jsi_ObjListifyArray(interp, obj);
    if (jsi_ArraySizer(interp, obj, curlen+argc)<=0) {
        Jsi_LogError("too long");
        return JSI_ERROR;
    }
    memmove(obj->arr+argc, obj->arr, (curlen)*sizeof(Jsi_Value*));
    obj->arrCnt += argc;
    int i;
    for (i = 0; i < argc; ++i) {
        v = Jsi_ValueNew(interp);
        Jsi_Value *ov = Jsi_ValueArrayIndex(interp, args, i);
        obj->arr[i] = NULL;
        if (!ov) { Jsi_LogBug("Arguments Error\n"); continue; }

        Jsi_ValueCopy(interp,v, ov);
        obj->arr[i] = v;
    }
    Jsi_ObjSetLength(interp, obj, curlen+argc);
    
    Jsi_ValueMakeNumber(interp, *ret, Jsi_ObjGetLength(interp, obj));
    return JSI_OK;
}


static int ArraySliceCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr) {
   if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        Jsi_ValueMakeNumber(interp, *ret, 0);
        return JSI_OK;
    }
    Jsi_Value *start = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_Value *end = Jsi_ValueArrayIndex(interp, args, 1);
    int istart = 0, iend;
    
    if (!start) {
        goto bail;
    }
    Jsi_Obj *obj = _this->d.obj;
    int n = Jsi_ObjGetLength(interp, obj);
    if (Jsi_ValueIsType(interp,start, JSI_VT_NUMBER)) {
        istart = (int) start->d.num;
        if (istart >= n)
            goto bail;
        if (istart < 0)
            istart = (n+istart);
        if (istart<0)
            goto bail;
    }
      
    if (n == 0) {
        goto bail;
    }
    iend = n-1;
    if (end && Jsi_ValueIsType(interp,end, JSI_VT_NUMBER)) {
        iend = (int) end->d.num;
        if (iend >= n)
            iend = n;
        if (iend < 0)
            iend = (n+iend);
        if (iend<0)
            goto bail;
    }
    int nsiz = iend-istart+1;
    if (nsiz<=0)
        goto bail;
    Jsi_ObjListifyArray(interp, obj);
    
    Jsi_Obj *nobj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);

    nobj->arr = Jsi_Calloc(nsiz, sizeof(Jsi_Value*));
    nobj->arrMaxSize = nsiz;

    int i, m;
    for (m = 0, i = istart; i <= iend; i++, m++)
    {
        if (!obj->arr[i]) continue;
        nobj->arr[m] = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp, nobj->arr[m], obj->arr[i]);
    }
    Jsi_ObjSetLength(interp, nobj, nsiz);
    Jsi_ValueMakeArrayObject(interp, *ret, nobj);
    return JSI_OK;
bail:
    Jsi_ValueMakeNull(interp, *ret);
    return JSI_OK;
}

typedef struct {
    Jsi_Interp *interp;
    int flags;
} SortInfo;

#ifdef NO_QSORT_R
static Jsi_Interp *curInterp = NULL;
static int sortFlags = 0;

static int SortSubCmd(const void *p1, const void *p2) {
#else
static int SortSubCmd(const void *p1, const void *p2, void *thunk) {
    SortInfo *si = thunk;
    Jsi_Interp *curInterp = si->interp;
    int sortFlags = si->flags;
#endif

    Jsi_Value *v1 = *(Jsi_Value**)p1, *v2 = *(Jsi_Value**)p2;
    if (curInterp == NULL || curInterp->deleting)
        return 0;
    VALCHK(v1);
    VALCHK(v2);
    int rc;
    if (v1 != NULL && v2 != NULL)
        rc = Jsi_ValueCmp(curInterp, v1, v2, sortFlags);
    else {
        if (v1 == v2) 
            rc = 0;
        else if (v1 == NULL)
            rc = 1;
        else
            rc = -1;
    }
    if ((sortFlags&JSI_SORT_DESCEND))
        return rc;
    return -rc;
}

int Jsi_ValueArraySort(Jsi_Interp *interp, Jsi_Value *val, int flags)
{
    if (val->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, val->d.obj)) {
        return JSI_ERROR;
    }
    Jsi_Obj *obj = val->d.obj;
    Jsi_ObjListifyArray(interp, obj);
    if (obj->arrCnt <= 0) {
        return JSI_OK;
    }
#ifdef __WIN32
#define qsort_r qsort_s
#endif

#ifdef NO_QSORT_R
    curInterp = interp;
    sortFlags = flags;
    qsort(obj->arr, obj->arrCnt, sizeof(Jsi_Value*), SortSubCmd);
    curInterp = NULL;
#else
    SortInfo si;
    si.interp = interp;
    si.flags = flags;
    qsort_r(obj->arr, obj->arrCnt, sizeof(Jsi_Value*), SortSubCmd, &si);
#endif
    return JSI_OK;
}

static int ArraySortCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *v;
    char *sortDesc;
     if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        goto bail;
    }
    Jsi_Obj *obj = _this->d.obj;
    int curlen = Jsi_ObjGetLength(interp, obj);

    if (curlen <= 0) {
        goto done;
    }
    Jsi_Value *sd = Jsi_ValueArrayIndex(interp, args, 0);
    int flags = 0;
    if (sd && (sortDesc = Jsi_ValueString(interp, sd, NULL))) {
        /* TODO: cleanup/argchk */
        if (strstr(sortDesc,"desc")) flags |= JSI_SORT_DESCEND;
        if (strstr(sortDesc,"ascii")) flags |= JSI_SORT_ASCII;
        if (strstr(sortDesc,"nocase")) flags |= JSI_SORT_NOCASE;
    }
    Jsi_ObjListifyArray(interp, obj);
#ifdef NO_QSORT_R
    /* TODO: mutex. */
    curInterp = interp;
    sortFlags = flags;
    qsort(obj->arr, curlen, sizeof(Jsi_Value*), SortSubCmd);
    curInterp = NULL;
#else
    SortInfo si;
    si.interp = interp;
    si.flags = flags;
    qsort_r(obj->arr, curlen, sizeof(Jsi_Value*), SortSubCmd, &si);
#endif

    if (interp->deleting)
        return JSI_ERROR;
done:
    v = Jsi_ValueMakeObject(interp, NULL, obj);
    Jsi_IncrRefCount(interp, v);
    Jsi_ValueCopy(interp, *ret, v);
    Jsi_ObjIncrRefCount(interp, obj);
    return JSI_OK;
bail:
    Jsi_ValueMakeNull(interp, *ret);
    return JSI_OK;
}

static int ArraySpliceCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *va;
    Jsi_Obj *obj;
    if (_this->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, _this->d.obj)) {
        goto bail2;
    }
    obj = _this->d.obj;
    
    Jsi_Value *start = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_Value *howmany = Jsi_ValueArrayIndex(interp, args, 1);
    int argc = Jsi_ValueGetLength(interp, args);
    int istart = 0, rhowmany, ilen = (argc>=2 ? argc - 2 : 0);
    int n = Jsi_ObjGetLength(interp, obj), curlen = n;
    
    if (!start) {
        goto bail2;
    }

    Jsi_Obj *nobj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    Jsi_ValueMakeArrayObject(interp, *ret, nobj);
    Jsi_ObjSetLength(interp, nobj, 0);
    
    /* Determine start index. */
    if (Jsi_ValueIsType(interp, start, JSI_VT_NUMBER)) {
        istart = (int) start->d.num;
        if (istart >= n)
            goto bail;
        if (istart < 0)
            istart = (n+istart);
        if (istart<0)
            istart=0;
    }
      
    rhowmany = n-istart;
    if (howmany && Jsi_ValueIsType(interp,howmany, JSI_VT_NUMBER)) {
        rhowmany = (int) howmany->d.num;
        if (rhowmany >= (n-istart))
            rhowmany = n-istart;
        if (rhowmany < 0)
            rhowmany = (n-istart);
        if (rhowmany<0)
            goto bail;
    }
    int asiz = istart+1;

    if (curlen < 0) {
        Jsi_ObjSetLength(interp, obj, 0);
    }
    Jsi_ObjListifyArray(interp, obj);
   
    nobj->arr = Jsi_Calloc(asiz, sizeof(Jsi_Value*));
    nobj->arrMaxSize = asiz;

    /* Move elements to return object. */
    int i, j, m;
    for (m=0, j = 0, i = istart; m<rhowmany && m<curlen; m++,i++, j++)
    {
        if (!obj->arr[i]) continue;
        nobj->arr[m] = Jsi_ValueNew(interp);
        Jsi_ValueCopy(interp, nobj->arr[m], obj->arr[i]);
    }
    Jsi_ObjSetLength(interp, nobj, m);
    
    /* Shift remaining down. */
    for (; rhowmany && i<curlen; i++)
    {
        obj->arr[i-rhowmany] = obj->arr[i];
        obj->arr[i] = NULL;
    }
    curlen -= j;
    /* Add elements. */
    int newlen = curlen + argc - (argc>=2?2:1);
    if (jsi_ArraySizer(interp, obj, newlen+3) <= 0) {
        Jsi_LogError("too long");
        Jsi_ValueMakeUndef(interp, *ret);
        return JSI_ERROR;
    }
    if (ilen>0) {
        for (i = curlen-1; i>=istart; i--) {
            obj->arr[i+ilen] = obj->arr[i];
            obj->arr[i] = NULL;
        }
        for (m=istart, i = 2; i<argc; m++,i++) {
            va = Jsi_ValueArrayIndex(interp, args, i);
            if (!va) continue;
            obj->arr[m] = Jsi_ValueNew(interp);
            Jsi_ValueCopy(interp, obj->arr[m], va);
        }
    }
    Jsi_ObjSetLength(interp, obj, newlen);
bail:    
    return JSI_OK;
     
            
bail2:
    Jsi_ValueMakeNull(interp,*ret);
    return JSI_OK;
}

static int ArrayConstructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *target;
    
    if (Jsi_FunctionIsConstructor(funcPtr)) target = _this;
    else {
        Jsi_Obj *o = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
        o->__proto__ = interp->Array_prototype;
        Jsi_ValueMakeObject(interp, *ret, o);
        target = *ret;
    }

    if (argc == 1) {
        Jsi_Value *v = Jsi_ValueArrayIndex(interp, args, 0);
        if (v && Jsi_ValueIsType(interp,v, JSI_VT_NUMBER)) {
            int len = (int)v->d.num;
            if (!jsi_is_integer(v->d.num) || len < 0) {
                Jsi_LogError("Invalid array length\n");
                return JSI_ERROR;
            }
            target->d.obj->isArray = 1;
            if (jsi_ArraySizer(interp, target->d.obj, len) <= 0)
                return JSI_ERROR;
            return JSI_OK;
        }
    }

    int i;
    target->d.obj->isArray = 1;
    if (jsi_ArraySizer(interp, target->d.obj, 0) <= 0)
            return JSI_ERROR;
    
    for (i = 0; i < argc; ++i) {
        Jsi_Value *v = Jsi_ValueNew(interp);
        Jsi_Value *argv = Jsi_ValueArrayIndex(interp, args, i);        
        Jsi_ValueCopy(interp,v, argv);
        jsi_ValueInsertArray(interp, _this, i, v, 0);
    }
    return JSI_OK;
}
            
static Jsi_CmdSpec arrayCmds[] = {
    { "Array",      ArrayConstructor,   0,-1, "?val,val,...?", JSI_CMD_IS_CONSTRUCTOR, .help="Array constructor" },
    { "concat",     ArrayConcatCmd,     1,-1, "arr?,arr,...?", .help="Return string with args appended" },
    { "filter",     ArrayFilterCmd,     1, 2, "callback?,this?", .help="Return a filtered array" },
    { "forEach",    ArrayForeachCmd,    1, 2, "function?,this?", .help="Invoke function with each item in array" },
    { "indexOf",    ArrayIndexOfCmd,    1, 2, "str?,startIdx?", .help="Return index of substring" },
    { "isArray",    ArrayIsarrayCmd,    0, 0, "", .help="Return true if converted to an internal C array" },
    { "join",       ArrayJoinCmd,       0, 1, "?char?", .help="Return elements joined by char"  },
    { "lastIndexOf",ArrayLastindexOfCmd,1, 2, "seq?,start?", .help="Return index of last occurence in array" },
    { "map",        ArrayMapCmd,        1, 2, "callback?,this?", .help="Creates a new array with the results of calling a provided function on every element in this array" },
    { "pop",        ArrayPopCmd,        0, 0, "", .help="Remove and return last element of array"  },
    { "push",       ArrayPushCmd,       1,-1, "val?,val,...?", .help="Push one or more elements onto array"  },
    { "shift",      ArrayShiftCmd,      0, 0, "", .help="Remove first element and shift downwards" },
    { "sizeOf",     ArraySizeOfCmd,     0, 0, "", .help="Return size of array" },
    { "slice",      ArraySliceCmd,      1, 2, "start?,end?", .help="Return sub-array" },
    { "some",       ArraySomeCmd,       1, 2, "function?,this?", .help="Return true on first element function returns true on" },
    { "sort",       ArraySortCmd,       0, 1, "?comparefunc?", .help="Sort an array" },
    { "splice",     ArraySpliceCmd,     1,-1, "start,?howmany,val,val,...?", .help="Change the content of an array, adding new elements while removing old elements" },
    { "reverse",    ArrayReverseCmd,    0, 0, "", .help="Reverse order of all elements in an array" },
    { "unshift",    ArrayUnshiftCmd,    0,-1, "?arg,arg...?", .help="Add new elements to start of array and return size" },
    { NULL, .help="Provide access to array objects" }
};

int jsi_ArrayInit(Jsi_Interp *interp)
{
    interp->Array_prototype = Jsi_CommandCreateSpecs(interp, "Array", arrayCmds, NULL, 0);
    return JSI_OK;
}

#endif
