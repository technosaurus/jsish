#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif
#include <errno.h>
#include <sys/time.h>

#ifdef HAVE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

void* Jsi_Realloc(void *m,unsigned int size) { return realloc(m,size); }
void* Jsi_Malloc(unsigned int size) { return malloc(size); }
void* Jsi_Calloc(unsigned int n,unsigned int size) { return calloc(n,size); }
void  Jsi_Free(void *n) { free(n); }

/* Get time in milliseconds since Jan 1, 1970 */
Jsi_Wide Jsi_DateTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    Jsi_Wide num = ((Jsi_Wide)tv.tv_sec*1000LL + (Jsi_Number)tv.tv_usec/1000LL);
    return num;
}


int jsi_fatalexit = JSI_LOG_FATAL;

static void (*logHook)(const char *buf, va_list va) = NULL;

void Jsi_LogMsg(Jsi_Interp *interp, int code, const char *format,...) {
    va_list va;
    va_start (va, format);
    char pbuf[BUFSIZ/8] = "";
    char buf[BUFSIZ], *mt = "", *term = "", *pterm=pbuf;
    const char *curFile = NULL;
    if (interp)
        curFile = (interp->curIp && interp->curIp->fname? interp->curIp->fname:interp->curFile);
    if (!curFile) curFile = "";
    /* Filter out try/catch (TODO: and non-syntax errors??). */
    if (code == JSI_LOG_PARSE || interp == NULL) {
        if (logHook)
            (*logHook)(format, va);
        else
            vfprintf(stderr, format, va);
        va_end(va);
        return;
    }
    if (code == JSI_LOG_ERROR && (interp->tryDepth-interp->withDepth)>0 && interp->inParse<=0 
        /*&& !interp->errMsgBuf[0]*/) { /* TODO: should only do the first or traceback? */
        vsnprintf(interp->errMsgBuf, sizeof(interp->errMsgBuf), format, va);
        interp->errFile = curFile;
        interp->errLine = interp->curIp->line;
        va_end(va);
        return;
    }
    switch (code) {
        case JSI_LOG_FATAL: mt = "fatal"; break;
        case JSI_LOG_ERROR: mt = "error"; break;
        case JSI_LOG_PARSE: mt = "parse"; break;
        case JSI_LOG_WARN:  mt = "warning"; break;
        case JSI_LOG_INFO: mt = "info"; break;
        case JSI_LOG_BUG: mt = "bug"; break;
        case JSI_LOG_TODO: mt = "todo"; break;
    } 
    if (!strchr(format,'\n')) term = "\n";
    if (interp && interp->lastPushStr && interp->lastPushStr[0]) {
        char *ss = interp->lastPushStr;
        char psbuf[BUFSIZ/6];
        if (strchr(ss,'%')) {
            char *s = ss, *sd = psbuf;
            int plen=0, llen = sizeof(psbuf)-2;
            while (*s && plen<llen) {
                if (*s == '%')
                    sd[plen++] = '%';
                sd[plen++] = *s;
                s++;
            }
            sd[plen] = 0;
            ss = psbuf;
        }
        while (*ss && isspace(*ss))
            ss++;
        if (*ss)
            snprintf(pbuf, sizeof(pbuf), "    (at or near \"%s\")\n", ss);
    }
    pbuf[sizeof(pbuf)-1] = 0;
    if (interp && interp->curIp && curFile && strchr(curFile,'%')==0 && interp->curIp->line)
        snprintf(buf, sizeof(buf), "%s:%d: %s: %s%s%s",  curFile, interp->curIp->line, mt,format, pterm, term);
    else
        snprintf(buf, sizeof(buf), "%s: %s%s%s", mt, format, pterm, term);
    buf[sizeof(buf)-1]=0;

    if (logHook)
        (*logHook)(buf, va);
    else
        vfprintf(stderr, buf, va);
    va_end(va);
    if (code & jsi_fatalexit)
        exit(1);
}

#ifndef JSI_LITE_ONLY

extern int Jsi_PkgProvide(Jsi_Interp *interp, const char *name, const char *version)
{
    return JSI_ERROR;
}

extern int Jsi_PkgRequire(Jsi_Interp *interp, const char *name, const char *version)
{
    return JSI_ERROR;
}

Jsi_Value *Jsi_VarLookup(Jsi_Interp *interp, const char *varname)
{
    Jsi_Value *v;
    v = Jsi_ValueObjLookup(interp, interp->incsc, (char*)varname, 0);
    if (!v)
        v = jsi_ScopeChainObjLookupUni(interp->ingsc, (char*)varname);
    return v;
}

static char *FindEndB(char *cp) {
    
    if (*cp == '\"'||*cp == '\'') {
        char endc = *cp;
        cp++;
        while (*cp && *cp != endc) {
            if (*cp == '\\' && cp[1]) cp++;
            cp++;
        }
        if (*cp == endc)
            cp++;
        if (*cp != ']')
            return NULL;
        return cp;
    } else
        return strchr(cp, ']');
}

/* Lookup a name, eg.  "a[b].c  a.b.c  a[b][c]  a.b[c]  a["b"].c  a[1].c  */
Jsi_Value *Jsi_NameLookup(Jsi_Interp *interp, const char *name)
{
    int cnt = 0, len, isq;
    char *nam = (char*)name, *cp, *cp2, *ocp, *kstr;
    Jsi_Value *v = NULL, tv = VALINIT, nv = VALINIT, key = VALINIT;
    Jsi_DString dStr = {};
    cp2 = strchr(nam,'[');
    cp = strchr(nam, '.');
    if (cp2 && (cp==0 || cp2<cp))
        cp = cp2;
    if (!cp)
        return Jsi_VarLookup(interp, nam);
    Jsi_DSSetLength(&dStr, 0);
    Jsi_DSAppendLen(&dStr, nam, cp-nam);
    v = Jsi_VarLookup(interp, Jsi_DSValue(&dStr));
    if (!v)
        goto bail;
    while (cnt++ < 1000) {
        ocp = cp;
        nam = cp+1;
        isq = 0;
        if (*cp == '[') {
            cp = FindEndB(cp+1); /* handle [] in strings. */
            if (!cp) goto bail;
            len = cp-nam;
            cp++;
            if (len>=2 && ((nam[0] == '\"' && nam[len-1] == '\"') || (nam[0] == '\'' && nam[len-1] == '\''))) {
                nam += 1;
                len -= 2;
                isq = 1;
            }
        } else if (*cp == '.') {
            cp2 = strchr(nam,'[');
            cp = strchr(nam, '.');
            if (cp2 && (cp==0 || cp2<cp))
                cp = cp2;
            len = (cp ? cp-nam : strlen(nam));
        } else {
            goto bail;
        }
        Jsi_DSSetLength(&dStr, 0);
        Jsi_DSAppendLen(&dStr, nam, len);
        kstr = Jsi_DSValue(&dStr);
        if (*ocp == '[' && isq == 0 && isdigit(kstr[0]) && Jsi_ValueIsArray(interp, v)) {
            int nn;
            if (Jsi_GetInt(interp, kstr, &nn, 0) != JSI_OK)
                goto bail;
            v = Jsi_ValueArrayIndex(interp, v, nn);
            if (!v)
                goto bail;
        } else if (*ocp == '[' && isq == 0) {
            Jsi_Value *kv = Jsi_VarLookup(interp, kstr);
            if (!kv)
                goto bail;
            jsi_ValueSubscriptLen(interp, v, kv, &nv, 1);
            goto keyon;
        } else {
            Jsi_ValueMakeStringKey(interp, &key, kstr); 
            jsi_ValueSubscriptLen(interp, v, &key, &nv, 1);
keyon:
            if (nv.vt == JSI_VT_UNDEF)
                goto bail;
            else {
                tv = nv;
                v = &tv;
            }
        }
        if (cp == 0 || *cp == 0) break;
    }
    //Jsi_ValueReset(interp, &ret);
    Jsi_DSFree(&dStr);
    if (v && v == &tv) {
        v = Jsi_ValueNew(interp);
        *v = tv;
    }
    return v;
bail:
    Jsi_DSFree(&dStr);
    return NULL;
}

Jsi_Value *jsi_GlobalContext(Jsi_Interp *interp)
{
    return interp->csc;
}

typedef struct {
    Jsi_DString *dStr;
    int quote; /* Set to JSI_OUTPUT_JSON, etc*/
    int depth;
} objwalker;

static int IsAlnum(const char *cp)
{
    while (*cp)
        if (isalnum(*cp) || *cp == '_')
            cp++;
        else
            return 0;
    return 1;
}

static void jsiValueGetString(Jsi_Interp *interp, Jsi_Value* v, Jsi_DString *dStr, objwalker *owPtr);

static int IsKeyword(Jsi_Interp *interp, char *str) {
    return (Jsi_HashEntryFind(interp->lexkeyTbl, str) != 0);
}

static int _object_get_callback(Jsi_Tree *tree, Jsi_TreeEntry *hPtr, void *data)
{
    Jsi_Value *v;
    objwalker *ow = data;
    Jsi_DString *dStr = ow->dStr;
    int len;
    char *str;
    if ((hPtr->f.bits.dontenum))
        return JSI_OK;
    v = Jsi_TreeValueGet(hPtr);
    if ((ow->quote&JSI_OUTPUT_JSON) && v && v->vt == JSI_VT_UNDEF)
        return JSI_OK;
    str = Jsi_TreeKeyGet(hPtr);
    char *cp = Jsi_DSValue(dStr);
    len = Jsi_DSLength(dStr);
    if (len>=2 && cp[len-2] != '{')
        Jsi_DSAppend(dStr, ", ", NULL);
    if (((ow->quote&JSI_OUTPUT_JSON) == 0 || (ow->quote&JSI_JSON_STRICT) == 0) && IsAlnum(str) && !IsKeyword(tree->interp, str))
        Jsi_DSAppend(dStr, str, NULL);
    else
        /* JSON/spaces, etc requires quoting the name. */
        Jsi_DSAppend(dStr, "\"", str, "\"", NULL);
    Jsi_DSAppend(dStr, ":", NULL);
    ow->depth++;
    jsiValueGetString(tree->interp, v, dStr, ow);
    ow->depth--;
    return JSI_OK;
}

/* Format value into dStr.  Toplevel caller does init/free. */
static void jsiValueGetString(Jsi_Interp *interp, Jsi_Value* v, Jsi_DString *dStr, objwalker *owPtr)
{
    char buf[100], *str;
    if (owPtr->depth > interp->maxDepth) {
        Jsi_LogError("recursive ToString");
        return;
    }
    int quote = owPtr->quote;
    int isjson = owPtr->quote&JSI_OUTPUT_JSON;
    double num;
    switch(v->vt) {
        case JSI_VT_UNDEF:
            Jsi_DSAppend(dStr, "undefined", NULL);
            return;
        case JSI_VT_NULL:
            Jsi_DSAppend(dStr, "null", NULL);
            return;
        case JSI_VT_VARIABLE:
            Jsi_DSAppend(dStr, "variable", NULL);
            return;
        case JSI_VT_BOOL:
            Jsi_DSAppend(dStr, (v->d.val ? "true":"false"), NULL);
            return;
        case JSI_VT_NUMBER:
            num = v->d.num;
outnum:
            if (jsi_is_integer(num)) {
                sprintf(buf, "%d", (int)num);
                Jsi_DSAppend(dStr, buf, NULL);
            } else if (jsi_is_wide(num)) {
                sprintf(buf, "%Ld", (Jsi_Wide)num);
                Jsi_DSAppend(dStr, buf, NULL);
            } else if (jsi_ieee_isnormal(num)) {
                sprintf(buf, "%" JSI_NUMGFMT, num);
                Jsi_DSAppend(dStr, buf, NULL);
            } else if (jsi_ieee_isnan(num)) {
                Jsi_DSAppend(dStr, "NaN", NULL);
            } else {
                int s = jsi_ieee_infinity(num);
                if (s > 0) Jsi_DSAppend(dStr, "+Infinity", NULL);
                else if (s < 0) Jsi_DSAppend(dStr, "-Infinity", NULL);
                else Jsi_LogBug("Ieee function problem");
            }
            return;
        case JSI_VT_STRING:
            str = v->d.s.str;
outstr:
            if (!quote) {
                Jsi_DSAppend(dStr, str, NULL);
                return;
            }
            Jsi_DSAppend(dStr,"\"", NULL);
            while (*str) {
                if ((*str == '\'' && (!(isjson))) || *str == '\"' ||
                    (*str == '\n' && (!(owPtr->quote&JSI_OUTPUT_NEWLINES))) || *str == '\r' || *str == '\t' || *str == '\f' || *str == '\b'  ) {
                    char pcp[2];
                    *pcp = *str;
                    pcp[1] = 0;
                    Jsi_DSAppendLen(dStr,"\\", 1);
                    switch (*str) {
                        case '\r': *pcp = 'r'; break;
                        case '\n': *pcp = 'n'; break;
                        case '\t': *pcp = 't'; break;
                        case '\f': *pcp = 'f'; break;
                        case '\b': *pcp = 'b'; break;
                    }
                    Jsi_DSAppendLen(dStr,pcp, 1);
                } else if (!isprint(*str))
                    /* TODO: encode */
                    if (isjson) {
                        char ubuf[10];
                        sprintf(ubuf, "\\u00%.02x", (unsigned char)*str);
                        Jsi_DSAppend(dStr,ubuf, NULL);
                        //Jsi_DSAppendLen(dStr,".", 1);
                    } else
                        Jsi_DSAppendLen(dStr,str, 1);
                    
                else
                    Jsi_DSAppendLen(dStr,str, 1);
                str++;
            }
            Jsi_DSAppend(dStr,"\"", NULL);
            return;
        case JSI_VT_OBJECT: {
            Jsi_Obj *o = v->d.obj;
            switch(o->ot) {
                case JSI_OT_BOOL:
                    Jsi_DSAppend(dStr, (o->d.val ? "true":"false"), NULL);
                    return;
                case JSI_OT_NUMBER:
                    num = o->d.num;
                    goto outnum;
                    return;
                case JSI_OT_STRING:
                    str = o->d.s.str;
                    goto outstr;
                    return;
                case JSI_OT_FUNCTION: {
                    Jsi_FuncObjToString(interp, o, dStr);
                    return;
                }
                case JSI_OT_REGEXP:
                    Jsi_DSAppend(dStr, o->d.robj->pattern, NULL);
                    return;
                case JSI_OT_USEROBJ: 
                    jsi_UserObjToName(interp, o->d.uobj, dStr);
                    return;
                case JSI_OT_ITER:
                    Jsi_DSAppend(dStr, "*ITER*", NULL);
                    return;
            }
                        
            if (o->isArray)
            {
                Jsi_Value *nv;
                int i, len = o->arrCnt;
                
                if (!o->arr)
                    len = Jsi_ValueGetLength(interp, v);
                Jsi_DSAppend(dStr,"[ ", NULL);
                for (i = 0; i < len; ++i) {
                    nv = Jsi_ValueArrayIndex(interp, v, i);
                    if (i) Jsi_DSAppend(dStr,", ", NULL);
                    if (nv) jsiValueGetString(interp, nv, dStr, owPtr);
                    else Jsi_DSAppend(dStr, "undefined", NULL);
                }
                Jsi_DSAppend(dStr," ]", NULL);
            } else {
                Jsi_DSAppend(dStr,"{ ", NULL);
                owPtr->depth++;
                Jsi_TreeWalk(o->tree, _object_get_callback, owPtr, 0);
                owPtr->depth--;
                Jsi_DSAppend(dStr," }", NULL);
            }
            return;
        }
        default:
            Jsi_LogBug("Unexpected value type: %d\n", v->vt);
    }
}

/* Format value into dStr.  Toplevel caller does init/free. */
const char* Jsi_ValueGetDString(Jsi_Interp *interp, Jsi_Value* v, Jsi_DString *dStr, int quote)
{
    objwalker ow;
    ow.quote = quote;
    ow.depth = 0;
    ow.dStr = dStr;
    Jsi_DSInit(dStr);
    jsiValueGetString(interp, v, dStr, &ow);
    return Jsi_DSValue(dStr);
}

void Jsi_Puts(Jsi_Interp* interp, Jsi_Value *v, int flags)
{
    int quote = (flags&JSI_OUTPUT_QUOTE);
    int iserr = (flags&JSI_OUTPUT_STDERR);
    Jsi_DString dStr = {};
    const char *cp = Jsi_ValueString(interp, v, 0);
    if (cp) {
        fprintf((iserr?stderr:stdout),"%s", cp);
        return;
    }
    Jsi_DSInit(&dStr);
    Jsi_ValueGetDString(interp, v, &dStr, quote);
    fprintf((iserr?stderr:stdout),"%s",Jsi_DSValue(&dStr));
    Jsi_DSFree(&dStr);
    return;
}

extern int yyparse(jsi_Pstate *ps);

const char* Jsi_KeyAdd(Jsi_Interp *interp, const char *str)
{
    Jsi_HashEntry *hPtr;
    int isNew;
    hPtr = Jsi_HashEntryCreate(interp->strKeyTbl, str, &isNew);
    assert(hPtr) ;
    return (const char*)Jsi_HashKeyGet(hPtr);
}

char* jsi_KeyLookup(Jsi_Interp *interp, const char *str)
{
    Jsi_HashEntry *hPtr;
    hPtr = Jsi_HashEntryFind(interp->strKeyTbl, str);
    if (!hPtr) {
        return NULL;
    }
    return (char*)Jsi_HashKeyGet(hPtr);
}

char* jsi_KeyFind(Jsi_Interp *interp, const char *str, int nocreate, int *isKey)
{
    Jsi_HashEntry *hPtr;
    if (isKey) *isKey = 0;
    if (!nocreate) {
        *isKey = 1;
         if (isKey) *isKey = 1;
        return (char*)Jsi_KeyAdd(interp, str);
    }
    hPtr = Jsi_HashEntryFind(interp->strKeyTbl, str);
    if (!hPtr) {
        return Jsi_Strdup(str);;
    }
    if (isKey) *isKey = 1;
    *isKey = 1;
    return (char*)Jsi_HashKeyGet(hPtr);
}

static int balanced(char *str) {
    int cnt = 0, quote = 0;
    char *cp = str;
    while (*cp) {
        switch (*cp) {
        case '\\':
            cp++;
            break;
        case '{': case '(': case '[':
            cnt++;
            break;
        case '\'': case '\"':
            quote++;
            break;
        case '}': case ')': case ']':
            cnt--;
            break;
        }
        if (*cp == 0)
            break;
        cp++;
    }
    return ((quote%2) == 0 && cnt <= 0);
}

static char *get_inputline(int istty, char *prompt)
{
    char *res;
#ifdef HAVE_READLINE
    if (istty) {
        res = readline(prompt);
        if (res && *res) add_history(res);
        return res;
    }
#endif
    int done = 0;
    char bbuf[BUFSIZ];
    Jsi_DString dStr = {};
    if (istty)
        fputs(prompt, stdout);
    fflush(stdout);
    while (!done) { /* Read a line. */
        bbuf[0] = 0;
        if (fgets(bbuf, sizeof(bbuf), stdin) == NULL)
            return NULL;
        Jsi_DSAppend(&dStr, bbuf, NULL);
        if (strlen(bbuf) < (sizeof(bbuf)-1) || bbuf[sizeof(bbuf)-1] == '\n')
            break;
    }
    res = strdup(Jsi_DSValue(&dStr));
    Jsi_DSFree(&dStr);
    return res;
}


/* Collect and execute code from stdin.  The first byte of flags are passed to Jsi_ValueGetDString(). */
int Jsi_Interactive(Jsi_Interp* interp, int flags) {
    int rc = 0, done = 0, len, quote = (flags & 0xff), istty = 1;
    char *prompt = "# ", *buf;
    Jsi_DString dStr;
    Jsi_DSInit(&dStr);
#ifndef __WIN32
    istty = isatty(fileno(stdin));
#else
    istty = _isatty(_fileno(stdin));
#endif
#ifdef HAVE_READLINE
    Jsi_DString dHist = {};
    char *hist = NULL;
    if(interp->noreadline == 0 && !interp->parent)
    {
        hist = Jsi_NormalPath(interp, "~/.jsish_history", &dHist);
        if (hist)
            read_history(hist);
    }
#endif
    interp->level++;
    while (done==0 && interp->exited==0) {
        buf = get_inputline(istty, prompt);
        if (buf) {
          Jsi_DSAppend(&dStr, buf, NULL);
          free(buf);
        } else {
          done = 1;
        }
        len = Jsi_DSLength(&dStr);
        if (done && len == 0)
            break;
        buf = Jsi_DSValue(&dStr);
        if (done == 0 && (!balanced(buf))) {
            prompt = "> ";
            if (len<5)
                break;
            continue;
        }
        prompt = "# ";
        while ((len = Jsi_Strlen(buf))>0 && (isspace(buf[len-1])))
            buf[len-1] = 0;
        if (buf[0] == 0)
            continue;
        /* Convenience: add semicolon to "var" statements (required by parser). */
        if (strncmp(buf,"var ", 4) == 0 && strchr(buf, '\n')==NULL && strchr(buf, ';')==NULL)
            strcat(buf, ";");
        rc = Jsi_EvalString(interp, buf, JSI_EVAL_RETURN);
        if (interp->exited)
            break;
        if (rc == 0) {
             if (interp->ret.vt != JSI_VT_UNDEF || interp->noUndef==0) {
                Jsi_DString eStr = {};
                fputs(Jsi_ValueGetDString(interp,&interp->ret, &eStr, quote), stdout);
                Jsi_DSFree(&eStr);
                fputs("\n", stdout);
             }
        } else if (!interp->exited) {
            fputs("ERROR\n", stderr);
        }
        Jsi_DSSetLength(&dStr, 0);
        len = 0;
    }
    interp->level--;
#ifdef HAVE_READLINE
    if (hist) {
        stifle_history(100);
        write_history(hist);
    }
    Jsi_DSFree(&dHist);
#endif
    Jsi_DSFree(&dStr);
    if (interp->exited && interp->level <= 0)
    {
        rc = interp->exitCode;
        Jsi_InterpDelete(interp);
    }
    return rc;
}

int Jsi_PackageProvide(Jsi_Interp *interp, const char *name, const char *version)
{
    return JSI_OK;
}
int Jsi_PackageRequire(Jsi_Interp *interp, const char *name, const char *version)
{
    return JSI_OK;
}

int Jsi_ThisDataSet(Jsi_Interp *interp, Jsi_Value *_this, void *value)
{
    int isNew;
    Jsi_HashEntry *hPtr;
    hPtr = Jsi_HashEntryCreate(interp->thisTbl, _this, &isNew);
    if (!hPtr)
        return -1;
    Jsi_HashValueSet(hPtr, value);
    return isNew;
}

void *Jsi_ThisDataGet(Jsi_Interp *interp, Jsi_Value *_this)
{
    Jsi_HashEntry *hPtr;
    hPtr = Jsi_HashEntryFind(interp->thisTbl, _this);
    if (!hPtr)
        return NULL;
    return Jsi_HashValueGet(hPtr);
}

int Jsi_PrototypeDefine(Jsi_Interp *interp, const char *key, Jsi_Value *value)
{
    int isNew;
    Jsi_HashEntry *hPtr;
    hPtr = Jsi_HashEntryCreate(interp->protoTbl, key, &isNew);
    if (!hPtr)
        return -1;
    Jsi_HashValueSet(hPtr, value);
    return isNew;
}

void *Jsi_PrototypeGet(Jsi_Interp *interp, const char *key)
{
    Jsi_HashEntry *hPtr;
    hPtr = Jsi_HashEntryFind(interp->protoTbl, key);
    if (!hPtr)
        return NULL;
    return Jsi_HashValueGet(hPtr);
}

int Jsi_PrototypeObjSet(Jsi_Interp *interp, const char *key, Jsi_Obj *obj)
{
    Jsi_HashEntry *hPtr;
    Jsi_Value *val;
    hPtr = Jsi_HashEntryFind(interp->protoTbl, key);
    if (!hPtr)
        return JSI_ERROR;
    val = Jsi_HashValueGet(hPtr);
    obj->__proto__ = val;
    return JSI_OK;
}

const char *Jsi_ObjTypeStr(Jsi_Interp *interp, Jsi_Obj *o)
{
     switch (o->ot) {
        case JSI_OT_BOOL: return "boolean"; break;
        case JSI_OT_FUNCTION: return "function"; break;
        case JSI_OT_NUMBER: return "number"; break;
        case JSI_OT_STRING: return "string"; break;  
        case JSI_OT_REGEXP: return "regexp"; break;  
        case JSI_OT_ITER: return "iter"; break;  
        case JSI_OT_ARRAY: return "array"; break;  
        case JSI_OT_OBJECT: return "object"; break;
        case JSI_OT_USEROBJ:
            if (o->__proto__) {
                Jsi_HashEntry *hPtr;
                Jsi_HashSearch search;
                            
                for (hPtr = Jsi_HashEntryFirst(interp->thisTbl,&search); hPtr != NULL;
                    hPtr = Jsi_HashEntryNext(&search))
                    if (Jsi_HashValueGet(hPtr) == o->__proto__)
                        return (char*)Jsi_HashKeyGet(hPtr);
            }
            
            return "userdef";
            break;
            //return Jsi_ObjGetType(interp, v->d.obj);
     }
     return "";
}

extern Jsi_otype Jsi_ObjTypeGet(Jsi_Obj *obj)
{
    return obj->ot;
}

const char *Jsi_ValueTypeStr(Jsi_Interp *interp, Jsi_Value *v)
{
    switch (v->vt) {
        case JSI_VT_BOOL: return "boolean"; break;
        case JSI_VT_UNDEF: return "undefined"; break;
        case JSI_VT_NULL: return "null"; break;
        case JSI_VT_NUMBER: return "number"; break;
        case JSI_VT_STRING: return "string"; break;  
        case JSI_VT_VARIABLE: return "variable"; break;  
        case JSI_VT_OBJECT: return Jsi_ObjTypeStr(interp, v->d.obj);
    }
    return "";
}

/* Shim to instantiate a new obj-command and return its userobj data pointer for C use. */
void *Jsi_NewCmdObj(Jsi_Interp *interp, char *name, char *arg1, char *opts, char *var) {
    char buf[BUFSIZ];
    if (arg1)
        sprintf(buf, "%s%snew %s('%s', %s);", var?var:"", var?"=":"return ", name, arg1, opts?opts:"null");
    else
        sprintf(buf, "%s%snew %s(%s);", var?var:"", var?"=":"return ", name, opts?opts:"null");
    int rc = Jsi_EvalString(interp, buf, 0);
    if (rc != JSI_OK)
        return NULL;
    Jsi_Value *vObj = &interp->ret;
    if (var)
        vObj = Jsi_NameLookup(interp, var);
    if (!vObj)
        return NULL;
    return Jsi_UserObjGetData(interp, vObj, NULL);
}


#endif
