#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

/* Return value from call to function will is not used. */
int Jsi_FunctionReturnIgnored(Jsi_Interp *interp, Jsi_Func *funcPtr) {
    return funcPtr->callflags.bits.isdiscard;
}

int Jsi_FunctionIsConstructor(Jsi_Func *funcPtr)
{
    return (funcPtr->f.bits.iscons);
}

Jsi_CmdSpec *Jsi_FunctionGetSpecs(Jsi_Func *funcPtr)
{
    return funcPtr->cmdSpec;
}

void *Jsi_FunctionPrivData(Jsi_Func *funcPtr)
{
    return funcPtr->privData;
}

/* TODO: if not in a file (an eval) save copy of body string from pstate->lexer??? */
Jsi_Func *jsi_FuncMake(jsi_Pstate *pstate, Jsi_ScopeStrs *args, struct OpCodes *ops, jsi_Pline* line, char *name)
{
    Jsi_Interp *interp = pstate->interp;
    Jsi_ScopeStrs *localvar = jsi_ScopeGetVarlist(pstate);
    Jsi_Func *f = Jsi_Calloc(1, sizeof(*f));
    SIGINIT(f,FUNC);
    f->type = FC_NORMAL;
    f->opcodes = ops;
    f->argnames = args;
    f->localnames = localvar;
    f->script = interp->curFile;
    f->bodyline = *line;
    if (name)
        f->name = Jsi_KeyAdd(interp, name);
    return f;
}

char **Jsi_FunctionArguments(Jsi_Interp *interp, Jsi_Value *func, int *argcPtr)
{
    Jsi_FuncObj *funcPtr;
    Jsi_Func *f;
    if (!Jsi_ValueIsFunction(interp, func))
        return NULL;
    funcPtr = func->d.obj->d.fobj;
    f = funcPtr->func;
    SIGASSERT(f, FUNC);
    *argcPtr = f->argnames->count;
    return f->argnames->strings;
}

char **Jsi_FunctionLocals(Jsi_Interp *interp, Jsi_Value *func, int *argcPtr)
{
    Jsi_FuncObj *funcPtr;
    Jsi_Func *f;
    if (!Jsi_ValueIsFunction(interp, func))
        return NULL;
    funcPtr = func->d.obj->d.fobj;
    f = funcPtr->func;
    SIGASSERT(f, FUNC);
    *argcPtr = f->localnames->count;
    return f->localnames->strings;
}

void jsi_InitLocalVar(Jsi_Interp *interp, Jsi_Value *arguments, Jsi_Func *who)
{
    SIGASSERT(who, FUNC);
    if (who->localnames) {
        int i;
        Jsi_Value key = VALINIT;
        for (i = 0; i < who->localnames->count; ++i) {
            const char *argkey = jsi_ScopeStrsGet(who->localnames, i);
            if (argkey) {
                Jsi_ValueMakeStringKey(interp, &key, argkey);
                jsi_ValueObjKeyAssign(interp, arguments, &key, NULL, JSI_OM_DONTENUM);
            }
        }
    }
}

int Jsi_FuncObjToString(Jsi_Interp *interp, Jsi_Obj *o, Jsi_DString *dStr)
{
    if (o->ot != JSI_OT_FUNCTION)
        return JSI_ERROR;
    Jsi_Func *f = o->d.fobj->func;
    if (f->type == FC_NORMAL) {
        Jsi_DSAppend(dStr, "function ", f->name?f->name:"", "(", NULL);
        int i;
        for (i = 0; i < f->argnames->count; ++i) {
            if (i) Jsi_DSAppend(dStr, ",", NULL);
            Jsi_DSAppend(dStr,  jsi_ScopeStrsGet(f->argnames, i), NULL);
        }
        Jsi_DSAppend(dStr, ") {...}", NULL);
    } else {
        Jsi_DSAppend(dStr, "function ", f->name?f->name:"", "() { [native code] }", NULL);
    }
    return JSI_OK;
}

Jsi_Value *jsi_MakeFuncValue(Jsi_Interp *interp, Jsi_CmdProc *callback, const char *name)
{
    Jsi_Obj *o = Jsi_ObjNew(interp);
    Jsi_Func *f = Jsi_Calloc(1,sizeof(*f));
    SIGINIT(f,FUNC);
#ifdef USE_VALCOPY
    o->refcnt = 1;
#endif
    o->ot = JSI_OT_FUNCTION;
    f->type = FC_BUILDIN;
    f->callback = callback;
    f->privData = NULL;
    o->d.fobj = jsi_FuncObjNew(interp, f);
    f->cmdSpec = Jsi_Calloc(1,sizeof(Jsi_CmdSpec));
    Jsi_HashSet(interp->genDataTbl, f->cmdSpec, f->cmdSpec);
    f->cmdSpec->maxArgs = -1;
    if (name)
        f->cmdSpec->name = (char*)Jsi_KeyAdd(interp, name);
    f->script = interp->curFile;
    f->callback = callback;
    return Jsi_ValueMakeObject(interp,NULL, o);
}

Jsi_Value *jsi_MakeFuncValueSpec(Jsi_Interp *interp, Jsi_CmdSpec *cmdSpec, void *privData)
{
    Jsi_Obj *o = Jsi_ObjNew(interp);
    Jsi_Func *f = Jsi_Calloc(1,sizeof(*f));
    o->ot = JSI_OT_FUNCTION;
    SIGINIT(f,FUNC);
    f->type = FC_BUILDIN;
    f->cmdSpec = cmdSpec;
    f->callback = cmdSpec->proc;
    f->privData = privData;
    f->f.flags = (cmdSpec->flags & JSI_CMD_MASK);
    f->script = interp->curFile;
    o->d.fobj = jsi_FuncObjNew(interp, f);
    return Jsi_ValueMakeObject(interp, NULL, o);
}


/* Call a function with args: args and/or ret can be NULL. */
int Jsi_FunctionInvoke(Jsi_Interp *interp, Jsi_Value *func, Jsi_Value *args, Jsi_Value **ret, Jsi_Value *_this)
{
    if (interp->deleting)
        return JSI_ERROR;
    if (!Jsi_ValueIsFunction(interp, func)) {
        Jsi_LogError("can not execute expression, expression is not a function\n");
        return JSI_ERROR;
    }
    if (!func->d.obj->d.fobj) {   /* empty function */
        return JSI_OK;
    }
    if (!ret) {
        if (!interp->nullFuncRet) {
            interp->nullFuncRet = Jsi_ValueNew(interp);
            Jsi_IncrRefCount(interp, interp->nullFuncRet);
        }
        *ret = interp->nullFuncRet;
        Jsi_ValueMakeUndef(interp, *ret);
    }
    if (!args) {
        if (!interp->nullFuncArg) {
            interp->nullFuncArg = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, NULL, 0));
            Jsi_IncrRefCount(interp, interp->nullFuncArg);
        }
        args = interp->nullFuncArg;
    }
    /* func to call */
    Jsi_Func *fstatic = func->d.obj->d.fobj->func;
    SIGASSERT(fstatic, FUNC);

    /* new this */
    Jsi_Value newthis = VALINIT;
    if (!_this) {
        Jsi_ValueCopy(interp, &newthis, func);
        Jsi_ValueToObject(interp, &newthis);
    } else {
        Jsi_ValueCopy(interp, &newthis, _this);
    }
    
    /* prepare args */
    if (args->vt != JSI_VT_OBJECT || !Jsi_ObjIsArray(interp, args->d.obj)) {
        Jsi_LogError("argument must be an array\n");
        return JSI_ERROR;
    }
    jsi_SharedArgs(interp, args, fstatic->argnames, 1);
    jsi_InitLocalVar(interp, args, fstatic);
    jsi_SetCallee(interp, args, func);

    int res = JSI_OK;
    Jsi_IncrRefCount(interp, args);
    if (fstatic->type == FC_NORMAL) {
        res = jsi_evalcode(interp->ps, fstatic->opcodes, func->d.obj->d.fobj->scope, 
            args, &newthis, ret);
    } else {
        res = fstatic->callback(interp, args, &newthis, ret, fstatic);
    }
    jsi_SharedArgs(interp, args, fstatic->argnames, 0);
    Jsi_DecrRefCount(interp, args);
    Jsi_ValueReset(interp,&newthis);
    return res;
}

/* Special case: Call function with a single argument.  Return 1 if returns true. */
int Jsi_FunctionInvokeBool(Jsi_Interp *interp, Jsi_Value *func, Jsi_Value *arg)
{
    Jsi_Value *vpargs, fret = VALINIT, *frPtr = &fret;
    int rc, bres = 0;
    if (interp->deleting)
        return JSI_ERROR;
    if (!arg) {
        if (!interp->nullFuncArg) {
            interp->nullFuncArg = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, NULL, 0));
            Jsi_IncrRefCount(interp, interp->nullFuncArg);
        }
        vpargs = interp->nullFuncArg;
    } else {
        //Jsi_ValueCopy(interp, items[0], arg);
        vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, &arg, 1));
    }
    Jsi_IncrRefCount(interp, vpargs);
    rc = Jsi_FunctionInvoke(interp, func, vpargs, &frPtr, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    if (rc == JSI_OK)
        bres = Jsi_ValueIsTrue(interp, &fret);
    else
        Jsi_LogError("function call failed");
    Jsi_ValueMakeUndef(interp, &fret);
    return bres;
}
       

Jsi_FuncObj *jsi_FuncObjNew(Jsi_Interp *interp, Jsi_Func *func)
{
    Jsi_FuncObj *f = Jsi_Calloc(1,sizeof(Jsi_FuncObj));
    f->interp = interp;
    SIGINIT(f,FUNCOBJ);
    f->func = func;
    return f;
}

void jsi_FuncObjFree(Jsi_FuncObj *fobj)
{
    if (fobj->scope)
        jsi_ScopeChainFree(fobj->interp, fobj->scope);
    if (fobj->func->bindArgs)
        Jsi_DecrRefCount(fobj->interp, fobj->func->bindArgs);
    //Jsi_Free(fobj->func);
    MEMCLEAR(fobj);
    Jsi_Free(fobj);
}

#endif
