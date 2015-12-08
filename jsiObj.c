#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

/******************* TREE ACCESS **********************/

Jsi_Value *Jsi_TreeObjGetValue(Jsi_Obj* obj, const char *key, int isstrkey) {
    Jsi_Tree *treePtr = obj->tree;
    
    if (!isstrkey) {
        Jsi_HashEntry *hPtr = Jsi_HashEntryFind(treePtr->interp->strKeyTbl, key);
        if (!hPtr)
            return NULL;
        key = Jsi_HashKeyGet(hPtr);
    }
    Jsi_Value *v = Jsi_TreeGet(treePtr, (void*)key);
    return v;
}

Jsi_TreeEntry *Jsi_TreeObjSetValue(Jsi_Obj *obj, const char *key, Jsi_Value *val, int isstrkey) {
    Jsi_Tree *treePtr = obj->tree;
    int isNew;
    Jsi_TreeEntry *hPtr;
    Jsi_Value *oldVal;
    Jsi_Interp *interp = treePtr->interp;
    if (!isstrkey) {
        Jsi_HashEntry *hePtr = Jsi_HashEntryCreate(interp->strKeyTbl, key, &isNew);
        key = Jsi_HashKeyGet(hePtr);
    }
    //return Jsi_TreeSet(treePtr, key, val);
    hPtr = Jsi_TreeEntryCreate(treePtr, key, &isNew);
    if (!hPtr)
        return NULL;
    SIGASSERT(val,VALUE);
    Assert(val->refCnt>0);
    if (!isNew) {
        oldVal = Jsi_TreeValueGet(hPtr);
        if (oldVal) {
            Jsi_ValueReset(interp, oldVal);
            Jsi_ValueCopy(interp, oldVal, val);
        }
    }
    else
        hPtr->value = val;
    return hPtr;
}

/*****************************************/

int Jsi_ObjIsArray(Jsi_Interp *interp, Jsi_Obj *o)  {
    return ((o)->ot == JSI_OT_OBJECT && o->isArray);
}

static int ObjListifyCallback(Jsi_Tree *tree, Jsi_TreeEntry *hPtr, void *data)
{
    Jsi_Interp *interp = tree->interp;
    Jsi_Obj *obj = data;
    int n;
    if (!hPtr->f.bits.dontenum) {
        char *cp = Jsi_TreeKeyGet(hPtr), *ocp = cp;
        /* TODO: accept hex??? */
        while (*cp && isdigit(*cp))
            cp++;
        if (*cp)
            return JSI_OK;
        n = atoi(ocp);
        if (n >= tree->interp->maxArrayList)
            return JSI_OK;
        hPtr->f.bits.isarrlist = 1;
        if (jsi_ArraySizer(tree->interp, obj, n) <= 0) {
            Jsi_LogError("too long");
            return JSI_ERROR;
        }
        obj->arr[n] = Jsi_TreeValueGet(hPtr);
       // obj->arrCnt++;
    }
    return JSI_OK;
}

static int ObjListifyArrayCallback(Jsi_Tree *tree, Jsi_TreeEntry *hPtr, void *data)
{
    if (hPtr->f.bits.isarrlist) {
        Jsi_TreeEntryDelete(hPtr);
        tree->interp->delRBCnt++;
        return JSI_ERROR;
    }
    return JSI_OK;
}

void Jsi_ObjListifyArray(Jsi_Interp *interp, Jsi_Obj *obj)
{
    if (!obj->isArray) {
        Jsi_LogBug("Can not listify a non-array\n");
        return;
    }
    if (obj->arr) return;
    Jsi_TreeWalk(obj->tree, ObjListifyCallback, obj, 0);

    do {
        interp->delRBCnt = 0;
        Jsi_TreeWalk(obj->tree, ObjListifyArrayCallback, obj, 0);
    } while (interp->delRBCnt);
}

void jsi_IterObjFree(Jsi_IterObj *iobj)
{
    if (!iobj->isArrayList) {
        int i;
        for (i = 0; i < iobj->count; i++) {
            if (iobj->keys[i]) {
                /*Jsi_TreeDecrRef(iobj->keys[i]); TODO: ??? */
            }
        }
        Jsi_Free(iobj->keys);
    }
    Jsi_Free(iobj);
}

Jsi_IterObj *jsi_IterObjNew(Jsi_Interp *interp, Jsi_IterProc *iterCmd)
{
    Jsi_IterObj *o = Jsi_Calloc(1,sizeof(Jsi_IterObj));
    o->interp = interp;
    SIGINIT(o,OBJ);
    o->iterCmd = iterCmd;
    return o;
}

static int DeleteTreeValue(Jsi_Interp *interp, void *p) {
    /* Cleanup tree value. */
    Jsi_TreeEntry* ti = p;
    SIGASSERT(ti,TREEENTRY);
    Jsi_Value *v = (Jsi_Value*)ti->value;
    SIGASSERT(v,VALUE);
    Jsi_DecrRefCount(interp, v);
    ti->value = NULL;
    return JSI_OK;
}

Jsi_Obj *Jsi_ObjNew(Jsi_Interp *interp)
{
    Jsi_Obj *obj = Jsi_Calloc(1,sizeof(*obj));
    SIGINIT(obj,OBJ);
    obj->ot = JSI_OT_OBJECT;
    obj->tree = Jsi_TreeNew(interp, JSI_KEYS_STRINGPTR);
    obj->tree->freeProc = DeleteTreeValue;
    obj->__proto__ = interp->Object_prototype;
    interp->objCnt++;
   return obj;
}

Jsi_Obj *Jsi_ObjNewType(Jsi_Interp *interp, Jsi_otype otype)
{
    Jsi_Obj *obj = Jsi_ObjNew(interp);
    obj->ot = (otype==JSI_OT_ARRAY?JSI_OT_OBJECT:otype);
    switch (otype) {
        case JSI_OT_OBJECT: obj->__proto__ = interp->Object_prototype; break;
        case JSI_OT_BOOL:   obj->__proto__ = interp->Boolean_prototype; break;
        case JSI_OT_NUMBER: obj->__proto__ = interp->Number_prototype; break;
        case JSI_OT_STRING: obj->__proto__ = interp->String_prototype; break;
        case JSI_OT_FUNCTION:obj->__proto__ = interp->Function_prototype; break;
        case JSI_OT_REGEXP: obj->__proto__ = interp->RegExp_prototype; break;
        case JSI_OT_ARRAY:  obj->__proto__ = interp->Array_prototype;
            obj->isArray = 1;
            obj->arr = Jsi_Calloc(1,sizeof(Jsi_Value*));
            obj->arrMaxSize = 1;
            break;
        default: assert(0); break;
    }
    return obj;
}

void Jsi_ObjFree(Jsi_Interp *interp, Jsi_Obj *obj)
{
    interp->objCnt--;
    /* printf("Free obj: %x\n", (int)obj); */
    switch (obj->ot) {
        case JSI_OT_STRING:
            if (!obj->isstrkey)
                Jsi_Free(obj->d.s.str);
            obj->d.s.str = 0;
            obj->isstrkey = 0;
            break;
        case JSI_OT_FUNCTION:
            jsi_FuncObjFree(obj->d.fobj);
            break;
        case JSI_OT_ITER:
            jsi_IterObjFree(obj->d.iobj);
            break;
        case JSI_OT_USEROBJ:
            jsi_UserObjFree(interp, obj->d.uobj);
            break;
        case JSI_OT_REGEXP:
            if ((obj->d.robj->eflags&JSI_REG_STATIC)==0) {
                regfree(&obj->d.robj->reg);
                Jsi_Free(obj->d.robj);
            }
            break;
        default:
            break;
    }
    if (obj->tree)
        Jsi_TreeDelete(obj->tree);
    if (obj->arr) {
        int i = -1;
        while (++i < obj->arrCnt)
            if (obj->arr[i])
                Jsi_DecrRefCount(interp, obj->arr[i]);
        Jsi_Free(obj->arr);
        obj->arr = NULL;
    }
    obj->tree = NULL;
    MEMCLEAR(obj);
    Jsi_Free(obj);
}


/**************************** ARRAY ******************************/

Jsi_Value *jsi_ObjArrayLookup(Jsi_Interp *interp, Jsi_Obj *obj, char *key) {
    char *cp = key;
    int n;
    /* TODO: accept hex??? */
    if (!obj->arr)
        return NULL;
    while (*cp && isdigit(*cp))
        cp++;
    if (*cp)
        return NULL;
    n = atoi(key);
    if (n >= obj->arrCnt)
        return NULL;
    Jsi_Value *v = obj->arr[n];
    return v;
}

int Jsi_ObjArrayAdd(Jsi_Interp *interp, Jsi_Obj *o, Jsi_Value *v)
{
    if (o->isArray == 0)
        return JSI_ERROR;
    if (!o->arr)
        Jsi_ObjListifyArray(interp, o);
    int len = o->arrCnt;
    if (jsi_ArraySizer(interp, o, len+1) <= 0)
        return JSI_ERROR;
    o->arr[len] = v;
    if (v)
        Jsi_IncrRefCount(interp, v);
    assert(o->arrCnt<=o->arrMaxSize);
    return JSI_OK;
}

int Jsi_ObjArraySet(Jsi_Interp *interp, Jsi_Obj *obj, Jsi_Value *value, int arrayindex)
{
    int m, n = arrayindex;
    if (jsi_ArraySizer(interp, obj, n) <= 0)
        return JSI_ERROR;
    if (obj->arr[n])
        Jsi_DecrRefCount(interp, obj->arr[n]);
    assert(obj->arrCnt<=obj->arrMaxSize);
    obj->arr[n] = value;
    if (value)
        Jsi_IncrRefCount(interp, value);
    m = Jsi_ObjGetLength(interp, obj);
    if ((n+1) > m)
       Jsi_ObjSetLength(interp, obj, n+1);
    return JSI_OK;
}

//TODO: unnecessary?
Jsi_Value *jsi_ObjArraySetDup(Jsi_Interp *interp, Jsi_Obj *obj, Jsi_Value *value, int n)
{
    if (jsi_ArraySizer(interp, obj, n) <= 0)
        return NULL;
    if (obj->arr[n])
    {
        Jsi_ValueCopy(interp, obj->arr[n], value);
        return obj->arr[n];
    }
    assert(obj->arrCnt<=obj->arrMaxSize);
    Jsi_Value *v = Jsi_ValueNew(interp);
    int m;
    Jsi_ValueCopy(interp,v, value);
    obj->arr[n] = v;
    m = Jsi_ObjGetLength(interp, obj);
    if ((n+1) > m)
       Jsi_ObjSetLength(interp, obj, n+1);
    return v;
}

int Jsi_ObjIncrRefCount(Jsi_Interp *interp, Jsi_Obj *obj) {
#ifdef USE_VALCOPY
    SIGASSERT(obj,OBJ);
    return ++obj->refcnt;
#else
    return 0;
#endif
}

int Jsi_ObjDecrRefCount(Jsi_Interp *interp, Jsi_Obj *obj)  {
#ifdef USE_VALCOPY
    SIGASSERT(obj,OBJ);
    int nref;
    if ((nref = --obj->refcnt) <= 0) {
        assert(obj->refcnt == 0);
        Jsi_ObjFree(interp, obj);
    }
    return nref;
#else
    return 0;
#endif
}


int jsi_ArraySizer(Jsi_Interp *interp, Jsi_Obj *obj, int len)
{
    int nsiz = len + 1, mod = ARRAY_MOD_SIZE;
    assert(obj->isArray);
    if (mod>1)
        nsiz = nsiz + ((mod-1) - (nsiz + mod - 1)%mod);
    if (nsiz > MAX_ARRAY_LIST) {
        Jsi_LogError("array size too large");
        return 0;
    }
    if (len >= obj->arrMaxSize) {
        int oldsz = (nsiz-obj->arrMaxSize);
        //obj->arr = jsi_ValuesAlloc(interp, nsiz, obj->arr, oldsz);
        obj->arr = Jsi_Realloc(obj->arr, nsiz*sizeof(Jsi_Value*));
        memset(obj->arr+obj->arrMaxSize, 0, oldsz*sizeof(Jsi_Value*));
        obj->arrMaxSize = nsiz;
    }
    if (len>obj->arrCnt)
        obj->arrCnt = len;
    return nsiz;
}

Jsi_Obj *Jsi_ObjNewArray(Jsi_Interp *interp, Jsi_Value **items, int count)
{
    Jsi_Obj *obj = Jsi_ObjNewType(interp, JSI_OT_ARRAY);
    if (count>=0) {
        int i;
        if (jsi_ArraySizer(interp, obj, count) <= 0) {
            Jsi_ObjFree(interp, obj);
            return NULL;
        }
        for (i = 0; i < count; ++i) {
            obj->arr[i] = Jsi_ValueNew(interp);
            Jsi_ValueCopy(interp, obj->arr[i], items[i]);
        }
    }
    obj->arrCnt = count;
    assert(obj->arrCnt<=obj->arrMaxSize);
    return obj;
}

/****** END ARRAY ************/

static Jsi_TreeEntry* ObjInsertFromValue(Jsi_Interp *interp, Jsi_Obj *obj, Jsi_Value *keyVal, Jsi_Value *nv)
{
    const char *key = NULL;
    int flags = 0;
    Jsi_DString dStr = {};
    if (keyVal->vt == JSI_VT_STRING) {
        flags = (keyVal->f.bits.isstrkey ? JSI_OM_ISSTRKEY : 0);
        key = keyVal->d.s.str;
    } else if (keyVal->vt == JSI_VT_OBJECT && keyVal->d.obj->ot == JSI_OT_STRING) {
        Jsi_Obj *o = keyVal->d.obj;
        flags = (o->isstrkey ? JSI_OM_ISSTRKEY : 0);
        key = o->d.s.str;
    }
    if (key == NULL)
        key = Jsi_ValueGetDString(interp, keyVal, &dStr, 0);
    /* TODO: maybe should allow all keys into string table? */
    return Jsi_ObjInsert(interp, obj, key, nv, flags);
}

Jsi_Obj *Jsi_ObjNewObj(Jsi_Interp *interp, Jsi_Value **items, int count)
{
    Jsi_Obj *obj = Jsi_ObjNewType(interp, JSI_OT_OBJECT);
    int i;
    for (i = 0; i < count; i += 2) {
        ObjInsertFromValue(interp, obj, items[i], jsi_ValueDup(interp, items[i+1]));
    }
    return obj;
}

/* Set length of an object */
void Jsi_ObjSetLength(Jsi_Interp *interp, Jsi_Obj *obj, int len)
{
    if (obj->isArray) {
        assert(len<=obj->arrMaxSize);
        obj->arrCnt = len;
        return;
    }
    Jsi_Value *r = Jsi_TreeObjGetValue(obj,"length", 0);
    if (!r) {
        Jsi_Value *n = Jsi_ValueMakeNumber(interp, NULL, len);
        Jsi_ObjInsert(interp, obj, "length", n, JSI_OM_DONTDEL | JSI_OM_DONTENUM | JSI_OM_READONLY);
    } else {
        Jsi_ValueReset(interp, r);
        Jsi_ValueMakeNumber(interp,r, len);
    }
}

int Jsi_ObjGetLength(Jsi_Interp *interp, Jsi_Obj *obj)
{
    if (obj->tree && obj->tree->numEntries) {
        Jsi_Value *r = Jsi_TreeObjGetValue(obj, "length", 0);
        if (r && Jsi_ValueIsType(interp,r, JSI_VT_NUMBER)) {
            if (jsi_is_integer(r->d.num))
                return (int)r->d.num;
        }
    }
    if (obj->arr)
        return obj->arrCnt;

    return 0;
}

Jsi_Value *jsi_ObjValueNew(Jsi_Interp *interp)
{
    return Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNew(interp));
}


/* Set result string into obj. */
void Jsi_ObjFromDS(Jsi_DString *dsPtr, Jsi_Obj *obj) {
    int len = dsPtr->len;
    if (obj->ot == JSI_OT_STRING && obj->d.s.str && !obj->isstrkey)
        Jsi_Free(obj->d.s.str);
    if (dsPtr->str == dsPtr->staticSpace) {
        obj->d.s.str = Jsi_Malloc(len+1);
        memcpy(obj->d.s.str, dsPtr->str, len+1);
    } else {
        obj->d.s.str = dsPtr->str;
    }
    obj->d.s.len = len;
    dsPtr->str = dsPtr->staticSpace;
    dsPtr->spaceAvl = dsPtr->staticSize;
    dsPtr->staticSpace[0] = 0;
    dsPtr->len = 0;
}
#endif
