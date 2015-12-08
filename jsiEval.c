/* The interpreter evaluation engine for jsi. */
#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif
#include <math.h>

/*#define USE_INLINE*/
#ifdef INLINE_ALL
#define INLINE inline
#else
#define INLINE
#endif

#define obj_this (interp->Obj_this)
#define stack (interp->Stack)
#define StackIdx(s) interp->Stack[s]
#define ObjThisIdx(s) interp->Obj_this[s]
#define TOP (interp->Stack[interp->Sp-1])
#define TOQ (interp->Stack[interp->Sp-2])
#define StackCopy(interp, l, r) Jsi_ValueCopy(interp, l, r)

static Jsi_Value** ValuesAlloc(Jsi_Interp *interp, int cnt, Jsi_Value**old, int oldsz) {
    Jsi_Value **v, *vt, vs = VALINIT;
    int i;
    v = Jsi_Realloc(old, cnt* sizeof(Jsi_Value*));
    for (i=oldsz; i<cnt; i++) {
        v[i] = NULL; continue;
        vt = Jsi_ValueNew1(interp);
        *vt = vs;
        v[i] = vt;
    }
    return v;
}

static void SetupStack(Jsi_Interp *interp)
{
    int oldsz = interp->maxStack;
    if (interp->maxStack)
        interp->maxStack += STACK_INCR_SIZE;
    else
        interp->maxStack = STACK_INIT_SIZE;
    stack = ValuesAlloc(interp, interp->maxStack, stack, oldsz);
    obj_this = ValuesAlloc(interp, interp->maxStack, obj_this, oldsz);
}

static void Push(Jsi_Interp* interp, int n) {
    int i = 0;
    do {
        if (!StackIdx(interp->Sp))
            StackIdx(interp->Sp) = Jsi_ValueNew1(interp);
        if (!ObjThisIdx(interp->Sp))
            ObjThisIdx(interp->Sp) = Jsi_ValueNew1(interp);
        if (i++ >= n) break;
        interp->Sp++;
    } while (1);
}

static void StackReplace(Jsi_Interp *interp, Jsi_Value *to, Jsi_Value *from) {
    Jsi_ValueMakeUndef(interp, to);
    *to = *from;
}

/* Before setting a value in the stack/obj, unlink any reference to it. */

static void ClearStack(register Jsi_Interp *interp, int ofs) {
    Jsi_Value *v = StackIdx(interp->Sp-ofs);
    if (!v) return;
#ifndef XX_NEWSTACK
    Jsi_ValueReset(interp, v);
#else
    if (v->refCnt<=1)
        Jsi_ValueReset(interp, v);
    else {
        Jsi_DecrRefCount(interp, v);
        StackIdx(interp->Sp-ofs) = Jsi_ValueNew1(interp);
    }
#endif
}

static void ClearThis(register Jsi_Interp *interp, int ofs) {
    Jsi_Value *v = ObjThisIdx(ofs);
    if (!v) return;
#ifndef XX_NEWSTACK
    Jsi_ValueReset(interp, v);
#else
    if (v->refCnt<=1)
        Jsi_ValueReset(interp, v);
    else {
        Jsi_DecrRefCount(interp, v);
        ObjThisIdx(ofs) = Jsi_ValueNew1(interp);
    }
#endif
}


/* interp->Sp[-2] = interp->Sp[-1]->var */
static void inline ValueAssign(Jsi_Interp *interp)
{
    Jsi_Value *v, *dst = TOQ, *src = TOP;
    if (dst->vt != JSI_VT_VARIABLE) {
        Jsi_LogError("operand not a left value");
    } else {
        v = dst->d.lval;
        SIGASSERT(v, VALUE);
        if (v == src)
            return;
        if (v->f.bits.readonly) {
            Jsi_LogWarn("ignore assign to readonly variable");
            return;
        }
        Jsi_ValueCopy(interp,v, src);
        SIGASSERT(v, VALUE);
    }
}

/* pop n values from stack */
static INLINE void Pop(Jsi_Interp* interp, int n) {
    int t = n;
    while (t > 0) {
        Assert((interp->Sp-t)>=0);
/*        Jsi_Value *v = StackIdx(interp->Sp-t);
         if (v->refCnt>1) puts("OO");*/
        ClearStack(interp,t);
        --t;
    }
    interp->Sp -= n;
}

/* Convert preceding stack variable(s) into value(s). */
static INLINE void VarDeref(Jsi_Interp* interp, int n) {
    Assert(interp->Sp>=n);
    int i;
    for (i=1; i<=n; i++) {
        Jsi_Value *vb = StackIdx(interp->Sp - i);
        if (vb->vt == JSI_VT_VARIABLE) {
            SIGASSERT(vb->d.lval, VALUE);
            /*Jsi_IncrRefCount(interp, v); TODO: memory??? */
            Jsi_ValueCopy(interp, vb, vb->d.lval);
        }
    }
}

#define common_math_opr(opr) {                      \
    VarDeref(interp,2);                                     \
    if (!Jsi_ValueIsType(interp,TOP, JSI_VT_NUMBER)) Jsi_ValueToNumber(interp, TOP);     \
    if (!Jsi_ValueIsType(interp,TOQ, JSI_VT_NUMBER)) Jsi_ValueToNumber(interp, TOQ);     \
    TOQ->d.num = TOQ->d.num opr TOP->d.num;            \
    Pop(interp, 1);                                          \
}

#define common_bitwise_opr(opr) {                       \
    int a, b;                                       \
    VarDeref(interp,2);                                     \
    if (!Jsi_ValueIsType(interp,TOP, JSI_VT_NUMBER)) Jsi_ValueToNumber(interp, TOP);     \
    if (!Jsi_ValueIsType(interp,TOQ, JSI_VT_NUMBER)) Jsi_ValueToNumber(interp, TOQ);     \
    a = TOQ->d.num; b = TOP->d.num;                   \
    TOQ->d.num = (Jsi_Number)(a opr b);                  \
    Pop(interp, 1);                                          \
}

static INLINE void logic_less(Jsi_Interp* interp, int i1, int i2) {
    Jsi_Value *v1 = stack[interp->Sp-i1], *v2 = stack[interp->Sp-i2], *res = TOQ;
    int val = 0;
    Jsi_ValueToPrimitive(interp, v1);
    Jsi_ValueToPrimitive(interp, v2);
    if (v1->vt == JSI_VT_STRING && v2->vt == JSI_VT_STRING) {
        int v1len = v1->d.s.len;
        int v2len = v2->d.s.len;
        if (v1len == -1 && v2len == -1) {
            val = strcmp(v1->d.s.str, v2->d.s.str);
        } else {
            if (v1len < 0) v1len = strlen(v1->d.s.str);
            if (v2len < 0) v1len = strlen(v2->d.s.str);
            int mlen = v1len < v2len ? v1len : v2len;
            /* TODO: use memcmp may cause bug */
            val = memcmp(v1->d.s.str, v2->d.s.str, mlen * sizeof(char));
        }
        if (val > 0) val = 0;
        else if (val < 0) val = 1;
        else val = (v1len < v2len);
        ClearStack(interp,2);
        Jsi_ValueMakeBool(interp,res, val);
    } else {
        Jsi_ValueToNumber(interp, v1);
        Jsi_ValueToNumber(interp, v2);
        if (jsi_ieee_isnan(v1->d.num) || jsi_ieee_isnan(v2->d.num)) {
            ClearStack(interp,2);
            Jsi_ValueMakeUndef(interp,res);
        } else {
            val = (v1->d.num < v2->d.num);
            ClearStack(interp,2);
            Jsi_ValueMakeBool(interp,res, val);
        }
    }
}

static const char *vprint(Jsi_Value *v)
{
    static char buf[100];
    if (v->vt == JSI_VT_NUMBER) {
        snprintf(buf, 100, "NUM:%" JSI_NUMGFMT " ", v->d.num);
    } else if (v->vt == JSI_VT_BOOL) {
        snprintf(buf, 100, "BOO:%d ", v->d.val);
    } else if (v->vt == JSI_VT_STRING) {
        snprintf(buf, 100, "STR:%s ", v->d.s.str);
    } else if (v->vt == JSI_VT_VARIABLE) {
        snprintf(buf, 100, "VAR:%x ", (int)v->d.lval);
    } else if (v->vt == JSI_VT_NULL) {
        snprintf(buf, 100, "NUL:null ");
    } else if (v->vt == JSI_VT_OBJECT) {
        snprintf(buf, 100, "OBJ:%x", (int)v->d.obj);
    } else if (v->vt == JSI_VT_UNDEF) {
        snprintf(buf, 100, "UND:undefined");
    }
    return buf;
}

typedef enum {
        TL_TRY,
        TL_WITH,
} ttype;                            /* type of try */

typedef struct TryList {
    ttype type;
    union {
        struct {                    /* try data */
            OpCode *tstart;         /* try start ip */
            OpCode *tend;           /* try end ip */
            OpCode *cstart;         /* ...*/
            OpCode *cend;
            OpCode *fstart;
            OpCode *fend;
            int tsp;
            enum {
                LOP_NOOP,
                LOP_THROW,
                LOP_JMP
            } last_op;              /* what to do after finally block */
                                    /* depend on last jmp code in catch block */
            union {
                OpCode *tojmp;
            } ld;                   /* jmp out of catch (target)*/
        } td;
        struct {                    /* with data */
            OpCode *wstart;         /* with start */
            OpCode *wend;           /* with end */
        } wd;
    } d;
    
    jsi_ScopeChain *scope_save;         /* saved scope (used in catch block/with block)*/
    Jsi_Value *curscope_save;           /* saved current scope */
    struct TryList *next;
} TryList;

/* destroy top of trylist */
#define pop_try(head) _pop_try(interp, &head)
static INLINE void _pop_try(Jsi_Interp* interp, TryList **head)
{
    interp->tryDepth--;
    TryList *t = (*head)->next;
    Jsi_Free((*head));
    (*head) = t;
}

#define push_try(head, n) _push_try(interp, &head, n)
static INLINE void _push_try(Jsi_Interp* interp, TryList **head, TryList *n)
{
    interp->tryDepth++;
    (n)->next = (*head);
    (*head) = (n);
}

/* restore scope chain */
#define restore_scope() _restore_scope(interp, ps, trylist, \
    &scope, &currentScope, &context_id)
static INLINE void _restore_scope(Jsi_Interp* interp, jsi_Pstate *ps, TryList* trylist,
  jsi_ScopeChain **scope, Jsi_Value **currentScope, int *context_id) {

/* restore_scope(scope_save, curscope_save)*/
    if (*scope != (trylist->scope_save)) {
        jsi_ScopeChainFree(interp, *scope);
        *scope = (trylist->scope_save);
        interp->ingsc = *scope;
    }
    if (*currentScope != (trylist->curscope_save)) {
        Jsi_DecrRefCount(interp, *currentScope);
        *currentScope = (trylist->curscope_save); 
        interp->incsc = *currentScope;
    }
    *context_id = ps->_context_id++; 
}

#define do_throw() if (_do_throw(interp, ps, &ip, &trylist,&scope, &currentScope, &context_id, (interp->Sp?TOP:NULL)) != JSI_OK) { rc = JSI_ERROR; break; }

static int _do_throw(Jsi_Interp *interp, jsi_Pstate *ps, OpCode **ipp, TryList **tlp,
    jsi_ScopeChain **scope, Jsi_Value **currentScope, int *context_id, Jsi_Value *top) {
    if (Jsi_InterpGone(interp)) return JSI_ERROR;
    TryList *trylist = *tlp;
    while (1) {                      
        if (trylist == NULL) {
            char *str = (top?Jsi_ValueString(interp, top, NULL):"");
            if (str)
                Jsi_LogError("%s", str);
            return JSI_ERROR;
        }             
        if (trylist->type == TL_TRY) {                  
            int n = interp->Sp - trylist->d.td.tsp;              
            Pop(interp, n);                                   
            if (*ipp >= trylist->d.td.tstart && *ipp < trylist->d.td.tend) {        
                *ipp = trylist->d.td.cstart - 1;                                  
                break;                                                          
            } else if (*ipp >= trylist->d.td.cstart && *ipp < trylist->d.td.cend) { 
                trylist->d.td.last_op = LOP_THROW;                              
                *ipp = trylist->d.td.fstart - 1;                                  
                break;                                                          
            } else if (*ipp >= trylist->d.td.fstart && *ipp < trylist->d.td.fend) { 
                _pop_try(interp, tlp);                                               
                trylist = *tlp;                                                
            } else Jsi_LogBug("Throw within a try, but not in its scope?");            
        } else {                                                                
            _restore_scope(interp, ps, trylist, scope, currentScope, context_id);         
            _pop_try(interp, tlp);   
            trylist = *tlp;                                                
        }                                                                       
    }
    return JSI_OK;
}

static TryList *trylist_new(ttype t, jsi_ScopeChain *scope_save, Jsi_Value *curscope_save)
{
    TryList *n = Jsi_Calloc(1,sizeof(*n));
    
    n->type = t;
    n->curscope_save = curscope_save;
    /*Jsi_IncrRefCount(interp, curscope_save);*/
    n->scope_save = scope_save;
    
    return n;
}

static void DumpInstr(Jsi_Interp *interp, jsi_Pstate *ps, Jsi_Value *_this,
    TryList *trylist, OpCode *ip, OpCodes *opcodes)
{
    int i;
    if (0 && interp->debug <= 0xf) {
        return;
        if (ip->op != OP_FCALL) return;
        const char *s = interp->lastPushStr;
        const char *ff, *fname = ip->fname?ip->fname:"";
        if (interp->debug<0xf && ((ff=strrchr(fname,'/'))))
            fname = ff+1;
        printf("CALL: %s() in %s:%d\n", s?s:"", fname, ip->line);
        return;
    }
    printf("STACK%d: ", interp->Sp);
    for (i = 0; i < interp->Sp; ++i) {
        printf("%s ", vprint(StackIdx(i)));
    }
    printf("\tthis: %s ", vprint(_this));
    TryList *tlt = trylist;
    for (i = 0; tlt; tlt = tlt->next) i++;
    printf("TL: %d, excpt: %s\n", i, vprint(&ps->last_exception));
    jsi_code_decode(ip, ip - opcodes->codes);
}

static int cmpstringp(const void *p1, const void *p2)
{
   return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static  void SortDString(Jsi_Interp *interp, Jsi_DString *dStr) {
    int argc, i;
    char **argv;
    Jsi_DString sStr;
    Jsi_DSInit(&sStr);
    Jsi_SplitStr(Jsi_DSValue(dStr), &argc, &argv, " ", &sStr);
    qsort(argv, argc, sizeof(char*), cmpstringp);
    Jsi_DSFree(dStr);
    Jsi_DSInit(dStr);
    for (i=0; i<argc; i++) {
        Jsi_DSAppend(dStr, (i?" ":""), argv[i], NULL);
    }
    Jsi_DSFree(&sStr);
}

static void ValueObjDelete(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *key, int force)
{
    const char *kstr;
    if (target->vt != JSI_VT_OBJECT) return;

    Jsi_ValueToString(interp, key);
    kstr = key->d.s.str;
    Jsi_TreeEntry *hPtr;
    if (!key->f.bits.isstrkey) {
        Jsi_HashEntry *hePtr = Jsi_HashEntryFind(target->d.obj->tree->interp->strKeyTbl, key->d.s.str);
        if (hePtr)
            kstr = Jsi_HashKeyGet(hePtr);
    }
    hPtr = Jsi_TreeEntryFind(target->d.obj->tree, kstr);
    if (hPtr == NULL || (hPtr->f.bits.dontdel && !force))
        return;
    Jsi_TreeEntryDelete(hPtr);
}

static void ObjGetNames(Jsi_Interp *interp, Jsi_Obj *obj, Jsi_DString* dStr, int flags) {
    Jsi_TreeEntry *hPtr;
    Jsi_TreeSearch srch;
    Jsi_Value *v;
    int m = 0;
    Jsi_DSInit(dStr);
    if (obj->isArray)
        obj = interp->Array_prototype->d.obj;
    for (hPtr=Jsi_TreeSearchFirst(obj->tree, &srch,  JSI_TREE_INORDER); hPtr; hPtr=Jsi_TreeSearchNext(&srch)) {
        v = Jsi_TreeValueGet(hPtr);
        if (!v) continue;
        if ((flags&JSI_NAME_FUNCTIONS) && !Jsi_ValueIsFunction(interp,v)) {
            continue;
        }
        if ((flags&JSI_NAME_DATA) && Jsi_ValueIsFunction(interp,v)) {
            continue;
        }

        Jsi_DSAppend(dStr, (m++?" ":""), Jsi_TreeKeyGet(hPtr), NULL);
    }
    Jsi_TreeSearchDone(&srch);
}

static void DumpFunctions(Jsi_Interp *interp) {
    Jsi_DString dStr;
    Jsi_DSInit(&dStr);
    Jsi_HashEntry *hPtr;
    Jsi_HashSearch search;
    Jsi_CmdSpecItem *csi = NULL;
    Jsi_CmdSpec *cs;
    Jsi_Value *lsf = &interp->lastSubscriptFail;
    const char *spnam = "";
    int m = 0;
    if (interp->lastFuncIndex) {
        if (interp->lastFuncIndex->cmdSpec)
            spnam = interp->lastFuncIndex->cmdSpec->name;
    } else if (lsf->vt == JSI_VT_OBJECT) {
    
        spnam = interp->lastSubscriptFailStr;
        if (!spnam) spnam = interp->lastPushStr;
        if (lsf->d.obj->ot == JSI_OT_USEROBJ && lsf->d.obj->d.uobj->reg && lsf->d.obj->d.uobj->interp == interp) {
            cs = lsf->d.obj->d.uobj->reg->spec;
            if (cs)
                goto dumpspec;
        } else if (lsf->d.obj->ot == JSI_OT_OBJECT) {
            ObjGetNames(interp, lsf->d.obj, &dStr, JSI_NAME_FUNCTIONS);
            Jsi_LogError("'%s', functions are: %s.",
                spnam, Jsi_DSValue(&dStr));
            Jsi_DSFree(&dStr);
            return;
        } else {
            const char *sustr = NULL;
            switch (lsf->d.obj->ot) {
                case JSI_OT_STRING: sustr = "String"; break;
                case JSI_OT_NUMBER: sustr = "Number"; break;
                case JSI_OT_BOOL: sustr = "Boolean"; break;
            }
            if (sustr) {
                hPtr = Jsi_HashEntryFind(interp->cmdSpecTbl, sustr);
                csi = Jsi_HashValueGet(hPtr);
                cs = csi->spec;
                if (!spnam[0])
                    spnam = sustr;
                goto dumpspec;
            }
        }
    }
    if (!*spnam) {
        for (hPtr = Jsi_HashEntryFirst(interp->cmdSpecTbl, &search);
            hPtr; hPtr = Jsi_HashEntryNext(&search)) {
            csi = Jsi_HashValueGet(hPtr);
            if (csi->name && csi->name[0])
                Jsi_DSAppend(&dStr, (m++?" ":""), csi->name, NULL);
        }
    }
    
    if ((hPtr = Jsi_HashEntryFind(interp->cmdSpecTbl, spnam))) {
        csi = Jsi_HashValueGet(hPtr);
        while (csi) {
            int n;
            cs = csi->spec;
dumpspec:
            n = 0;
            while (cs->name) {
                if (n != 0 || !(cs->flags & JSI_CMD_IS_CONSTRUCTOR)) {
                    if (!*cs->name) continue;
                    Jsi_DSAppend(&dStr, (m?" ":""), cs->name, NULL);
                    n++; m++;
                }
                cs++;
            }
            csi = (csi?csi->next:NULL);
        }
        SortDString(interp, &dStr);
        if (*spnam==0 && interp->lastPushVar)
            spnam = interp->lastPushVar;
        Jsi_LogError("'%s' is not one of: %s.",
            spnam, Jsi_DSValue(&dStr));
        Jsi_DSFree(&dStr);
    } else {
        Jsi_LogError("can not execute expression: '%s' not a function\n",
            interp->lastPushVar ? interp->lastPushVar : "");
    }
}

/* Attempt to dynamically load function XX by doing an eval of jsiIndex.XX */
/* TODO: prevent infinite loop/recursion. */
static Jsi_Value *LoadFunction(Jsi_Interp *interp, const char *str, Jsi_Value *tret) {
    Jsi_DString dStr = {};
    Jsi_Value *v;
    int i;
    for (i=0; i<2; i++) {
        Jsi_DSAppend(&dStr, "jsiIndex.", str, NULL);
        Jsi_VarLookup(interp, Jsi_DSValue(&dStr));
        v = Jsi_NameLookup(interp, Jsi_DSValue(&dStr));
        Jsi_DSFree(&dStr);
        if (v) {
            const char *cp = Jsi_ValueGetDString(interp, v, &dStr, 0);
            /*printf("JSS: %s\n", cp);*/
            v = NULL;
            if (Jsi_EvalString(interp, cp, 0) == JSI_OK)
                v = Jsi_NameLookup(interp, str);
            Jsi_DSFree(&dStr);
            if (v) {
                tret = v;
                break;
            }
        }
        if (interp->indexLoaded++ || i>0)
            break;
        /*  Index not in memory, so try loading jsiIndex from the file jsiIndex.jsi */
        if (interp->indexFiles == NULL)
            return tret;
        Jsi_Value **ifs = &interp->indexFiles;
        int i, ifn = 1;
        if (Jsi_ValueIsArray(interp, interp->indexFiles)) {
            ifs = interp->indexFiles->d.obj->arr;
            ifn = interp->indexFiles->d.obj->arrCnt;
        }
        for (i=0; i<ifn; i++) {  
            if (Jsi_EvalFile(interp, ifs[i], 0) != JSI_OK)
                break;
            interp->indexLoaded++;
        }
    }
    interp->lastPushVar = (char*)str;
    return tret;
}

static INLINE int EvalFunction(register jsi_Pstate *ps, OpCode *ip, int discard) {
    int excpt_ret = JSI_OK;
    register Jsi_Interp *interp = ps->interp;
    int as_constructor = (ip->op == OP_NEWFCALL);
    int stackargc = (int)ip->data;
    VarDeref(interp, stackargc + 1);
    
    int tocall_index = interp->Sp - stackargc - 1, adds;
    Jsi_Value *tocall = StackIdx(tocall_index);
    char* failStr = interp->lastSubscriptFailStr;
    if (Jsi_ValueIsUndef(interp, tocall) && interp->lastPushVar && failStr == 0) {
        tocall = LoadFunction(interp, interp->lastPushVar, tocall);
        interp->curIp = ip;
    }
    if (!Jsi_ValueIsFunction(interp, tocall)) {
        DumpFunctions(interp);
        excpt_ret = JSI_ERROR;
        interp->lastSubscriptFailStr = NULL;
        goto empty_func;
    }
    interp->lastSubscriptFailStr = NULL;
    interp->lastFuncIndex = NULL;

    if (!tocall->d.obj->d.fobj) {   /* empty function */
empty_func:
        Pop(interp, stackargc);
        ClearStack(interp,1);
        Jsi_ValueMakeUndef(interp,TOP);
    } else {
        Jsi_Func *fstatic = tocall->d.obj->d.fobj->func;
        if (interp->nDebug && fstatic->callback == jsi_AssertCmd)
            goto empty_func;
        adds = fstatic->callflags.bits.addargs;
        fstatic->callflags.bits.addargs = 0;
        if (adds && (fstatic->cmdSpec->flags&JSI_CMDSPEC_NONTHIS))
            adds = 0;
        /* create new scope, prepare arguments */
        /* here we shared scope and 'arguments' with the same object */
        /* so that arguments[0] is easier to shared space with first local variable */
        Jsi_Value *newscope = Jsi_ValueNew1(interp);
        Jsi_Obj *ao = Jsi_ObjNewArray(interp, stack+(interp->Sp - stackargc), stackargc);

        if (fstatic->bindArgs) {
            int nc, bargc = Jsi_ValueGetLength(interp, fstatic->bindArgs)-1;
            if (bargc > 0) {
                jsi_ArraySizer(interp, ao, nc=(ao->arrCnt+bargc));
                memmove(ao->arr+bargc, ao->arr, sizeof(Jsi_Value*)*ao->arrCnt);
                memmove(ao->arr, fstatic->bindArgs->d.obj->arr+1, sizeof(Jsi_Value*)*bargc);
                ao->arrCnt = nc;
            }
        }
        Jsi_ValueMakeObject(interp, newscope, ao);
        newscope->d.obj->__proto__ = interp->Object_prototype;          /* ecma */
        
        jsi_SharedArgs(interp, newscope, fstatic->argnames, 1); /* make arg vars to shared arguments */
        jsi_InitLocalVar(interp, newscope, fstatic);
        jsi_SetCallee(interp, newscope, tocall);
        
        Pop(interp, stackargc);
    
        Jsi_Value newthis = VALINIT;
        if (fstatic->bindArgs && Jsi_ValueTypeGet(fstatic->bindArgs->d.obj->arr[0]) != JSI_VT_UNDEF) {
            Jsi_ValueCopy(interp,&newthis, Jsi_ValueArrayIndex(interp, fstatic->bindArgs, 0));
            
        } else if (ObjThisIdx(tocall_index)->vt == JSI_VT_OBJECT) {
            Jsi_ValueCopy(interp,&newthis, ObjThisIdx(tocall_index));
            ClearThis(interp, tocall_index);
        } else {
            Jsi_ValueCopy(interp,&newthis, interp->Top_object);
        }

        if (as_constructor) {                       /* new Constructor */
            Jsi_Obj *newobj = Jsi_ObjNewType(interp, JSI_OT_OBJECT);
            Jsi_Value *proto = Jsi_ValueObjLookup(interp, tocall, "prototype", 0);
            if (proto && proto->vt == JSI_VT_OBJECT)
                newobj->__proto__ = proto;
            Jsi_ValueReset(interp, &newthis);
            Jsi_ValueMakeObject(interp, &newthis, newobj);            
            /* TODO: constructor specifics??? */
        }
        
        
        if (interp->traceCalls && fstatic->name) {
            const char *ff, *fname = ip->fname?ip->fname:"";
            if (interp->debug<0xf && ((ff=strrchr(fname,'/'))))
                fname = ff+1;
            if (interp->traceHook)
                (*interp->traceHook)(interp, fstatic->name, ip->fname, ip->line, fstatic->cmdSpec, &newthis, newscope);
            else
                printf("%*s<#%d>: %s() in %s:%d\n", (interp->level-1)*2, "", interp->level, fstatic->name, fname, ip->line);
        }

        Jsi_Value spret = VALINIT;
        Jsi_Value *spretPtr = &spret;
        if (fstatic->type == FC_NORMAL) {
            excpt_ret = jsi_evalcode(ps, fstatic->opcodes, tocall->d.obj->d.fobj->scope, 
                newscope, &newthis, &spretPtr);
            interp->funcCallCnt++;
        } else if (!fstatic->callback) {
            Jsi_LogError("can not call:\"%s()\"", fstatic->name);
        } else {
            int docall = 1, oldcf = fstatic->callflags.i;
            fstatic->callflags.bits.iscons = (as_constructor?JSI_CALL_CONSTRUCTOR:0);
            if (fstatic->f.bits.hasattr)
            {
#define SPTR(s) (s?s:"")
                if ((fstatic->f.bits.isobj) && newthis.vt != JSI_VT_OBJECT) {
                    excpt_ret = JSI_ERROR;
                    docall = 0;
                    Jsi_LogError("'this' is not object: \"%s()\"", fstatic->name);
                } else if ((!(fstatic->f.bits.iscons)) && as_constructor) {
                    excpt_ret = JSI_ERROR;
                    docall = 0;
                    Jsi_LogError("can not call as constructor: \"%s()\"", fstatic->name);
                } else {
                    int aCnt = Jsi_ValueGetLength(interp, newscope);
                    if (aCnt<(fstatic->cmdSpec->minArgs+adds)) {
                        Jsi_LogError("missing args, expected \"%s(%s)\" ", fstatic->cmdSpec->name, SPTR(fstatic->cmdSpec->argStr));
                        excpt_ret = JSI_ERROR;
                        docall = 0;
                    } else if (fstatic->cmdSpec->maxArgs>=0 && (aCnt>fstatic->cmdSpec->maxArgs+adds)) {
                        Jsi_LogError("extra args, expected \"%s(%s)\" ", fstatic->cmdSpec->name, SPTR(fstatic->cmdSpec->argStr));
                        excpt_ret = JSI_ERROR;
                        docall = 0;
                    }
                }
            }
            if (docall) {
                fstatic->callflags.bits.isdiscard = discard;
                excpt_ret = fstatic->callback(interp, newscope, 
                    &newthis, &spretPtr, fstatic);
                interp->cmdCallCnt++;
            }
            fstatic->callflags.i = oldcf;
        }
    
        if (as_constructor) {
            if (newthis.vt == JSI_VT_OBJECT)
                newthis.d.obj->constructor = tocall->d.obj;
            if (spret.vt != JSI_VT_OBJECT) {
                Jsi_ValueReset(interp,&spret);
                Jsi_ValueCopy(interp,&spret, &newthis);
            }
        }
        
        jsi_SharedArgs(interp, newscope, fstatic->argnames, 0); /* make arg vars to shared arguments */
        Jsi_ValueReset(interp,&newthis);
        ClearStack(interp,1);
        if (spretPtr == &spret)
            StackReplace(interp, TOP, &spret); 
        else {
            /*TODO: support c called extension returning a (non-copied) value reference?*/
            Jsi_DecrRefCount(interp, TOP);
            *TOP = *spretPtr;
        }
        Jsi_DecrRefCount(interp, newscope);
    }
    return excpt_ret;
}

static INLINE void PushVar(jsi_Pstate *ps, OpCode *ip, jsi_ScopeChain *scope, Jsi_Value *currentScope, int context_id) {
    register Jsi_Interp *interp = ps->interp;
    FastVar *n = ip->data;
    SIGASSERT(n,FASTVAR);
    Jsi_Value *dv = StackIdx(interp->Sp), *v = NULL;
    int isglob = 0;
    if (n->context_id == context_id && n->ps == ps) {
        v = n->var.lval;
    } else {
        char *varname = n->var.varname;
        if (*varname == 'n' && Jsi_Strcmp(varname,"null") == 0)
            v = interp->NullValue;
        else {
            v = Jsi_ValueObjLookup(interp, currentScope, varname, 1);
        }
        if (!v)
            v = jsi_ScopeChainObjLookupUni(scope, varname);
        if (!v) {
            /* add to global scope.  TODO: do not define if a right_val??? */
            Jsi_Value *global_scope = scope->chains_cnt > 0 ? scope->chains[0]:currentScope;
            Jsi_Value key = VALINIT;
            Jsi_ValueMakeStringKey(interp, &key, varname);
            interp->lastPushVar = key.d.s.str;
            v = jsi_ValueObjKeyAssign(interp, global_scope, &key, NULL, JSI_OM_DONTENUM);
            int isNew;
            Jsi_HashEntry *hPtr = Jsi_HashEntryCreate(interp->varTbl, varname, &isNew);
            if (hPtr && isNew)
                Jsi_HashValueSet(hPtr, 0);
        }
        
        Jsi_IncrRefCount(interp, v);
        if (v->vt == JSI_VT_OBJECT && v->d.obj->ot == JSI_OT_OBJECT)
            v->f.bits.onstack = 1;  /* Indicate that a double free is required for object. */
        
        n->context_id = context_id;
        if (n->var.lval != v) {
            n->isglob = isglob;
            n->var.lval = v;
            SIGASSERT(v, VALUE);
        }
    }
    if (dv->vt != JSI_VT_VARIABLE || dv->d.lval != v) {
        if (dv->vt != JSI_VT_VARIABLE)
            Jsi_ValueReset(interp,dv);
        dv->vt = JSI_VT_VARIABLE;
        SIGASSERT(v, VALUE);
        dv->d.lval = v;
    }
    SIGASSERT(v, VALUE);
    Push(interp,1);
}

static INLINE void PushFunc(jsi_Pstate *ps, OpCode *ip, jsi_ScopeChain *scope, Jsi_Value *currentScope) {
    /* TODO: now that we're caching ps, may need to reference function ps for context_id??? */
    Jsi_Interp *interp = ps->interp;
    Jsi_FuncObj *fo = jsi_FuncObjNew(interp, (Jsi_Func *)ip->data);
    fo->scope = jsi_ScopeChainDupNext(interp, scope, currentScope);
    
    Jsi_Obj *obj = Jsi_ObjNewType(interp, JSI_OT_FUNCTION);
    obj->d.fobj = fo;
    
    Jsi_Value *v = StackIdx(interp->Sp), *fun_prototype = jsi_ObjValueNew(interp);
    fun_prototype->d.obj->__proto__ = interp->Object_prototype;                
    Jsi_ValueMakeObject(interp, v, obj);
    Jsi_ValueInsert(interp, v, "prototype", fun_prototype, JSI_OM_DONTDEL|JSI_OM_DONTENUM);
    /* TODO: make own prototype and prototype.constructor */
    
    if (interp->lastPushVar && interp->Sp == 1 && StackIdx(0)->vt == JSI_VT_VARIABLE) {
        int isNew;
        char *key = interp->lastPushVar;
        Jsi_HashEntry *hPtr;
        hPtr = Jsi_HashEntryCreate(interp->funcTbl, key, &isNew);
        if (hPtr && isNew) {
            if (!fo->func->name)
                fo->func->name = interp->lastPushVar;
            Jsi_HashValueSet(hPtr, obj);
            hPtr = Jsi_HashEntryCreate(interp->varTbl, key, &isNew);
            if (hPtr)
                Jsi_HashValueSet(hPtr, obj);
        }
    }
    Push(interp,1);
}

int _jsi_evalcode(register jsi_Pstate *ps, OpCodes *opcodes, 
     jsi_ScopeChain *scope, Jsi_Value *currentScope,
     Jsi_Value *_this, Jsi_Value *vret)
{
    register Jsi_Interp* interp = ps->interp;
    OpCode *ip = &opcodes->codes[0];
    int rc = JSI_OK, context_id = ps->_context_id++;
    OpCode *end = &opcodes->codes[opcodes->code_len];
    TryList  *trylist = NULL;
    
    if (currentScope->vt != JSI_VT_OBJECT) {
        Jsi_LogBug("Eval: current scope is not a object\n");
        return JSI_ERROR;
    }
    
    while(ip < end && rc == JSI_OK) {
        if (interp->exited) {
            rc = JSI_ERROR;
            break;
        }
        interp->opCnt++;
        if (interp->maxOpCnt && interp->maxOpCnt > interp->opCnt) {
            Jsi_LogError("Execution cap exceeded");
            rc = JSI_ERROR;
        }
        if (interp->level > interp->maxDepth) {
            Jsi_LogError("Infinite recursion detected?");
            rc = JSI_ERROR;
            break;
        }
        if (interp->debug) {
            DumpInstr(interp, ps, _this, trylist, ip, opcodes);
        }
        
#ifndef USE_STATIC_STACK
        if ((interp->maxStack-interp->Sp)<STACK_MIN_PAD)
            SetupStack(interp);
#endif
        Push(interp,0);
        interp->curIp = ip;
        if (interp->evalHook && (*interp->evalHook)(interp, interp->curFile, interp->curIp->line) != JSI_OK) {
            rc = JSI_ERROR;
            break;
        }
        switch(ip->op) {
            case OP_NOP:
            case OP_LASTOP:
                break;
            case OP_PUSHUND:
                Jsi_ValueMakeUndef(interp,StackIdx(interp->Sp));
                Push(interp,1);
                break;
            case OP_PUSHBOO:
                Jsi_ValueMakeBool(interp,StackIdx(interp->Sp), (int)ip->data);
                Push(interp,1);
                break;
            case OP_PUSHNUM:
                Jsi_ValueMakeNumber(interp,StackIdx(interp->Sp), (*((Jsi_Number *)ip->data)));
                Push(interp,1);
                break;
            case OP_PUSHSTR: {
                interp->lastFuncIndex = NULL;
                Jsi_ValueMakeStringKey(interp,StackIdx(interp->Sp), ip->data);
                interp->lastPushStr = StackIdx(interp->Sp)->d.s.str;
                Push(interp,1);
                break;
            }
            case OP_PUSHVAR: {
                PushVar(ps, ip, scope, currentScope, context_id);
                break;
            }
            case OP_PUSHFUN: {
                PushFunc(ps, ip, scope, currentScope);
                break;
            }
            case OP_NEWFCALL:
                if (interp->maxUserObjs && interp->userObjCnt > interp->maxUserObjs) {
                    Jsi_LogError("Max 'new' count exceeded");
                    rc = JSI_ERROR;
                    break;
                }
            case OP_FCALL: {
                /* TODO: need reliable way to capture func string name to handle unknown functions.*/
                int discard = ((ip+2)<end && ip[1].op == OP_POP);
                if (EvalFunction(ps, ip, discard) != JSI_OK) {        /* throw an execption */
                    do_throw();

                }
                /* TODO: new Function return a function without scopechain, add here */
                break;
            }
            case OP_SUBSCRIPT: {
                Jsi_Value *src = TOQ, *idx = TOP;
                VarDeref(interp,2);
                Jsi_Value res = VALINIT;
                if (Jsi_ValueIsString(interp, src) && Jsi_ValueIsNumber(interp, idx)) {
                    int sLen;
                    char bbuf[10], *cp = Jsi_ValueString(interp, src, &sLen);
                    int n = (int)idx->d.num;
                    if (n<0 || n>=sLen) {
                        Jsi_ValueMakeUndef(interp, src);
                    } else {
                        bbuf[1] = 0;
                        bbuf[0] = cp[n];
                        Jsi_ValueMakeStringDup(interp, src, bbuf);
                    }
                    Pop(interp, 1);
                    break;
                }
                Jsi_ValueToObject(interp, src);
                if (interp->hasCallee && src->d.obj == currentScope->d.obj) {
                    if (idx->vt == JSI_VT_STRING && Jsi_Strcmp(idx->d.s.str, "callee") == 0) {
                        ClearStack(interp,1);
                        Jsi_ValueMakeStringKey(interp, idx, "\1callee\1");
                    }
                }
                if (interp->lastSubscriptFail.vt != JSI_VT_UNDEF) {
                    Jsi_ValueReset(interp, &interp->lastSubscriptFail);
                    interp->lastSubscriptFailStr = NULL;
                }
                if (src->vt != JSI_VT_UNDEF) {
                    jsi_ValueSubscriptLen(interp, src, idx, &res, (int)ip->data);
                    int isfcall = ((ip+3)<end && ip[2].op == OP_FCALL);
                    if (res.vt == JSI_VT_UNDEF && isfcall) {
                        /* eg. so we can list available commands for  "db.xx()" */
                        Jsi_ValueCopy(interp, &interp->lastSubscriptFail, src);
                        if (idx->vt == JSI_VT_STRING)
                            interp->lastSubscriptFailStr = idx->d.s.str;
                    }
                    ClearStack(interp,2);
                    StackCopy(interp, src, &res);      /*TODO: is this right*/
                }
                Pop(interp, 1);
                break;
            }
            case OP_PUSHREG: {
                Jsi_Obj *obj = Jsi_ObjNewType(interp, JSI_OT_REGEXP);
                obj->d.robj = (Jsi_Regex *)ip->data;
                Jsi_ValueMakeObject(interp,StackIdx(interp->Sp), obj);
                Push(interp,1);
                break;
            }
            case OP_PUSHARG:
                Jsi_ValueCopy(interp,StackIdx(interp->Sp), currentScope);
                Jsi_ObjIncrRefCount(interp, currentScope->d.obj);
                Push(interp,1);
                break;
            case OP_PUSHTHS:
                Jsi_ValueCopy(interp,StackIdx(interp->Sp), _this);
                Jsi_ObjIncrRefCount(interp, _this->d.obj);
                Push(interp,1);
                break;
            case OP_PUSHTOP:
                Jsi_ValueCopy(interp,StackIdx(interp->Sp), TOP);
                Push(interp,1);
                break;
            case OP_UNREF:
                VarDeref(interp,1);
                break;
            case OP_PUSHTOP2:
                Jsi_ValueCopy(interp, StackIdx(interp->Sp), TOQ);
                Jsi_ValueCopy(interp, StackIdx(interp->Sp+1), TOP);
                Push(interp, 2);
                break;
            case OP_CHTHIS: {
                if (ip->data) {
                    int t = interp->Sp - 2;
                    Assert(t>=0);
                    Jsi_Value *v = ObjThisIdx(t);
                    ClearThis(interp, t);
                    Jsi_ValueCopy(interp, v, TOQ);
                    if (v->vt == JSI_VT_VARIABLE) {
                        Jsi_ValueCopy(interp, v, v->d.lval);
                    }
                    Jsi_ValueToObject(interp, v);
                }
                break;
            }
            case OP_LOCAL: {
                Jsi_Value key = VALINIT;
                Jsi_ValueMakeString(interp, &key, ip->data);
                jsi_ValueObjKeyAssign(interp, currentScope, &key, NULL, JSI_OM_DONTENUM);
                
                /* make all FastVar to be relocated */
                context_id = ps->_context_id++;
                break;
            }
            case OP_POP:
                if ((interp->evalFlags&JSI_EVAL_RETURN) && (ip+1) >= end && 
                (Jsi_ValueIsObjType(interp, TOP, JSI_OT_ITER)==0 &&
                Jsi_ValueIsObjType(interp, TOP, JSI_OT_FUNCTION)==0)) {
                    /* Interactive and last instruction is a pop: save result. */
                    StackCopy(interp, vret,TOP); /*TODO: correct?*/
                    TOP->vt = JSI_VT_UNDEF;
                }
                Pop(interp, (int)ip->data);
                break;
            case OP_NEG:
                VarDeref(interp,1);
                Jsi_ValueToNumber(interp, TOP);
                TOP->d.num = -(TOP->d.num);
                break;
            case OP_POS:
                VarDeref(interp,1);
                Jsi_ValueToNumber(interp, TOP);
                break;
            case OP_NOT: {
                int val = 0;
                VarDeref(interp,1);
                
                val = Jsi_ValueIsTrue(interp, TOP);
                
                ClearStack(interp,1);
                Jsi_ValueMakeBool(interp,TOP, !val);
                break;
            }
            case OP_BNOT: {
                VarDeref(interp,1);
                jsi_ValueToOInt32(interp, TOP);
                TOP->d.num = (Jsi_Number)(~((int)TOP->d.num));
                break;
            }
            case OP_ADD: {
                VarDeref(interp,2);
                Jsi_ValueToPrimitive(interp, TOP);
                Jsi_ValueToPrimitive(interp, TOQ);
                
                if (TOP->vt == JSI_VT_STRING || TOQ->vt == JSI_VT_STRING) {
                    Jsi_ValueToString(interp, TOP);
                    Jsi_ValueToString(interp, TOQ);
                    
                    char *v = Jsi_Strcatdup(TOQ->d.s.str, TOP->d.s.str);
                    
                    ClearStack(interp,2);
                    Jsi_ValueMakeStringDup(interp,TOQ, v);
                    Jsi_Free(v);
                } else {
                    Jsi_ValueToNumber(interp, TOP);
                    Jsi_ValueToNumber(interp, TOQ);
                    Jsi_Number n = TOP->d.num + TOQ->d.num;
                    ClearStack(interp,2);
                    Jsi_ValueMakeNumber(interp,TOQ, n);
                }
                Pop(interp,1);
                break;
            }
            case OP_IN: {
                Jsi_Value *v, *vl;
                const char *cp = NULL;
                Jsi_Number nval;
                VarDeref(interp,2);
                vl = TOQ;
                v = TOP;
                if (Jsi_ValueIsString(interp,vl))
                    cp = Jsi_ValueGetStringLen(interp, vl, NULL);
                else if (Jsi_ValueIsNumber(interp,vl))
                    Jsi_ValueGetNumber(interp, vl, &nval);
                else {
                    /*Jsi_LogError("expected string or number before IN");*/
                    Jsi_ValueMakeBool(interp, TOQ, 0);
                    Pop(interp,1);
                    break;
                }
                
                if (v->vt == JSI_VT_VARIABLE) {
                    v = v->d.lval;
                    SIGASSERT(v, VALUE);
                }
                if (v->vt != JSI_VT_OBJECT || v->d.obj->ot != JSI_OT_OBJECT) {
                    /*Jsi_LogError("expected object after IN");*/
                    Jsi_ValueMakeBool(interp, TOQ, 0);
                    Pop(interp,1);
                    break;
                }
                int bval = 0;
                char nbuf[100];
                Jsi_Value *vv;
                Jsi_Obj *obj = v->d.obj;
                if (!cp) {
                    sprintf(nbuf, "%d", (int)nval);
                    cp = nbuf;
                }
                if (obj->arr) {
                    vv = jsi_ObjArrayLookup(interp, obj, (char*)cp);
                } else {
                    vv = Jsi_TreeObjGetValue(obj, (char*)cp, 1);
                }
                bval = (vv != 0);
                Jsi_ValueMakeBool(interp, TOQ, bval);
                Pop(interp,1);
                break;
            }
            case OP_SUB: 
                common_math_opr(-); break;
            case OP_MUL:
                common_math_opr(*); break;
            case OP_DIV:
                common_math_opr(/); break;
            case OP_MOD: {
                VarDeref(interp,2);
                if (!Jsi_ValueIsType(interp,TOP, JSI_VT_NUMBER)) Jsi_ValueToNumber(interp, TOP);
                if (!Jsi_ValueIsType(interp,TOQ, JSI_VT_NUMBER)) Jsi_ValueToNumber(interp, TOQ);

                TOQ->d.num = fmod(TOQ->d.num, TOP->d.num);
                Pop(interp,1);
                break;
            }
            case OP_LESS:
                VarDeref(interp,2);
                logic_less(interp,2,1);
                Pop(interp,1);
                break;
            case OP_GREATER:
                VarDeref(interp,2);
                logic_less(interp,1,2);
                Pop(interp,1);
                break;
            case OP_LESSEQU:
                VarDeref(interp,2);
                logic_less(interp,1,2);
                TOQ->d.val = !TOQ->d.val;
                Pop(interp,1);
                break;
            case OP_GREATEREQU:
                VarDeref(interp,2);
                logic_less(interp,2,1);
                TOQ->d.val = !TOQ->d.val;
                Pop(interp,1);
                break;
            case OP_EQUAL:
            case OP_NOTEQUAL: {
                VarDeref(interp,2);
                int r = Jsi_ValueCmp(interp, TOP, TOQ, 0);
                r = (ip->op == OP_EQUAL ? !r : r);
                ClearStack(interp,2);
                Jsi_ValueMakeBool(interp,TOQ, r);
                Pop(interp,1);
                break;
            }
            case OP_STRICTEQU:
            case OP_STRICTNEQ: {
                int r = 0;
                VarDeref(interp,2);
                r = Jsi_ValueCmp(interp, TOP, TOQ, JSI_CMP_EXACT);
                r = (ip->op == OP_STRICTEQU ? !r : r);
                ClearStack(interp,2);
                Jsi_ValueMakeBool(interp,TOQ, r);
                Pop(interp,1);
                break;
            }
            case OP_BAND: 
                common_bitwise_opr(&); break;
            case OP_BOR:
                common_bitwise_opr(|); break;
            case OP_BXOR:
                common_bitwise_opr(^); break;
            case OP_SHF: {
                VarDeref(interp,2);
                jsi_ValueToOInt32(interp, TOQ);
                jsi_ValueToOInt32(interp, TOP);
                int t1 = (int)TOQ->d.num;
                int t2 = ((unsigned int)TOP->d.num) & 0x1f;
                if (ip->data) {                 /* shift right */
                    if ((int)ip->data == 2) {   /* unsigned shift */
                        unsigned int t3 = (unsigned int)t1;
                        t3 >>= t2;
                        Jsi_ValueMakeNumber(interp,TOQ, t3);
                    } else {
                        t1 >>= t2;
                        Jsi_ValueMakeNumber(interp,TOQ, t1);
                    }
                } else {
                    t1 <<= t2;
                    Jsi_ValueMakeNumber(interp,TOQ, t1);
                }
                Pop(interp,1);
                break;
            }
            case OP_ASSIGN: {
                if ((int)ip->data == 1) {
                    VarDeref(interp,1);
                    ValueAssign(interp);                    
                    Pop(interp,1);
                } else {
                    VarDeref(interp, 3);
                    Jsi_Value *v3 = StackIdx(interp->Sp-3);
                    if (v3->vt == JSI_VT_OBJECT) {
                        jsi_ValueObjKeyAssign(interp, v3, TOQ, TOP, 0);
                    } else
                        Jsi_LogWarn("assign to a non-exist object\n");
                    ClearStack(interp,3);
                    Jsi_ValueCopy(interp,v3, TOP);
                    Pop(interp, 2);
                }
                break;
            }
            case OP_KEY: {
                VarDeref(interp,1);
                if (TOP->vt != JSI_VT_UNDEF && TOP->vt != JSI_VT_NULL)
                    Jsi_ValueToObject(interp, TOP);
                Jsi_Value spret = VALINIT;
                jsi_ValueObjGetKeys(interp, TOP, &spret);
                StackCopy(interp, stack[interp->Sp], &spret);
                Push(interp,1);
                break;
            }
            case OP_NEXT: {
                if (TOQ->vt != JSI_VT_OBJECT || TOQ->d.obj->ot != JSI_OT_ITER) Jsi_LogBug("next: TOQ not a iter\n");
                if (TOP->vt != JSI_VT_VARIABLE) {
                    Jsi_LogError ("invalid for/in left hand-side\n");
                    break;
                }
                
                Jsi_IterObj *io = TOQ->d.obj->d.iobj;
                if (io->iterCmd) {
                    io->iterCmd(io, TOP, StackIdx(interp->Sp-3), io->iter++);
                } else {
                    while (io->iter < io->count) {
                        if (!io->isArrayList) {
                            if (Jsi_ValueKeyPresent(interp, StackIdx(interp->Sp-3), io->keys[io->iter],1)) 
                                break;
                        } else {
                            while (io->cur < io->obj->arrCnt) {
                                if (io->obj->arr[io->cur]) break;
                                io->cur++;
                            }
                            if (io->cur >= io->obj->arrCnt) {
                                /* TODO: Is this really a bug??? */
                                /* Jsi_LogBug("NOT FOUND LIST ARRAY\n");*/
                                io->iter = io->count;
                                break;
                            } else if (io->obj->arr[io->cur]) {
                                io->cur++;
                                break;
                            }
                        }
                        io->iter++;
                    }
                    if (io->iter >= io->count) {
                        ClearStack(interp,1);
                        Jsi_ValueMakeNumber(interp,TOP, 0);
                    } else {
                        Jsi_Value *v = TOP->d.lval;
                        SIGASSERT(v, VALUE);
                        Jsi_ValueReset(interp,v);
                        if (io->isArrayList)
                            Jsi_ValueMakeNumber(interp, v, io->cur-1);
                        else
                            Jsi_ValueMakeStringKey(interp, v, io->keys[io->iter]);
                        io->iter++;
                        
                        ClearStack(interp,1);
                        Jsi_ValueMakeNumber(interp,TOP, 1);
                    }
                    break;
                }
            }
            case OP_INC:
            case OP_DEC: {
                int inc = ip->op == OP_INC ? 1 : -1;
                
                if (TOP->vt != JSI_VT_VARIABLE) {
                    Jsi_LogError("operand not left value\n");
                    break;
                }
                Jsi_Value *v = TOP->d.lval;
                SIGASSERT(v, VALUE);
                Jsi_ValueToNumber(interp, v);
                
                v->d.num += inc;
                    
                VarDeref(interp,1);
                if (ip->data) {
                    TOP->d.num -= inc;
                }
                break;
            }
            case OP_TYPEOF: {
                const char *typ;
                Jsi_Value *v = TOP;
                if (v->vt == JSI_VT_VARIABLE) {
                    v = v->d.lval;
                    SIGASSERT(v, VALUE);
                }
                typ = Jsi_ValueTypeStr(interp, v);
                VarDeref(interp,1);
                Jsi_ValueMakeStringKey(interp, TOP, (char*)typ);
                break;
            }
            case OP_INSTANCEOF: {

                VarDeref(interp,2);
                int bval = Jsi_ValueInstanceOf(interp, TOQ, TOP);
                Pop(interp,1);
                Jsi_ValueMakeBool(interp, TOP, bval);
                break;
            }
            case OP_JTRUE:
            case OP_JFALSE: 
            case OP_JTRUE_NP:
            case OP_JFALSE_NP: {
                VarDeref(interp,1);
                int off = (int)ip->data - 1; 
                int r = Jsi_ValueIsTrue(interp, TOP);
                
                if (ip->op == OP_JTRUE || ip->op == OP_JFALSE) Pop(interp,1);
                ip += ((ip->op == OP_JTRUE || ip->op == OP_JTRUE_NP) ^ r) ? 0 : off;
                break;
            }
            case OP_JMPPOP: 
                Pop(interp, ((JmpPopInfo *)ip->data)->topop);
            case OP_JMP: {
                int off = ip->op == OP_JMP ? (int)ip->data - 1
                            : ((JmpPopInfo *)ip->data)->off - 1;

                while (1) {
                    if (trylist == NULL) break;
                    OpCode *tojmp = ip + off;

                    /* jmp out of a try block, should execute the finally block */
                    /* while jmp out a 'with' block, restore the scope */

                    if (trylist->type == TL_TRY) { 
                        if (tojmp >= trylist->d.td.tstart && tojmp < trylist->d.td.fend) break;
                        
                        if (ip >= trylist->d.td.tstart && ip < trylist->d.td.cend) {
                            trylist->d.td.last_op = LOP_JMP;
                            trylist->d.td.ld.tojmp = tojmp;
                            
                            ip = trylist->d.td.fstart - 1;
                            off = 0;
                            break;
                        } else if (ip >= trylist->d.td.fstart && ip < trylist->d.td.fend) {
                            pop_try(trylist);
                        } else Jsi_LogBug("jmp within a try, but not in its scope?");
                    } else {
                        /* with block */
                        
                        if (tojmp >= trylist->d.wd.wstart && tojmp < trylist->d.wd.wend) break;
                        
                        restore_scope();
                        pop_try(trylist);
                    }
                }
                
                ip += off;
                break;
            }
            case OP_EVAL: {
                int stackargc = (int)ip->data;
                VarDeref(interp, stackargc);

                Jsi_Value spret = VALINIT;
                int r = 0;
                if (stackargc > 0) {
                    if (StackIdx(interp->Sp - stackargc)->vt == JSI_VT_UNDEF) {
                        Jsi_LogError("eval undefined value\n");
                        goto undef_eval;
                    }
                    if (StackIdx(interp->Sp - stackargc)->vt == JSI_VT_STRING) {
                        char *pro = Jsi_Strdup(StackIdx(interp->Sp - stackargc)->d.s.str);
                        Jsi_Value *spPtr = &spret;
                        r = jsi_global_eval(interp, ps, pro, scope, currentScope, _this, &spPtr);
                        Jsi_Free(pro);
                    } else {
                        Jsi_ValueCopy(interp,&spret, StackIdx(interp->Sp - stackargc));
                    }
                }
undef_eval:
                Pop(interp, stackargc);
                StackCopy(interp, stack[interp->Sp], &spret); /*TODO: is this correct?*/
                Push(interp,1);

                if (r) {
                    do_throw();
                }
                break;
            }
            case OP_RET: {
                if (interp->Sp>=1) {
                    VarDeref(interp,1);
                    Jsi_ValueCopy(interp,vret, TOP);
                }
                Pop(interp, (int)ip->data);
                interp->didReturn = 1;
                if (trylist) {
                    int isTry = 0;
                    while (trylist) {
                        if (trylist->type == TL_TRY)
                            isTry = 1;
                        pop_try(trylist);
                    }
                    if (isTry)
                        Jsi_LogWarn("return inside try/catch is unsupported");
                    goto done;
                }
                return JSI_OK;
            }
            case OP_DELETE: {
                int count = (int)ip->data;
                if (count == 1) {
                    if (TOP->vt != JSI_VT_VARIABLE) {
                        Jsi_LogError("delete a right value\n");
                    } else {
                        Jsi_Value *v = TOP->d.lval;
                        SIGASSERT(v, VALUE);
                       if (v != currentScope) {
                            Jsi_ValueReset(interp,v);     /* not allow to delete arguments */
                        }
                        else Jsi_LogWarn("Delete arguments\n");
                    }
                    Pop(interp,1);
                } else if (count == 2) {
                    VarDeref(interp,2);
                    assert(interp->Sp>=2);
                    if (TOQ->vt != JSI_VT_OBJECT) Jsi_LogWarn("delete non-object key, ignore\n");
                    if (TOQ->d.obj == currentScope->d.obj) Jsi_LogWarn("Delete arguments\n");
                    
                    ValueObjDelete(interp, TOQ, TOP, 0);
                    
                    Pop(interp,2);
                } else Jsi_LogBug("delete");
                break;
            }
            case OP_OBJECT: {
                int itemcount = (int)ip->data;
                Assert(itemcount>=0);
                VarDeref(interp, itemcount * 2);
                Jsi_Obj *obj = Jsi_ObjNewObj(interp, stack+(interp->Sp-itemcount*2), itemcount*2);
                Pop(interp, itemcount * 2 - 1);       /* one left */
                ClearStack(interp,1);
                Jsi_ValueMakeObject(interp,TOP, obj);
                break;
            }
            case OP_ARRAY: {
                int itemcount = (int)ip->data;
                Assert(itemcount>=0);
                VarDeref(interp, itemcount);
                Jsi_Obj *obj = Jsi_ObjNewArray(interp, stack+(interp->Sp-itemcount), itemcount);
                Pop(interp, itemcount - 1);
                ClearStack(interp,1);
                Jsi_ValueMakeObject(interp,TOP, obj);
                break;
            }
            case OP_STRY: {
                TryInfo *ti = (TryInfo *)ip->data;
                TryList *n = trylist_new(TL_TRY, scope, currentScope);
                
                n->d.td.tstart = ip;                            /* make every thing pointed to right pos */
                n->d.td.tend = n->d.td.tstart + ti->trylen;
                n->d.td.cstart = n->d.td.tend + 1;
                n->d.td.cend = n->d.td.tend + ti->catchlen;
                n->d.td.fstart = n->d.td.cend + 1;
                n->d.td.fend = n->d.td.cend + ti->finallen;
                n->d.td.tsp = interp->Sp;

                push_try(trylist, n);
                break;
            }
            case OP_ETRY: {             /* means nothing happen go to final */
                if (trylist == NULL || trylist->type != TL_TRY)
                    Jsi_LogBug("Unexpected ETRY opcode??");

                ip = trylist->d.td.fstart - 1;
                break;
            }
            case OP_SCATCH: {
                if (trylist == NULL || trylist->type != TL_TRY)
                    Jsi_LogBug("Unexpected SCATCH opcode??");

                if (!ip->data) {
                    do_throw();
                } else {
                    /* new scope and make var */
                    scope = jsi_ScopeChainDupNext(interp, scope, currentScope);
                    currentScope = jsi_ObjValueNew(interp);
                    interp->ingsc = scope;
                    interp->incsc = currentScope;
                    Jsi_IncrRefCount(interp, currentScope);
                    Jsi_Value *excpt = Jsi_ValueNew1(interp);
                    if (ps->last_exception.vt != JSI_VT_UNDEF) {
                        Jsi_ValueCopy(interp, excpt, &ps->last_exception);
                        Jsi_ValueReset(interp, &ps->last_exception);
                    } else if (interp->errMsgBuf[0]) {
                        Jsi_ValueMakeStringDup(interp, excpt, interp->errMsgBuf);
                        interp->errMsgBuf[0] = 0;
                    }
                    Jsi_ValueInsert(interp, currentScope, ip->data, excpt, JSI_OM_DONTENUM);

                    context_id = ps->_context_id++;
                }
                break;
            }
            case OP_ECATCH: {
                if (trylist == NULL || trylist->type != TL_TRY)
                    Jsi_LogBug("Unexpected ECATCH opcode??");

                ip = trylist->d.td.fstart - 1;
                break;
            }
            case OP_SFINAL: {
                if (trylist == NULL || trylist->type != TL_TRY)
                    Jsi_LogBug("Unexpected SFINAL opcode??");

                /* restore scatch scope chain */
                restore_scope();
                break;
            }
            case OP_EFINAL: {
                if (trylist == NULL || trylist->type != TL_TRY)
                    Jsi_LogBug("Unexpected EFINAL opcode??");

                int last_op = trylist->d.td.last_op;
                OpCode *tojmp = (last_op == LOP_JMP ? trylist->d.td.ld.tojmp : 0);
                
                pop_try(trylist);

                if (last_op == LOP_THROW) {
                    do_throw();
                } else if (last_op == LOP_JMP) {
                    while (1) {
                        if (trylist == NULL) {
                            ip = tojmp;
                            break;
                        }
                        /* same as jmp opcode, see above */
                        if (trylist->type == TL_TRY) {
                            if (tojmp >= trylist->d.td.tstart && tojmp < trylist->d.td.fend) {
                                ip = tojmp;
                                break;
                            }
                            
                            if (ip >= trylist->d.td.tstart && ip < trylist->d.td.cend) {
                                trylist->d.td.last_op = LOP_JMP;
                                trylist->d.td.ld.tojmp = tojmp;
                                
                                ip = trylist->d.td.fstart - 1;
                                break;
                            } else if (ip >= trylist->d.td.fstart && ip < trylist->d.td.fend) {
                                pop_try(trylist);
                            } else Jsi_LogBug("jmp within a try, but not in its scope?");
                        } else {        /* 'with' block */
                            if (tojmp >= trylist->d.wd.wstart && tojmp < trylist->d.wd.wend) {
                                ip = tojmp;
                                break;
                            }
                            restore_scope();
                            pop_try(trylist);
                        }
                    }
                }
                break;
            }
            case OP_THROW: {
                VarDeref(interp,1);
                Jsi_ValueCopy(interp,&ps->last_exception, TOP);
                interp->didReturn = 1; /* TODO: could possibly hide stack problem */
                do_throw();
                break;
            }
            case OP_WITH: {
                VarDeref(interp,1);
                Jsi_ValueToObject(interp, TOP);
                
                TryList *n = trylist_new(TL_WITH, scope, currentScope);
                
                n->d.wd.wstart = ip;
                n->d.wd.wend = n->d.wd.wstart + (int)ip->data;

                push_try(trylist, n);
                interp->withDepth++;
                
                /* make expr to top of scope chain */
                scope = jsi_ScopeChainDupNext(interp, scope, currentScope);
                currentScope = Jsi_ValueNew1(interp);
                interp->ingsc = scope;
                interp->incsc = currentScope;
                Jsi_ValueCopy(interp, currentScope, TOP);
                Pop(interp,1);
                
                context_id = ps->_context_id++;
                break;
            }
            case OP_EWITH: {
                if (trylist == NULL || trylist->type != TL_WITH)
                    Jsi_LogBug("Unexpected EWITH opcode??");

                restore_scope();
                
                pop_try(trylist);
                interp->withDepth--;
                break;
            }
            case OP_DEBUG: {
                VarDeref(interp,1);
                if (TOP->vt == JSI_VT_OBJECT) {
                    printf("R%d:", TOP->refCnt);
                }
                Jsi_ValueToString(interp, TOP);
                printf("%s\n", TOP->d.s.str);
                break;
            }
            case OP_RESERVED: {
                ReservedInfo *ri = ip->data;
                const char *cmd = ri->type == RES_CONTINUE ? "continue" : "break";
                /* TODO: continue/break out of labeled scope not working. */
                if (ri->label) {
                    Jsi_LogError("%s: label(%s) not found\n", cmd, ri->label);
                } else {
                    Jsi_LogError("%s must be inside loop(or switch)\n", cmd);
                }
                break;
            }
        }
        ip++;
    }
done:
    while (trylist) {
        pop_try(trylist);
    }
    return rc;
}


int jsi_evalcode(jsi_Pstate *ps, OpCodes *opcodes, 
         jsi_ScopeChain *scope, Jsi_Value *currentScope,    /* scope chain */
         Jsi_Value *_this,
         Jsi_Value **vret)
{
    Jsi_Interp *interp = ps->interp;
    int rc, oldTry = interp->tryDepth;
    jsi_ScopeChain *oldscope = interp->ingsc;
    Jsi_Value *oldcurrentScope = interp->incsc;
    Jsi_Value *oldthis = interp->inthis;
    int oldSp = interp->Sp;
    if (interp->exited)
        return JSI_ERROR;
    interp->ingsc = scope;
    interp->incsc = currentScope;
    interp->inthis = _this;
    interp->level++;
    interp->refCount++;
    Jsi_IncrRefCount(interp, currentScope);
    rc = _jsi_evalcode(ps, opcodes, scope, currentScope, _this, *vret);
    Jsi_DecrRefCount(interp, currentScope);
    if (interp->didReturn == 0 && !interp->exited) {
        if ((interp->evalFlags&JSI_EVAL_RETURN)==0)
            Jsi_ValueMakeUndef(interp, *vret);
        /*if (interp->Sp != oldSp) //TODO: at some point after memory refs???
            Jsi_LogBug("Stack not balance after execute script\n");*/
    }
    interp->didReturn = 0;
    interp->Sp = oldSp;
    interp->refCount--;
    interp->level--;
    interp->ingsc = oldscope;
    interp->incsc = oldcurrentScope;
    interp->inthis = oldthis;
    interp->tryDepth = oldTry;
    if (interp->exited)
        rc = JSI_ERROR;
    return rc;
}



static jsi_Pstate* NewParser(Jsi_Interp* interp, char *codeStr, Jsi_Channel fp, int iseval)
{
    int isNew, cache = (interp->nocacheOpCodes==0);
    Jsi_HashEntry *hPtr = NULL;
    hPtr = Jsi_HashEntryCreate(interp->codeTbl, (void*)codeStr, &isNew);
    if (!hPtr) return NULL;
    jsi_Pstate *ps;

    if (cache && isNew==0 && ((ps = Jsi_HashValueGet(hPtr)))) {
        interp->codeCacheHit++;
        return ps;
    }
    ps = jsi_PstateNew(interp);
    ps->eval_flag = iseval;
    if (codeStr)
        jsi_PstateSetString(ps, codeStr);
    else
        jsi_PstateSetFile(ps, fp, 1);
        
    interp->inParse++;
    yyparse(ps);
    interp->inParse--;
    
    if (ps->err_count) {
        if (cache) Jsi_HashEntryDelete(hPtr);
        jsi_PstateFree(ps);
        return NULL;
    }
    if (isNew) {
        if (cache) {
            Jsi_HashValueSet(hPtr, ps);
            ps->hPtr = hPtr;
        } else {
            /* only using caching now. */
            assert(0);
        }
    }
    return ps;
}

/* eval here is diff from Jsi_CmdProc, current scope Jsi_LogWarn should be past to eval */
/* make evaling script execute in the same context */
int jsi_global_eval(Jsi_Interp* interp, jsi_Pstate *ps, char *program,
                       jsi_ScopeChain *scope, Jsi_Value *currentScope, Jsi_Value *_this, Jsi_Value **ret)
{
    int r = JSI_OK;
    jsi_Pstate *newps = NewParser(interp, program, NULL, 1);
    if (newps) {
        int oef = newps->eval_flag;
        newps->eval_flag = 1;
        interp->ps = newps;
        r = jsi_evalcode(newps, newps->opcodes, scope, currentScope, _this, ret);
        if (r) {
            Jsi_ValueCopy(interp,&ps->last_exception, &newps->last_exception);
        }
        newps->eval_flag = oef;
        interp->ps = ps;
    } else  {
        Jsi_ValueMakeString(interp,&ps->last_exception, Jsi_Strdup("Syntax Error"));
        r = JSI_ERROR;
    }
    return r;
}

static int jsi_eval(Jsi_Interp* interp, char *str, Jsi_Channel fp, int flags)
{
    int rc = JSI_OK, oldef = interp->evalFlags;
    jsi_Pstate *oldps = interp->ps, *ps = NewParser(interp, str, fp, 0);
    interp->evalFlags = flags;
    if (!ps)
        return JSI_ERROR;
    Jsi_ValueMakeUndef(interp, &interp->ret);
    interp->ps = ps;
    Jsi_Value *retPtr = &interp->ret;
    if (jsi_evalcode(ps, ps->opcodes, interp->gsc, interp->csc, interp->csc, &retPtr)) {
        rc = JSI_ERROR;
    } else {
        Jsi_ValueCopy(interp,&oldps->last_exception, &ps->last_exception);
    }
    interp->ps = oldps;
    interp->evalFlags = oldef;
    return rc;
}

static int jsi_evalStrFile(Jsi_Interp* interp, Jsi_Value *path, char *str, int flags)
{
    Jsi_Channel input = Jsi_GetStdChannel(interp, 0);
    Jsi_Value *npath = path;
    if (Jsi_MutexLock(interp, interp->Mutex) != JSI_OK)
        return JSI_OK;
    int rc, oldSp;
    const char *oldFile = interp->curFile;
    char *origFile = Jsi_ValueString(interp, path, NULL);
    const char *fname = origFile;
    char *oldDir = interp->curDir;
    char dirBuf[1024];

    oldSp = interp->Sp;
    dirBuf[0] = 0;
    Jsi_DString dStr = {};
    Jsi_DSInit(&dStr);
    if (str == NULL) {
        if (fname != NULL) {
            char *cp;
            if (!Jsi_Strcmp(fname,"-"))
                input = Jsi_GetStdChannel(interp, 0);
            else {
    
                /* Use translated FileName. */
                if (interp->curDir && fname[0] != '/' && fname[0] != '~') {
                    char dirBuf2[1024], *np;
                    snprintf(dirBuf, sizeof(dirBuf), "%s/%s", interp->curDir, fname);
                    if ((np=Jsi_FileRealpathStr(interp, dirBuf, dirBuf2)) == NULL) {
                        Jsi_LogError("Can not open '%s'\n", fname);
                        rc = -1;
                        goto bail;
                    }
                    npath = Jsi_ValueNewStringDup(interp, np);
                    Jsi_IncrRefCount(interp, npath);
                    fname = Jsi_ValueString(interp, npath, NULL);
                    if (flags&JSI_EVAL_ARGV0) {
                        interp->argv0 = Jsi_ValueNewStringDup(interp, np);
                        Jsi_IncrRefCount(interp, interp->argv0);
                    }
                } else {
                    if (flags&JSI_EVAL_ARGV0) {
                        interp->argv0 = Jsi_ValueNewStringDup(interp, fname);
                        Jsi_IncrRefCount(interp, interp->argv0);
                    }
                }
                
                input = Jsi_Open(interp, npath, "r");
                if (!input) {
                    Jsi_LogError("Can not open '%s'\n", fname);
                    rc = -1;
                    goto bail;
                }
            }
            int isNew;
            Jsi_HashEntry *hPtr;
            jsi_FileInfo *fi = NULL;
            hPtr = Jsi_HashEntryCreate(interp->fileTbl, fname, &isNew);
            if (isNew == 0 && hPtr) {
                fi = Jsi_HashValueGet(hPtr);
                interp->curFile = fi->fileName;
                interp->curDir = fi->dirName;
                
            } else {
                fi = Jsi_Calloc(1,sizeof(*fi));
                Jsi_HashValueSet(hPtr, fi);
                fi->origFile = (char*)Jsi_KeyAdd(interp, origFile);
                interp->curFile = fi->fileName = (char*)Jsi_KeyAdd(interp, fname);
                char *dfname = Jsi_Strdup(fname);
                if ((cp = strrchr(dfname,'/')))
                    *cp = 0;
                interp->curDir = fi->dirName = (char*)Jsi_KeyAdd(interp, dfname);
                Jsi_Free(dfname);
            }
            if (!input->fname)
                input->fname = interp->curFile;
            if (1 /*|| input->isNative==0 || interp->nocacheOpCodes==0*/) {
                /* We use in-memory parse always now. */
                int cnt = 0;
                char buf[BUFSIZ];
                while (cnt<MAX_LOOP_COUNT) {
                    if (!Jsi_Gets(input, buf, sizeof(buf)))
                        break;
                    if (++cnt==1 && (!(flags&JSI_EVAL_NOSKIPBANG)) && (buf[0] == '#' && buf[1] == '!')) {
                        Jsi_DSAppend(&dStr, "\n", NULL);
                        continue;
                    }
                    Jsi_DSAppend(&dStr, buf,  NULL);
                }
                if (cnt>=MAX_LOOP_COUNT)
                    Jsi_LogError("source file too large");
                Jsi_Close(input);
                str = Jsi_DSValue(&dStr);
            }
        }
        if (interp->curDir && (flags&JSI_EVAL_INDEX))
            Jsi_AddIndexFiles(interp, interp->curDir);
    }

    /* TODO: cleanup interp->Sp stuff. */
    oldSp = interp->Sp;

    rc = jsi_eval(interp, str, input, flags);
    
bail:
    interp->curFile = oldFile;
    interp->curDir = oldDir;
    interp->Sp = oldSp;
    if (path != npath)
        Jsi_DecrRefCount(interp, npath);
    Jsi_DSFree(&dStr);
    Jsi_MutexUnlock(interp, interp->Mutex);
    if (interp->exited && interp->level <= 0)
    {
        rc = interp->exitCode;
        Jsi_InterpDelete(interp);
    }

    return rc;
}

int Jsi_EvalFile(Jsi_Interp* interp, Jsi_Value *fname, int flags)
{
    return jsi_evalStrFile(interp, fname, NULL, flags);
}

int Jsi_EvalString(Jsi_Interp* interp, const char *str, int flags)
{
    return jsi_evalStrFile(interp, NULL, (char*)str, flags);
}

#undef obj_this
#undef stack
#undef StackIdx
#undef ObjThisIdx
#undef TOP
#undef TOQ
#undef StackCopy
#endif
