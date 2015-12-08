#ifndef __JSI_H__
#define __JSI_H__

#define JSI_VERSION_MAJOR   1
#define JSI_VERSION_MINOR   1
#define JSI_VERSION_RELEASE 1

#define JSI_VERSION (JSI_VERSION_MAJOR + ((Jsi_Number)JSI_VERSION_MINOR/100.0) + ((Jsi_Number)JSI_VERSION_RELEASE/10000.0))

#ifndef EXTERN
#define EXTERN extern
#endif

#ifdef offsetof
#define Jsi_Offset(type, field) ((int) offsetof(type, field))
#else
#define Jsi_Offset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif

#ifdef __GNUC__
#define JSI_GCC_ATTRIBUTE(X) __attribute__(X)
#else
#define JSI_GCC_ATTRIBUTE(X)
#endif

/* Optional compile-out commands/options string information. */
#ifdef JSI_WITHOUT_INFO
#define JSI_INFO(n) NULL
#endif
#ifndef JSI_INFO
#define JSI_INFO(n) n
#endif

typedef enum {
    
    /* Return codes. */
    JSI_OK=0, JSI_ERROR=1, JSI_RETURN=2, JSI_BREAK=3,
    JSI_CONTINUE=4, JSI_SIGNAL=5, JSI_EXIT=6, JSI_EVAL=7,
    
    /* General flags. */
    JSI_NONE=0, JSI_NO_ERRMSG=1, 
    JSI_CMP_NOCASE=1, JSI_CMP_CHARSET_SCAN=2,
    JSI_CMP_EXACT=0x4,
    JSI_EVAL_ARGV0=0x1, JSI_EVAL_GLOBAL=0x2, JSI_EVAL_NOSKIPBANG=0x4, JSI_EVAL_INDEX=0x8,
    JSI_EVAL_RETURN         =0x10, /* Return top of stack as result */

    /* Flags for Jsi_CmdProc */
    JSI_CALL_CONSTRUCTOR    =0x1,
    JSI_CALL_BUILTIN        =0x2,
    
    JSI_CMDSPEC_NOTOBJ      = 0x1,
    JSI_CMDSPEC_PROTO       = 0x2,
    JSI_CMDSPEC_NONTHIS     = 0x4,
    
    JSI_CMD_HAS_ATTR        = 0x100,
    JSI_CMD_IS_CONSTRUCTOR  = 0x200,
    JSI_CMD_IS_OBJ          = 0x400,
    JSI_CMD_MASK            = 0xffff,
    
    JSI_OM_READONLY         = 0x01,     /* ecma read-only */
    JSI_OM_DONTENUM         = 0x02,     /* ecma emumerable */
    JSI_OM_DONTDEL          = 0x04,     /* ecma configurable */
    JSI_OM_ISARRAYLIST      = 0x08,
    JSI_OM_INNERSHARED      = 0x10,
    JSI_OM_ISGLOB           = 0x20,
    JSI_OM_ISSTRKEY         = 0x40,
    
    JSI_LOG_FATAL = 0x1, JSI_LOG_ERROR = 0x2, JSI_LOG_WARN = 0x4, 
    JSI_LOG_INFO = 0x8, JSI_LOG_DEBUG = 0x10, JSI_LOG_BUG = 0x20, 
    JSI_LOG_TODO = 0x40, JSI_LOG_PARSE=0x80,
    
    JSI_SORT_NOCASE = 0x1, JSI_SORT_DESCEND = 0x2, JSI_SORT_ASCII = 0x4,

    JSI_KEYS_ONEWORD = 1, JSI_KEYS_STRINGPTR = 2, JSI_KEYS_STRING = 3,
    JSI_KEYS_STRUCT_MINSIZE = 4,

    JSI_NAME_FUNCTIONS = 0x1, JSI_NAME_DATA = 0x2,
    
    JSI_TREE_INORDER=0, JSI_TREE_PREORDER=1, JSI_TREE_POSTORDER=2,
    JSI_TREE_LEVELORDER=3, JSI_TREE_ORDER_MASK=0x3,
    
    JSI_FS_NOCLOSE=0x1, JSI_FS_READONLY=0x2, JSI_FS_WRITEONLY=0x4, JSI_FS_APPEND=0x8,
    JSI_FS_COMPRESS=0x100,
    JSI_FSMODESIZE=15,
    JSI_FILE_TYPE_FILES=0x1, JSI_FILE_TYPE_DIRS=0x2,    JSI_FILE_TYPE_MOUNT=0x4,
    JSI_FILE_TYPE_LINK=0x8,  JSI_FILE_TYPE_PIPE=0x10,   JSI_FILE_TYPE_BLOCK=0x20,
    JSI_FILE_TYPE_CHAR=0x40, JSI_FILE_TYPE_SOCKET=0x80, JSI_FILE_TYPE_HIDDEN=0x100,
    
    JSI_OUTPUT_QUOTE = 0x1,
    JSI_OUTPUT_JSON = 0x2,
    JSI_OUTPUT_NEWLINES = 0x4,
    JSI_OUTPUT_STDERR = 0x8,
    JSI_JSON_STRICT   = 0x101, /* property names must be quoted. */
    JSI_STUBS_STRICT  = 0x1, JSI_STUBS_SIG = 0xdeadbee0,

    JSI_EVENT_TIMER=0, JSI_EVENT_SIGNAL=1, JSI_EVENT_ALWAYS=2,
    JSI_ZIP_MAIN=0x1,  JSI_ZIP_INDEX=0x2,
    
    JSI_DB_PTRS          =0x0100, /* Data is a fixed length array of pointers to (allocated) structs. */
    JSI_DB_PTR_PTRS      =0x0200, /* Address of data array is passed, and this is resized to fit results. */
    JSI_DB_MEMCLEAR     =0x0400, /* Before query previously used data items are reset to empty (eg. DStrings). */
    JSI_DB_MEMFREE      =0x0800, /* Reset as above, then free data items (query is normally empty). */
    JSI_DB_DIRTY_ONLY   =0x1000, /* Used by sqlite UPDATE/INSERT/REPLACE. */
    JSI_DB_NO_BEGINEND  =0x2000, /* Do not wrap multi-item UPDATE in BEGIN/COMMIT. */
    JSI_DB_NO_CACHE     =0x4000, /* Do not cache statement. */
    JSI_DB_NO_STATIC    =0x8000, /* Do not bind text with SQLITE_STATIC. */

    JSI_DBI_READONLY     =0x0001, /* Db is created readonly */
    JSI_DBI_NOCREATE     =0x0002, /* Db must already exist. */
    JSI_DBI_NO_MUTEX     =0x0004, /* Disable mutex. */
    JSI_DBI_FULL_MUTEX   =0x0008, /* Use full mutex. */

} Jsi_Enums; /* We use enum as debugging is easier than #define. */

typedef long long Jsi_Wide;
typedef unsigned long long Jsi_UWide;
#ifdef JSI_USE_LONG_DOUBLE
typedef long double Jsi_Number;
#define JSI_NUMLMOD "L"
#else
typedef double Jsi_Number;
#define JSI_NUMLMOD
#endif
#define JSI_NUMGFMT JSI_NUMLMOD "g"
#define JSI_NUMFFMT JSI_NUMLMOD "f"
#define JSI_NUMEFMT JSI_NUMLMOD "e"

typedef enum {
    JSI_OT_BOOL=1,      /* Boolean object, use d.val */
    JSI_OT_NUMBER,      /* Number object, use d.num */
    JSI_OT_STRING,      /* String object, use d.str */
    JSI_OT_OBJECT,      /* common object, not use d */
    JSI_OT_ARRAY,       /* Actual in object is JSI_OT_OBJECT */
    JSI_OT_FUNCTION,    /* Function object, use d.fobj */
    JSI_OT_REGEXP,      /* RegExp object, use d.robj */
    JSI_OT_ITER,        /* Iter object, use d.iobj */
    JSI_OT_USEROBJ,     /* UserDefined object, use d.uobj */
    JSI_OT__MAX = JSI_OT_USEROBJ
} Jsi_otype;

typedef enum {
                        /* TYPE         CONSTRUCTOR JSI_VALUE-DATA  IMPLICIT-PROTOTYPE  */
    JSI_VT_UNDEF,       /* undefined    none        none            none                */
    JSI_VT_BOOL,        /* boolean      Boolean     d.val           none                */
    JSI_VT_NUMBER,      /* number       Number      d.num           Number.prototype    */
    JSI_VT_STRING,      /* string       String      d.str           String.prototype    */
    JSI_VT_OBJECT,      /* object       Jsi_Obj     d.obj           Jsi_Obj.prototype    */
    JSI_VT_NULL,        /* null         none        none            none                */
    JSI_VT_VARIABLE,    /* lvalue       none        d.lval          none                */
    JSI_VT__MAX = JSI_VT_VARIABLE
} Jsi_vtype;

struct Jsi_Interp;
struct Jsi_Value;
struct Jsi_Obj;
struct Jsi_IterObj;
struct Jsi_FuncObj;
struct Jsi_Func;
struct Jsi_UserObj;
struct Jsi_UserObjReg;
struct Jsi_HashEntry;
struct Jsi_Hash;
struct Jsi_HashSearch;
struct Jsi_TreeEntry;
struct Jsi_Tree;
struct Jsi_TreeSearch;
struct Jsi_CmdSpec;
struct Jsi_OptionSpec;
struct Jsi_Db;

typedef struct Jsi_Interp Jsi_Interp;
typedef struct Jsi_Obj Jsi_Obj;
typedef struct Jsi_Value Jsi_Value;
typedef struct Jsi_Func Jsi_Func;
typedef struct Jsi_IterObj Jsi_IterObj;
typedef struct Jsi_FuncObj Jsi_FuncObj;
typedef struct Jsi_UserObjReg Jsi_UserObjReg;
typedef struct Jsi_UserObj Jsi_UserObj;
typedef struct Jsi_HashEntry Jsi_HashEntry;
typedef struct Jsi_Hash Jsi_Hash;
typedef struct Jsi_HashSearch Jsi_HashSearch;
typedef struct Jsi_TreeEntry Jsi_TreeEntry;
typedef struct Jsi_Tree Jsi_Tree;
typedef struct Jsi_TreeSearch Jsi_TreeSearch;
typedef struct Jsi_Regex Jsi_Regex;
typedef struct Jsi_OptionSpec Jsi_OptionSpec;
typedef struct Jsi_Db Jsi_Db;

typedef int (Jsi_InitProc)(Jsi_Interp *interp);
typedef int (Jsi_DeleteProc)(Jsi_Interp *interp, void *data);
typedef int (Jsi_EventHandlerProc)(Jsi_Interp *interp, void *data);


/* INTERP */
EXTERN Jsi_Interp* Jsi_InterpCreate(Jsi_Interp *parent, int argc, char **argv, Jsi_Value *opts); /*STUB = 1*/
EXTERN void Jsi_InterpDelete( Jsi_Interp* interp); /*STUB = 2*/
EXTERN Jsi_DeleteProc* Jsi_InterpOnDelete(Jsi_Interp *interp, Jsi_DeleteProc *freeProc);  /*STUB = 3*/
EXTERN int Jsi_Interactive(Jsi_Interp* interp, int flags); /*STUB = 4*/
EXTERN int Jsi_InterpGone( Jsi_Interp* interp); /*STUB = 5*/
EXTERN Jsi_Value* Jsi_InterpResult(Jsi_Interp *interp); /*STUB = 6*/
EXTERN void* Jsi_InterpGetData(Jsi_Interp *interp, const char *key, Jsi_DeleteProc **proc); /*STUB = 7*/
EXTERN void Jsi_InterpSetData(Jsi_Interp *interp, const char *key, Jsi_DeleteProc *proc, void *data); /*STUB = 8*/
EXTERN void Jsi_InterpFreeData(Jsi_Interp *interp, const char *key); /*STUB = 9*/

/* CMD/FUNC/VAR */
typedef void (Jsi_DelCmdProc)(Jsi_Interp *interp, void *privData);
typedef int (Jsi_CmdProc)(Jsi_Interp *interp, Jsi_Value *args, 
    Jsi_Value *_this, Jsi_Value **ret, Jsi_Func *funcPtr);
#define Jsi_CmdProcDecl(name,...) int name(Jsi_Interp *interp, Jsi_Value *args, \
    Jsi_Value *_this, Jsi_Value **ret, Jsi_Func *funcPtr, ##__VA_ARGS__)

typedef struct {
    char *name;             /* Cmd name */
    Jsi_CmdProc *proc;       /* Command handler */
    int minArgs;
    int maxArgs;            /* Max args or -1 */
    char *argStr;           /* Argument description */
    int flags;              /* JSI_CMD_* flags. */
    char *help;             /* Short help string. */
    char *info;             /* Detailed description. Use JSI_DETAIL macro. */
    Jsi_OptionSpec *opts;  /* Options for arg, default is first. */
    Jsi_DelCmdProc *delProc;/* Callback to handle command delete. */
    void *udata1, *udata2, *udata3;  /* reserved for internal use. */
} Jsi_CmdSpec;

typedef struct {
    char *str;
    int len;
} Jsi_String;

EXTERN Jsi_Value* Jsi_CommandCreate(Jsi_Interp *interp, const char *name, Jsi_CmdProc *cmdProc, void *privData); /*STUB = 10*/
EXTERN Jsi_Value* Jsi_CommandCreateSpecs(Jsi_Interp *interp, const char *name, Jsi_CmdSpec *cmdSpecs, void *privData, int flags); /*STUB = 14*/
EXTERN int Jsi_CommandInvokeJSON(Jsi_Interp *interp, const char *cmd, const char *json, Jsi_Value **ret); /*STUB = 11*/
EXTERN int Jsi_CommandInvoke(Jsi_Interp *interp, const char *cmdstr, Jsi_Value *args, Jsi_Value **ret); /*STUB = 12*/
EXTERN int Jsi_CommandDelete(Jsi_Interp *interp, const char *name); /*STUB = 13*/
EXTERN Jsi_CmdSpec* Jsi_FunctionGetSpecs(Jsi_Func *funcPtr); /*STUB = 15*/
EXTERN int Jsi_FunctionIsConstructor(Jsi_Func *funcPtr); /*STUB = 16*/
EXTERN int Jsi_FunctionReturnIgnored(Jsi_Interp *interp, Jsi_Func *funcPtr); /*STUB = 17*/
EXTERN void* Jsi_FunctionPrivData(Jsi_Func *funcPtr); /*STUB = 293*/
EXTERN char** Jsi_FunctionLocals(Jsi_Interp *interp, Jsi_Value *func, int *argcPtr); /*STUB = 18*/
EXTERN char** Jsi_FunctionArguments(Jsi_Interp *interp, Jsi_Value *func, int *argcPtr); /*STUB = 19*/
EXTERN int Jsi_FunctionApply(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this, Jsi_Value **ret); /*STUB = 20*/
EXTERN int Jsi_FunctionInvoke(Jsi_Interp *interp, Jsi_Value *tocall, Jsi_Value *args, Jsi_Value **ret, Jsi_Value *_this); /*STUB = 21*/
EXTERN int Jsi_FunctionInvokeJSON(Jsi_Interp *interp, Jsi_Value *tocall, const char *json, Jsi_Value **ret); /*STUB = 22*/
EXTERN int Jsi_FunctionInvokeBool(Jsi_Interp *interp, Jsi_Value *func, Jsi_Value *arg); /*STUB = 23*/
EXTERN Jsi_Value* Jsi_VarLookup(Jsi_Interp *interp, const char *varname); /*STUB = 24*/
EXTERN Jsi_Value* Jsi_NameLookup(Jsi_Interp *interp, const char *varname); /*STUB = 25*/
EXTERN int Jsi_PkgProvide(Jsi_Interp *interp, const char *name, const char *version); /*STUB = 26*/
EXTERN int Jsi_PkgRequire(Jsi_Interp *interp, const char *name, const char *version); /*STUB = 27*/


/* MEMORY */
EXTERN void* Jsi_Malloc(unsigned int size); /*STUB = 28*/
EXTERN void* Jsi_Calloc(unsigned int n, unsigned int size); /*STUB = 29*/
EXTERN void* Jsi_Realloc(void *m, unsigned int size); /*STUB = 30*/
EXTERN void  Jsi_Free(void *m); /*STUB = 31*/
EXTERN int Jsi_ObjIncrRefCount(Jsi_Interp* interp, Jsi_Obj *obj); /*STUB = 32*/
EXTERN int Jsi_ObjDecrRefCount(Jsi_Interp* interp, Jsi_Obj *obj); /*STUB = 33*/
EXTERN int Jsi_IncrRefCount(Jsi_Interp* interp, Jsi_Value *v); /*STUB = 34*/
EXTERN int Jsi_DecrRefCount(Jsi_Interp* interp, Jsi_Value *v); /*STUB = 35*/
EXTERN int Jsi_IsShared(Jsi_Interp* interp, Jsi_Value *v); /*STUB = 36*/


/* CHARACTER STRINGS */
EXTERN int Jsi_Strlen(const char *str); /*STUB = 37*/
EXTERN int Jsi_StrlenSet(const char *str, int len); /*STUB = 38*/
EXTERN int Jsi_Strcmp(const char *str1, const char *str2); /*STUB = 39*/
EXTERN int Jsi_Strncmp(const char *str1, const char *str2, int n); /*STUB = 40*/
EXTERN int Jsi_Strncasecmp(const char *str1, const char *str2, int n); /*STUB = 41*/
EXTERN int Jsi_StrcmpDict(const char *str1, const char *str2, int nocase, int dict); /*STUB = 42*/
EXTERN char* Jsi_Strcpy(char *dst, const char *src); /*STUB = 43*/
EXTERN char* Jsi_Strncpy(char *dst, const char *src, int len); /*STUB = 44*/
EXTERN char* Jsi_Strdup(const char *n); /*STUB = 45*/
EXTERN char* Jsi_SubstrDup(const char *a, int start, int len); /*STUB = 46*/
EXTERN char* Jsi_Strcatdup(const char *str1, const char *str2); /*STUB = 47*/
EXTERN char* Jsi_Strchr(const char *str, int c); /*STUB = 48*/
EXTERN int Jsi_Strpos(char *str, int start, char *nid, int nocase); /*STUB = 49*/
EXTERN int Jsi_Strrpos(char *str, int start, char *nid, int nocase); /*STUB = 50*/
#define Jsi_Stzcpy(buf,src) Jsi_Strncpy(buf, src, sizeof(buf))

/* DSTRING : dynamic string support. */
#ifndef JSI_DSTRING_STATIC_SIZE
#define JSI_DSTRING_STATIC_SIZE 200
#endif

#define JSI_DSTRING_DECL(siz) \
struct { \
    char *str; \
    int len; \
    int spaceAvl; \
    int staticSize; \
    char staticSpace[siz]; \
}

typedef JSI_DSTRING_DECL(JSI_DSTRING_STATIC_SIZE) Jsi_DString;

/* For declaring a custom Jsi_DString* variable with other than the default size... */
#define JSI_DSTRING_VAR(namPtr, siz) \
    JSI_DSTRING_DECL(siz) _STATIC_##namPtr; \
    Jsi_DString *namPtr = (Jsi_DString *)&_STATIC_##namPtr; \
    namPtr->staticSize = siz; namPtr->str=namPtr->staticSpace; \
    namPtr->staticSpace[0] = 0; namPtr->spaceAvl = namPtr->len = 0

#ifdef JSI_EMULATE_TCL
#define TCL_OK JSI_OK
#define TCL_ERROR JSI_ERROR
#define Tcl_SplitList       Jsi_SplitStr
#define Tcl_DString         Jsi_DString
#define Tcl_DStringInit     Jsi_DSInit
#define Tcl_DStringFree     Jsi_DSFree
#define Tcl_DStringAppend   Jsi_DSAppendLen
#define Tcl_DStringValue    Jsi_DSValue
#define Tcl_DStringLength   Jsi_DSLength
#define Tcl_DStringSetLength Jsi_DSSetLength
#endif

EXTERN char*   Jsi_DSAppendLen(Jsi_DString *dsPtr,const char *bytes, int length);  /*STUB = 54*/
EXTERN char*   Jsi_DSAppend(Jsi_DString *dsPtr, const char *str, ...)  /*STUB = 55*/  JSI_GCC_ATTRIBUTE((sentinel));
EXTERN void    Jsi_DSFree(Jsi_DString *dsPtr);  /*STUB = 52*/
EXTERN char*   Jsi_DSFreeDup(Jsi_DString *dsPtr);  /*STUB = 57*/
EXTERN void    Jsi_DSInit(Jsi_DString *dsPtr);  /*STUB = 56*/
EXTERN int     Jsi_DSLength(Jsi_DString *dsPtr);  /*STUB = 59*/
EXTERN char*   Jsi_DSPrintf(Jsi_DString *dsPtr, const char *fmt, ...)  /*STUB = 62*/ JSI_GCC_ATTRIBUTE((format (printf,2,3)));
EXTERN char*   Jsi_DSSet(Jsi_DString *dsPtr, const char *str);  /*STUB = 51*/
EXTERN int     Jsi_DSSetLength(Jsi_DString *dsPtr, int length);  /*STUB = 60*/
EXTERN char*   Jsi_DSValue(Jsi_DString *dsPtr);  /*STUB = 53*/

/* OBJ */
EXTERN Jsi_Obj* Jsi_ObjNew(Jsi_Interp* interp); /*STUB = 63*/
EXTERN Jsi_Obj* Jsi_ObjNewType(Jsi_Interp* interp, Jsi_otype type); /*STUB = 64*/
EXTERN void Jsi_ObjFree(Jsi_Interp* interp, Jsi_Obj *obj); /*STUB = 65*/
EXTERN Jsi_Obj* Jsi_ObjNewObj(Jsi_Interp *interp, Jsi_Value **items, int count); /*STUB = 66*/
EXTERN Jsi_Obj* Jsi_ObjNewArray(Jsi_Interp *interp, Jsi_Value **items, int count); /*STUB = 67*/

EXTERN int      Jsi_ObjIsArray(Jsi_Interp *interp, Jsi_Obj *o); /*STUB = 68*/
EXTERN void     Jsi_ObjSetLength(Jsi_Interp *interp, Jsi_Obj *obj, int len); /*STUB = 69*/
EXTERN int      Jsi_ObjGetLength(Jsi_Interp *interp, Jsi_Obj *obj); /*STUB = 70*/
EXTERN const char* Jsi_ObjTypeStr(Jsi_Interp *interp, Jsi_Obj *obj); /*STUB = 71*/
EXTERN Jsi_otype Jsi_ObjTypeGet(Jsi_Obj *obj); /*STUB = 72*/
EXTERN void     Jsi_ObjListifyArray(Jsi_Interp *interp, Jsi_Obj *obj); /*STUB = 73*/
EXTERN int      Jsi_ObjArraySet(Jsi_Interp *interp, Jsi_Obj *obj, Jsi_Value *value, int arrayindex); /*STUB = 74*/
EXTERN int      Jsi_ObjArrayAdd(Jsi_Interp *interp, Jsi_Obj *o, Jsi_Value *v); /*STUB = 75*/
EXTERN Jsi_TreeEntry* Jsi_ObjInsert(Jsi_Interp *interp, Jsi_Obj *obj, const char *key, Jsi_Value *nv, int flags); /*STUB = 76*/
EXTERN void    Jsi_ObjFromDS(Jsi_DString *dsPtr, Jsi_Obj *obj);  /*STUB = 61*/

/* VALUE */
EXTERN Jsi_Value* Jsi_ValueNew(Jsi_Interp *interp); /*STUB = 77*/
EXTERN Jsi_Value* Jsi_ValueNew1(Jsi_Interp *interp); /*STUB = 78*/
EXTERN void Jsi_ValueFree(Jsi_Interp *interp, Jsi_Value* v); /*STUB = 79*/

EXTERN Jsi_Value* Jsi_ValueNewNull(Jsi_Interp *interp); /*STUB = 80*/
EXTERN Jsi_Value* Jsi_ValueNewBoolean(Jsi_Interp *interp, int bval); /*STUB = 81*/
EXTERN Jsi_Value* Jsi_ValueNewNumber(Jsi_Interp *interp, Jsi_Number n); /*STUB = 82*/
EXTERN Jsi_Value* Jsi_ValueNewBlob(Jsi_Interp *interp, unsigned char *s, int len); /*STUB = 83*/
EXTERN Jsi_Value* Jsi_ValueNewString(Jsi_Interp *interp, const char *s); /*STUB = 84*/
EXTERN Jsi_Value* Jsi_ValueNewStringKey(Jsi_Interp *interp, const char *s); /*STUB = 85*/
EXTERN Jsi_Value* Jsi_ValueNewStringDup(Jsi_Interp *interp, const char *s); /*STUB = 86*/
EXTERN Jsi_Value* Jsi_ValueNewArray(Jsi_Interp *interp, char **items, int count); /*STUB = 87*/

EXTERN int Jsi_GetStringFromValue(Jsi_Interp* interp, Jsi_Value *value, const char **s); /*STUB = 88*/
EXTERN int Jsi_GetNumberFromValue(Jsi_Interp* interp, Jsi_Value *value, Jsi_Number *n); /*STUB = 89*/
EXTERN int Jsi_GetBoolFromValue(Jsi_Interp* interp, Jsi_Value *value, int *n); /*STUB = 90*/
EXTERN int Jsi_GetIntFromValue(Jsi_Interp* interp, Jsi_Value *value, int *n); /*STUB = 91*/
EXTERN int Jsi_GetLongFromValue(Jsi_Interp* interp, Jsi_Value *value, long *n); /*STUB = 92*/
EXTERN int Jsi_GetWideFromValue(Jsi_Interp* interp, Jsi_Value *value, Jsi_Wide *n); /*STUB = 93*/
EXTERN int Jsi_GetDoubleFromValue(Jsi_Interp* interp, Jsi_Value *value, Jsi_Number *n); /*STUB = 94*/
EXTERN int Jsi_GetIntFromValueBase(Jsi_Interp* interp, Jsi_Value *value, int *n, int base, int flags); /*STUB = 95*/
EXTERN int Jsi_ValueGetBoolean(Jsi_Interp *interp, Jsi_Value *pv, int *val); /*STUB = 96*/
EXTERN int Jsi_ValueGetNumber(Jsi_Interp *interp, Jsi_Value *pv, Jsi_Number *val); /*STUB = 97*/

EXTERN int Jsi_ValueIsType(Jsi_Interp *interp, Jsi_Value *pv, Jsi_vtype vtype); /*STUB = 98*/
EXTERN int Jsi_ValueIsObjType(Jsi_Interp *interp, Jsi_Value *v, Jsi_otype otype); /*STUB = 99*/
EXTERN int Jsi_ValueIsTrue(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 100*/
EXTERN int Jsi_ValueIsFalse(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 101*/
EXTERN int Jsi_ValueIsNumber(Jsi_Interp *interp, Jsi_Value *pv); /*STUB = 102*/
EXTERN int Jsi_ValueIsArray(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 103*/
EXTERN int Jsi_ValueIsBoolean(Jsi_Interp *interp, Jsi_Value *pv); /*STUB = 104*/
EXTERN int Jsi_ValueIsNull(Jsi_Interp *interp, Jsi_Value *pv); /*STUB = 105*/
EXTERN int Jsi_ValueIsUndef(Jsi_Interp *interp, Jsi_Value *pv); /*STUB = 106*/
EXTERN int Jsi_ValueIsFunction(Jsi_Interp *interp, Jsi_Value *pv); /*STUB = 107*/
EXTERN int Jsi_ValueIsString(Jsi_Interp *interp, Jsi_Value *pv); /*STUB = 108*/

EXTERN Jsi_Value* Jsi_ValueMakeObject(Jsi_Interp *interp, Jsi_Value *v, Jsi_Obj *o); /*STUB = 109*/
EXTERN Jsi_Value* Jsi_ValueMakeArrayObject(Jsi_Interp *interp, Jsi_Value *v, Jsi_Obj *o); /*STUB = 110*/
EXTERN Jsi_Value* Jsi_ValueMakeNumber(Jsi_Interp *interp, Jsi_Value *v, Jsi_Number n); /*STUB = 111*/
EXTERN Jsi_Value* Jsi_ValueMakeBool(Jsi_Interp *interp, Jsi_Value *v, int b); /*STUB = 112*/
EXTERN Jsi_Value* Jsi_ValueMakeString(Jsi_Interp *interp, Jsi_Value *v, const char *s); /*STUB = 113*/
EXTERN Jsi_Value* Jsi_ValueMakeStringDup(Jsi_Interp *interp, Jsi_Value *v, const char *s); /*STUB = 114*/
EXTERN Jsi_Value* Jsi_ValueMakeStringKey(Jsi_Interp *interp, Jsi_Value *v, const char *s); /*STUB = 115*/
EXTERN Jsi_Value* Jsi_ValueMakeBlob(Jsi_Interp *interp, Jsi_Value *v, unsigned char *s, int len); /*STUB = 116*/
EXTERN Jsi_Value* Jsi_ValueMakeNull(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 117*/
EXTERN Jsi_Value* Jsi_ValueMakeUndef(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 118*/
EXTERN Jsi_Value* Jsi_ValueMakeDStringObject(Jsi_Interp *interp, Jsi_Value *v, Jsi_DString *dsPtr); /*STUB = 119*/

EXTERN void         Jsi_ValueToPrimitive(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 120*/
EXTERN const char*  Jsi_ValueToString(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 121*/
EXTERN int          Jsi_ValueToBool(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 122*/
EXTERN Jsi_Number       Jsi_ValueToNumber(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 123*/
EXTERN Jsi_Number       Jsi_ValueToNumberInt(Jsi_Interp *interp, Jsi_Value *v, int isInt); /*STUB = 124*/
EXTERN void         Jsi_ValueToObject(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 125*/

EXTERN void     Jsi_ValueReset(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 126*/
EXTERN const char* Jsi_ValueGetDString(Jsi_Interp* interp, Jsi_Value* v, Jsi_DString *dStr, int quote); /*STUB = 127*/
EXTERN char*    Jsi_ValueString(Jsi_Interp* interp, Jsi_Value* v, int *lenPtr); /*STUB = 128*/
EXTERN char*    Jsi_ValueGetStringLen(Jsi_Interp *interp, Jsi_Value *pv, int *lenPtr); /*STUB = 129*/
EXTERN int      Jsi_ValueStrlen(Jsi_Value* v); /*STUB = 130*/
EXTERN void     Jsi_ValueFromDS(Jsi_Interp *interp, Jsi_DString *dsPtr, Jsi_Value *ret);  /*STUB = 58*/
EXTERN int      Jsi_ValueInstanceOf( Jsi_Interp *interp, Jsi_Value* v1, Jsi_Value* v2); /*STUB = 131*/
EXTERN Jsi_Obj* Jsi_ValueGetObj(Jsi_Interp* interp, Jsi_Value* v); /*STUB = 132*/
EXTERN Jsi_vtype Jsi_ValueTypeGet(Jsi_Value *pv); /*STUB = 133*/
EXTERN const char* Jsi_ValueTypeStr(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 134*/
EXTERN int      Jsi_ValueCmp(Jsi_Interp *interp, Jsi_Value *v1, Jsi_Value* v2, int cmpFlags); /*STUB = 135*/

EXTERN int Jsi_ValueGetIndex( Jsi_Interp *interp, Jsi_Value *valPtr, const char **tablePtr, const char *msg, int flags, int *indexPtr); /*STUB = 136*/
EXTERN int Jsi_ValueArraySort(Jsi_Interp *interp, Jsi_Value *val, int sortFlags); /*STUB = 137*/
EXTERN Jsi_Value * Jsi_ValueArrayConcat(Jsi_Interp *interp, Jsi_Value *arg1, Jsi_Value *arg2); /*STUB = 138*/
EXTERN void Jsi_ValueArrayShift(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 139*/

#define Jsi_ValueInsertFixed(i,t,k,v) Jsi_ValueInsert(i,t,k,v,JSI_OM_READONLY | JSI_OM_DONTDEL | JSI_OM_DONTENUM)
EXTERN void Jsi_ValueInsert(Jsi_Interp *interp, Jsi_Value *target, const char *key, Jsi_Value *val, int flags); /*STUB = 140*/
EXTERN int Jsi_ValueGetLength(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 141*/
EXTERN Jsi_Value* Jsi_ValueArrayIndex(Jsi_Interp *interp, Jsi_Value *args, int index); /*STUB = 142*/
EXTERN char* Jsi_ValueArrayIndexToStr(Jsi_Interp *interp, Jsi_Value *args, int index, int *lenPtr); /*STUB = 143*/
EXTERN Jsi_Value* Jsi_ValueObjLookup(Jsi_Interp *interp, Jsi_Value *target, char *key, int iskeystr); /*STUB = 144*/
EXTERN int Jsi_ValueKeyPresent(Jsi_Interp *interp, Jsi_Value *target, const char *k, int isstrkey); /*STUB = 145*/
EXTERN int Jsi_ValueGetKeys(Jsi_Interp *interp, Jsi_Value *target, Jsi_Value *ret); /*STUB = 146*/
#define Jsi_ValueArraySet(interp, dest, value, index) Jsi_ObjArraySet(interp, Jsi_ValueGetObj(interp, dest), value, index)

EXTERN void Jsi_ValueCopy(Jsi_Interp *interp, Jsi_Value *to, Jsi_Value *from ); /*STUB = 147*/
EXTERN void Jsi_ValueReplace(Jsi_Interp *interp, Jsi_Value **to, Jsi_Value *from ); /*STUB = 148*/

/* USEROBJ */
typedef int (Jsi_UserObjIsTrueProc)(void *data);
typedef int (Jsi_UserObjIsEquProc)(void *data1, void *data2);
typedef Jsi_Obj* (Jsi_UserGetObjProc)(Jsi_Interp *interp, void *data);

struct Jsi_UserObjReg {
    const char *name;
    Jsi_CmdSpec *spec;
    Jsi_DeleteProc *freefun;
    Jsi_UserObjIsTrueProc *istrue;
    Jsi_UserObjIsEquProc *isequ;
};

EXTERN Jsi_Hash* Jsi_UserObjRegister    (Jsi_Interp *interp, Jsi_UserObjReg *reg); /*STUB = 149*/
EXTERN int Jsi_UserObjUnregister  (Jsi_Interp *interp, Jsi_UserObjReg *reg); /*STUB = 150*/
EXTERN int Jsi_UserObjNew    (Jsi_Interp *interp, Jsi_UserObjReg* reg, Jsi_Obj *obj, void *data); /*STUB = 151*/
EXTERN void* Jsi_UserObjGetData(Jsi_Interp *interp, Jsi_Value* value, Jsi_Func *funcPtr); /*STUB = 152*/

/* UTILITY */
#define JSI_NOTUSED(n) n=n /* To get rid of compiler warning. */
EXTERN Jsi_Number Jsi_Version(void); /*STUB = 153*/
EXTERN Jsi_Value* Jsi_ReturnValue(Jsi_Interp *interp); /*STUB = 154*/
EXTERN int Jsi_Mount( Jsi_Interp *interp, Jsi_Value *archive, Jsi_Value *mount, Jsi_Value *ret); /*STUB = 155*/
EXTERN Jsi_Value* Jsi_Executable(Jsi_Interp *interp); /*STUB = 156*/
EXTERN Jsi_Regex* Jsi_RegExpNew(Jsi_Interp *interp, char *regtxt, int flag); /*STUB = 157*/
EXTERN void Jsi_RegExpFree(Jsi_Regex* re); /*STUB = 158*/
EXTERN int Jsi_RegExpMatch( Jsi_Interp *interp,  Jsi_Value *pattern, const char *str, int *rc, Jsi_DString *dStr); /*STUB = 159*/
EXTERN int Jsi_RegExpMatches(Jsi_Interp *interp, Jsi_Value *pattern, const char *str, Jsi_Value *ret); /*STUB = 160*/
EXTERN int Jsi_GlobMatch(const char *pattern, const char *string, int nocase); /*STUB = 161*/
EXTERN char* Jsi_FileRealpath(Jsi_Interp *interp, Jsi_Value *path, char *newpath); /*STUB = 162*/
EXTERN char* Jsi_FileRealpathStr(Jsi_Interp *interp, const char *path, char *newpath); /*STUB = 163*/
EXTERN char* Jsi_NormalPath(Jsi_Interp *interp, const char *path, Jsi_DString *dStr); /*STUB = 164*/
EXTERN char* Jsi_ValueNormalPath(Jsi_Interp *interp, Jsi_Value *path, Jsi_DString *dStr); /*STUB = 165*/
EXTERN int Jsi_JSONParse(Jsi_Interp *interp, const char *js, Jsi_Value *ret, int flags); /*STUB = 166*/
EXTERN int Jsi_JSONParseFmt(Jsi_Interp *interp, Jsi_Value *ret, const char *fmt, ...) /*STUB = 167*/ JSI_GCC_ATTRIBUTE((format (printf,3,4)));;
EXTERN char* Jsi_JSONQuote(Jsi_Interp *interp, const char *cp, int len, Jsi_DString *dStr); /*STUB = 168*/
EXTERN char* Jsi_Itoa(int n); /*STUB = 169*/
EXTERN int Jsi_EvalString(Jsi_Interp* interp, const char *str, int flags); /*STUB = 170*/
EXTERN int Jsi_EvalFile(Jsi_Interp* interp, Jsi_Value *fname, int flags); /*STUB = 171*/
EXTERN int Jsi_EvalCmdJSON(Jsi_Interp *interp, const char *cmd, const char *jsonArgs, Jsi_DString *dStr); /*STUB = 172*/
EXTERN void Jsi_SetResultFormatted(Jsi_Interp *interp, const char *fmt, ...); /*STUB = 173*/
EXTERN int Jsi_DictionaryCompare(const char *left, const char *right); /*STUB = 174*/
EXTERN int Jsi_GetBool(Jsi_Interp* interp, const char *string, int *n); /*STUB = 175*/
EXTERN int Jsi_GetInt(Jsi_Interp* interp, const char *string, int *n, int base); /*STUB = 176*/
EXTERN int Jsi_GetWide(Jsi_Interp* interp, const char *string, Jsi_Wide *n, int base); /*STUB = 177*/
EXTERN int Jsi_GetDouble(Jsi_Interp* interp, const char *string, Jsi_Number *n); /*STUB = 178*/
EXTERN int Jsi_FormatString(Jsi_Interp *interp, Jsi_Value *args, Jsi_DString *dStr); /*STUB = 179*/
EXTERN void Jsi_SplitStr(const char *str, int *argcPtr, char ***argvPtr,  char *s, Jsi_DString *dStr); /*STUB = 180*/
EXTERN void Jsi_Puts(Jsi_Interp *interp, Jsi_Value *v, int quote); /*STUB = 181*/
EXTERN int Jsi_Sleep(Jsi_Interp *interp, Jsi_Number dtim); /*STUB = 182*/
EXTERN void Jsi_Preserve(Jsi_Interp* interp, void *data); /*STUB = 183*/
EXTERN void Jsi_Release(Jsi_Interp* interp, void *data); /*STUB = 184*/
EXTERN void Jsi_EventuallyFree(Jsi_Interp* interp, void *data, Jsi_DeleteProc* proc); /*STUB = 185*/
EXTERN void Jsi_ShiftArgs(Jsi_Interp *interp); /*STUB = 186*/
EXTERN Jsi_Value* Jsi_StringSplit(Jsi_Interp *interp, char *str, char *spliton); /*STUB = 187*/
EXTERN int Jsi_GetIndex( Jsi_Interp *interp, char *str, const char **tablePtr, const char *msg, int flags, int *indexPtr); /*STUB = 188*/
EXTERN void* Jsi_PrototypeGet(Jsi_Interp *interp, const char *key); /*STUB = 189*/
EXTERN int  Jsi_PrototypeDefine(Jsi_Interp *interp, const char *key, Jsi_Value *proto); /*STUB = 190*/
EXTERN int Jsi_PrototypeObjSet(Jsi_Interp *interp, const char *key, Jsi_Obj *obj); /*STUB = 191*/
EXTERN int Jsi_ThisDataSet(Jsi_Interp *interp, Jsi_Value *_this, void *value); /*STUB = 192*/
EXTERN void* Jsi_ThisDataGet(Jsi_Interp *interp, Jsi_Value *_this); /*STUB = 193*/
EXTERN int Jsi_FuncObjToString(Jsi_Interp *interp, Jsi_Obj *o, Jsi_DString *dStr); /*STUB = 282*/
EXTERN void *Jsi_UserObjDataFromVar(Jsi_Interp *interp, char *var); /*STUB = 295 */
EXTERN const char *Jsi_KeyAdd(Jsi_Interp *interp, const char *str); /*STUB = 296 */
EXTERN int Jsi_DatetimeFormat(Jsi_Interp *interp, Jsi_Wide date, const char *fmt, int isUtc, Jsi_DString *dStr);  /*STUB = 297 */
EXTERN int Jsi_DatetimeParse(Jsi_Interp *interp, const char *str, const char *fmt, int isUtc, Jsi_Wide *datePtr); /*STUB = 298 */
EXTERN Jsi_Wide Jsi_DateTime(void); /*STUB = 304 */
#define JSI_DATE_JULIAN2UNIX(d)  (Jsi_Number)(((Jsi_Number)d - 2440587.5)*86400.0)
#define JSI_DATE_UNIX2JULIAN(d)  (Jsi_Number)((Jsi_Number)d/86400.0+2440587.5)

/* HASH TABLE */
struct Jsi_HashSearch {
    Jsi_Hash *tablePtr;
    unsigned long nextIndex; 
    struct Jsi_HashEntry *nextEntryPtr;
};

EXTERN Jsi_Hash* Jsi_HashNew (Jsi_Interp *interp, unsigned int keyType, Jsi_DeleteProc *freeProc); /*STUB = 194*/
EXTERN void Jsi_HashDelete (Jsi_Hash *hashPtr); /*STUB = 195*/
EXTERN Jsi_HashEntry* Jsi_HashSet(Jsi_Hash *hashPtr, void *key, void *value); /*STUB = 196*/
EXTERN void* Jsi_HashGet(Jsi_Hash *hashPtr, void *key); /*STUB = 197*/
EXTERN void* Jsi_HashKeyGet(Jsi_HashEntry *h); /*STUB = 198*/
EXTERN int Jsi_HashKeysDump(Jsi_Interp *interp, Jsi_Hash *hashPtr, Jsi_Value *ret); /*STUB = 199*/
EXTERN void* Jsi_HashValueGet(Jsi_HashEntry *h); /*STUB = 200*/
EXTERN void Jsi_HashValueSet(Jsi_HashEntry *h, void *value); /*STUB = 201*/
EXTERN Jsi_HashEntry* Jsi_HashEntryFind (Jsi_Hash *hashPtr, const void *key); /*STUB = 202*/
EXTERN Jsi_HashEntry* Jsi_HashEntryCreate (Jsi_Hash *hashPtr, const void *key, int *isNew); /*STUB = 203*/
EXTERN void Jsi_HashEntryDelete (Jsi_HashEntry *entryPtr); /*STUB = 204*/
EXTERN Jsi_HashEntry* Jsi_HashEntryFirst (Jsi_Hash *hashPtr, Jsi_HashSearch *searchPtr); /*STUB = 205*/
EXTERN Jsi_HashEntry* Jsi_HashEntryNext (Jsi_HashSearch *srchPtr); /*STUB = 206*/
EXTERN int Jsi_HashSize(Jsi_Hash *hashPtr); /*STUB = 286*/ 
EXTERN void* Jsi_HashConf(Jsi_Hash *hashPtr, int op, ...) /*STUB = 287*/  JSI_GCC_ATTRIBUTE((sentinel));

/*  TREE (Red-Black) */
typedef int (Jsi_TreeWalkProc)(Jsi_Tree* treePtr, Jsi_TreeEntry* hPtr, void *data);
typedef int (Jsi_RBCompareProc)(Jsi_Tree *treePtr, const void *key1, const void *key2);
typedef Jsi_TreeEntry* (Jsi_RBCreateProc)(struct Jsi_Tree *treePtr, const void *key, int *newPtr);

struct Jsi_TreeSearch {
    Jsi_Tree *treePtr;
    unsigned int top, max, left, epoch; 
    int flags;
    Jsi_TreeEntry *staticPtrs[200], *current;
    Jsi_TreeEntry **Ptrs;
};

EXTERN Jsi_Tree* Jsi_TreeNew(Jsi_Interp *interp, unsigned int keyType); /*STUB = 207*/
EXTERN void Jsi_TreeDelete(Jsi_Tree *treePtr); /*STUB = 208*/
EXTERN Jsi_TreeEntry* Jsi_TreeObjSetValue(Jsi_Obj* obj, const char *key, Jsi_Value *val, int isstrkey); /*STUB = 209*/
EXTERN Jsi_Value*     Jsi_TreeObjGetValue(Jsi_Obj* obj, const char *key, int isstrkey); /*STUB = 210*/
EXTERN void* Jsi_TreeValueGet(Jsi_TreeEntry *hPtr); /*STUB = 211*/
EXTERN void Jsi_TreeValueSet(Jsi_TreeEntry *hPtr, void *value); /*STUB = 212*/
EXTERN void* Jsi_TreeKeyGet(Jsi_TreeEntry *hPtr); /*STUB = 213*/
EXTERN Jsi_TreeEntry* Jsi_TreeEntryFind(Jsi_Tree *treePtr, const void *key); /*STUB = 214*/
EXTERN Jsi_TreeEntry* Jsi_TreeEntryCreate(Jsi_Tree *treePtr, const void *key, int *isNew); /*STUB = 215*/
EXTERN void Jsi_TreeEntryDelete(Jsi_TreeEntry *entryPtr); /*STUB = 216*/
EXTERN Jsi_TreeEntry* Jsi_TreeSearchFirst(Jsi_Tree *treePtr, Jsi_TreeSearch *searchPtr, int flags); /*STUB = 217*/
EXTERN void Jsi_TreeSearchDone(Jsi_TreeSearch *srchPtr); /*STUB = 218*/
EXTERN Jsi_TreeEntry* Jsi_TreeSearchNext(Jsi_TreeSearch *searchPtr); /*STUB = 219*/
EXTERN int Jsi_TreeWalk(Jsi_Tree* treePtr, Jsi_TreeWalkProc* callback, void *data, int flags); /*STUB = 220*/
EXTERN Jsi_TreeEntry* Jsi_TreeSet(Jsi_Tree *treePtr, const void *key, void *value); /*STUB = 221*/
EXTERN void* Jsi_TreeGet(Jsi_Tree *treePtr, void *key); /*STUB = 222*/
EXTERN int Jsi_TreeSize(Jsi_Tree *treePtr); /*STUB = 288*/ 
EXTERN void* Jsi_TreeConf(Jsi_Tree *treePtr, int op, ...) /*STUB = 289*/  JSI_GCC_ATTRIBUTE((sentinel));
EXTERN Jsi_Tree* Jsi_TreeFromValue(Jsi_Interp *interp, Jsi_Value *v); /*STUB = 294 */


/* OPTIONS */
typedef int (Jsi_OptionParseProc) (
    Jsi_Interp *interp, Jsi_OptionSpec *spec, Jsi_Value *value, const char *str, void *record);
typedef int (Jsi_OptionFormatProc) (
    Jsi_Interp *interp, Jsi_OptionSpec *spec, Jsi_Value *retValue, Jsi_DString *retStr, void *record);
typedef int (Jsi_OptionFormatStringProc) (
    Jsi_Interp *interp, Jsi_OptionSpec *spec, Jsi_DString *retValue, void *record);
typedef void (Jsi_OptionFreeProc) (Jsi_Interp *interp, Jsi_OptionSpec *spec, void *ptr);

typedef struct {
    const char *name;
    Jsi_OptionParseProc *parseProc;
    Jsi_OptionFormatProc *formatProc;
    Jsi_OptionFreeProc *freeProc;
    char *help;
    char *info;
    void* data;
} Jsi_OptionCustom;

typedef enum {
    JSI_OPTION_NONE, JSI_OPTION_BOOL, JSI_OPTION_INT, JSI_OPTION_WIDE,
    JSI_OPTION_DOUBLE, JSI_OPTION_STRING, JSI_OPTION_DSTRING, JSI_OPTION_STRKEY, JSI_OPTION_STRBUF,
    JSI_OPTION_VALUE, JSI_OPTION_VAR, JSI_OPTION_OBJ, JSI_OPTION_ARRAY,  JSI_OPTION_FUNC,
    JSI_OPTION_DATETIME,/* Timestamp: milliseconds since Jan 1, 1970. */
    JSI_OPTION_CUSTOM,  /* Custom */
    JSI_OPTION_END
} Jsi_OptionTypes;

typedef char Jsi_Strbuf[];

struct Jsi_OptionSpec {
    Jsi_OptionTypes type;
    char *name;         /* The field name. */
    int offset;         /* Jsi_Offset of field. */
    int flags;          /* Lower 16 bytes: the JSI_OPT_* flags below. Upper 16 for custom/other. */
    Jsi_OptionCustom *custom;
    void *data;         /* User data for custom options: eg. the bit for BOOLBIT. */
    char *help;         /* A short, one-line help string. */
    char *info;         /* Longer command description. Use JSI_DETAIL macro to allow compile-out.*/
    const char *init;   /* Initial string value for info.cmds to display. */
    char *extName;      /* External name: used by the DB interface. */
    int size;           /* Sizeof of field. */
    void *userData;     /* User extension data. */
    union { /* Simple compile-time type checking for arg type: JSI_OPT() will assign, but field is unused. */
        char           *ini_BOOL;
        int            *ini_INT;
        Jsi_Wide       *ini_WIDE;
        Jsi_Number     *ini_DOUBLE;
        Jsi_Value*     *ini_STRING;
        Jsi_DString    *ini_DSTRING;
        const char*    *ini_STRKEY;
        Jsi_Strbuf     *ini_STRBUF;
        Jsi_Value*     *ini_VALUE;
        Jsi_Value*     *ini_VAR;
        Jsi_Obj*       *ini_OBJ;
        Jsi_Value*     *ini_ARRAY;
        Jsi_Value*     *ini_FUNC;
        Jsi_Wide       *ini_DATETIME;
        void           *ini_CUSTOM;
    } iniVal;
};

/* JSI_OPT: a macro that simplifies defining options, eg:
 * 
 *      typedef struct { int debug; int bool; } MyStruct;
 *      Jsi_OptionSpec MyOptions[] = {
 *          JSI_OPT(BOOL,  MyStruct,  debug, .help="Debug flag"),
 *          JSI_OPT(INT,   MyStruct,  max,   .help="Max value"),
 *          JSI_OPT_END(   MyStruct )
 *      }
*/
#define JSI_OPT(typ, strct, nam, ...) \
    { .type=JSI_OPTION_##typ, .name=#nam, .offset=Jsi_Offset(strct, nam), .size=sizeof(((strct *) 0)->nam), \
      .iniVal.ini_##typ=(&((strct *) 0)->nam), ##__VA_ARGS__ }
#define JSI_OPT_END(strct,...) { .type=JSI_OPTION_END, .name=#strct, .size=sizeof(strct), \
      .extName=__FILE__, .offset=__LINE__, ##__VA_ARGS__}

/* Custom options that are builtin. */
#define Jsi_Opt_SwitchEnum          (Jsi_OptionCustom*)0x1 /* Enum with .data=stringlist */
#define Jsi_Opt_SwitchBitset        (Jsi_OptionCustom*)0x2 /* Bits with .data=stringlist */
#define Jsi_Opt_SwitchSuboption     (Jsi_OptionCustom*)0x3 /* Sub-structs with .data=<sub-Option> */
#define Jsi_Opt_SwitchValueVerify   (Jsi_OptionCustom*)0x4 /* Provide callback to verify Jsi_Value* is correct. */

enum {
    /* Flags for Jsi_OptionsProcess, etc */
    JSI_OPTS_PREFIX         =   (1<<0), /* Allow matching unique prefix of object members. */
    JSI_OPTS_IS_UPDATE      =   (1<<1), /* This is an update/conf (do not reset the specified flags) */
    JSI_OPTS_IGNORE_EXTRA   =   (1<<2), /* Ignore extra members not found in spec. */
    JSI_OPTS_FORCE_STRICT   =   (1<<3), /* Complain about unknown options, even when noStrict is used. */
    JSI_OPTS_VERBOSE        =   (1<<4), /* Dump verbose options */

    /* Per option flags. */
    JSI_OPT_IS_SPECIFIED    =   (1<<0),  /* User set the option. */
    JSI_OPT_INIT_ONLY       =   (1<<1),  /* Allow set only at init, disallow update/conf. */
    JSI_OPT_READ_ONLY       =   (1<<2),  /* Value can never be set. */
    JSI_OPT_NO_DUPVALUE     =   (1<<3),  /* Values are not to be duped. */
    JSI_OPT_DB_DIRTY        =   (1<<4),  /* Used to limit DB updates. */
    JSI_OPT_DB_IGNORE       =   (1<<5),  /* Field is not to be used for DB. */
    JSI_OPT_CUST_NOCASE     =   (1<<6),  /* Ignore case (eg. for ENUM and BITSET). */
    JSI_OPT_RESERVE_FLAG1   =   (1<<7),  /* Reserved flags. */
    JSI_OPT_RESERVE_FLAG2   =   (1<<8),  /* Reserved flags. */
    JSI_OPT_RESERVE_FLAG3   =   (1<<8),  /* Reserved flags. */
    JSI_OPT_RESERVE_FLAG4   =   (1<<9),  /* Reserved flags. */
    
    JSI_OPTIONS_USER_FIRSTBIT  =   16,      /* User flags bit start: the lower 2 bytes are reserved. */
};


EXTERN const char* Jsi_OptionTypeStr(Jsi_OptionTypes typ); /*STUB = 223*/
EXTERN int Jsi_OptionsProcess(Jsi_Interp *interp, Jsi_OptionSpec *specs, Jsi_Value *value, void *data, int flags); /*STUB = 224*/
EXTERN int Jsi_OptionsConf(Jsi_Interp *interp, Jsi_OptionSpec *specs, Jsi_Value *value, void *data, Jsi_Value *ret, int flags); /*STUB = 225*/
EXTERN int Jsi_OptionsChanged (Jsi_Interp *interp, Jsi_OptionSpec *specs, const char *pattern, ...) /*STUB = 226*/ JSI_GCC_ATTRIBUTE((sentinel));
EXTERN void Jsi_OptionsFree(Jsi_Interp *interp, Jsi_OptionSpec *specs, void *data, int flags); /*STUB = 227*/
EXTERN int Jsi_OptionsGet(Jsi_Interp *interp, Jsi_OptionSpec *specs, void *data, const char *option, Jsi_Value* valuePtr, int flags); /*STUB = 228*/
EXTERN int Jsi_OptionsSet(Jsi_Interp *interp, Jsi_OptionSpec *specs, char *option, void* data, Jsi_Value *value, int flags); /*STUB = 229*/
EXTERN int Jsi_OptionsDump(Jsi_Interp *interp, Jsi_OptionSpec *specs, void *data, Jsi_Value* ret, int flags); /*STUB = 230*/
EXTERN Jsi_Value* Jsi_OptionsCustomPrint(void* clientData, Jsi_Interp *interp, char *optionName, void *data, int offset); /*STUB = 231*/
EXTERN Jsi_OptionCustom* Jsi_OptionCustomBuiltin(Jsi_OptionCustom* cust); /*STUB = 299 */
/* Create a duplicate of static specs.   Use this for threaded access to Jsi_OptionsChanged(). */
EXTERN Jsi_OptionSpec* Jsi_OptionsDup(Jsi_Interp *interp, const Jsi_OptionSpec *staticSpecs); /*STUB = 232*/

/* Struct for array to bind a different Option/Data pair to each of the SQLite binding chars. */
typedef struct Jsi_DbMultipleBind {
    char prefix;            /* Char bind prefix. One of: '@' '$' ':' '?' or 0 for any */
    Jsi_OptionSpec *opts;   /* Option fields for the above struct. */
    void *data;             /* Struct data pointer/array */
    int numData;            /* Number of elements in data array: <= 0 means 1. */
    int flags;
    int (*callback)(Jsi_Interp *interp, struct Jsi_DbMultipleBind* obPtr, void *data); /* Select callback. */
} Jsi_DbMultipleBind;

EXTERN Jsi_Db* Jsi_DbNew(const char *zFile, int inFlags); /*STUB = 305 */
EXTERN int Jsi_DbQuery(Jsi_Db *jdb, Jsi_OptionSpec *spec, void *data, int numData, const char *query, int flags); /*STUB = 300 */
EXTERN void *Jsi_DbHandle(Jsi_Interp *interp, Jsi_Db* db); /*STUB = 302 */
EXTERN int Jsi_CDataRegister(Jsi_Interp *interp, const char *name, Jsi_OptionSpec *spec, void *data, int numData, int flags); /*STUB = 303 */
EXTERN Jsi_DbMultipleBind *Jsi_CDataLookup(Jsi_Interp *interp, const char *name); /*STUB = 306 */ /*LAST STUB*/

/* STACK */
typedef struct {
    int len;
    int maxlen;
    void **vector;
} Jsi_Stack;

EXTERN void Jsi_StackInit(Jsi_Stack *stack); /*STUB = 233*/
EXTERN void Jsi_StackFree(Jsi_Stack *stack); /*STUB = 234*/
EXTERN int Jsi_StackLen(Jsi_Stack *stack); /*STUB = 235*/
EXTERN void Jsi_StackPush(Jsi_Stack *stack, void *element); /*STUB = 236*/
EXTERN void* Jsi_StackPop(Jsi_Stack *stack); /*STUB = 237*/
EXTERN void* Jsi_StackPeek(Jsi_Stack *stack); /*STUB = 238*/
EXTERN void Jsi_StackFreeElements(Jsi_Stack *stack, Jsi_DeleteProc *freeFunc); /*STUB = 239*/


/* THREADS/MUTEX */
EXTERN int Jsi_MutexLock(Jsi_Interp *interp, void *mtx); /*STUB = 240*/
EXTERN void Jsi_MutexUnlock(Jsi_Interp *interp, void *mtx); /*STUB = 241*/
EXTERN void Jsi_MutexInit(Jsi_Interp *interp, void *mtx); /*STUB = 242*/
EXTERN void Jsi_MutexDone(Jsi_Interp *interp, void *mtx); /*STUB = 243*/
EXTERN void* Jsi_MutexNew(Jsi_Interp *interp); /*STUB = 244*/
EXTERN void* Jsi_CurrentThread(void); /*STUB = 245*/
EXTERN void* Jsi_InterpThread(Jsi_Interp *interp); /*STUB = 246*/


/* LOG MESSAGES */
#ifdef _JSI_LOG_PRINTF_  /* Define this to allow non-gcc compiler to do printf checking of arguments. */
#define Jsi_LogFatal printf
#define Jsi_LogError printf
#define Jsi_LogParse printf
#define Jsi_LogWarn printf
#define Jsi_LogInfo printf
#define Jsi_LogBug printf
#else
#define Jsi_LogFatal(fmt,...) Jsi_LogMsg(interp, JSI_LOG_FATAL, fmt, ##__VA_ARGS__)
#define Jsi_LogError(fmt,...) Jsi_LogMsg(interp, JSI_LOG_ERROR, fmt, ##__VA_ARGS__)
#define Jsi_LogParse(fmt,...) Jsi_LogMsg(interp, JSI_LOG_PARSE, fmt, ##__VA_ARGS__)
#define Jsi_LogWarn(fmt,...) Jsi_LogMsg(interp, JSI_LOG_WARN, fmt, ##__VA_ARGS__)
#define Jsi_LogInfo(fmt,...) Jsi_LogMsg(interp, JSI_LOG_INFO, fmt, ##__VA_ARGS__)
#define Jsi_LogBug(fmt,...) Jsi_LogMsg(interp, JSI_LOG_BUG, fmt, ##__VA_ARGS__)
#endif
EXTERN void Jsi_LogMsg(Jsi_Interp *interp, int level, const char *format,...)  /*STUB = 247*/ JSI_GCC_ATTRIBUTE((format (printf,3,4)));


/* EVENT: */
typedef struct {
    int sig;
    unsigned int id;
    int evType;                 /* Is signal handler. */
    int sigNum;
    int once;                   /* Execute once */
    long initialms;             /* initial relative timer value */
    long when_sec;              /* seconds */
    long when_ms;               /* milliseconds */
    unsigned int count;         /* Times executed */
    Jsi_HashEntry *hPtr;
    Jsi_Value *funcVal;         /* JS Function to call. */
    Jsi_EventHandlerProc *handler;  /* C-function handler. */
    void *data;
} Jsi_Event;

EXTERN Jsi_Event* Jsi_EventNew(Jsi_Interp *interp, Jsi_EventHandlerProc *callback, void* data); /*STUB = 248*/
EXTERN void Jsi_EventFree(Jsi_Interp *interp, Jsi_Event* event); /*STUB = 249*/
EXTERN int Jsi_EventProcess(Jsi_Interp *interp, int maxEvents); /*STUB = 250*/


/* VFS - VIRTUAL FILESYSTEM */
#include <sys/stat.h>
#include <stdio.h> 
#include <dirent.h>

struct Jsi_Chan;
struct Jsi_StatBuf;
struct Jsi_LoadHandle;
struct Jsi_Dirent;

typedef struct Jsi_LoadHandle Jsi_LoadHandle;
typedef struct Jsi_Chan* Jsi_Channel;
typedef struct stat Jsi_StatBuf;
typedef struct dirent Jsi_Dirent;

typedef int (Jsi_FSStatProc) (Jsi_Interp *interp, Jsi_Value* path, Jsi_StatBuf *buf);
typedef int (Jsi_FSAccessProc) (Jsi_Interp *interp, Jsi_Value* path, int mode);
typedef int (Jsi_FSChmodProc) (Jsi_Interp *interp, Jsi_Value* path, int mode);
typedef Jsi_Channel (Jsi_FSOpenProc) (Jsi_Interp *interp, Jsi_Value* path, const char* modes);
typedef int (Jsi_FSLstatProc) (Jsi_Interp *interp, Jsi_Value* path, Jsi_StatBuf *buf);
typedef int (Jsi_FSCreateDirectoryProc) (Jsi_Interp *interp, Jsi_Value* path);
typedef int (Jsi_FSRemoveProc) (Jsi_Interp *interp, Jsi_Value* path, int flags);
typedef int (Jsi_FSCopyDirectoryProc) (Jsi_Interp *interp, Jsi_Value *srcPathPtr, Jsi_Value *destPathPtr, Jsi_Value **errorPtr);
typedef int (Jsi_FSCopyFileProc) (Jsi_Interp *interp, Jsi_Value *srcPathPtr, Jsi_Value *destPathPtr);
typedef int (Jsi_FSRemoveDirectoryProc) (Jsi_Interp *interp, Jsi_Value* path, int recursive, Jsi_Value **errorPtr);
typedef int (Jsi_FSRenameProc) (Jsi_Interp *interp, Jsi_Value *srcPathPtr, Jsi_Value *destPathPtr);
typedef Jsi_Value * (Jsi_FSListVolumesProc) (Jsi_Interp *interp);
typedef char* (Jsi_FSRealPathProc) (Jsi_Interp *interp, Jsi_Value* path, char *newPath);
typedef int (Jsi_FSLinkProc) (Jsi_Interp *interp, Jsi_Value* path, Jsi_Value *toPath, int linkType);
typedef int (Jsi_FSReadlinkProc)(Jsi_Interp *interp, Jsi_Value *path, char *buf, int size);
typedef int (Jsi_FSReadProc)(Jsi_Channel chan, char *buf, int size);
typedef int (Jsi_FSGetcProc)(Jsi_Channel chan);
typedef int (Jsi_FSEofProc)(Jsi_Channel chan);
typedef int (Jsi_FSTruncateProc)(Jsi_Channel chan, unsigned int len);
typedef int (Jsi_FSUngetcProc)(Jsi_Channel chan, int ch);
typedef char *(Jsi_FSGetsProc)(Jsi_Channel chan, char *s, int size);
typedef int (Jsi_FSWriteProc)(Jsi_Channel chan, const char *buf, int size);
typedef int (Jsi_FSFlushProc)(Jsi_Channel chan);
typedef int (Jsi_FSSeekProc)(Jsi_Channel chan, Jsi_Wide offset, int mode);
typedef int (Jsi_FSTellProc)(Jsi_Channel chan);
typedef int (Jsi_FSCloseProc)(Jsi_Channel chan);
typedef int (Jsi_FSRewindProc)(Jsi_Channel chan);
typedef int (Jsi_FSPathInFilesystemProc) (Jsi_Interp *interp, Jsi_Value* path,void* *clientDataPtr);
typedef int (Jsi_FSScandirProc)(Jsi_Interp *interp, Jsi_Value *path, Jsi_Dirent ***namelist,
  int (*filter)(const Jsi_Dirent *), int (*compar)(const Jsi_Dirent **, const Jsi_Dirent**));

typedef struct Jsi_Filesystem {
    const char *typeName;
    int structureLength;    
    int version;
    Jsi_FSPathInFilesystemProc *pathInFilesystemProc;
    Jsi_FSRealPathProc *realpathProc;
    Jsi_FSStatProc *statProc;
    Jsi_FSLstatProc *lstatProc;
    Jsi_FSAccessProc *accessProc;
    Jsi_FSChmodProc *chmodProc;
    Jsi_FSOpenProc *openProc;
    Jsi_FSScandirProc *scandirProc;
    Jsi_FSReadProc *readProc;
    Jsi_FSWriteProc *writeProc;
    Jsi_FSGetsProc *getsProc;
    Jsi_FSGetcProc *getcProc;
    Jsi_FSUngetcProc *ungetcProc;
    
    Jsi_FSFlushProc *flushProc;
    Jsi_FSSeekProc *seekProc;
    Jsi_FSTellProc *tellProc;
    Jsi_FSEofProc *eofProc;
    Jsi_FSTruncateProc *truncateProc;
    Jsi_FSRewindProc *rewindProc;
    Jsi_FSCloseProc *closeProc;
    Jsi_FSLinkProc *linkProc;
    Jsi_FSReadlinkProc *readlinkProc;
    Jsi_FSListVolumesProc *listVolumesProc;
    Jsi_FSCreateDirectoryProc *createDirectoryProc;
    Jsi_FSRemoveProc *removeProc;
    Jsi_FSRenameProc *renameProc;
} Jsi_Filesystem;

typedef struct Jsi_Chan {
    Jsi_Interp *interp;
    FILE *fp;
    const char *fname;  /* May be set by fs or by source */
    Jsi_Filesystem *fsPtr;
    int isNative;
    int flags;
    char modes[JSI_FSMODESIZE];
    void *data;
} Jsi_Chan;

EXTERN int Jsi_FSRegister(Jsi_Filesystem *fsPtr, void *data); /*STUB = 251*/
EXTERN int Jsi_FSUnregister(Jsi_Filesystem *fsPtr); /*STUB = 252*/
EXTERN Jsi_Channel Jsi_FSNameToChannel(Jsi_Interp *interp, const char *name); /*STUB = 253*/
EXTERN char* Jsi_GetCwd(Jsi_Interp *interp, Jsi_DString *cwdPtr); /*STUB = 254*/
EXTERN int Jsi_Lstat(Jsi_Interp *interp, Jsi_Value* path, Jsi_StatBuf *buf); /*STUB = 255*/
EXTERN int Jsi_Stat(Jsi_Interp *interp, Jsi_Value* path, Jsi_StatBuf *buf); /*STUB = 256*/
EXTERN int Jsi_Access(Jsi_Interp *interp, Jsi_Value* path, int mode); /*STUB = 257*/
EXTERN int Jsi_Remove(Jsi_Interp *interp, Jsi_Value* path, int flags); /*STUB = 258*/
EXTERN int Jsi_Rename(Jsi_Interp *interp, Jsi_Value *src, Jsi_Value *dst); /*STUB = 259*/
EXTERN int Jsi_Chdir(Jsi_Interp *interp, Jsi_Value* path); /*STUB = 260*/
EXTERN Jsi_Channel Jsi_Open(Jsi_Interp *interp, Jsi_Value *file, const char *modeString); /*STUB = 261*/
EXTERN int Jsi_Eof(Jsi_Channel chan); /*STUB = 262*/
EXTERN int Jsi_Close(Jsi_Channel chan); /*STUB = 263*/
EXTERN int Jsi_Read(Jsi_Channel chan, char *bufPtr, int toRead); /*STUB = 264*/
EXTERN int Jsi_Write(Jsi_Channel chan, const char *bufPtr, int slen); /*STUB = 265*/
EXTERN Jsi_Wide Jsi_Seek(Jsi_Channel chan, Jsi_Wide offset, int mode); /*STUB = 266*/
EXTERN Jsi_Wide Jsi_Tell(Jsi_Channel chan); /*STUB = 267*/
EXTERN int Jsi_Truncate(Jsi_Channel chan, unsigned int len); /*STUB = 268*/
EXTERN Jsi_Wide Jsi_Rewind(Jsi_Channel chan); /*STUB = 269*/
EXTERN int Jsi_Flush(Jsi_Channel chan); /*STUB = 270*/
EXTERN int Jsi_Getc(Jsi_Channel chan); /*STUB = 271*/
EXTERN int Jsi_Printf(Jsi_Channel chan, const char *fmt, ...) /*STUB = 272*/ JSI_GCC_ATTRIBUTE((format (printf,2,3))); 
EXTERN int Jsi_Ungetc(Jsi_Channel chan, int ch); /*STUB = 273*/
EXTERN char* Jsi_Gets(Jsi_Channel chan, char *s, int size); /*STUB = 274*/
typedef int (Jsi_ScandirFilter)(const Jsi_Dirent *);
typedef int (Jsi_ScandirCompare)(const Jsi_Dirent **, const Jsi_Dirent**);
EXTERN int Jsi_Scandir(Jsi_Interp *interp, Jsi_Value *path, Jsi_Dirent ***namelist, Jsi_ScandirFilter *filter, Jsi_ScandirCompare *compare ); /*STUB = 275*/
EXTERN int Jsi_SetChannelOption(Jsi_Interp *interp, Jsi_Channel chan, const char *optionName, const char *newValue); /*STUB = 276*/
EXTERN char* Jsi_Realpath(Jsi_Interp *interp, Jsi_Value *path, char *newname); /*STUB = 277*/
EXTERN void Jsi_UNUSED_1(void); /*STUB = 278*/
EXTERN int Jsi_Readlink(Jsi_Interp *interp, Jsi_Value* path, char *ret, int len); /*STUB = 279*/
EXTERN Jsi_Channel Jsi_GetStdChannel(Jsi_Interp *interp, int id); /*STUB = 280*/
EXTERN int Jsi_IsNative(Jsi_Interp *interp, Jsi_Value* path); /*STUB = 281*/
EXTERN int Jsi_Link(Jsi_Interp *interp, Jsi_Value* src, Jsi_Value *dest, int typ); /*STUB = 291*/
EXTERN int Jsi_Chmod(Jsi_Interp *interp, Jsi_Value* path, int mode); /*STUB = 292*/

EXTERN int Jsi_StubLookup(Jsi_Interp *interp, const char *name, void **ptr); /*STUB = 283*/
EXTERN int Jsi_AddIndexFiles(Jsi_Interp *interp, const char *dir);  /*STUB = 284*/
EXTERN int Jsi_ExecZip(Jsi_Interp *interp, const char *exeFile, const char *mntDir, int *jsFound); /*STUB = 285*/
EXTERN int Jsi_SafeAccess(Jsi_Interp *interp, Jsi_Value* file, int toWrite); /*STUB = 290*/

#ifndef JSI_ZVFS_DIR
#define JSI_ZVFS_DIR "/zvfs"
#endif

#ifdef JSI_USE_STUBS
#ifndef JSISTUBCALL
#define JSISTUBCALL(ptr,func) ptr->func
#endif
#include "jsiStubs.h"
#else
#define JSI_EXTENSION_INI
#define Jsi_StubsInit(i,f) JSI_OK
#endif



#endif /* __JSI_H__ */
