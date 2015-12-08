#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

#define MAX_SCOPE   (BUFSIZ/2)

Jsi_ScopeStrs *jsi_ScopeStrsNew(struct jsi_Pstate *ps)
{
    Jsi_ScopeStrs *ret = Jsi_Calloc(1, sizeof(*ret));
    return ret;
}

void jsi_ScopeStrsPush(struct jsi_Pstate *ps, Jsi_ScopeStrs *ss, const char *string)
{
    if (ss->count >= ss->_size) {
        ss->_size += 5;
        ss->strings = Jsi_Realloc(ss->strings, (ss->_size) * sizeof(char *));
    }
    ss->strings[ss->count] = (char*)Jsi_KeyAdd(ps->interp, string);
    ss->count++;
}

static Jsi_ScopeStrs *strs_dup(struct jsi_Pstate *ps, Jsi_ScopeStrs *ss)
{
    Jsi_ScopeStrs *n = jsi_ScopeStrsNew(ps);
    int i;
    if (!ss) return n;
    for (i = 0; i < ss->count; ++i) {
        jsi_ScopeStrsPush(ps, n, ss->strings[i]);
    }
    return n;
}

const char *jsi_ScopeStrsGet(Jsi_ScopeStrs *ss, int i)
{
    if (i < 0 || i >= ss->count) return NULL;
    return ss->strings[i];
}

void jsi_ScopeStrsFree(Jsi_ScopeStrs *ss)
{
    if (!ss) return;
    Jsi_Free(ss->strings);
    Jsi_Free(ss);
}

/* lexical scope */
static Jsi_ScopeStrs *scopes[MAX_SCOPE];
static int cur_scope;

void jsi_ScopePush(struct jsi_Pstate *ps)
{
    Jsi_Interp *interp = ps->interp;
    if (cur_scope >= MAX_SCOPE - 1) Jsi_LogBug("Scope chain to short\n");
    cur_scope++;
}

void jsi_ScopePop(struct jsi_Pstate *ps)
{
    Jsi_Interp *interp = ps->interp;
    if (cur_scope <= 0) Jsi_LogBug("No more scope to pop\n");
    jsi_ScopeStrsFree(scopes[cur_scope]);
    scopes[cur_scope] = NULL;
    cur_scope--;
}

void jsi_ScopeAddVar(jsi_Pstate *ps, const char *str)
{
    int i;
    if (scopes[cur_scope] == NULL) scopes[cur_scope] = jsi_ScopeStrsNew(ps);
    
    for (i = 0; i < scopes[cur_scope]->count; ++i) {
        if (Jsi_Strcmp(str, scopes[cur_scope]->strings[i]) == 0) return;
    }
    jsi_ScopeStrsPush(ps, scopes[cur_scope], str);
}

Jsi_ScopeStrs *jsi_ScopeGetVarlist(jsi_Pstate *ps)
{
    return strs_dup(ps, scopes[cur_scope]);
}


static int fastVarFree(Jsi_Interp *interp, void *ptr) {
    FastVar *fv = ptr;
    Jsi_Value *v = fv->var.lval;
    if (v) {
        //printf("FV FREE: %p (%d/%d)\n", fv, v->refCnt, v->vt == JSI_VT_OBJECT?v->d.obj->refcnt:-99);
        //Jsi_DecrRefCount(interp, v);
    }
    return JSI_OK;
}

jsi_Pstate *jsi_PstateNew(Jsi_Interp *interp)
{
    Jsi_Value sval = VALINITS;
    jsi_Pstate *ps = Jsi_Calloc(1,sizeof(*ps));
    SIGINIT(ps,PARSER);
    ps->lexer = Jsi_Calloc(1,sizeof(*ps->lexer));
    ps->lexer->pstate = ps;
    ps->interp = interp;
    ps->last_exception = sval;
    ps->argsTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, jsi_ArglistFree);
    ps->fastVarTbl = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, fastVarFree);
    ps->strTbl = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL);
    return ps;
}

const char *jsi_PstateGetFilename(jsi_Pstate *ps)
{
    Jsi_Interp *interp = ps->interp;
    return interp->curFile;
}

void jsi_PstateClear(jsi_Pstate *ps)
{
    Lexer* l = ps->lexer;
    if (l->ltype == LT_FILE)
    {
        if (l->d.fp)
            Jsi_Close(l->d.fp);
        l->d.fp = NULL;
    }
    if (l->ltype == LT_STRING)
    {
        l->d.str = NULL;
    }
    l->ltype = LT_NONE;
    l->last_token = 0;
    l->cur_line = 1;
    l->cur_char = 0;
    l->cur = 0;
    ps->err_count = 0;
}

int jsi_PstateSetFile(jsi_Pstate *ps, Jsi_Channel fp, int skipbang)
{
    Lexer *l = ps->lexer;
    jsi_PstateClear(ps);
    l->ltype = LT_FILE;
    l->d.fp = fp;
    Jsi_Rewind(fp);
    if (skipbang) {
        char buf[1000];
        if (Jsi_Gets(fp, buf, 1000) && (buf[0] != '#' || buf[1] != '!')) {
            Jsi_Rewind(fp);
        }
    }
            
    return JSI_OK;
}


int jsi_PstateSetString(jsi_Pstate *ps, const char *str)
{
    Jsi_Interp *interp = ps->interp;
    Lexer *l = ps->lexer;
    jsi_PstateClear(ps);
    l->ltype = LT_STRING;
    Jsi_HashEntry *hPtr = Jsi_HashEntryCreate(interp->codeTbl, (void*)str, NULL);
    assert(hPtr);
    l->d.str = Jsi_HashKeyGet(hPtr);
    return JSI_OK;
}

void jsi_PstateFree(jsi_Pstate *ps)
{
    /* TODO: when do we free opcodes */
    jsi_PstateClear(ps);
    Jsi_Free(ps->lexer);
    if (ps->opcodes)
        jsi_FreeOpcodes(ps->opcodes);
    if (ps->hPtr)
        Jsi_HashEntryDelete(ps->hPtr);
    Jsi_HashDelete(ps->argsTbl);
    Jsi_HashDelete(ps->strTbl);
    Jsi_HashDelete(ps->fastVarTbl);
    MEMCLEAR(ps);
    Jsi_Free(ps);
}

#endif
