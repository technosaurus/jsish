#ifndef __JSIINT_H__
#define __JSIINT_H__

//#define HIDE_MEMLEAKS /* TODO: fix memory leaks and ref count bugs, ie Jsi_DecrRefCount */
#define USE_VALCOPY 1  /* TODO: change to not copying values to get rid of refcount for Jsi_Obj.  */

#define JSI_DEBUG_MEMORY
#ifdef JSI_DEBUG_MEMORY
#define Assert(n) assert(n)
#else
#define Assert(n)
#endif
#ifndef JSI_OMIT_SIGNATURES
#define JSI_HAS_SIG
#define JSI_HAS_SIG_HASHENTRY
#endif
#define _GNU_SOURCE
#define __USE_GNU
#define VAL_REFCNT
#define VAL_REFCNT2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

#ifdef __WIN32 /* TODO: support windows signal??? */
#define JSI_OMIT_SIGNAL
#endif

#ifndef JSI_AMALGAMATION

#ifdef __WIN32
#include "win/compat.h"
#include "win/regex.h"
#else
#define HAVE_REGCOMP
#endif
#endif

#include <time.h>

#ifndef JSI_IS64BIT
#ifdef __GNUC__
#ifdef __X86_64__
#define JSI_IS64BIT 1
#endif
#else /* GCC */
#if _WIN64 || __amd64__
#define JSI_IS64BIT 1
#endif
#endif /* GCC */
#endif /* JSI_IS64BIT */

#ifndef JSI_IS64BIT
#define JSI_IS64BIT 0
#endif

#define JSMN_FREE(p) Jsi_Free(p)
#define JSMN_MALLOC(l) Jsi_Malloc(l)
#define JSMN_REALLOC(p,l) Jsi_Realloc(p,l)

#define JSI_HAS_SIG /* Signatures to help with debugging */
#ifdef JSI_HAS_SIG
#ifndef SIGASSERT
#define SIGASSERT(s,n) assert((s) && (s)->sig == JSI_SIG_##n);
#endif
#define SIGINIT(s,n) s->sig = JSI_SIG_##n;
#define __VALSIG__ JSI_SIG_VALUE,
#else
#define SIGASSERT(s,n)
#define SIGINIT(s,n)
#define __VALSIG__
#endif

#define VALINIT { __VALSIG__ JSI_VT_UNDEF, .refCnt=1  }
#define VALINITS { __VALSIG__ .vt=JSI_VT_UNDEF, .refCnt=1  }

#ifndef uint
typedef unsigned int uint;
#endif

#ifndef JSI_AMALGAMATION
#include "jsi.h"
#endif

#define ARRAY_MOD_SIZE 16      /* Increase arrays by increments of 16. */
#define MAX_ARRAY_LIST 100000  /* Default Max size of an array convertable to list form */
#define MAX_LOOP_COUNT 10000000 /* Limit infinite loops */
#define JSI_MAX_ALLOC_BUF  100000000 /* Limit for dynamic memory allocation hunk */

typedef enum {
    JSI_SIG_ITEROBJ=0xdeadbee1, JSI_SIG_FUNCOBJ, JSI_SIG_SCOPE, JSI_SIG_VALUE,
    JSI_SIG_OBJ, JSI_SIG_USERDATA, JSI_SIG_INTERP, JSI_SIG_PARSER,
    JSI_SIG_FILEOBJ, JSI_SIG_INTERPOBJ, JSI_SIG_FUNC, JSI_SIG_CMDSPECITEM, JSI_SIG_HASH,
    JSI_SIG_HASHENTRY, JSI_SIG_TREE, JSI_SIG_TREEENTRY, JSI_SIG_USER_REG, JSI_SIG_EVENT,
    JSI_SIG_ARGLIST, JSI_SIG_FORINVAR, JSI_SIG_CASELIST, JSI_SIG_CASESTAT,
    JSI_SIG_FASTVAR, JSI_SIG_INTERPSTREVENT, JSI_SIG_ALIASCMD
} jsi_Sig;

#ifdef  JSI_DEBUG_MEMORY
extern void jsi_VALCHK(Jsi_Value *v);
extern void jsi_OBJCHK(Jsi_Obj *o);
#define VALCHK(val) jsi_VALCHK(val)
#define OBJCHK(val) jsi_OBJCHK(val)
#else
#define VALCHK(val)
#define OBJCHK(val)
#endif

/* Scope chain */
typedef struct jsi_ScopeChain {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Interp *interp;
    struct Jsi_Value **chains;  /* values(objects) */
    int chains_cnt;         /* count */
} jsi_ScopeChain;

/* Function obj */
/* a Jsi_FuncObj is a raw function with own scope chain */
struct Jsi_FuncObj {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Interp *interp;
    struct Jsi_Func *func;
    jsi_ScopeChain *scope;
};

typedef int (Jsi_IterProc)(Jsi_IterObj *iterObj, Jsi_Value *val, Jsi_Value *var, int index);

/* Jsi_IterObj, use only in for-in statement */
struct Jsi_IterObj {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Interp *interp;
    const char **keys;
    int size; 
    int count;
    int iter;
    int isArrayList;            /* If an array list do not store keys. */
    struct Jsi_Obj *obj;
    int cur;                    /* Current array cursor. */
    int depth;                  /* Used to create list of keys. */
    Jsi_IterProc *iterCmd;
};

typedef struct UserObjReg { /* Per interp userobj registration. */
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_UserObjReg *reg;
    Jsi_Hash* hashPtr;
    int idx;
} UserObjReg;

/* User defined object */
struct Jsi_UserObj {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Interp *interp;
    Jsi_Hash *id;
    void *data;
    const char *prefix;
    Jsi_UserObjReg *reg;
    struct UserObjReg *ureg;
    uint idx;
    Jsi_HashEntry* hPtr;
};

struct Jsi_Obj {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_otype ot:8;             /* object type */
    uint isArray:1;             /* Array type. */
    uint isstrkey:1;            /* Key string registered in interp->strKeyTbl (do not free) */
    uint isJSONstr:1;
    uint unused1:5;
    uint unused2:16;
#ifdef USE_VALCOPY
    int refcnt;                     /* reference count */
#endif
    union {                     /* switched on by value of "ot" */
        int val;
        Jsi_Number num;
        Jsi_String s;
        Jsi_Regex *robj;
        Jsi_FuncObj *fobj;
        Jsi_IterObj *iobj;
        Jsi_UserObj *uobj;
    } d;
    int arrMaxSize;                 /* Max allocated space for array. */
    int arrCnt;                     /* Count of actually set keys. */
    struct Jsi_Value **arr;   /* Array values. */  
    Jsi_Tree *tree;                 /* Tree storage (should be union with array). */
    Jsi_Value *__proto__;           /* implicit prototype */
    struct Jsi_Obj *constructor;
};

/*#pragma pack(1)*/


struct Jsi_Value {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    uint refCnt:24;
    Jsi_vtype vt:8;             /* value type */
    union {
        uint flag:8;
        struct vflagbit {
            uint readonly:1;
            uint dontenum:1;  /* Dont enumerate. */
            uint dontdel:1;
            uint isarrlist:1;
            uint innershared:1; /* All above used only for objkeys. */
            uint isglob:1;      /* Global value (do not free until interp delete). */
            uint isstrkey:1;    /* Key string registered in interp->strKeyTbl (do not free) */
            uint onstack:1;     /* Stack value. */  
        } bits;
    } f;
    union {                     /* see above */
        int val;
        Jsi_Number num;
        Jsi_String s;
        Jsi_Obj *obj;
        struct Jsi_Value *lval;
    } d;
#ifdef VALUE_DEBUG
    const char *fname;
    int line;
    const char *func;
    Jsi_HashEntry *hPtr;
#endif
};

#ifndef JSI_SMALL_HASH_TABLE
#define JSI_SMALL_HASH_TABLE 10
#endif

typedef union jsi_HashKey {
    void *oneWordValue;
    unsigned long words[1];
    char string[4]; 
} jsi_HashKey;

typedef unsigned int jsi_Hash;

struct Jsi_HashEntry {
#ifdef JSI_HAS_SIG_HASHENTRY
    jsi_Sig sig;
#endif
    struct Jsi_HashEntry *nextPtr;
    struct Jsi_Hash *tablePtr;
    jsi_Hash hval;
    void* clientData;
    jsi_HashKey key;
};

struct Jsi_Hash {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_HashEntry **buckets;
    Jsi_HashEntry *staticBuckets[JSI_SMALL_HASH_TABLE];
    int numBuckets;
    int numEntries;
    int rebuildSize;
    jsi_Hash mask;
    unsigned int downShift;
    int keyType;
    Jsi_HashEntry *(*createProc) (struct Jsi_Hash *tablePtr, const void *key, int *newPtr);
    Jsi_HashEntry *(*findProc) (struct Jsi_Hash *tablePtr, const void *key);
    Jsi_DeleteProc *freeProc;
    int (*lockProc) (struct Jsi_Hash *tablePtr, int lock);
    Jsi_Interp *interp;
};


struct Jsi_Tree {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Interp *interp;
    Jsi_TreeEntry *root;
    unsigned int numEntries, keyType, epoch;
    struct {
        unsigned int inserting:1, destroyed:1,
        nonredblack:1, /* Disable red/black handling on insert/delete. */
        internstr:1, /* STRINGPTR keys are stored in strHash */
        reserve:29;
    } flags;
    Jsi_Hash* strHash;  /* String hash table to use if INTERNSTR; setup on first Create if not defined. */
    Jsi_RBCreateProc *createProc;
    Jsi_RBCompareProc *compareProc;
    Jsi_DeleteProc *freeProc;
};


typedef union jsi_TreeKey {
    void *oneWordValue;
    char string[4];
    const char *stringKey;
} jsi_TreeKey;

typedef struct Jsi_ScopeStrs {
    char **strings;
    int count;
    int _size;
} Jsi_ScopeStrs;

struct OpCodes;
typedef struct OpCodes OpCodes;

/* Program/parse state(context) */
typedef struct jsi_Pstate {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    int err_count;              /* Jsi_LogError count after parse */
    int eval_flag;              /* 1 if currently executing in an eval function */
    int funcDefs;               /* Count of functions defined. 0 means we can delete this cache (eventually). */
    struct OpCodes *opcodes;    /* Execution codes. */
    struct Lexer *lexer;        /* seq provider */

    int _context_id;            /* used in FastVar-locating */
    Jsi_Value last_exception;
    Jsi_Interp *interp;
    Jsi_HashEntry *hPtr;
    Jsi_Hash *argsTbl;
    Jsi_Hash *strTbl;
    Jsi_Hash *fastVarTbl;
} jsi_Pstate;

#ifndef JSI_AMALGAMATION
#include "jsiPstate.h"
#include "parser.h"
#include "jsiCode.h"
#include "jsiLexer.h"
#endif

typedef struct FastVar {
    jsi_Sig sig;
    unsigned int context_id:31;
    unsigned int isglob:1;
    jsi_Pstate *ps;
    struct {
        char *varname;
        struct Jsi_Value *lval;
    } var;
} FastVar;


/* raw function data, with script function or system Jsi_CmdProc */
struct Jsi_Func {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    enum {
        FC_NORMAL,
        FC_BUILDIN
    } type;                         /* type */
    struct OpCodes *opcodes;    /* FC_NORMAL, codes of this function */
    Jsi_CmdProc *callback;            /* FC_BUILDIN, callback */

    Jsi_ScopeStrs *argnames;                 /* FC_NORMAL, argument names */
    Jsi_ScopeStrs *localnames;               /* FC_NORMAL, local var names */
    union {
        uint flags;
        struct {
            uint res:8, hasattr:1, isobj:1 , iscons:1, res2:5;
        } bits;
    } f;
    union {
        uint i;
        struct {
            uint addargs:1 , iscons:1, isdiscard:1, res:5;
        } bits;
    } callflags;
    void *privData;                 /* context data given in create. */
    Jsi_CmdSpec *cmdSpec;
    const char *name;  /* Name for non-anonymous function. */
    const char *script;  /* Script created in. */
    Jsi_Value *bindArgs;
    jsi_Pline bodyline; /* Body line info. */
};

typedef struct {
    char *origFile; /* Short file name. */
    char *fileName; /* Fully qualified name. */
    char *dirName;  /* Directory name. */
    int useCnt;
} jsi_FileInfo;

enum {
    STACK_INIT_SIZE=1024, STACK_INCR_SIZE=1024, STACK_MIN_PAD=100,
    JSI_MAX_CALLFRAME_DEPTH=1000, /* default max nesting depth for procs */
    JSI_MAX_EVAL_DEPTH=1000, /* default max nesting depth for eval */
    JSI_MAX_INCLUDE_DEPTH=50,  JSI_MAX_SUBINTERP_DEPTH=10
    /*,JSI_ON_STACK=0x80*/
};

typedef struct InterpStrEvent {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    int rc, isExec, tryDepth, errLine;
    const char *errFile;
    Jsi_Value *objData;
    Jsi_DString func;
    Jsi_DString data;
    struct InterpStrEvent *next;
    void *mutex;
} InterpStrEvent;

typedef void (*jsiCallTraceProc)(Jsi_Interp *interp, const char *funcName, const char *file, int line, Jsi_CmdSpec* spec, Jsi_Value* _this, Jsi_Value* args);

struct Jsi_Interp {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    int debug;
    uint deleting:1;
    uint destroying:1;
    uint hasEventHdl:1;
    uint indexLoaded:2;
    int exited;
    int exitCode;
    int refCount;
    int traceCalls;
    char nDebug; /* Disable asserts */
    char isInvoked;
    char strict;
    char doUnlock;
    char isSafe;
    char noUndef;
    char fileStrict;  /* Error out on file not found, etc. */
    char hasCallee;
    char noreadline;
    char nocacheOpCodes;
    char noSubInterps;
    char privKeys;
    char subthread;
    int flags;
    int evalFlags;
    jsiCallTraceProc traceHook;
    int opCnt;  /* Count of instructions eval'ed */
    int maxOpCnt;
    int maxUserObjs;
    int userObjCnt;
    int level;  /* Nesting level of eval/func/cmd calls. */
    int maxDepth;/* Max allowed eval recursion. */
    int maxIncDepth;
    int includeDepth;
    int maxInterpDepth;
    int interpDepth;
    int didReturn;
    uint codeCacheHit;
    uint funcCallCnt;
    uint cmdCallCnt;
    uint eventIdx;
    jsi_ScopeChain *gsc;
    Jsi_Value *csc;
    struct Jsi_Interp *parent, *mainInterp;
    jsi_ScopeChain *ingsc;
    Jsi_Value *incsc;
    Jsi_Value *inthis;
    Jsi_Value *onExit;
    Jsi_Value *safeReadDirs;
    Jsi_Value *safeWriteDirs;
    Jsi_Value *execZip;
    void (*logHook)(char *buf, va_list va);
    int (*evalHook)(struct Jsi_Interp* interp, const char *curFile, int curLine);
    const char *name;

    int tryDepth, withDepth, inParse;
    Jsi_Value ret;       /* Return value from eval */
    jsi_Pstate *ps;
    /*int argc;
    char **argv;*/
    Jsi_Value *argv0;
    Jsi_Value *args;
    Jsi_Value *console;
    Jsi_Value *scriptFile;  /* Start script returned by info.argv0(). */
    const char *scriptStr;
    const char *curFile;
    char *curDir;
    int asc;    /* "A constructor" used by IsConstructorCall(). */
    int Sp;
    int maxStack;

    Jsi_Hash *assocTbl;
    Jsi_Hash *cmdSpecTbl; /* Jsi_CmdSpecs registered. */
    Jsi_Hash *codeTbl; /* Scripts compiled table. */
    Jsi_Hash *eventTbl;
    Jsi_Hash *genValueTbl;
    Jsi_Hash *genObjTbl;
    Jsi_Hash *genDataTbl;
    Jsi_Hash *funcTbl;
    Jsi_Hash *fileTbl;
    Jsi_Hash *lexkeyTbl;
    Jsi_Hash *protoTbl;
    Jsi_Hash *regexpTbl;    
    Jsi_Hash *strKeyTbl;  /* Global strings table. */
    Jsi_Hash *thisTbl;
    Jsi_Hash *userdataTbl;
    Jsi_Hash *varTbl;
    Jsi_Hash *preserveTbl;
    Jsi_Hash *loadTbl;
    Jsi_Hash *optionDataHash;
#ifdef VAL_REFCNT
    Jsi_Value **Stack;
    Jsi_Value **Obj_this;
#else
    Jsi_Value *Stack;
    Jsi_Value *Obj_this;
#endif
            
    Jsi_Value *Object_prototype;
    Jsi_Value *Function_prototype_prototype;
    Jsi_Value *Function_prototype;
    Jsi_Value *String_prototype;
    Jsi_Value *Number_prototype;
    Jsi_Value *Boolean_prototype;
    Jsi_Value *Array_prototype;
    Jsi_Value *RegExp_prototype;
    Jsi_Value *Date_prototype;
    
    Jsi_Value *NaNValue;
    Jsi_Value *InfValue;
    Jsi_Value *NullValue;
    Jsi_Value *nullFuncArg; /* Efficient call of no-arg func */
    Jsi_Value *nullFuncRet;
    Jsi_Value *indexFiles;

    const char *logCallback;
    const char *evalCallback;
    
    Jsi_Value *Top_object;
    Jsi_Func *lastFuncIndex;

    Jsi_Value lastSubscriptFail;
    char* lastSubscriptFailStr;
    Jsi_Obj *lastBindSubscriptObj; /* Used for FUNC.bind() */
    
    int objCnt, valueCnt;
    int maxArrayList;
    int delRBCnt;
    OpCode *curIp;  /* These 2 used for debug Log msgs. */
    char *lastPushStr; 
    char *lastPushVar;
    Jsi_Wide sigmask;
    char errMsgBuf[200];  /* Error message space for when in try. */
    int errLine;
    int errCol;
    const char *errFile;
    void* Mutex;
    void* QMutex; /* For threads queues */
    void* threadId;
    int threadCnt;
    int threadShrCnt;
    int lockTimeout; /* in milliseconds. */
    uint lockRefCnt;
    int psEpoch;
    Jsi_DString interpEvalQ;
    Jsi_DString interpMsgQ;
    InterpStrEvent *interpStrEvents;
    const char *recvCmd;
    uint threadErrCnt;  /* Count of bad thread event return codes. */
    uint threadEvalCnt;
    uint threadMsgCnt;
    void *sleepData;
    Jsi_DeleteProc *onDeleteProc;  /* On delete, call this. */
#ifdef VALUE_DEBUG
    Jsi_Hash* valueDebugTbl;
#endif
};


enum { JSI_REG_GLOB=0x1, JSI_REG_NEWLINE=0x2, JSI_REG_DOT_NEWLINE=0x4, JSI_REG_STATIC=0x100 };

struct Jsi_Regex {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    regex_t reg;
    int eflags;
    int flags;
    char *pattern;
};


/* Entries in interp->cmdSpecTbl. */
typedef struct Jsi_CmdSpecItem {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    const char *name;  /* Parent cmd. */
    Jsi_CmdSpec *spec;
    Jsi_Value *proto;
    int flags;
    void *privData;
    Jsi_HashEntry *hPtr;
    struct Jsi_CmdSpecItem *next; /* TODO: support user-added sub-commands. */
    char *help;
    char *info;
    int isCons;
} Jsi_CmdSpecItem;

struct Jsi_TreeEntry {
#ifdef JSI_HAS_SIG
    jsi_Sig sig;
#endif
    Jsi_Tree *treePtr;
    struct Jsi_TreeEntry* left;
    struct Jsi_TreeEntry* right;
    struct Jsi_TreeEntry* parent;
    union { /* FLAGS: bottom 16 bits for JSI, upper 16 bits for users. First 7 map to JSI_OM_ above. */
        struct { 
            unsigned int readonly:1, dontenum:1, dontdel:1, isarrlist:1, innershared:1, isglob:1, isstrkey:1,
                color:1,
                reserve:8,
                user0:8,
                user1:1, user2:1, user3:1, user4:1, user5:1, user6:1, user7:1, user8:1;
        } bits;
        int flags;
    } f;
    void* value;
    jsi_TreeKey key;
};


/* SCOPE */
//typedef struct jsi_ScopeChain jsi_ScopeChain;

extern jsi_ScopeChain* jsi_ScopeChainNew(Jsi_Interp *interp, int cnt); /*STUB = 176*/
extern Jsi_Value* jsi_ScopeChainObjLookupUni(jsi_ScopeChain *sc, char *key); /*STUB = 177*/
extern jsi_ScopeChain* jsi_ScopeChainDupNext(Jsi_Interp *interp, jsi_ScopeChain *sc, Jsi_Value *next); /*STUB = 178*/
extern void jsi_ScopeChainFree(Jsi_Interp *interp, jsi_ScopeChain *sc); /*STUB = 179*/

extern Jsi_Interp *jsiMainInterp; /* The main interp */
extern Jsi_Interp *jsiDelInterp; /* Interp being delete: Used by cleanup callbacks. */
extern void jsi_CmdSpecDelete(Jsi_Interp *interp, void *ptr);
int jsi_global_eval(Jsi_Interp* interp, jsi_Pstate *ps, char *program,
        jsi_ScopeChain *scope, Jsi_Value *currentScope,
        Jsi_Value *_this, Jsi_Value **ret);

int jsi_FilesysInit(Jsi_Interp *interp);

int jsi_LoadInit(Jsi_Interp *interp);
int jsi_CmdsInit(Jsi_Interp *interp);
int jsi_InterpInit(Jsi_Interp *interp);
int jsi_FileCmdsInit(Jsi_Interp *interp);
int jsi_StringInit(Jsi_Interp *interp);
int jsi_ValueInit(Jsi_Interp *interp);
int jsi_NumberInit(Jsi_Interp *interp);
int jsi_ArrayInit(Jsi_Interp *interp);
int jsi_BooleanInit(Jsi_Interp *interp);
int jsi_MathInit(Jsi_Interp *interp);
int jsi_ProtoInit(Jsi_Interp *interp);
int jsi_RegexpInit(Jsi_Interp *interp);
int jsi_JSONInit(Jsi_Interp *interp);
int Jsi_InitSqlite(Jsi_Interp *interp);
int jsi_TreeInit(Jsi_Interp *interp);
int Jsi_InitWebsocket(Jsi_Interp *interp);
int Jsi_execInit(Jsi_Interp *interp);
int jsi_Initsignal(Jsi_Interp *interp);
int jsi_execCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_DString *dStr, Jsi_DString *cStr, int *code);
int Jsi_InitZvfs(Jsi_Interp *interp);

void jsi_SignalClear(Jsi_Interp *interp, int sigNum);
int jsi_SignalIsSet(Jsi_Interp *interp, int sigNum);
/* excute opcodes
 * 1. ps, program execution context
 * 2. opcodes, codes to be executed
 * 3. scope, current scopechain, not include current scope
 * 4. currentScope, current scope
 * 5. _this, where 'this' indicated
 * 6. vret, return value
 */
int jsi_evalcode(jsi_Pstate *ps, OpCodes *opcodes, 
        jsi_ScopeChain *scope, Jsi_Value *currentScope,
        Jsi_Value *_this,
        Jsi_Value **vret);
        
typedef int (*Jsi_Constructor)(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, int flags, void *privData);
void jsi_SharedArgs(Jsi_Interp *interp, Jsi_Value *args, struct Jsi_ScopeStrs *argnames, int alloc);
void jsi_SetCallee(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *tocall);
int jsi_AssertCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr);
int jsi_ArraySizer(Jsi_Interp *interp, Jsi_Obj *obj, int n);
int jsi_InterpInfo(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr);
    
enum {StrKeyAny = 0, StrKeyFunc = 0x1, StrKeyCmd = 0x2, StrKeyVar = 0x2};

char* jsi_KeyLookup(Jsi_Interp *interp, const char *str);
char* jsi_KeyFind(Jsi_Interp *interp, const char *str, int nocreate, int *isKey);
void jsi_InitLocalVar(Jsi_Interp *interp, Jsi_Value *arguments, Jsi_Func *who);
Jsi_Value *jsi_GlobalContext(Jsi_Interp *interp);
void jsi_AddEventHandler(Jsi_Interp *interp);

int jsi_ObjectToStringCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr);
int jsi_HasOwnPropertyCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr);
int jsi_Md5Str(Jsi_Interp *interp, Jsi_Value **ret, const char *str, int len);
int jsi_Md5File(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr);
int jsi_Sha1Str(Jsi_Interp *interp, Jsi_Value **ret, const char *str, int len);
int jsi_Sha1File(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr);
    
int jsi_RegExpValueNew(Jsi_Interp *interp, const char *regtxt, Jsi_Value *ret);
void jsi_DumpOptionSpecs(Jsi_Interp *interp, Jsi_Obj *nobj, Jsi_OptionSpec* spec);
Jsi_Func *jsi_FuncMake(jsi_Pstate *pstate, Jsi_ScopeStrs *args, struct OpCodes *ops, jsi_Pline *line, char *name);
void jsi_FreeOpcodes(OpCodes *ops);
void jsi_DelAssocData(Jsi_Interp *interp, void *data);
Jsi_Value* jsi_ValuesAlloc(Jsi_Interp *interp, int cnt, Jsi_Value* old, int oldsz);
extern void jsi_UserObjFree (Jsi_Interp *interp, Jsi_UserObj *uobj);
extern int jsi_UserObjIsTrue (Jsi_Interp *interp, Jsi_UserObj *uobj);
extern int jsi_UserObjDump   (Jsi_Interp *interp, const char *argStr, Jsi_Obj *obj);
extern int jsi_UserObjDelete (Jsi_Interp *interp, void *data);
extern void jsi_UserObjToName(Jsi_Interp *interp, Jsi_UserObj *uobj, Jsi_DString *dStr);
extern Jsi_Obj *jsi_UserObjFromName(Jsi_Interp *interp, const char *name);
extern int Zvfs_Mount( Jsi_Interp *interp, Jsi_Value *archive, Jsi_Value *mount, Jsi_Value *ret);
extern Jsi_Value* jsi_ObjArraySetDup(Jsi_Interp *interp, Jsi_Obj *obj, Jsi_Value *value, int arrayindex);
extern void jsi_ValueObjSet(Jsi_Interp *interp, Jsi_Value *target, const char *key, Jsi_Value *value, int flags, int isstrkey);
extern void jsi_ValueSubscript(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *key, Jsi_Value *ret, int right_val);
extern void jsi_ValueSubscriptLen(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *key, Jsi_Value *ret, int right_val);
extern Jsi_Value* jsi_ValueObjKeyAssign(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *key, Jsi_Value *value, int flag);
extern void jsi_ValueObjGetKeys(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *ret);
extern Jsi_Value* jsi_ObjArrayLookup(Jsi_Interp *interp, Jsi_Obj *obj, char *key);
extern void jsi_ValueInsertArray(Jsi_Interp *interp, Jsi_Value *target, int index, Jsi_Value *val, int flags);
extern Jsi_Value* jsi_ProtoObjValueNew1(Jsi_Interp *interp, const char *name);
extern Jsi_Value* jsi_ProtoValueNew(Jsi_Interp *interp, const char *name, const char *parent);
extern Jsi_Value* jsi_ObjValueNew(Jsi_Interp *interp);
extern Jsi_Value* jsi_ValueDup(Jsi_Interp *interp, Jsi_Value *v);
extern int jsi_ValueToOInt32(Jsi_Interp *interp, Jsi_Value *v);
extern int jsi_FreeOneLoadHandle(Jsi_Interp *interp, void *handle);
extern Jsi_Value* jsi_MakeFuncValue(Jsi_Interp *interp, Jsi_CmdProc *callback, const char *name);
extern Jsi_Value* jsi_MakeFuncValueSpec(Jsi_Interp *interp, Jsi_CmdSpec *cmdSpec, void *privData);
extern int jsi_FileStatCmd(Jsi_Interp *interp, Jsi_Value *fnam, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr, int islstat);

extern Jsi_IterObj *jsi_IterObjNew(Jsi_Interp *interp, Jsi_IterProc *iterProc);
extern void jsi_IterObjFree(Jsi_IterObj *iobj);
extern Jsi_FuncObj *jsi_FuncObjNew(Jsi_Interp *interp, Jsi_Func *func);
extern void jsi_FuncObjFree(Jsi_FuncObj *fobj);
extern int jsi_ArglistFree(Jsi_Interp *interp, void *ptr);

extern int jsi_ieee_isnormal(Jsi_Number a);
extern int jsi_ieee_isnan(Jsi_Number a);
extern int jsi_ieee_infinity(Jsi_Number a);
extern int jsi_is_integer(Jsi_Number n);
extern int jsi_is_wide(Jsi_Number n);

extern Jsi_Number jsi_ieee_makeinf(int i);
extern Jsi_Number jsi_ieee_makenan(void);

extern void jsi_num_itoa10(int value, char* str);
extern void jsi_num_uitoa10(unsigned int value, char* str);
extern void jsi_num_dtoa2(Jsi_Number value, char* str, int prec);
extern int jsi_num_isNaN(Jsi_Number value);
extern int jsi_num_isFinite(Jsi_Number value);

#ifndef MEMCLEAR
#define MEMCLEAR(ptr) memset(ptr, 0, sizeof(*ptr)) /* To aid debugging memory.*/
#endif

#define MAX_SUBREGEX    256
#define HAVE_LONG_LONG
#define UCHAR(s) (unsigned char)(s)
#define StrnCpy(d,s) strncpy(d,s,sizeof(d)), d[sizeof(d)-1] = 0

#ifdef VALUE_DEBUG
#define Jsi_ValueNew(interp) jsi_ValueNew(interp, __FILE__, __LINE__,__PRETTY_FUNCTION__)
#define Jsi_ValueNew1(interp) jsi_ValueNew1(interp, __FILE__, __LINE__,__PRETTY_FUNCTION__)
#define Jsi_ValueDup(interp,v) jsi_ValueDup(interp, v,__FILE__, __LINE__,__PRETTY_FUNCTION__)
extern Jsi_Value *jsi_ValueNew(Jsi_Interp *interp, const char *fname, int line, const char *func);
extern Jsi_Value *jsi_ValueNew1(Jsi_Interp *interp, const char *fname, int line, const char *func);
extern Jsi_Value *jsi_ValueDup(Jsi_Interp *interp, Jsi_Value *ov, const char *fname, int line, const char *func);
#endif
extern void jsi_FilesysDone(void);

struct Jsi_Stubs;
extern struct Jsi_Stubs *jsiStubsTblPtr;
extern char *jsi_execName;

#endif /* __JSIINT_H__ */
