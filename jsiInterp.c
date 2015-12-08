#ifndef JSI_LITE_ONLY
#define __JSIINT_C__
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

static int is_init = 0;
Jsi_Interp *jsiMainInterp = NULL;
Jsi_Interp *jsiDelInterp = NULL;
static Jsi_Hash *interpsTbl;

//#include "jsiRevision.h"

static Jsi_CmdSpec interpCmds[];

#define IIOF .flags=JSI_OPT_INIT_ONLY

static Jsi_OptionSpec InterpOptions[] = {
    JSI_OPT(ARRAY, Jsi_Interp, args,        .help="The console.arguments for interp", IIOF),
    JSI_OPT(INT,   Jsi_Interp, debug,       .help="Set debugging level"),
    JSI_OPT(BOOL,  Jsi_Interp, doUnlock,    .help="Unlock our mutex when evaling in other interps", IIOF, .init="true"),
    JSI_OPT(BOOL,  Jsi_Interp, noUndef,     .help="Suppress printing undefined value result when in interactive mode"),
    JSI_OPT(STRKEY,Jsi_Interp, evalCallback,.help="String name of callback function in parent to handle eval stepping", IIOF ),
    JSI_OPT(VALUE, Jsi_Interp, indexFiles,   .help="File(s) to source for loading index for unknown commands"),
    JSI_OPT(BOOL,  Jsi_Interp, isSafe,      .help="Interp is safe (ie. no file access)", IIOF),
    JSI_OPT(INT,   Jsi_Interp, lockTimeout, .help="Timeout for mutex lock-acquire (milliseconds)" ),
    JSI_OPT(STRKEY,Jsi_Interp, logCallback, .help="String name of callback function in parent to handle logging", IIOF ),
    JSI_OPT(INT,   Jsi_Interp, maxDepth,    .help="Recursion call depth limit", .init="1000"),
    JSI_OPT(INT,   Jsi_Interp, maxIncDepth, .help="Max file include nesting limit", .init="50" ),
    JSI_OPT(INT,   Jsi_Interp, maxInterpDepth,.help="Max nested subinterp create limit", .init="10" ),
    JSI_OPT(INT,   Jsi_Interp, maxUserObjs, .help="Cap on number of 'new' object calls (eg. File, Regexp, etc)" ),
    JSI_OPT(INT,   Jsi_Interp, maxOpCnt,    .help="Execution cap on opcodes evaluated" ),
    JSI_OPT(BOOL,  Jsi_Interp, nDebug,      .help="Make assert statements have no effect"),
    JSI_OPT(STRKEY,Jsi_Interp, name, .help="Name of interp", IIOF),
    JSI_OPT(BOOL,  Jsi_Interp, noreadline,  .help="Do not use readline in interactive mode", IIOF),
    JSI_OPT(FUNC,  Jsi_Interp, onExit,      .help="Command to call in parent on exit (which returns true to continue)", IIOF ),
    JSI_OPT(BOOL,  Jsi_Interp, noSubInterps,.help="Disallow sub-interp creation", IIOF),
    JSI_OPT(BOOL,  Jsi_Interp, privKeys,    .help="Disable string key sharing with other interps", IIOF, .init="true"),
    JSI_OPT(STRKEY,Jsi_Interp, recvCmd,     .help="Name of function to recv 'send' msgs"),
    JSI_OPT(ARRAY, Jsi_Interp, safeReadDirs,.help="In safe mode, directories to allow reads from", IIOF),
    JSI_OPT(ARRAY, Jsi_Interp, safeWriteDirs,.help="In safe mode, directories to allow writes to", IIOF),
    JSI_OPT(STRKEY,Jsi_Interp, scriptStr,   .help="Startup script string", IIOF),
    JSI_OPT(VALUE, Jsi_Interp, scriptFile,  .help="Startup script file name", IIOF),
    JSI_OPT(BOOL,  Jsi_Interp, strict,      .help="If set to false, option parse ignore unknown options",.init="true" ),
    JSI_OPT(BOOL,  Jsi_Interp, subthread,   .help="Create thread for interp", IIOF),
    JSI_OPT(INT,   Jsi_Interp, traceCalls,  .help="Echo method call/return value"),
    JSI_OPT_END(Jsi_Interp)

};

/* Object for each interp created. */
typedef struct InterpObj {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Interp *subinterp;
    Jsi_Interp *parent;
    Jsi_Hash *aliases;
    //char *interpname;
    char *mode;
    Jsi_Obj *fobj;
    int objId;
} InterpObj;

/* Global state of interps. */

typedef struct {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    int refCount;
    const char *cmdName;
    Jsi_Value *args;
    Jsi_Value *func;
    Jsi_Value *cmdVal;
    InterpObj *intobj;
    Jsi_Interp *interp;
} AliasCmd;


static void interpObjErase(InterpObj *fo);
static int interpObjFree(Jsi_Interp *interp, void *data);
static int interpObjIsTrue(void *data);
static int interpObjEqual(void *data1, void *data2);

static Jsi_UserObjReg interpobject = {
    "Interp",
    interpCmds,
    interpObjFree,
    interpObjIsTrue,
    interpObjEqual
};

#ifndef JSI_OMIT_THREADS

#ifdef __WIN32
#include <windows.h>
static int MutexLock(Jsi_Interp *interp, CRITICAL_SECTION* mtx) {
    if (interp->lockTimeout<0)
        EnterCriticalSection(mtx);
    else {
        uint cnt = interp->lockTimeout;
        while (cnt-- >= 0) {
            if (TryEnterCriticalSection(mtx))
                return JSI_OK;
            usleep(1000);
        }
        Jsi_LogError("lock timed out");
        interp->threadErrCnt++;
        return JSI_ERROR;
    }
    return JSI_OK;
}
static void MutexUnlock(CRITICAL_SECTION* mtx) { LeaveCriticalSection(mtx); }
static void MutexInit(CRITICAL_SECTION *mtx) {  InitializeCriticalSection(mtx); }
static void* MutexNew(void) {
    CRITICAL_SECTION *mtx = Jsi_Calloc(1,sizeof(*mtx));
    InitializeCriticalSection(mtx);
    return mtx;
}
static void MutexDone(CRITICAL_SECTION *mtx) { DeleteCriticalSection(mtx); }
#else /* ! __WIN32 */

#include <pthread.h>
static int MutexLock(Jsi_Interp *interp, pthread_mutex_t *mtx) {
    if (interp->lockTimeout<0)
        pthread_mutex_lock(mtx);
    else {
        struct timespec ts;
        ts.tv_sec = interp->lockTimeout/1000;
        ts.tv_nsec = 1000 * (interp->lockTimeout%1000);
        int rc = pthread_mutex_timedlock(mtx, &ts);
        if (rc != 0) {
            Jsi_LogError("lock timed out");
            interp->threadErrCnt++;
            return JSI_ERROR;
        }
    }
    return JSI_OK;
}
static void MutexUnlock(pthread_mutex_t *mtx) { pthread_mutex_unlock(mtx); }
static void MutexInit(pthread_mutex_t *mtx) {
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mtx, &Attr);
}
static void* MutexNew(void) { pthread_mutex_t* mtx = Jsi_Malloc(sizeof(pthread_mutex_t)); MutexInit(mtx); return mtx; }
static void MutexDone(pthread_mutex_t *mtx) { pthread_mutex_destroy(mtx); }
#endif

int Jsi_MutexLock(Jsi_Interp *interp, void *mtx) { interp->lockRefCnt++; return MutexLock(interp, mtx);}
void Jsi_MutexUnlock(Jsi_Interp *interp, void *mtx) { MutexUnlock(mtx); interp->lockRefCnt--; }
void* Jsi_MutexNew(Jsi_Interp *interp) { return MutexNew(); }
void Jsi_MutexDone(Jsi_Interp *interp, void *mtx) { MutexDone(mtx); }
void Jsi_MutexInit(Jsi_Interp *interp, void *mtx) { MutexInit(mtx); }
void* Jsi_InterpThread(Jsi_Interp *interp) { return interp->threadId; }
void* Jsi_CurrentThread(void) {
#ifdef __WIN32
    return (void*)GetCurrentThreadId();
#else
    return (void*)pthread_self();
#endif
}

#else /* ! JSI_OMIT_THREADS */
int Jsi_MutexLock(Jsi_Interp *interp, void *mtx) { return JSI_OK; }
void Jsi_MutexUnlock(Jsi_Interp *interp, void *mtx) { }
void* Jsi_MutexNew(Jsi_Interp *interp) { return NULL; }
void Jsi_MutexDone(Jsi_Interp *interp, void *mtx) { }
void* Jsi_CurrentThread(void) { return NULL; }
void* Jsi_InterpThread(Jsi_Interp *interp) { return NULL; }
void Jsi_MutexInit(Jsi_Interp *interp, void *mtx) { }
#endif

Jsi_Number Jsi_Version(void) {
    Jsi_Number d = JSI_VERSION;
    return d;
}
static void ConvertReturn(Jsi_Interp *interp, Jsi_Interp *inInterp, Jsi_Value *ret)
{
    Jsi_DString dStr = {};

    switch (ret->vt) {
        case JSI_VT_UNDEF:
        case JSI_VT_BOOL:
        case JSI_VT_NUMBER:
        case JSI_VT_NULL:
            break;
        default:
            Jsi_DSInit(&dStr);
            char *cp = (char*)Jsi_ValueGetDString(inInterp, ret, &dStr, JSI_OUTPUT_JSON);
            Jsi_JSONParse(interp, cp, ret, 0);
            Jsi_DSFree(&dStr);
    }
}

/* Call a command with JSON args.  Returned value is converted to JSON. */
int Jsi_EvalCmdJSON(Jsi_Interp *interp, const char *cmd, const char *jsonArgs, Jsi_DString *dStr)
{
    if (Jsi_MutexLock(interp, interp->Mutex) != JSI_OK)
        return JSI_ERROR;
    Jsi_Value nret = VALINIT, *nrPtr = &nret;
    int rc = Jsi_CommandInvokeJSON(interp, cmd, jsonArgs, &nrPtr);
    Jsi_DSInit(dStr);
    Jsi_ValueGetDString(interp, &nret, dStr, JSI_OUTPUT_JSON);
    Jsi_ValueMakeUndef(interp, &nret);
    Jsi_MutexUnlock(interp, interp->Mutex);
    return rc;
}

/* Call a function with JSON args.  Return a primative. */
int Jsi_FunctionInvokeJSON(Jsi_Interp *interp, Jsi_Value *func, const char *json, Jsi_Value **ret)
{
    int rc;
    Jsi_Value args = VALINIT;
    rc = Jsi_JSONParse(interp, json, &args, 0);
    if (rc == JSI_OK) {
        rc = Jsi_FunctionInvoke(interp, func, &args, ret, NULL);
        Jsi_ValueReset(interp, &args);
    }
    return rc;
}
/* Lookup cmd from cmdstr and invoke with JSON args. */
/*
 *   Jsi_CommandInvokeJSON(interp, "info.cmds", "[\"*\",true]", ret);
 */
int Jsi_CommandInvokeJSON(Jsi_Interp *interp, const char *cmdstr, const char *json, Jsi_Value **ret)
{
    Jsi_Value *func = Jsi_NameLookup(interp, cmdstr);
    if (func)
        return Jsi_FunctionInvokeJSON(interp, func, json, ret);
    Jsi_LogError("can not find cmd: %s", cmdstr);
    return JSI_ERROR;
}

static int AliasFree(Jsi_Interp *interp, void *data) {
    /* TODO: deal with other copies of func may be floating around (refCount). */
    AliasCmd *ac = data;
    SIGASSERT(ac,ALIASCMD);
    if (ac->func)
        Jsi_DecrRefCount(ac->interp, ac->func);
    if (ac->args)
        Jsi_DecrRefCount(ac->interp, ac->args);
    Jsi_Func *fobj = ac->cmdVal->d.obj->d.fobj->func;
    fobj->cmdSpec->udata3 = NULL;
    fobj->cmdSpec->proc = NULL;
    if (ac->intobj->subinterp)
        Jsi_CommandDelete(ac->intobj->subinterp, ac->cmdName);
    Jsi_DecrRefCount(ac->interp, ac->cmdVal);
    MEMCLEAR(ac);
    Jsi_Free(ac);
    return JSI_OK;
}

static int NeedClean(Jsi_Interp *interp, Jsi_Value *arg)
{
    switch (arg->vt) {
        case JSI_VT_BOOL: return 0;
        case JSI_VT_NULL: return 0;
        case JSI_VT_NUMBER: return 0;
        case JSI_VT_STRING: return (interp->privKeys || arg->f.bits.isstrkey);
        case JSI_VT_UNDEF: return 0;
        //case JSI_VT_VARIABLE: return 1;
        case JSI_VT_OBJECT: {
            Jsi_Obj *o = arg->d.obj;
            switch (o->ot) {
                case JSI_OT_NUMBER: return 0;
                case JSI_OT_BOOL: return 0;
                case JSI_OT_STRING: return (interp->privKeys || arg->d.obj->isstrkey);
                case JSI_OT_FUNCTION: return 1;
                case JSI_OT_REGEXP: return 1;
                case JSI_OT_USEROBJ: return 1;
                case JSI_OT_ITER: return 1;     
                case JSI_OT_OBJECT:
                case JSI_OT_ARRAY:
                    if (o->isArray && o->arr)
                    {
                        int i;
                        for (i = 0; i < o->arrCnt; ++i) {
                            if (o->arr[i] && NeedClean(interp, o->arr[i]))
                                return 1;
                        }
                    } else if (o->tree) {
                        int trc = 0;
                        Jsi_TreeEntry *tPtr;
                        Jsi_TreeSearch search;
                        Jsi_Value *v;
                        for (tPtr = Jsi_TreeSearchFirst(o->tree, &search, 0);
                            tPtr; tPtr = Jsi_TreeSearchNext(&search)) {
                            v = Jsi_TreeValueGet(tPtr);
                            if (v && (trc = NeedClean(interp, v)))
                                break;
                        }
                        Jsi_TreeSearchDone(&search);
                        return trc;
                    } else {
                        return 1;
                    }
                    return 0;
                default:
                    return 1;
            }
        }
    }
    return 1;
}

static  int Jsi_CleanValue(Jsi_Interp *interp, Jsi_Interp *tointerp, Jsi_Value *args, Jsi_Value *ret)
{
    if (tointerp->threadId == interp->threadId && !NeedClean(interp, args)) {
        Jsi_ValueCopy(tointerp, ret, args);
        return JSI_OK;
    }
    /* Cleanse input args by convert to JSON and back. */
    Jsi_DString dStr;
    const char *cp = Jsi_ValueGetDString(interp, args, &dStr, JSI_OUTPUT_JSON);
    if (Jsi_JSONParse(tointerp, cp, ret, 0) != JSI_OK) {
        Jsi_DSFree(&dStr);
        Jsi_LogError("bad subinterp parse");
        return JSI_ERROR;
    }
    Jsi_DSFree(&dStr);
    return JSI_OK;
}

static int AliasInvoke(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Interp *pinterp = interp->parent;
    if (!pinterp)
        return JSI_ERROR;
    AliasCmd *ac = funcPtr->cmdSpec->udata3;
    Jsi_Value *nargs = NULL;
    int argc = Jsi_ValueGetLength(interp, args);
    if (!ac) {
        Jsi_LogBug("BAD ALIAS INVOKE OF DELETED");
        return JSI_ERROR;
    }
    SIGASSERT(ac,ALIASCMD);
    Jsi_Value nret = VALINIT;
    if (argc == 0 && ac->args)
        nargs = ac->args;
    else if (argc) {
        if (Jsi_CleanValue(interp, pinterp, args, &nret) != JSI_OK)
            return JSI_ERROR;
        if (ac->args) {
            nargs = Jsi_ValueArrayConcat(pinterp, ac->args, &nret);
        } else {
            nargs = &nret;
        }
    }
    if (interp->doUnlock) Jsi_MutexUnlock(interp, interp->Mutex);
    if (Jsi_MutexLock(interp, pinterp->Mutex) != JSI_OK) {
        if (interp->doUnlock) Jsi_MutexLock(interp, interp->Mutex);
        return JSI_ERROR;
    }
    ac->refCount++;
    if (nargs && nargs != &nret)
        Jsi_IncrRefCount(interp, nargs);
    int rc = Jsi_FunctionInvoke(pinterp, ac->func, nargs, ret, NULL);
    ac->refCount--;
    Jsi_MutexUnlock(interp, pinterp->Mutex);
    if (interp->doUnlock && Jsi_MutexLock(interp, interp->Mutex) != JSI_OK) {
        return JSI_ERROR;
    }
    Jsi_ValueMakeUndef(interp, &nret);
    if (nargs && nargs != &nret)
        Jsi_DecrRefCount(interp, nargs);
    ConvertReturn(pinterp, interp, *ret);
    return rc;
}


static int InterpAliasCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    InterpObj *udf = Jsi_UserObjGetData(interp, _this, funcPtr);
    if (!udf) {
        Jsi_LogError("Apply Interp.eval in a non-interp object");
        return JSI_ERROR;
    }
    if (!udf->aliases) {
        Jsi_LogError("Sub-interp gone");
        return JSI_ERROR;
    }
    int argc = Jsi_ValueGetLength(interp, args);
    if (argc == 0) {
        return Jsi_HashKeysDump(interp, udf->aliases, *ret);
    }
    Jsi_HashEntry *hPtr;
    char *key = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    if (!key) {
        Jsi_LogError("expected string");
        return JSI_ERROR;
    }
    AliasCmd* ac;
    if (argc == 1) {
        hPtr = Jsi_HashEntryFind(udf->aliases, (void*)key);
        if (!hPtr)
            return JSI_OK;
        ac = Jsi_HashValueGet(hPtr);
        SIGASSERT(ac,ALIASCMD);
        Jsi_ValueCopy(interp, *ret, ac->func);
        return JSI_OK;
    }
    Jsi_Value *afunc = Jsi_ValueArrayIndex(interp, args, 1);
    if (argc == 2) {
        hPtr = Jsi_HashEntryFind(udf->aliases, (void*)key);
        if (!hPtr)
            return JSI_OK;
        ac = Jsi_HashValueGet(hPtr);
        if (!Jsi_ValueIsFunction(interp, afunc)) {
            Jsi_LogError("arg 2: expected function");
            return JSI_ERROR;
        }
        Jsi_ValueCopy(interp, *ret, ac->args);
        return JSI_OK;
    }
    if (argc == 3) {
        int isNew;
        Jsi_Value *aargs = Jsi_ValueArrayIndex(interp, args, 2);
        if (Jsi_ValueIsNull(interp, afunc) && Jsi_ValueIsNull(interp, aargs)) {
            hPtr = Jsi_HashEntryFind(udf->aliases, (void*)key);
            if (hPtr == NULL)
                return JSI_OK;
            AliasFree(interp, Jsi_HashValueGet(hPtr));
            Jsi_HashEntryDelete(hPtr);
            return JSI_OK;
        }
        hPtr = Jsi_HashEntryCreate(udf->aliases, (void*)key, &isNew);
        if (!hPtr) {
            Jsi_LogError("create failed: %s", key);
            return JSI_ERROR;
        }
        if (!Jsi_ValueIsFunction(interp, afunc)) {
            Jsi_LogError("arg 2: expected function");
            return JSI_ERROR;
        }
        if (Jsi_ValueIsNull(interp, aargs) == 0 && Jsi_ValueIsArray(interp, aargs) == 0) {
            Jsi_LogError("arg 3: expected array or null");
            return JSI_ERROR;
        }
        AliasCmd *ac;
        if (!isNew) {
            AliasFree(interp, Jsi_HashValueGet(hPtr));
        }
        ac = Jsi_Calloc(1, sizeof(AliasCmd));
        SIGINIT(ac, ALIASCMD);
        ac->cmdName = Jsi_HashKeyGet(hPtr);
        ac->func = afunc;
        Jsi_IncrRefCount(interp, afunc);
        if (!Jsi_ValueIsNull(interp, aargs)) {
            ac->args = aargs;
            Jsi_IncrRefCount(interp, aargs);
        }
        ac->intobj = udf;
        ac->interp = interp;
        Jsi_HashValueSet(hPtr, ac);
        Jsi_Value *cmd = Jsi_CommandCreate(udf->subinterp, key, AliasInvoke, NULL);
        if (!cmd) {
            Jsi_LogError("command create failure");
            return JSI_ERROR;
        }
        ac->cmdVal = cmd;
        Jsi_IncrRefCount(udf->subinterp, ac->cmdVal);
        Jsi_Func *fobj = cmd->d.obj->d.fobj->func;
        fobj->cmdSpec->udata3 = ac;
    }
    return JSI_OK;
}

static int freeCodeTbl(Jsi_Interp *interp, void *ptr) {
    jsi_Pstate *ps = ptr;
    if (!ps) return JSI_OK;
    ps->hPtr = NULL;
    jsi_PstateFree(ps);
    return JSI_OK;
}

static int freeAssocTbl(Jsi_Interp *interp, void *ptr) {
    if (!ptr) return JSI_OK;
    jsi_DelAssocData(interp, ptr);
    return JSI_OK;
}

static int freeEventTbl(Jsi_Interp *interp, void *ptr) {
    Jsi_Event *event = ptr;
    SIGASSERT(event,EVENT);
    if (!ptr) return JSI_OK;
    event->hPtr = NULL;
    Jsi_EventFree(interp, event);
    return JSI_OK;
}
static int jsiFree(Jsi_Interp *interp, void *ptr) {
    Jsi_Free(ptr);
    return JSI_OK;
}

static int regExpFree(Jsi_Interp *interp, void *ptr) {
    Jsi_RegExpFree(ptr);
    return JSI_OK;
}

static int freeCmdSpecTbl(Jsi_Interp *interp, void *ptr) {
    if (!ptr) return JSI_OK;
    jsi_CmdSpecDelete(interp, ptr);
    return JSI_OK;
}

static int freeGenObjTbl(Jsi_Interp *interp, void *ptr) {
    Jsi_Obj *obj = ptr;
    SIGASSERT(obj,OBJ);
    if (!obj) return JSI_OK;
    Jsi_ObjDecrRefCount(interp, obj);
    return JSI_OK;
}

/* TODO: incr ref before add then just decr till done. */
static int freeValueTbl(Jsi_Interp *interp, void *ptr) {
    Jsi_Value *val = ptr;
    SIGASSERT(val,VALUE);
    if (!val) return JSI_OK;
    //printf("GEN: %p\n", val);
    Jsi_DecrRefCount(interp, val);
    return JSI_OK;
}

static int freeUserdataTbl(Jsi_Interp *interp, void *ptr) {
    if (ptr) 
        jsi_UserObjDelete(interp, ptr);
    return JSI_OK;
}

void Jsi_ShiftArgs(Jsi_Interp *interp) {
    Jsi_Value *v = interp->args; //Jsi_NameLookup(interp, "console.args");
    if (v==NULL || v->vt != JSI_VT_OBJECT || v->d.obj->arr == NULL || v->d.obj->arrCnt <= 0)
        return;
    Jsi_Obj *obj = v->d.obj;
    int n = v->d.obj->arrCnt;
    n--;
    v = obj->arr[0];
    if (n>0)
        memmove(obj->arr, obj->arr+1, n*sizeof(Jsi_Value*));
    obj->arr[n] = NULL;
    Jsi_ObjSetLength(interp, obj, n);    
}

char *jsi_execName = NULL;
static Jsi_Value *jsi_execValue = NULL;

Jsi_Value *Jsi_Executable(Jsi_Interp *interp)
{
    return jsi_execValue;
}

static int KeyLocker(Jsi_Hash* tbl, int lock)
{
    if (!lock)
        Jsi_MutexUnlock(jsiMainInterp, jsiMainInterp->Mutex);
    else
        return Jsi_MutexLock(jsiMainInterp, jsiMainInterp->Mutex);
    return JSI_OK;
}

Jsi_Interp* Jsi_InterpCreate(Jsi_Interp *parent, int argc, char **argv, Jsi_Value *opts)
{
    Jsi_Interp* interp = Jsi_Calloc(1,sizeof(*interp));
    char buf[BUFSIZ];
    if (jsiMainInterp == NULL && parent == NULL)
        jsiMainInterp = interp;
    interp->parent = parent;
#ifdef VALUE_DEBUG
    interp->valueDebugTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, NULL);
#endif    
    if (!parent)
        interp->maxInterpDepth = JSI_MAX_SUBINTERP_DEPTH;
    else {
        if (parent->noSubInterps) {
            Jsi_Free(interp);
            interp = parent;
            Jsi_LogError("subinterps disallowed");
            return NULL;
        }
        interp->maxInterpDepth = parent->maxInterpDepth;
        interp->interpDepth = parent->interpDepth+1;
        if (interp->interpDepth > interp->maxInterpDepth) {
            Jsi_Free(interp);
            interp = parent;
            Jsi_LogError("exceeded max subinterp depth");
            return NULL;
        }
    }
#ifndef DISABLE_GETENV
    interp->debug = (getenv("JSI_DEBUG") != NULL);
    interp->traceCalls = (getenv("JSI_TRACE") != NULL);
#endif
    interp->maxDepth = JSI_MAX_EVAL_DEPTH;
    interp->maxIncDepth = JSI_MAX_INCLUDE_DEPTH;
    SIGINIT(interp,INTERP);
    SIGINIT((&interp->lastSubscriptFail), VALUE);
    interp->NullValue = Jsi_ValueNewNull(interp);
    Jsi_IncrRefCount(interp, interp->NullValue);
    interp->curDir = Jsi_Strdup(getcwd(buf, sizeof(buf)));   
    interp->assocTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, freeAssocTbl);
    interp->cmdSpecTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, freeCmdSpecTbl);
    interp->eventTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, freeEventTbl);
    interp->genDataTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, jsiFree);
    interp->fileTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, jsiFree);
    interp->funcTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL/*freeGenObjTbl*/);
    interp->protoTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL/*freeValueTbl*/);
    //interp->protoTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, freeValueTbl);
    interp->regexpTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, regExpFree);
    interp->preserveTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, jsiFree);
    interp->loadTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, jsi_FreeOneLoadHandle);
    interp->optionDataHash = Jsi_HashNew(interp, JSI_KEYS_STRING, jsiFree);
    interp->lockTimeout = -1;
#ifdef JSI_LOCK_TIMEOUT
    interp->lockTimeout JSI_LOCK_TIMEOUT;
#endif
#ifndef JSI_DO_UNLOCK
#define JSI_DO_UNLOCK 1
#endif
    interp->doUnlock = JSI_DO_UNLOCK;

    if (interp == jsiMainInterp || interp->threadId != jsiMainInterp->threadId) {
        interp->strKeyTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL);
        interp->privKeys = 1;
    }
    if (!interp->strKeyTbl)
        interp->strKeyTbl = jsiMainInterp->strKeyTbl;
#ifndef JSI_USE_STRICT
#define JSI_USE_STRICT 1
#endif
    interp->strict = JSI_USE_STRICT;
    const char *scp;
    if ((scp = getenv("JSI_STRICT")))
        interp->strict = atoi(scp);
    if (opts && opts->vt != JSI_VT_NULL && Jsi_OptionsProcess(interp, InterpOptions, opts, interp, 0) < 0) {
        Jsi_InterpDelete(interp);
        return NULL;
    }
    if (interp == jsiMainInterp) {
        interp->subthread = 0;
    } else {
        if (interp->privKeys && interp->strKeyTbl == jsiMainInterp->strKeyTbl) {
            //Jsi_HashDelete(interp->strKeyTbl);
            Jsi_OptionsFree(interp, InterpOptions, interp, 0); /* Reparse options to populate new key table. */
            interp->strKeyTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL);
            if (opts->vt != JSI_VT_NULL) Jsi_OptionsProcess(interp, InterpOptions, opts, interp, 0);
        } else if (interp->privKeys == 0 && interp->strKeyTbl != jsiMainInterp->strKeyTbl) {
            Jsi_OptionsFree(interp, InterpOptions, interp, 0); /* Reparse options to populate new key table. */
            Jsi_HashDelete(interp->strKeyTbl);
            interp->strKeyTbl = jsiMainInterp->strKeyTbl;
            if (opts->vt != JSI_VT_NULL) Jsi_OptionsProcess(interp, InterpOptions, opts, interp, 0);
        }
        if (interp->subthread)
            jsiMainInterp->threadCnt++;
        if (interp->subthread && interp->strKeyTbl == jsiMainInterp->strKeyTbl)
            jsiMainInterp->threadShrCnt++;
        if (jsiMainInterp->threadShrCnt)
            jsiMainInterp->strKeyTbl->lockProc = KeyLocker;
    }
    if (parent && parent->isSafe)
        interp->isSafe = 1;
    char *ocp = getenv("JSI_INTERP_OPTS");
    Jsi_DString oStr = {};
#ifdef JSI_INTERP_OPTS  /* eg. "nonStrict: true, maxOpCnt:1000000" */
    if (ocp && *ocp)
        Jsi_DSAppend(&oStr, "{", JSI_INTERP_OPTS, ", ", ocp+1, NULL);
    else
        Jsi_DSAppend(&oStr, "{", JSI_INTERP_OPTS, "}", NULL);
#else
    Jsi_DSAppend(&oStr, ocp, NULL);
#endif
    ocp = Jsi_DSValue(&oStr);
    if (interp == jsiMainInterp  && *ocp) {
        Jsi_Value *popts = Jsi_ValueNew1(interp);
        if (Jsi_JSONParse(interp, ocp, popts, 0) != JSI_OK ||
            Jsi_OptionsProcess(interp, InterpOptions, popts, interp, JSI_OPTS_IS_UPDATE) < 0) {
            Jsi_InterpDelete(interp);
            Jsi_DSFree(&oStr);
            return NULL;
        }
        Jsi_DecrRefCount(interp, popts);
    }
    Jsi_DSFree(&oStr);
    interp->threadId = Jsi_CurrentThread();
    if (interp == jsiMainInterp)
        interp->lexkeyTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL);
    else
        interp->lexkeyTbl = jsiMainInterp->lexkeyTbl;
    interp->thisTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, freeValueTbl);
    interp->userdataTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, freeUserdataTbl);
    interp->varTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL);
    interp->codeTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, freeCodeTbl);
    interp->genValueTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD,freeValueTbl);
    interp->genObjTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, freeGenObjTbl);
    interp->maxArrayList = MAX_ARRAY_LIST;
    if (!is_init) {
        is_init = 1;
        jsi_ValueInit(interp);
        interpsTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, 0);
    }
    

    /* current scope, also global */
    interp->csc = Jsi_ValueNew1(interp);
    interp->csc->f.bits.isglob = 1;
    Jsi_ValueMakeObject(interp,interp->csc, Jsi_ObjNew(interp));
    interp->incsc = interp->csc;
    
    /* initial scope chain, nothing */
    interp->ingsc = interp->gsc = jsi_ScopeChainNew(interp, 0);
    
    interp->ps = jsi_PstateNew(interp); /* Default parser. */

    if (interp->args && argc) {
        Jsi_LogFatal("args may not be specified both as options and parameter");
        Jsi_InterpDelete(interp);
        return NULL;
    }
    if (interp->maxDepth>JSI_MAX_EVAL_DEPTH)
        interp->maxDepth = JSI_MAX_EVAL_DEPTH;

#define DOINIT(nam) if (jsi_##nam##Init(interp) != JSI_OK) { Jsi_LogFatal("Init failure in %s", #nam); }

    DOINIT(Proto);

    if (argc >= 0) {
        Jsi_Value *iargs = Jsi_ValueNew1(interp);
        iargs->f.bits.isglob = 1;
        iargs->f.bits.dontdel = 1;
        iargs->f.bits.readonly = 1;
        Jsi_Obj *iobj = Jsi_ObjNew(interp);
        Jsi_ValueMakeArrayObject(interp,iargs, iobj);
        int msiz = (argc?argc-1:0);
        iobj->arr = Jsi_Calloc(msiz+1, sizeof(Jsi_Value*));
        iobj->arrMaxSize = msiz;
        int i;
        for (i = 1; i < argc; ++i) {
            iobj->arr[i-1] = Jsi_ValueNewStringDup(interp, argv[i]);
            Jsi_IncrRefCount(interp, iobj->arr[i-1]);
        }
        Jsi_ObjSetLength(interp, iobj, msiz);
        interp->args = iargs;
    }

    jsi_CmdsInit(interp);
    jsi_InterpInit(interp);
    jsi_JSONInit(interp);

    static Jsi_Value nret = VALINIT;
    interp->ret = nret;
    interp->Mutex = Jsi_MutexNew(interp);
    if (1 || interp->subthread) {
        interp->QMutex = Jsi_MutexNew(interp);
        Jsi_DSInit(&interp->interpEvalQ);
        Jsi_DSInit(&interp->interpMsgQ);
    }
    if (interp != jsiMainInterp && !parent)
        Jsi_HashEntryCreate(interpsTbl, interp, NULL);
        
    if (!interp->isSafe) {
        DOINIT(Load);
#ifndef JSI_OMIT_FILESYS
        Jsi_execInit(interp);
#endif
#ifndef JSI_OMIT_SIGNAL
        jsi_Initsignal(interp);
#endif
    }
    if (interp->isSafe == 0 || interp->safeWriteDirs!=NULL || interp->safeReadDirs!=NULL) {
#ifndef JSI_OMIT_FILESYS
        jsi_FileCmdsInit(interp);
        jsi_FilesysInit(interp);
#endif
#ifdef HAVE_SQLITE
        Jsi_InitSqlite(interp);
#endif
    }
#ifdef HAVE_WEBSOCKET
    Jsi_InitWebsocket(interp);
#endif

    if (argc > 0) {
        char *ss = argv[0];
        char epath[PATH_MAX] = "";
#ifdef __WIN32
        if (GetModuleFileName(NULL, epath, sizeof(epath))>0)
            ss = epath;
#else
#ifndef PROC_SELF_DIR
#define PROC_SELF_DIR "/proc/self/exe"
#endif
        if (ss && *ss != '/' && readlink(PROC_SELF_DIR, epath, sizeof(epath))) {
            ss = epath;
        }
#endif
        Jsi_Value *src = Jsi_ValueNewStringDup(interp, ss);
        Jsi_IncrRefCount(interp, src);
        jsi_execName = Jsi_Realpath(interp, src, NULL);
        Jsi_DecrRefCount(interp, src);
        jsi_execValue = Jsi_ValueNewString(interp, jsi_execName);
    }
    
    //interp->nocacheOpCodes = 1;
    return interp;
}

int Jsi_InterpGone( Jsi_Interp* interp)
{
    return (interp == NULL || interp->deleting || interp->destroying || interp->exited);
}

static void DeleteAllInterps() { /* Delete toplevel interps. */
    Jsi_HashEntry *hPtr;
    Jsi_HashSearch search;
    for (hPtr = Jsi_HashEntryFirst(interpsTbl, &search); hPtr; hPtr = Jsi_HashEntryNext(&search)) {
        Jsi_Interp *interp = Jsi_HashKeyGet(hPtr);
        Jsi_HashEntryDelete(hPtr);
        interp->destroying = 1;
        Jsi_InterpDelete(interp);
    }
}

static int jsiInterpDelete(Jsi_Interp* interp, void *unused)
{
    SIGASSERT(interp,INTERP);
    if (interp == jsiMainInterp) { /* cleanup all toplevel interps. */
        DeleteAllInterps();
    }
    jsiDelInterp = interp;
    if (interp->gsc) jsi_ScopeChainFree(interp, interp->gsc);
    //if (interp->csc->d.obj->refcnt>1) /* TODO: This is a hack to release global. */
       // Jsi_ObjDecrRefCount(interp, interp->csc->d.obj);
    if (interp->csc) Jsi_DecrRefCount(interp, interp->csc);
    if (interp->ps) jsi_PstateFree(interp->ps);
    int i;
    for (i=0; i<interp->maxStack; i++) {
        if (interp->Stack[i]) Jsi_DecrRefCount(interp, interp->Stack[i]);
        if (interp->Obj_this[i]) Jsi_DecrRefCount(interp, interp->Obj_this[i]);
    }
    Jsi_Free(interp->Stack);
    Jsi_Free(interp->Obj_this);

    Jsi_HashDelete(interp->assocTbl);
    Jsi_HashDelete(interp->codeTbl);
    Jsi_HashDelete(interp->cmdSpecTbl);
    Jsi_HashDelete(interp->fileTbl);
    Jsi_HashDelete(interp->funcTbl);
    if (interp == jsiMainInterp)
        Jsi_HashDelete(interp->lexkeyTbl);
    Jsi_HashDelete(interp->protoTbl);
    Jsi_HashDelete(interp->regexpTbl);
    if (interp->subthread)
        jsiMainInterp->threadCnt--;
    if (interp->subthread && interp->strKeyTbl == jsiMainInterp->strKeyTbl)
        jsiMainInterp->threadShrCnt--;
    if (!jsiMainInterp->threadShrCnt)
        jsiMainInterp->strKeyTbl->lockProc = NULL;
    if (interp == jsiMainInterp || interp->strKeyTbl != jsiMainInterp->strKeyTbl)
        Jsi_HashDelete(interp->strKeyTbl);
    Jsi_ValueMakeUndef(interp, &interp->ret);
    Jsi_HashDelete(interp->thisTbl);
    Jsi_HashDelete(interp->userdataTbl);
    Jsi_HashDelete(interp->eventTbl);
    Jsi_HashDelete(interp->varTbl);
    Jsi_HashDelete(interp->genValueTbl);
    Jsi_HashDelete(interp->genObjTbl);
    Jsi_HashDelete(interp->genDataTbl);
    Jsi_HashDelete(interp->loadTbl);
    Jsi_HashDelete(interp->optionDataHash);
    if (interp->preserveTbl->numEntries!=0)
        Jsi_LogBug("Preserves unbalanced");
    Jsi_HashDelete(interp->preserveTbl);
    if (interp->argv0)
        Jsi_DecrRefCount(interp, interp->argv0);
    if (interp->console)
        Jsi_DecrRefCount(interp, interp->console);
    if (interp->curDir)
        Jsi_Free(interp->curDir);
    if (interp == jsiMainInterp) {
        jsiMainInterp = NULL;
        jsi_FilesysDone();
    }
    if (interp->Mutex) {
        Jsi_MutexDone(interp, interp->Mutex);
        Jsi_Free(interp->Mutex);
    }
    if (interp->QMutex) {
        Jsi_MutexDone(interp, interp->QMutex);
        Jsi_Free(interp->QMutex);
        Jsi_DSFree(&interp->interpEvalQ);
        Jsi_DSFree(&interp->interpMsgQ);
    }
    Jsi_DecrRefCount(interp, interp->NullValue);
#ifdef VALUE_DEBUG
    Jsi_HashSearch search;
    Jsi_HashEntry *hPtr;

    for (hPtr = Jsi_HashEntryFirst(interp->valueDebugTbl, &search);
        hPtr != NULL; hPtr = Jsi_HashEntryNext(&search)) {
        Jsi_Value *vp = Jsi_HashKeyGet(hPtr);
        if (vp==NULL || vp->sig != JSI_SIG_VALUE)
            printf("BAD VALUE: %p\n", vp);
        else
            printf("VALUE: %s:%d in func %s\n", vp->fname, vp->line, vp->func);
    }
    Jsi_HashDelete(interp->valueDebugTbl);
#endif
    Jsi_OptionsFree(interp, InterpOptions, interp, 0);
    SIGASSERT(interp,INTERP);
    MEMCLEAR(interp);
    jsiDelInterp = NULL;
    Jsi_Free(interp);
    return JSI_OK;
}

void Jsi_InterpDelete(Jsi_Interp* interp)
{
    if (interp->deleting || interp->level > 0)
        return;
    if (interp->onDeleteProc)
        (*interp->onDeleteProc)(interp, (void*)interp->exitCode);
    interp->deleting = 1;
    Jsi_EventuallyFree(interp, interp, jsiInterpDelete);
}

typedef struct {
    void *data;
    Jsi_Interp *interp;
    int refCnt;
    Jsi_DeleteProc* proc;
} PreserveData;

void Jsi_Preserve(Jsi_Interp* interp, void *data) {
    int isNew;
    PreserveData *ptr;
    Jsi_HashEntry *hPtr = Jsi_HashEntryCreate(interp->preserveTbl, data, &isNew);
    assert(hPtr);
    if (!isNew) {
        ptr = Jsi_HashValueGet(hPtr);
        assert(interp == ptr->interp);
        ptr->refCnt++;
    } else {
        ptr = Jsi_Calloc(1,sizeof(*ptr));
        Jsi_HashValueSet(hPtr, ptr);
        ptr->interp = interp;
        ptr->data = data;
        ptr->refCnt = 1;
    }
}

void Jsi_Release(Jsi_Interp* interp, void *data) {
    Jsi_HashEntry *hPtr = Jsi_HashEntryFind(interp->preserveTbl, data);
    if (!hPtr) return;
    PreserveData *ptr = Jsi_HashValueGet(hPtr);
    assert(ptr->interp == interp);
    if (--ptr->refCnt > 0) return;
    if (ptr->proc)
        (*ptr->proc)(interp, data);
    Jsi_Free(ptr);
    Jsi_HashEntryDelete(hPtr);
}

void Jsi_EventuallyFree(Jsi_Interp* interp, void *data, Jsi_DeleteProc* proc) {
    Jsi_HashEntry *hPtr = Jsi_HashEntryFind(interp->preserveTbl, data);
    if (!hPtr) {
        (*proc)(interp, data);
        return;
    }
    PreserveData *ptr = Jsi_HashValueGet(hPtr);
    assert(ptr && ptr->interp == interp);
    Jsi_HashEntryDelete(hPtr);
}

Jsi_DeleteProc* Jsi_InterpOnDelete(Jsi_Interp *interp, Jsi_DeleteProc *freeProc)
{
    Jsi_DeleteProc* old = interp->onDeleteProc;
    interp->onDeleteProc = freeProc;
    return old;
}

static void interpObjErase(InterpObj *fo)
{
    SIGASSERT(fo,INTERPOBJ);
    if (fo->subinterp) {
        Jsi_Interp *interp = fo->subinterp;        
        fo->subinterp = NULL;
        Jsi_HashDelete(fo->aliases);
        Jsi_InterpDelete(interp);
        /*fclose(fo->fp);
        Jsi_Free(fo->interpname);
        Jsi_Free(fo->mode);*/
    }
    fo->subinterp = NULL;
}

static int interpObjFree(Jsi_Interp *interp, void *data)
{
    InterpObj *fo = data;
    SIGASSERT(fo,INTERPOBJ);
    interpObjErase(fo);
    Jsi_Free(fo);
    return JSI_OK;
}

static int interpObjIsTrue(void *data)
{
    InterpObj *fo = data;
    SIGASSERT(fo,INTERPOBJ);
    if (!fo->subinterp) return 0;
    else return 1;
}

static int interpObjEqual(void *data1, void *data2)
{
    return (data1 == data2);
}


int Jsi_DoneInterp(Jsi_Interp *interp)
{
    Jsi_UserObjUnregister(interp, &interpobject);
    return JSI_OK;
}

static int InterpConfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    InterpObj *udf = Jsi_UserObjGetData(interp, _this, funcPtr);
  
    return Jsi_OptionsConf(interp, InterpOptions, Jsi_ValueArrayIndex(interp, args, 0), udf?udf->subinterp:interp, *ret, 0);

}

#define FN_eval JSI_INFO("\
Unless an 'async' parameter of true is given, we wait until the sub-interp is idle, \
make the call, and return the result.  Otherwise the call is acyncronous (threaded only)")

/* TODO: support async func-callback.  Same for call/send. */
static int InterpEvalCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int async = 0, rc = JSI_OK, isthrd;
    Jsi_ValueMakeUndef(interp, *ret);
    InterpObj *udf = Jsi_UserObjGetData(interp, _this, funcPtr);
    if (!udf) {
        Jsi_LogError("Apply Interp.eval in a non-interp object");
        return JSI_ERROR;
    }
    Jsi_Interp *sinterp = udf->subinterp;
    if (Jsi_InterpGone(interp)) {
        Jsi_LogError("Sub-interp gone");
        return JSI_ERROR;
    }
    isthrd = (interp->threadId != sinterp->threadId);
    Jsi_Value *nw = Jsi_ValueArrayIndex(interp, args, 1);
    if (nw && Jsi_GetBoolFromValue(interp, nw, &async))
        return JSI_ERROR;
    char *cp = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    if (cp==NULL || *cp == 0)
        return JSI_OK;
    if (async && isthrd) {
        /* Post to thread event in sub-interps queue. TODO: could just use event like below... */
        if (Jsi_MutexLock(interp, sinterp->QMutex) != JSI_OK)
            return JSI_ERROR;
        Jsi_DSAppend(&sinterp->interpEvalQ, Jsi_Strlen(Jsi_DSValue(&sinterp->interpEvalQ))?";":"", cp, NULL);
        Jsi_MutexUnlock(interp, sinterp->QMutex);
        return JSI_OK;
    }
    if (interp->doUnlock) Jsi_MutexUnlock(interp, interp->Mutex);
    if (!isthrd) {
        sinterp->level++;
        if (interp->tryDepth)
            sinterp->tryDepth++;
        rc = Jsi_EvalString(sinterp, cp, 0);
        if (interp->tryDepth) {
            sinterp->tryDepth--;
            if (rc != JSI_OK) {
                strcpy(interp->errMsgBuf, sinterp->errMsgBuf);
                interp->errLine = sinterp->errLine;
                interp->errFile = sinterp->errFile;
            }
        }
        sinterp->level--;
    } else {
        if (Jsi_MutexLock(interp, sinterp->QMutex) != JSI_OK)
            return JSI_ERROR;
        InterpStrEvent *se, *s = Jsi_Calloc(1, sizeof(*s));
        SIGINIT(s,INTERPSTREVENT);
        s->isExec = 1;
        s->tryDepth = interp->tryDepth;
        Jsi_DSInit(&s->data);
        Jsi_DSAppend(&s->data, cp, NULL);
        Jsi_DSInit(&s->func);
        //s->mutex = Jsi_MutexNew(interp);
        //Jsi_MutexLock(s->mutex);
        se = sinterp->interpStrEvents;
        if (!se)
            sinterp->interpStrEvents = s;
        else {
            while (se->next)
                se = se->next;
            se->next = s;
        }
    
        Jsi_MutexUnlock(interp, sinterp->QMutex);
        while (s->isExec)      /* Wait until done. TODO: timeout??? */
            Jsi_Sleep(interp, 1);
        rc = s->rc;
        if (rc != JSI_OK)
            Jsi_LogError("eval failed: %s", Jsi_DSValue(&s->data));
        Jsi_DSFree(&s->func);
        Jsi_DSFree(&s->data);
        Jsi_Free(s);
    }

    if (interp->doUnlock && Jsi_MutexLock(interp, interp->Mutex) != JSI_OK) {
        return JSI_ERROR;
    }

    if (Jsi_InterpGone(sinterp))
    {
        /* TODO: should exit() be able to delete??? */
        //Jsi_InterpDelete(sinterp);
        return JSI_OK;
    }
    if (rc != JSI_OK && !async)
        return rc;
    if (sinterp->ret.vt != JSI_VT_UNDEF) {
        //Jsi_ValueCopy(interp, ret, &sinterp->ret);
        Jsi_CleanValue(sinterp, interp, &sinterp->ret, *ret);
    }
    return JSI_OK;
}

static int InterpInfoCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    InterpObj *udf = Jsi_UserObjGetData(interp, _this, funcPtr);
    Jsi_Interp *subinterp = interp;
    if (udf) {
        if (!udf->subinterp) {
            Jsi_LogError("Sub-interp gone");
            return JSI_ERROR;
        }
        subinterp = udf->subinterp;
    }
    return jsi_InterpInfo(subinterp, args, _this, ret, funcPtr);
}

int jsi_InterpInfo(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Interp *sinterp = interp;
    Jsi_DString dStr = {}, cStr = {};
    char tbuf[1024];
    Jsi_Value *v = Jsi_ValueArrayIndex(interp, args, 0);
    tbuf[0] = 0;
    if (v) {
        InterpObj *udf = NULL;
        if (v && v->vt == JSI_VT_OBJECT && v->d.obj->ot == JSI_OT_USEROBJ)
            udf = v->d.obj->d.uobj->data;
        if (udf && udf->subinterp) {
            SIGASSERT(udf, INTERPOBJ);
            sinterp = udf->subinterp;
        } else {
            Jsi_LogError("unknown interp");
            return JSI_ERROR;
        }
    }
    if (sinterp->subthread)
        sprintf(tbuf, ", thread:{errorCnt:%u, evalCnt:%u, msgCnt:%u }",
            sinterp->threadErrCnt, sinterp->threadEvalCnt, sinterp->threadMsgCnt );
    Jsi_DSPrintf(&dStr, "{curLevel:%d, hasExited:%d, opCnt:%d, isSafe:%s, codeCacheHits: %d, "
        "funcCallCnt:%d, cmdCallCnt:%d, cwd:\"%s\", lockTimeout: %d, name, \"%s\", parent: %p %s%s};",
        sinterp->level, sinterp->exited, sinterp->opCnt, sinterp->isSafe?"true":"false", sinterp->codeCacheHit,
        sinterp->funcCallCnt, sinterp->cmdCallCnt,
        (sinterp->curDir?sinterp->curDir:Jsi_GetCwd(sinterp,&cStr)),
        sinterp->lockTimeout, sinterp->name?sinterp->name:"", sinterp->parent, tbuf[0]?",":"", tbuf);
    int rc = Jsi_JSONParse(interp, Jsi_DSValue(&dStr), *ret, 0);
    Jsi_DSFree(&dStr);
    Jsi_DSFree(&cStr);
    return rc;
}

static int SubInterpEvalCallback(Jsi_Interp *interp, void* data)
{
    int rc = JSI_OK;
    Jsi_DString dStr = {};
    if (Jsi_MutexLock(interp, interp->QMutex) != JSI_OK)
        return JSI_ERROR;
    char *cp = Jsi_DSValue(&interp->interpEvalQ);
    if (*cp) {
        Jsi_DSAppend(&dStr, cp, NULL);
        Jsi_DSSetLength(&interp->interpEvalQ, 0);
        interp->threadEvalCnt++;
        Jsi_MutexUnlock(interp, interp->QMutex);
        if (Jsi_EvalString(interp, Jsi_DSValue(&dStr), 0) != JSI_OK)
            rc = JSI_ERROR;
        Jsi_DSSetLength(&dStr, 0);
        if (Jsi_MutexLock(interp, interp->QMutex) != JSI_OK)
            return JSI_ERROR;
    }
    cp = Jsi_DSValue(&interp->interpMsgQ);
    if (*cp) {
        //if (!interp->parent) printf("RECIEVING: %s\n", cp);
        Jsi_DSAppend(&dStr, cp, NULL);
        Jsi_DSSetLength(&interp->interpEvalQ, 0);
        interp->threadMsgCnt++;
        Jsi_MutexUnlock(interp, interp->QMutex);
        if (Jsi_CommandInvokeJSON(interp, interp->recvCmd, Jsi_DSValue(&dStr), NULL) != JSI_OK)
            rc = JSI_ERROR;
        Jsi_DSSetLength(&interp->interpMsgQ, 0);
    } else
        Jsi_MutexUnlock(interp, interp->QMutex);
    Jsi_DSFree(&dStr);
    if (Jsi_MutexLock(interp, interp->QMutex) != JSI_OK)
        return JSI_ERROR;
        
    /* Process subevents. */
    InterpStrEvent *oldse, *se = interp->interpStrEvents;
    Jsi_MutexUnlock(interp, interp->QMutex);
    while (se) {
        oldse = se;
        int isExec = se->isExec;
        if (isExec) {
            if (se->tryDepth)
                interp->tryDepth++;
            se->rc = Jsi_EvalString(interp, Jsi_DSValue(&se->data), 0);
            Jsi_DSSetLength(&se->data, 0);
            if (se->rc != JSI_OK && se->tryDepth) {
                Jsi_DSAppend(&se->data, interp->errMsgBuf, NULL);
                se->errLine = interp->errLine;
                se->errFile = interp->errFile;
            } else {
                Jsi_ValueGetDString(interp, &interp->ret, &se->data, JSI_OUTPUT_JSON);
            }
            if (se->tryDepth)
                interp->tryDepth--;
                
        /* Otherwise, async calls. */
        } else if (se->objData) {
            if (JSI_OK != Jsi_CommandInvoke(interp, Jsi_DSValue(&se->func), se->objData, NULL))
                rc = JSI_ERROR;
        } else {
            if (JSI_OK != Jsi_CommandInvokeJSON(interp, Jsi_DSValue(&se->func), Jsi_DSValue(&se->data), NULL))
                rc = JSI_ERROR;
        }
        if (!isExec) {
            Jsi_DSFree(&se->func);
            Jsi_DSFree(&se->data);
        }
        if (Jsi_MutexLock(interp, interp->QMutex) != JSI_OK)
            return JSI_ERROR;
        interp->interpStrEvents = se->next;
        if (!isExec) Jsi_Free(se);
        se = interp->interpStrEvents;
        Jsi_MutexUnlock(interp, interp->QMutex);
        if (isExec)
            oldse->isExec = 0;
    }

    return rc;
}


static int ThreadEvalCallback(Jsi_Interp *interp, void* data) {
    int rc;

    if ((rc=SubInterpEvalCallback(interp, data)) != JSI_OK)
        interp->threadErrCnt++;
    return rc;
}

/* Create an event handler in interp to handle call/eval/send asyncronously via 'sys.update()'. */
void jsi_AddEventHandler(Jsi_Interp *interp)
{
    Jsi_Event *ev;
    while (!interp->hasEventHdl) { /* Find an empty event slot. */
        int isNew, id;
        id = interp->eventIdx++;
        Jsi_HashEntry *hPtr = Jsi_HashEntryCreate(interp->eventTbl, (void*)id, &isNew);
        if (!isNew)
            continue;
        ev = Jsi_Calloc(1, sizeof(*ev));
        SIGINIT(ev,EVENT);
        ev->id = id;
        ev->handler = ThreadEvalCallback;
        ev->hPtr = hPtr;
        ev->evType = JSI_EVENT_ALWAYS;
        Jsi_HashValueSet(hPtr, ev);
        interp->hasEventHdl = 1;
    }
}

#define FN_call JSI_INFO("\
Invoke function in sub-interp with arguments.  Since interps are not allowed to share objects, \
data is automatically cleansed by encoding/decoding \
to/from JSON if required.  Unless an 'async' parameter of true is given, we wait until the sub-interp is idle, \
make the call, and return the result.  Otherwise the call is acyncronous.")

static int InterpCallCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    InterpObj *udf = Jsi_UserObjGetData(interp, _this, funcPtr);
    int isthrd;
    Jsi_Interp *sinterp;
    if (udf)
        sinterp = udf->subinterp;
    else {
        Jsi_LogError("Apply Interp.call in a non-subinterp");
        return JSI_ERROR;
    }
    if (Jsi_InterpGone(sinterp)) {
        Jsi_LogError("Sub-interp gone");
        return JSI_ERROR;
    }
    isthrd = (interp->threadId != sinterp->threadId);
    
    Jsi_Value *func = NULL;
    char *fname = NULL; 
    func = Jsi_ValueArrayIndex(interp, args, 0);   
    fname = Jsi_ValueString(interp, func, NULL);
    if (!fname) {
        Jsi_LogError("function name must be a string");
        return JSI_ERROR;
    }
    if (Jsi_MutexLock(interp, sinterp->Mutex) != JSI_OK)
        return JSI_ERROR;
    Jsi_Value *namLU = Jsi_NameLookup(sinterp, fname);
    Jsi_MutexUnlock(interp, sinterp->Mutex);
    if (namLU == NULL) {
        Jsi_LogError("unknown function: %s", fname);
        return JSI_ERROR;
    }
    
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 1);
    char *cp = Jsi_ValueString(interp, arg, NULL);

    if (cp == NULL && !Jsi_ValueIsArray(interp, arg)) {
        Jsi_LogError("expected string or array");
        return JSI_ERROR;
    }

    Jsi_Value *vasync = Jsi_ValueArrayIndex(interp, args, 2);
    int rc = JSI_OK, async = 0;
    if (vasync && Jsi_GetBoolFromValue(interp, vasync, &async))
        return JSI_ERROR;
    
    if (!async) {
        Jsi_Value sret = VALINIT, *srPtr = &sret;
        Jsi_DString dStr = {};
        if (cp == NULL)
            cp = (char*)Jsi_ValueGetDString(interp, arg, &dStr, JSI_OUTPUT_JSON);
        if (interp->doUnlock) Jsi_MutexUnlock(interp, interp->Mutex);
        if (Jsi_MutexLock(interp, sinterp->Mutex) != JSI_OK) {
            if (interp->doUnlock) Jsi_MutexLock(interp, interp->Mutex);
            return JSI_ERROR;
        }
        /* TODO: call from this interp may not be safe if threaded. 
         * Could instead use async code below then wait for unlock on se->mutex. */
        rc = Jsi_CommandInvokeJSON(sinterp, fname, cp, &srPtr);
        Jsi_DSSetLength(&dStr, 0);
        ConvertReturn(interp, sinterp, &sret);
        Jsi_ValueCopy(interp, *ret, &sret);
        Jsi_ValueMakeUndef(interp, &sret);
        Jsi_DSFree(&dStr);
        Jsi_MutexUnlock(interp, sinterp->Mutex);
        if (interp->doUnlock && Jsi_MutexLock(interp, interp->Mutex) != JSI_OK) {
            Jsi_LogBug("mutex re-get failed");
            return JSI_ERROR;
        }
        return rc;
    }
    
    /* Post to thread event in sub-interps queue. */
    if (Jsi_MutexLock(interp, sinterp->QMutex) != JSI_OK)
        return JSI_ERROR;
        
    /* Is an async call. */
    InterpStrEvent *se, *s = Jsi_Calloc(1, sizeof(*s));
    if (!cp) {
        Jsi_ValueGetDString(interp, arg, &s->data, JSI_OUTPUT_JSON);
    }
    Jsi_DSInit(&s->data);
    Jsi_DSAppend(&s->data, cp, NULL);
    Jsi_DSInit(&s->func);
    Jsi_DSAppend(&s->func, fname, NULL);
    se = sinterp->interpStrEvents;
    if (!se)
        sinterp->interpStrEvents = s;
    else {
        while (se->next)
            se = se->next;
        se->next = s;
    }

    Jsi_MutexUnlock(interp, sinterp->QMutex);
    if (!isthrd) {
        if (SubInterpEvalCallback(sinterp, NULL) != JSI_OK)
            sinterp->threadErrCnt++;
    }
    return JSI_OK;
}

int Jsi_Mount( Jsi_Interp *interp, Jsi_Value *archive, Jsi_Value *mount, Jsi_Value *ret)
{
#ifdef JSI_OMIT_ZVFS
    return JSI_ERROR;
#else
    return Zvfs_Mount(interp, archive, mount, ret);
#endif
}

int Jsi_AddIndexFiles(Jsi_Interp *interp, const char *dir) {
    /* Look for jsiIndex.js to setup. */
    Jsi_DString dStr = {};
    Jsi_StatBuf stat;
    int i, cnt = 0;
    for (i=0; i<2; i++) {
        Jsi_DSAppend(&dStr, dir, (i==0?"/lib":""),"/jsiIndex.jsi", NULL);
        Jsi_Value *v = Jsi_ValueNewStringKey(interp, Jsi_DSValue(&dStr));
        if (Jsi_Stat(interp, v, &stat) != 0)
            Jsi_ValueFree(interp, v);
        else {
            if (!interp->indexFiles) {
                interp->indexFiles = Jsi_ValueNewArray(interp, 0, 0);
                Jsi_IncrRefCount(interp, interp->indexFiles);
            }
            Jsi_ObjArrayAdd(interp, interp->indexFiles->d.obj, v);
            cnt++;
            interp->indexLoaded = 0;
        }
        Jsi_DSSetLength(&dStr, 0);
    }
    Jsi_DSFree(&dStr);
    return cnt;
}

int Jsi_ExecZip(Jsi_Interp *interp, const char *exeFile, const char *mntDir, int *jsFound)
{
#ifndef JSI_OMIT_ZVFS
    Jsi_StatBuf stat;
    Jsi_Value *vinit, *vmnt = NULL;
    Jsi_Value *vexe = Jsi_ValueNewStringKey(interp, exeFile);
    Jsi_Value *ret = NULL;
    int rc;
    const char *omntDir = mntDir;
    if (jsFound)
        *jsFound = 0;
    if (!mntDir)
        ret = Jsi_ValueNew(interp);
    else
        vmnt = Jsi_ValueNewStringKey(interp, mntDir);
    rc =Jsi_Mount(interp, vexe, vmnt, ret);
    if (rc != JSI_OK)
        return -1;
    Jsi_DString dStr = {};
    if (!mntDir)
        mntDir = Jsi_ValueString(interp, ret, NULL);
    Jsi_DSAppend(&dStr, mntDir, "/main.jsi", NULL);
    vinit = Jsi_ValueNewStringKey(interp,  Jsi_DSValue(&dStr));
    Jsi_DSFree(&dStr);
    if (Jsi_Stat(interp, vinit, &stat) == 0) {
        if (jsFound)
            *jsFound |= JSI_ZIP_MAIN;
        interp->execZip = vexe;
        return Jsi_EvalFile(interp, vinit, JSI_EVAL_ARGV0|JSI_EVAL_INDEX);
    } else {
        if (Jsi_AddIndexFiles(interp, mntDir) && omntDir)
            *jsFound = JSI_ZIP_INDEX;
    }
#endif
    return JSI_OK;
}

#define FN_send JSI_INFO("\
Add messages to queue to be processed by the 'recvCmd' interp option.")

static int InterpSendCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    //return SendCmd(interp, args, _this, ret, funcPtr, 1);
    InterpObj *udf = Jsi_UserObjGetData(interp, _this, funcPtr);
    Jsi_Interp *sinterp = NULL;
    int isthrd;
    if (udf) {
        sinterp = udf->subinterp;
    } else {
        sinterp = interp->parent;
        if (!sinterp) {
            Jsi_LogError("Apply Interp.send in a non-subinterp");
            return JSI_ERROR;
        }
    }

    if (Jsi_InterpGone(sinterp)) {
        Jsi_LogError("Sub-interp gone");
        return JSI_ERROR;
    }
    isthrd = (interp->threadId != sinterp->threadId);
    if (!sinterp->recvCmd) {
        Jsi_LogError("interp was not created with 'recvCmd' option");
        return JSI_ERROR;
    }

    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);
    char *cp = Jsi_ValueString(interp, arg, NULL);

    /* Post to thread event in sub-interps queue. */
    if (Jsi_MutexLock(interp, sinterp->QMutex) != JSI_OK)
        return JSI_ERROR;
        
    int slen = Jsi_Strlen(Jsi_DSValue(&sinterp->interpMsgQ));
    Jsi_DString eStr = {};
    if (!cp) {
        cp = (char*)Jsi_ValueGetDString(interp, arg, &eStr, JSI_OUTPUT_JSON);
    }
    if (!slen)
        Jsi_DSAppend(&sinterp->interpMsgQ, "[", cp, "]", NULL);
    else {
        Jsi_DSSetLength(&sinterp->interpMsgQ, slen-1);
        Jsi_DSAppend(&sinterp->interpMsgQ, ", ", cp, "]", NULL);
    }
    // if (interp->parent) printf("SENDING: %s\n", Jsi_DSValue(&sinterp->interpMsgQ));
    Jsi_DSFree(&eStr);

    Jsi_MutexUnlock(interp, sinterp->QMutex);
    if (!isthrd) {
        if (SubInterpEvalCallback(sinterp, NULL) != JSI_OK)
            sinterp->threadErrCnt++;
    }
    return JSI_OK;

}

#ifndef JSI_OMIT_THREADS
#ifndef __WIN32
static void *NewInterpThread(void* iPtr)
#else
static void NewInterpThread(void* iPtr)
#endif
{
    int rc = JSI_OK;
    InterpObj *udf = iPtr;
    Jsi_Interp *interp = udf->subinterp;
    interp->threadId = Jsi_CurrentThread();
   if (interp->scriptStr)
        rc = Jsi_EvalString(interp, interp->scriptStr, 0);
    else if (interp->scriptStr)
        rc = Jsi_EvalFile(interp, interp->scriptFile, 0);
    else {
        jsi_AddEventHandler(interp); 
        int mrc = Jsi_MutexLock(interp, interp->Mutex);      
        while (mrc == JSI_OK) {
            if (Jsi_EventProcess(interp, -1)<0)
                break;
            Jsi_Sleep(interp, 1);
        }
    }
    if (rc != JSI_OK) {
        Jsi_LogError("eval failure");
        interp->threadErrCnt++;
    }
    /* TODO: should we wait/notify parent??? */
    interpObjErase(udf);
#ifndef __WIN32
    return NULL;
#endif
}
#endif

#define FN_interp JSI_INFO("\
The new interp may optionally be threaded.")
static int InterpConstructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *toacc = NULL;
    if (Jsi_FunctionIsConstructor(funcPtr)) {
        toacc = _this;
    } else {
        Jsi_Obj *o = Jsi_ObjNew(interp);
        Jsi_PrototypeObjSet(interp, "Interp", o);
        Jsi_ValueMakeObject(interp, *ret, o);
        toacc = *ret;
    }

    InterpObj *cmdPtr = Jsi_Calloc(1,sizeof(*cmdPtr));
    SIGINIT(cmdPtr,INTERPOBJ);
    cmdPtr->parent = interp;

    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);

    if (!(cmdPtr->subinterp = Jsi_InterpCreate(interp, 0,0, arg))) {
        Jsi_Free(cmdPtr);
        return JSI_ERROR;
    }
    Jsi_Interp *sinterp = cmdPtr->subinterp;
    if (sinterp->scriptStr != 0 && sinterp->scriptFile != 0) {
        Jsi_LogError("can not use both scriptStr and scriptFile options");
        goto bail;
    }

    Jsi_Obj *fobj = Jsi_ValueGetObj(interp, toacc);
    if ((cmdPtr->objId = Jsi_UserObjNew(interp, &interpobject, fobj, cmdPtr))<0)
        goto bail;
    cmdPtr->fobj = fobj;
    cmdPtr->aliases = Jsi_HashNew(interp, JSI_KEYS_STRING, AliasFree);
#ifndef JSI_OMIT_THREADS
    if (sinterp->subthread) {
       /* if (sinterp->scriptStr == 0 && sinterp->scriptFile == 0) {
            Jsi_LogError("must give scriptStr or scriptFile option with thread");
            goto bail;
        }*/
#ifdef __WIN32
        if (!_beginthread( NewInterpThread, 0, cmdPtr )) {
            Jsi_LogError("thread create failed");
            goto bail;
        }
#else
        pthread_t nthread;
        if (pthread_create(&nthread, NULL, NewInterpThread, cmdPtr) != 0) {
            Jsi_LogError("thread create failed");
            goto bail;
        }
#endif
#else
    if (0) {
#endif
    } else {
        int rc = JSI_OK;
        if (sinterp->scriptStr != 0) {
            rc = Jsi_EvalString(sinterp, sinterp->scriptStr, 0);
        } else if (sinterp->scriptFile != 0) {
            rc = Jsi_EvalFile(sinterp, sinterp->scriptFile, 0);        
        }
        if (rc != JSI_OK)
            goto bail;
    }
    return JSI_OK;
    
bail:
    interpObjErase(cmdPtr);
    return JSI_ERROR;
}

static Jsi_CmdSpec interpCmds[] = {
    { "Interp", InterpConstructor,0,  1,  "?options?", JSI_CMD_IS_CONSTRUCTOR, .help="Create a new interp", .info=FN_interp, .opts=InterpOptions },
    { "alias",  InterpAliasCmd,   0,  3, "?name,func,args?",.help="Set/get alias command in the interp"},
    { "conf",   InterpConfCmd,    0,  1, "?string|options?",.help="Configure options" , .opts=InterpOptions},
    { "send",   InterpSendCmd,    1,  1, "msg", .help="Send message to enqueue on subinterps recvCmd handler", .info=FN_send },
    { "eval",   InterpEvalCmd,    1,  2, "js?,async?", .help="Interpet javascript code within subinterp", .info=FN_eval },
    { "info",   InterpInfoCmd,    0,  0,  "", .help="Return detailed info about interp" },
    { "call",   InterpCallCmd,    2,  3, "funcName,args?,async?", .help="Call named function in subinterp", .info=FN_call },
    { NULL,     NULL, .help="Commands for accessing interps" }
};
/*
    { "alias",      InterpAliasCmd,     NULL,   3,  3, "interp,funcName,function",  },
    { "aliases",    InterpAliasesCmd,   NULL,   1,  2, "interp?,funcName?",  },
    { "children",   InterpChildrenCmd,  NULL,   0,  0, "",  },
    { "create",     InterpCreateCmd,    NULL,   0,  1, "?isSafe?",  },
    { "delete",     InterpDeleteCmd,    NULL,   1,  1, "interp",  },
    { "exists",     InterpExistsCmd,    NULL,   1,  1, "interp",  },
    { "eval",       InterpEvalCmd,      NULL,   2,  2, "interp,string",  },
    { "limit",      InterpLimitCmd,     NULL,   1,  2, "interp?,options?",  },
*/

Jsi_Value *Jsi_ReturnValue(Jsi_Interp *interp) {
    return &interp->ret;
}

int jsi_InterpInit(Jsi_Interp *interp)
{
    Jsi_Hash *isys;
    if (!(isys = Jsi_UserObjRegister(interp, &interpobject)))
        Jsi_LogFatal("Can not init interp\n");

    Jsi_CommandCreateSpecs(interp, interpobject.name, interpCmds, isys, 0);
    return JSI_OK;
}

int Jsi_SafeAccess(Jsi_Interp *interp, Jsi_Value* file, int toWrite)
{
    Jsi_Value *v, *dirs = (toWrite ? interp->safeWriteDirs : interp->safeReadDirs);
    if (!interp->isSafe)
        return JSI_OK;
    if (!dirs)
        return JSI_ERROR;
    int i, n, m, argc = Jsi_ValueGetLength(interp, dirs);
    char *str, *dstr = Jsi_ValueString(interp, file, &n); /* TODO: normalize? */
    if (!dstr)
        return JSI_ERROR;
    char *scp = strrchr(dstr, '/');
    if (scp)
        n -= strlen(scp);
    for (i=0; i<argc; i++) {
        v = Jsi_ValueArrayIndex(interp, dirs, i);
        str = Jsi_ValueString(interp, v, &m);
        if (v && str && strncmp(str, dstr, m) == 0 && (n==m || dstr[m] == '/'))
            return JSI_OK;
    }
    return JSI_ERROR;
}

#endif
