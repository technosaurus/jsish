#ifdef HAVE_SQLITE
/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** A JSI (Javascript) Interface to SQLite.
*/
//#define _SQLITEN_(OMIT_TRACE)
//#define _SQLITEN_(OMIT_AUTHORIZATION)

typedef enum { SQLITE_SIG_DB = 0xbeefdead, SQLITE_SIG_FUNC, SQLITE_SIG_EXEC, SQLITE_SIG_STMT } Sqlite_Sig;

#define SQLSIGASSERT(s,n) assert(s->sig == SQLITE_SIG_##n)
#ifndef NDEBUG
#ifndef MEMCLEAR
#define MEMCLEAR(s) memset(s, 0, sizeof(*s));
#endif
#else
#define MEMCLEAR(s)
#endif
#ifndef JSI_DB_DSTRING_SIZE
#define JSI_DB_DSTRING_SIZE 2048
#endif

#include <errno.h>

//#define USE_SQLITE_V4
#ifdef USE_SQLITE_V4
#define _SQL_LITE_N_(nam) sqlite4##nam
#define _SQLITEN_(nam) SQLITE4_##nam
#define sqlite_uint64 sqlite4_uint64
#define sqlite_int64 sqlite4_int64
#define _SQLBIND_END_ ,NULL
#define _SQLITE_PENV_(n) n->pEnv,
#define OMIT_SQLITE_COLLATION
#define OMIT_SQLITE_HOOK_COMMANDS
#define SQLITE_OMIT_AUTHORIZATION
#define SQLITE_OMIT_LOAD_EXTENSION

#else

#define _SQL_LITE_N_(nam) sqlite3##nam
#define _SQLITEN_(nam) SQLITE_##nam
#define _SQLBIND_END_
#define _SQLITE_PENV_(n)
#endif
/*
** Some additional include files are needed if this file is not
** appended to the amalgamation.
*/
#ifdef __WIN32
#ifdef USE_SQLITE_V4
#include "../sqlite4/sqlite4.h"
#else
#include "../sqlite3/sqlite3.h"
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#ifndef SQLITE_AMALGAMATION
#ifdef USE_SQLITE_V4
#include "sqlite4.h"
#else
#include "sqlite3.h"
#endif
#endif
#include <ctype.h>
#include <stdio.h>

#ifndef JSI_AMALGAMATION
#ifdef VALUE_DEBUG
#include "jsiInt.h"
#else
#include "jsi.h"
JSI_EXTENSION_INI
#endif
#endif

#ifndef NUM_PREPARED_STMTS
#define NUM_PREPARED_STMTS 100
#endif
#ifndef MAX_PREPARED_STMTS
#define MAX_PREPARED_STMTS 10000
#endif

/*
** When JSI uses UTF-8 and SQLite is configured to use iso8859, then we
** have to do a translation when going between the two.  Set the
** UTF_TRANSLATION_NEEDED macro to indicate that we need to do
** this translation.
*/
#if defined(JSI_UTF_MAX) && !defined(SQLITE_UTF8)
# define UTF_TRANSLATION_NEEDED 1
#endif

/*
** New SQL functions can be created as JSI scripts.  Each such function
** is described by an instance of the following structure.
*/
typedef struct SqlFunc SqlFunc;
struct SqlFunc {
    Sqlite_Sig sig;
    Jsi_Interp  *interp;    /* The JSI interpret to execute the function */
    Jsi_Value   *tocall;    /* Callee */
    char        *pScript;   /* The char* representation of the script */
    Jsi_DString dScript;
    char        *zName;     /* Name of this function */
    SqlFunc     *pNext;     /* Next function on the list of them all */
};

/*
** New collation sequences function can be created as JSI scripts.  Each such
** function is described by an instance of the following structure.
*/
typedef struct SqlCollate SqlCollate;
struct SqlCollate {
    Sqlite_Sig sig;
    Jsi_Interp  *interp;   /* The JSI interpret to execute the function */
    Jsi_Value   *zScript;  /* The function to be run */
    SqlCollate  *pNext;    /* Next function on the list of them all */
};

/*
** Prepared statements are cached for faster execution.  Each prepared
** statement is described by an instance of the following structure.
*/
typedef struct SqlPreparedStmt SqlPreparedStmt;
struct SqlPreparedStmt {
    Sqlite_Sig sig;
    SqlPreparedStmt *pNext;  /* Next in linked list */
    SqlPreparedStmt *pPrev;  /* Previous on the list */
    _SQL_LITE_N_(_stmt)    *pStmt;  /* The prepared statement */
    int nSql;                /* chars in zSql[] */
    const char *zSql;        /* Text of the SQL statement */
    Jsi_HashEntry *entry;
    //int nParm;               /* Size of apParm array */
    //Jsi_Value **apParm;      /* Array of referenced object pointers */
};

static const char *execFmtStrs[] = {
    "rows", "arrays", "array1d", "list", "column", "json",
    "json2", "html", "csv", "insert", "line", "tabs", "none", NULL
};

typedef enum {
    EF_ROWS, EF_ARRAYS, EF_ARRAY1D, EF_LIST, EF_COLUMN, EF_JSON,
    EF_JSON2, EF_HTML, EF_CSV, EF_INSERT, EF_LINE, EF_TABS, EF_NONE
} Output_Mode;

typedef struct ExecFmt {
    Sqlite_Sig sig;
    Jsi_Value *callback;
    int limit;
    Output_Mode mode;
    char mapundef;
    char nocache;
    char headers;
    const char *separator;
    const char *nullvalue;
    const char *table;
    const char *cdata; // Name of cdata to use for query.
    Jsi_Value *width;
} ExecFmt;

static const char *mtxStrs[] = { "default", "none", "full", NULL };
typedef enum { MUTEX_DEFAULT, MUTEX_NONE, MUTEX_FULL } Mutex_Type;

static const char *trcModeStrs[] = {"eval", "delete", "prepare", NULL}; // Bit-set packed into an int.
enum {TMODE_EVAL=0x1, TMODE_DELETE=0x2, TMODE_PREPARE=0x4};

/*
** There is one instance of this structure for each SQLite database
** that has been opened by the SQLite JSI interface.
*/
struct Jsi_Db {
    Sqlite_Sig sig;
    _SQL_LITE_N_() *db;               /* The "real" database structure. MUST BE FIRST */
    Jsi_Interp *interp;        /* The interpreter used for this database */
    Jsi_Value *zBusy;               /* The busy callback routine */
    Jsi_Value *zCommit;             /* The commit hook callback routine */
    Jsi_Value *zTrace;              /* The trace callback routine */
    Jsi_Value *zProfile;            /* The profile callback routine */
    Jsi_Value *zProgress;           /* The progress callback routine */
    Jsi_Value *zAuth;               /* The authorization callback routine */
    int disableAuth;           /* Disable the authorizer if it exists */
    char *zNull;               /* Text to substitute for an SQL NULL value */
    SqlFunc *pFunc;            /* List of SQL functions */
    Jsi_Value *pUpdateHook;      /* Update hook script (if any) */
    Jsi_Value *pRollbackHook;    /* Rollback hook script (if any) */
    Jsi_Value *pWalHook;        /* Wal hook script (if any) */
    Jsi_Value *pUnlockNotify;    /* Unlock notify script (if any) */
    SqlCollate *pCollate;      /* List of SQL collation functions */
    int rc;                    /* Return code of most recent _SQL_LITE_N_(_exec)() */
    Jsi_Value *pCollateNeeded;   /* Collation needed script */
    SqlPreparedStmt *stmtList; /* List of prepared statements*/
    SqlPreparedStmt *stmtLast; /* Last statement in the list */
    Jsi_Hash *stmtHash;        /* Hash table for statements. */
    int maxStmts;               /* The next maximum number of stmtList */
    int nStmt;                 /* Number of statements in stmtList */
    /*IncrblobChannel *pIncrblob; * Linked list of open incrblob channels */
    Jsi_Hash *strKeyTbl;       /* Used with JSI_LITE_ONLY */
    char bindWarn;
    char forceInt;
    char readonly;
    char nocreate;
    int nStep, nSort;          /* Statistics for most recent operation */
    int nTransaction;          /* Number of nested [transaction] methods */
    int errorCnt;               /* Count of errors. */
    Jsi_Value *key;             /* Key, for codec. */
    Jsi_Value *vfs;
    int hasOpts;
    Jsi_Obj *userObjPtr;
    ExecFmt execOpts;
    int objId;
    Mutex_Type mutex;
    int debug;
    int trace;
    Jsi_DString name;
#ifdef USE_SQLITE_V4
    sqlite4_env *pEnv;
#endif
};

/*
** Structure used with dbEvalXXX() functions:
**
**   dbEvalInit(interp,)
**   dbEvalStep()
**   dbEvalFinalize()
**   dbEvalRowInfo()
**   dbEvalColumnValue()
*/
#define SQL_MAX_STATIC_TYPES 100
typedef struct DbEvalContext {
    Jsi_Db *jdb;                /* Database handle */
    Jsi_DString *dSql;               /* Object holding string zSql */
    const char *zSql;               /* Remaining SQL to execute */
    SqlPreparedStmt *pPreStmt;      /* Current statement */
    int nCol;                       /* Number of columns returned by pStmt */
    char **apColName;             /* Array of column names */
    int *apColType;
    char staticColNames[BUFSIZ];  /* Attempt to avoid mallocing space for name storage. */
    int staticColTypes[SQL_MAX_STATIC_TYPES];
    Jsi_Value *tocall;
    Jsi_Value *ret;
    /*OBS */
    Jsi_Value *pArray;              /* Name of array variable */
    Jsi_Value *pValVar;             /* Name of list for values. */
    int nocache;
} DbEvalContext;

#ifndef JSI_LITE_ONLY
static int IsNumArray(Jsi_Interp *interp, Jsi_Value *value);

static Jsi_OptionSpec ExecFmtOptions[] =
{
    JSI_OPT(FUNC,   ExecFmt, callback, .help="Function to call with each row result" ),
    JSI_OPT(BOOL,   ExecFmt, headers, .help="First row returned contains column labels"),
    JSI_OPT(INT,    ExecFmt, limit, .help="Maximum number of returned values"),
    JSI_OPT(BOOL,   ExecFmt, mapundef, .help="In variable bind, map an 'undefined' var to null"),
    JSI_OPT(CUSTOM, ExecFmt, mode, .custom=Jsi_Opt_SwitchEnum,  .data=execFmtStrs, .help="Set output mode of returned data"),
    JSI_OPT(BOOL,   ExecFmt, nocache, .help="Query is not to be cached"),
    JSI_OPT(STRKEY, ExecFmt, nullvalue, .help="Null string output (for non-json mode)"),
    JSI_OPT(STRKEY, ExecFmt, separator, .help="Separator string (for csv and text mode)"),
    JSI_OPT(STRKEY, ExecFmt, cdata, .help="Name of cdata to use"),
    JSI_OPT(STRKEY, ExecFmt, table, .help="Table name for mode=insert"),
    JSI_OPT(CUSTOM, ExecFmt, width, .custom=Jsi_Opt_SwitchValueVerify, .data=(void*)IsNumArray, .help="In column mode, set column widths"),
    JSI_OPT_END(ExecFmt)
};

static Jsi_CmdSpec sqliteCmds[];

#define IIOF .flags=JSI_OPT_INIT_ONLY
static Jsi_OptionSpec SqlOptions[] =
{
    JSI_OPT(INT,    Jsi_Db, debug, .help="Set debugging level"),
    JSI_OPT(BOOL,   Jsi_Db, bindWarn, .help="Treat failed variable binds as a warning", IIOF, .init="false"),
#ifdef SQLITE_HAS_CODEC
    JSI_OPT(VALUE,  Jsi_Db, key), .help="codec key", IIOF),
#endif
    JSI_OPT(INT,    Jsi_Db, errorCnt, .help="Count of errors"),
    JSI_OPT(CUSTOM, Jsi_Db, execOpts, .help="Default options for exec", .custom=Jsi_Opt_SwitchSuboption, .data=ExecFmtOptions),
    JSI_OPT(BOOL,   Jsi_Db, forceInt, .help="Bind float as int if possible"),
    JSI_OPT(INT,    Jsi_Db, maxStmts, .help="Max cache size for compiled statements"),
    JSI_OPT(CUSTOM, Jsi_Db, mutex,    .help="Mutex type to use", .custom=Jsi_Opt_SwitchEnum, .data=mtxStrs, IIOF),
    JSI_OPT(DSTRING,Jsi_Db, name,     .help="Name for this db handle"),
    JSI_OPT(BOOL,   Jsi_Db, readonly, .help="Database is readonly", IIOF, .init="false"),
    JSI_OPT(BOOL,   Jsi_Db, nocreate, .help="Database is must already exist", IIOF, .init="false"),
    JSI_OPT(VALUE,  Jsi_Db, vfs,      .help="VFS to use", IIOF),
    JSI_OPT(CUSTOM, Jsi_Db, trace, .custom=Jsi_Opt_SwitchBitset,  .data=trcModeStrs, .help="Enable trace for various operations"),
    JSI_OPT_END(Jsi_Db)
};
#endif

/* Start of code. */

/*
** Compute a string length that is limited to what can be stored in
** lower 30 bits of a 32-bit signed integer.
*/
static int strlen30(const char *z) {
    const char *z2 = z;
    while( *z2 ) {
        z2++;
    }
    return 0x3fffffff & (int)(z2 - z);
}

#define SQLITE_OMIT_INCRBLOB

static void dbEvalRowInfo(
    DbEvalContext *p,               /* Evaluation context */
    int *pnCol,                     /* OUT: Number of column names */
    char ***papColName,           /* OUT: Array of column names */
    int **papColType
) {
    /* Compute column names */
    // Jsi_Interp *interp = p->pDb->interp;

    if( 0==p->apColName ) {
        _SQL_LITE_N_(_stmt) *pStmt = p->pPreStmt->pStmt;
        int i;                        /* Iterator variable */
        int nCol;                     /* Number of columns returned by pStmt */
        char **apColName = 0;      /* Array of column names */
        int *apColType = 0;
        const char *zColName;         /* Column name */
        int numRid = 0;               /* Number of times rowid seen. */

        p->nCol = nCol = _SQL_LITE_N_(_column_count)(pStmt);
        if( nCol>0 && (papColName || p->pArray) ) {
            int cnLen = sizeof(char*)*nCol, cnStart = cnLen;
            for(i=0; i<nCol && cnLen<sizeof(p->staticColNames); i++)
                cnLen += Jsi_Strlen(_SQL_LITE_N_(_column_name)(pStmt,i))+1;
            if (cnLen>=sizeof(p->staticColNames)) {
                apColName = (char**)Jsi_Calloc(nCol, sizeof(char*) );
                cnStart = 0;
            } else {
                apColName = (char**)p->staticColNames;
            }
            if (papColType) {
                if (nCol < SQL_MAX_STATIC_TYPES)
                    apColType = p->staticColTypes;
                else
                    apColType = (int*)Jsi_Calloc(nCol, sizeof(int));
            }
            for(i=0; i<nCol; i++) {
                zColName = _SQL_LITE_N_(_column_name)(pStmt,i);
                if (cnStart==0)
                    apColName[i] = Jsi_Strdup(zColName);
                else {
                    apColName[i] = p->staticColNames+cnStart;
                    Jsi_Strcpy(apColName[i], zColName);
                    cnStart += Jsi_Strlen(zColName)+1;
                }
                if (apColType)
                    apColType[i] = _SQL_LITE_N_(_column_type)(pStmt,i);
                /* Check if rowid appears first, and more than once. */
                if ((i == 0 || numRid>0) &&
                        (zColName[0] == 'r' && Jsi_Strcmp(zColName,"rowid") == 0)) {
                    numRid++;
                }
            }
            /* Change first rowid to oid. */
            if (numRid > 1) {
                if (apColName != (char**)p->staticColNames) {
                    Jsi_Free(apColName[0]);
                    apColName[0] = Jsi_Strdup("oid");
                } else {
                    Jsi_Strcpy(apColName[0], "oid");
                }
            }
            p->apColName = apColName;
            p->apColType = apColType;
        }
    }
    if( papColName ) {
        *papColName = p->apColName;
    }
    if( papColType ) {
        *papColType = p->apColType;
    }
    if( pnCol ) {
        *pnCol = p->nCol;
    }
}

#ifndef JSI_LITE_ONLY
static int dbPrepareAndBind( Jsi_Db *pDb, char const *zIn, char const **pzOut,  SqlPreparedStmt **ppPreStmt );
#endif
static int dbEvalStepSub(DbEvalContext *p, int release);
static void dbReleaseColumnNames(DbEvalContext *p);
static void dbReleaseStmt( Jsi_Db *pDb, SqlPreparedStmt *pPreStmt, int discard );


/* Step statement. Return JSI_OK if there is a ROW result, JSI_BREAK if done, else JSI_ERROR. */
static int dbEvalStepSub(DbEvalContext *p, int release) {
    int rcs;
    Jsi_Db *jdb = p->jdb;
    Jsi_Interp *interp = jdb->interp;
    SqlPreparedStmt *pPreStmt = p->pPreStmt;
    SQLSIGASSERT(pPreStmt, STMT);
    _SQL_LITE_N_(_stmt) *pStmt = pPreStmt->pStmt;

    rcs = _SQL_LITE_N_(_step)(pStmt);
    if( rcs==_SQLITEN_(ROW) ) {
        return JSI_OK;
    }
    if( p->pArray ) {
        dbEvalRowInfo(p, 0, 0, 0);
    }
    rcs = _SQL_LITE_N_(_reset)(pStmt);

    jdb->nStep = _SQL_LITE_N_(_stmt_status)(pStmt,_SQLITEN_(STMTSTATUS_FULLSCAN_STEP),1);
    jdb->nSort = _SQL_LITE_N_(_stmt_status)(pStmt,_SQLITEN_(STMTSTATUS_SORT),1);
    if (release==0 && rcs==_SQLITEN_(OK))
        return JSI_BREAK;
    dbReleaseColumnNames(p);
    p->pPreStmt = 0;

    if( rcs!=_SQLITEN_(OK) ) {
        /* If a run-time error occurs, report the error and stop reading
        ** the SQL.  */
        Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(jdb->db));
        dbReleaseStmt(jdb, pPreStmt, 1);
        return JSI_ERROR;
    } else {
        dbReleaseStmt(jdb, pPreStmt, p->nocache);
    }
    return JSI_BREAK;
}

static void dbEvalInit(
    Jsi_Interp *interp,
    DbEvalContext *p,               /* Pointer to structure to initialize */
    Jsi_Db *jdb,                  /* Database handle */
    const char* zSql,                /* Value containing SQL script */
    Jsi_DString *dStr,
    Jsi_Obj *pArray,                /* Name of Jsi array to set (*) element of */
    Jsi_Obj *pValVar                  /* Name element in array for list. */
) {
    memset(p, 0, sizeof(DbEvalContext));
    p->dSql = dStr;
    p->zSql = Jsi_DSAppend(p->dSql, zSql?zSql:"", NULL);
    p->jdb = jdb;
}

/*
** Release a statement reference obtained by calling dbPrepareAndBind().
** There should be exactly one call to this function for each call to
** dbPrepareAndBind().
**
** If the discard parameter is non-zero, then the statement is deleted
** immediately. Otherwise it is added to the LRU list and may be returned
** by a subsequent call to dbPrepareAndBind().
*/
static void dbReleaseStmt(
    Jsi_Db *pDb,                  /* Database handle */
    SqlPreparedStmt *pPreStmt,      /* Prepared statement handle to release */
    int discard                     /* True to delete (not cache) the pPreStmt */
) {
    //int i;
    //Jsi_Interp *interp = pDb->interp;

    /* Free the bound string and blob parameters */
    /*for(i=0; i<pPreStmt->nParm; i++) {
        Jsi_DecrRefCount(interp, pPreStmt->apParm[i]);
    }*/
    //pPreStmt->nParm = 0;

    if( pDb->maxStmts<=0 || discard ) {
        /* If the cache is turned off, deallocated the statement */
        _SQL_LITE_N_(_finalize)(pPreStmt->pStmt);
        Jsi_HashEntryDelete(pPreStmt->entry);
        Jsi_Free((char *)pPreStmt);
    } else {
        /* Add the prepared statement to the beginning of the cache list. */
        pPreStmt->pNext = pDb->stmtList;
        pPreStmt->pPrev = 0;
        if( pDb->stmtList ) {
            pDb->stmtList->pPrev = pPreStmt;
        }
        pDb->stmtList = pPreStmt;
        if( pDb->stmtLast==0 ) {
            assert( pDb->nStmt==0 );
            pDb->stmtLast = pPreStmt;
        } else {
            assert( pDb->nStmt>0 );
        }
        pDb->nStmt++;

        /* If we have too many statement in cache, remove the surplus from
        ** the end of the cache list.  */
        while( pDb->nStmt>pDb->maxStmts ) {
            _SQL_LITE_N_(_finalize)(pDb->stmtLast->pStmt);
            pDb->stmtLast = pDb->stmtLast->pPrev;
            Jsi_HashEntryDelete(pDb->stmtLast->pNext->entry);
            Jsi_Free((char*)pDb->stmtLast->pNext);
            pDb->stmtLast->pNext = 0;
            pDb->nStmt--;
        }
    }
}

/*
** Release any cache of column names currently held as part of
** the DbEvalContext structure passed as the first argument.
*/
static void dbReleaseColumnNames(DbEvalContext *p) {
    //Jsi_Interp *interp = p->pDb->interp;

    if( p->apColName && p->apColName != (char**)p->staticColNames) {
        int i;
        for(i=0; i<p->nCol; i++) {
            Jsi_Free(p->apColName[i]);
        }
        Jsi_Free((char *)p->apColName);
    }
    if( p->apColType && p->apColType != p->staticColTypes) {
        Jsi_Free((char *)p->apColType);
    }
    p->apColName = NULL;
    p->apColType = NULL;
    p->nCol = 0;
}

sqlite_uint64 DbLastInsertRowid(Jsi_Db* jdb)
{
#ifdef USE_SQLITE_V4
#warning "unimplemented last insert rowid"
    return 0;
#else
    return _SQL_LITE_N_(_last_insert_rowid)(jdb->db);
#endif
}

#ifdef USE_SQLITE_V4
static int sqlite4_prepare_v2(sqlite4 *db, const char *zSql, int nByte, sqlite4_stmt **ppStmt, const char **pzTail ) {
    int nUsed, rc;
    rc = sqlite4_prepare(db, zSql, nByte, ppStmt, &nUsed);
    if (rc == SQLITE4_OK)
        *pzTail = (zSql+nUsed);
    return rc;
}

#define sqlite4_open_v2(filename, ppDb, flags, zVfs) sqlite4_open(db->pEnv, filename, ppDb, flags, NULL) 

#define sqlite4_sql sqlite4_stmt_sql

#endif

/*
** Search the cache for a prepared-statement object that implements the
** first SQL statement in the buffer pointed to by parameter zIn. If
** no such prepared-statement can be found, allocate and prepare a new
** one. In either case, bind the current values of the relevant Jsi
** variables to any $var, :var or @var variables in the statement. Before
** returning, set *ppPreStmt to point to the prepared-statement object.
**
** Output parameter *pzOut is set to point to the next SQL statement in
** buffer zIn, or to the '\0' byte at the end of zIn if there is no
** next statement.
**
** If successful, JSI_OK is returned. Otherwise, JSI_ERROR is returned
** and an error message loaded into interpreter pDb->interp.
*/
static int dbPrepareStmt(
    Jsi_Db *pDb,                  /* Database object */
    char const *zIn,                /* SQL to compile */
    char const **pzOut,             /* OUT: Pointer to next SQL statement */
    SqlPreparedStmt **ppPreStmt     /* OUT: Object used to cache statement */
) {
    const char *zSql = zIn;         /* Pointer to first SQL statement in zIn */
    _SQL_LITE_N_(_stmt) *pStmt;            /* Prepared statement object */
    SqlPreparedStmt *pPreStmt = 0;  /* Pointer to cached statement */
   // int nSql;                       /* Length of zSql in bytes */
    //int nVar;                       /* Number of variables in statement */
    //int iParm = 0;                  /* Next free entry in apParm */
    int rc = JSI_OK;
    Jsi_Interp *interp = pDb->interp;

    *ppPreStmt = 0;

    /* Trim spaces from the start of zSql and calculate the remaining length. */
    while( isspace(zSql[0]) ) {
        zSql++;
    }
    //nSql = strlen30(zSql);
    Jsi_HashEntry *entry = Jsi_HashEntryFind(pDb->stmtHash, zSql);
    if (entry && ((pPreStmt = Jsi_HashValueGet(entry)))) {
        
        if (pDb->trace & TMODE_PREPARE)
            fprintf(stderr, "TRACE: prepare cache-hit: %s\n", zSql);
        pStmt = pPreStmt->pStmt;
        *pzOut = &zSql[pPreStmt->nSql];

        /* When a prepared statement is found, unlink it from the
        ** cache list.  It will later be added back to the beginning
        ** of the cache list in order to implement LRU replacement.
        */
        if( pPreStmt->pPrev ) {
            pPreStmt->pPrev->pNext = pPreStmt->pNext;
        } else {
            pDb->stmtList = pPreStmt->pNext;
        }
        if( pPreStmt->pNext ) {
            pPreStmt->pNext->pPrev = pPreStmt->pPrev;
        } else {
            pDb->stmtLast = pPreStmt->pPrev;
        }
        pDb->nStmt--;
    }

    /* If no prepared statement was found. Compile the SQL text. Also allocate
    ** a new SqlPreparedStmt structure.  */
    if( pPreStmt==0 ) {
        int nByte;

        if( _SQLITEN_(OK)!=_SQL_LITE_N_(_prepare_v2)(pDb->db, zSql, -1, &pStmt, pzOut) )
        {
            Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
            return JSI_ERROR;
        }
        if( pStmt==0 ) {
            if( _SQLITEN_(OK)!=_SQL_LITE_N_(_errcode)(pDb->db) ) {
                /* A compile-time error in the statement. */
                Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
                return JSI_ERROR;
            } else {
                /* The statement was a no-op.  Continue to the next statement
                ** in the SQL string.
                */
                return JSI_OK;
            }
        }

        if (pDb->trace & TMODE_PREPARE)
            fprintf(stderr, "TRACE: prepare new: %s\n", zSql);
        assert( pPreStmt==0 );
        //nVar = _SQL_LITE_N_(_bind_parameter_count)(pStmt);
        nByte = sizeof(SqlPreparedStmt); // + nVar*sizeof(Jsi_Obj *);
        pPreStmt = (SqlPreparedStmt*)Jsi_Calloc(1, nByte);
        pPreStmt->sig = SQLITE_SIG_STMT;

        pPreStmt->pStmt = pStmt;
        pPreStmt->nSql = (*pzOut - zSql);
        pPreStmt->zSql = _SQL_LITE_N_(_sql)(pStmt);
        int isNew = 0;
        pPreStmt->entry = Jsi_HashEntryCreate(pDb->stmtHash, zSql, &isNew);
        if (!isNew)
            fprintf(stderr, "sqlite dup stmt entry");
        Jsi_HashValueSet(pPreStmt->entry, pPreStmt);
            
        //pPreStmt->apParm = (Jsi_Value **)&pPreStmt[1];
    }
    assert( pPreStmt );
    assert( strlen30(pPreStmt->zSql)==pPreStmt->nSql );
    assert( 0==memcmp(pPreStmt->zSql, zSql, pPreStmt->nSql) );
    *ppPreStmt = pPreStmt;
    //pPreStmt->nParm = iParm; 
    return rc;
}


#ifndef JSI_LITE_ONLY

/*
** Return one of JSI_OK, JSI_BREAK or JSI_ERROR. If JSI_ERROR is
** returned, then an error message is stored in the interpreter before
** returning.
**
** A return value of JSI_OK means there is a row of data available. The
** data may be accessed using dbEvalRowInfo() and dbEvalColumnValue(). This
** is analogous to a return of _SQLITEN_(ROW) from _SQL_LITE_N_(_step)(). If JSI_BREAK
** is returned, then the SQL script has finished executing and there are
** no further rows available. This is similar to _SQLITEN_(DONE).
*/
static int dbEvalStep(DbEvalContext *p) {
    while( p->zSql[0] || p->pPreStmt ) {
        int rc;
        if( p->pPreStmt==0 ) {
            rc = dbPrepareAndBind(p->jdb, p->zSql, &p->zSql, &p->pPreStmt);
            if( rc!=JSI_OK ) return rc;
        }
        rc = dbEvalStepSub(p, 1);
        if (rc != JSI_BREAK)
            return rc;
    }
    
    /* Finished */
    return JSI_BREAK;
}
static int dbBindStmt(
    Jsi_Db *pDb,               /* Database object */
    _SQL_LITE_N_(_stmt) *pStmt     /* Object used to cache statement */
)
{
    Jsi_Interp *interp = pDb->interp;
    int i, rc = JSI_OK;
 
    int nVar = _SQL_LITE_N_(_bind_parameter_count)(pStmt);
   /* Bind values to parameters that begin with @, $ or : */
    for(i=1; i<=nVar; i++) {
        const char *zVar = _SQL_LITE_N_(_bind_parameter_name)(pStmt, i);
        if( zVar!=0 && (zVar[0]=='$' || zVar[0]==':' || zVar[0]=='@') ) {
            int zvLen = strlen(zVar);
            char *zcp;
            Jsi_Value *pv = NULL;
            if (zVar[0] =='$' && ((zcp = strchr(zVar,'('))) && zVar[zvLen-1] == ')')
            {
                Jsi_DString vStr;
                Jsi_DSInit(&vStr);
                Jsi_DSAppendLen(&vStr, zVar+1, (zcp-zVar-1));
                Jsi_DSAppendLen(&vStr, "[", 1);
                Jsi_DSAppendLen(&vStr, zcp+1, strlen(zcp)-2);
                Jsi_DSAppendLen(&vStr, "]", 1);
                pv = Jsi_NameLookup(interp, Jsi_DSValue(&vStr));
                Jsi_DSFree(&vStr);
            } else
                pv = Jsi_VarLookup(interp, &zVar[1]);
            if(!pv ) {
                if (!pDb->bindWarn) {
                    Jsi_LogError("unknown bind param: %s", zVar);
                    rc = JSI_ERROR;
                    break;
                } else
                    Jsi_LogWarn("unknown bind param: %s", zVar);
            } else {
                int n;
                if (Jsi_ValueIsBoolean(interp, pv)) {
                    Jsi_GetBoolFromValue(interp, pv, &n);
                    _SQL_LITE_N_(_bind_int)(pStmt, i, n);
                } else if (Jsi_ValueIsNumber(interp, pv)) {
                    Jsi_Number r;
                    Jsi_Wide wv;
                    Jsi_GetNumberFromValue(interp, pv, &r);
                    wv = (Jsi_Wide)r;
                    if (pDb->forceInt && (((Jsi_Number)wv)-r)==0)
                        _SQL_LITE_N_(_bind_int64)(pStmt, i,wv);
                    else
                        _SQL_LITE_N_(_bind_double)(pStmt, i,(double)r);
                } else if (Jsi_ValueIsNull(interp, pv) || (Jsi_ValueIsUndef(interp, pv) && pDb->execOpts.mapundef)) {
                    _SQL_LITE_N_(_bind_null)(pStmt, i);
                } else if (Jsi_ValueIsString(interp, pv)) {
                    const char *sstr = Jsi_ValueGetStringLen(interp, pv, &n);
                    if (zVar[0] ==':')
                        _SQL_LITE_N_(_bind_blob)(pStmt, i, (char *)sstr, n, _SQLITEN_(STATIC) _SQLBIND_END_);
                    else
                        _SQL_LITE_N_(_bind_text)(pStmt, i, (char *)sstr, n, _SQLITEN_(STATIC) _SQLBIND_END_);
                } else {
                    if (!pDb->bindWarn) {
                        Jsi_LogError("bind param must be string/number/bool/null: %s", zVar);
                        rc = JSI_ERROR;
                        break;
                    } else
                        Jsi_LogWarn("bind param must be string/number/bool/null: %s", zVar);
                    _SQL_LITE_N_(_bind_null)(pStmt, i);
                }
            }
        }
    }
    return rc;
}

static int dbPrepareAndBind(
    Jsi_Db *pDb,                  /* Database object */
    char const *zIn,                /* SQL to compile */
    char const **pzOut,             /* OUT: Pointer to next SQL statement */
    SqlPreparedStmt **ppPreStmt     /* OUT: Object used to cache statement */
) {
    if (dbPrepareStmt(pDb, zIn, pzOut, ppPreStmt) != JSI_OK)
        return JSI_ERROR;
    return dbBindStmt(pDb, (*ppPreStmt)->pStmt);
}
#endif

/*
** Free all resources currently held by the DbEvalContext structure passed
** as the first argument. There should be exactly one call to this function
** for each call to dbEvalInit(interp,).
*/
static void dbEvalFinalize(DbEvalContext *p) {
//  Jsi_Interp *interp = p->pDb->interp;

    if( p->pPreStmt ) {
        _SQL_LITE_N_(_reset)(p->pPreStmt->pStmt);
        dbReleaseStmt(p->jdb, p->pPreStmt, p->nocache);
        p->pPreStmt = 0;
    }
    Jsi_DSFree(p->dSql);
    dbReleaseColumnNames(p);
}


#ifndef JSI_LITE_ONLY

static void ValueCopy(Jsi_Interp *interp, Jsi_Value **to, Jsi_Value *from );
static int sqliteObjFree(Jsi_Interp *interp, void *data);
static int  sqliteObjEqual(void *data1, void *data2);
static int  sqliteObjIsTrue(void *data);

static Jsi_UserObjReg sqliteobject = {
    .name   = "Sqlite",
    .spec   = sqliteCmds,
    .freefun= sqliteObjFree,
    .istrue = sqliteObjIsTrue,
    .isequ  = sqliteObjEqual
};


static void ValueCopy(Jsi_Interp *interp, Jsi_Value **to, Jsi_Value *from ) {
    Jsi_ValueCopy(interp, *to, from);
    Jsi_IncrRefCount(interp, *to);
}

static int IsNumArray(Jsi_Interp *interp, Jsi_Value *value)
{
    if (!Jsi_ValueIsArray(interp, value)) {
        Jsi_LogError("expected array of numbers");
        return JSI_ERROR;
    }
    int i, argc = Jsi_ValueGetLength(interp, value);
    for (i=0; i<argc; i++) {
        Jsi_Value *v = Jsi_ValueArrayIndex(interp, value, i);
        if (!Jsi_ValueIsNumber(interp, v)) {
            Jsi_LogError("expected array of numbers");
            return JSI_ERROR;
        }
    }
    return JSI_OK;
}


/*
** Finalize and free a list of prepared statements
*/
static void flushStmtCache( Jsi_Db *pDb ) {
    SqlPreparedStmt *pPreStmt;

    while(  pDb->stmtList ) {
        _SQL_LITE_N_(_finalize)( pDb->stmtList->pStmt );
        pPreStmt = pDb->stmtList;
        pDb->stmtList = pDb->stmtList->pNext;
        Jsi_Free( (char*)pPreStmt );
    }
    pDb->nStmt = 0;
    pDb->stmtLast = 0;
}

static void DbClose(_SQL_LITE_N_() *db) {
#ifdef USE_SQLITE_V4
        _SQL_LITE_N_(_close)(db, 0);
#else
        _SQL_LITE_N_(_close)(db);
#endif
}

/*
** JSI calls this procedure when an _SQL_LITE_N_() database command is
** deleted.
*/
static void DbDeleteCmd(Jsi_Db *pDb)
{
    Jsi_Interp *interp = pDb->interp;
    if (pDb->trace & TMODE_DELETE)
        fprintf(stderr, "TRACE: delete\n");
    flushStmtCache(pDb);
    if (pDb->stmtHash)
        Jsi_HashDelete(pDb->stmtHash);
    //closeIncrblobChannels(pDb);
    if (pDb->db) {
        DbClose(pDb->db);
    }
    while( pDb->pFunc ) {
        SqlFunc *pFunc = pDb->pFunc;
        pDb->pFunc = pFunc->pNext;
        Jsi_DSFree(&pFunc->dScript);
        Jsi_Free((char*)pFunc);
    }
    while( pDb->pCollate ) {
        SqlCollate *pCollate = pDb->pCollate;
        pDb->pCollate = pCollate->pNext;
        Jsi_Free((char*)pCollate);
    }
    if( pDb->zBusy ) {
        Jsi_DecrRefCount(interp, pDb->zBusy);
    }
    if( pDb->zTrace ) {
        Jsi_DecrRefCount(interp, pDb->zTrace);
    }
    if( pDb->zProfile ) {
        Jsi_DecrRefCount(interp, pDb->zProfile);
    }
    if( pDb->zAuth ) {
        Jsi_DecrRefCount(interp, pDb->zAuth);
    }
    if( pDb->zNull ) {
        Jsi_Free(pDb->zNull);
    }
    if( pDb->pUpdateHook ) {
        Jsi_DecrRefCount(interp, pDb->pUpdateHook);
    }
    if( pDb->pRollbackHook ) {
        Jsi_DecrRefCount(interp, pDb->pRollbackHook);
    }
    if( pDb->pCollateNeeded ) {
        Jsi_DecrRefCount(interp, pDb->pCollateNeeded);
    }
    Jsi_OptionsFree(interp, SqlOptions, pDb, 0);
}

#ifndef OMIT_SQLITE_HOOK_COMMANDS

static int getIntBool(Jsi_Interp *interp, Jsi_Value* v)
{
    if (Jsi_ValueIsNumber(interp, v)) {
        double d;
        Jsi_ValueGetNumber(interp, v, &d);
        return (int)d;
    }
    if (Jsi_ValueIsBoolean(interp, v)) {
        int n;
        Jsi_ValueGetBoolean(interp, v, &n);
        return n;
    }
    return 0;
}


/*
** This routine is called when a database file is locked while trying
** to execute SQL.
*/
static int DbBusyHandler(void *cd, int nTries) {
    int rc;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Value *vpargs, *items[3] = {}, *ret;
    Jsi_Interp *interp = pDb->interp;

    items[0] = Jsi_ValueMakeNumber(interp, NULL, (Jsi_Number)nTries);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 1));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->zBusy, vpargs, &ret, NULL);
    if( JSI_OK!=rc ) {
        pDb->errorCnt++;
        rc = 1;
    } else
        rc = getIntBool(interp, ret);
    Jsi_DecrRefCount(interp, vpargs);
    Jsi_DecrRefCount(interp, ret);
    return rc;
}

/*
** This routine is invoked as the 'progress callback' for the database.
*/
static int DbProgressHandler(void *cd) {
    int rc = 0;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Interp *interp = pDb->interp;
    Jsi_Value *ret = Jsi_ValueNew1(interp);
    assert(pDb->zProgress);
    if( JSI_OK!=Jsi_FunctionInvoke(pDb->interp, pDb->zProgress, NULL, &ret, NULL) ) {
        pDb->errorCnt++;
        rc = 1;
    } else
        rc = getIntBool(interp, ret);
    Jsi_DecrRefCount(interp, ret);

    return rc;
}
#endif

#ifndef SQLITE_OMIT_TRACE
/*
** This routine is called by the SQLite trace handler whenever a new
** block of SQL is executed.  The JSI script in pDb->zTrace is executed.
*/
static void DbTraceHandler(void *cd, const char *zSql)
{
    int rc;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Value *vpargs, *items[2] = {}, *ret;
    Jsi_Interp *interp = pDb->interp;
    items[0] = Jsi_ValueMakeStringDup(interp, NULL, zSql);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 1));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->zTrace, vpargs, &ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    Jsi_DecrRefCount(interp, ret);
    if (rc != JSI_OK)
        pDb->errorCnt++;
}
#endif

#ifndef SQLITEN_OMIT_TRACE
/*
** This routine is called by the SQLite profile handler after a statement
** SQL has executed.  The JSI script in pDb->zProfile is evaluated.
*/
static void DbProfileHandler(void *cd, const char *zSql, sqlite_uint64 tm) {
    int rc;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Interp *interp = pDb->interp;
    Jsi_Value *vpargs, *items[3] = {}, *ret;

    items[0] = Jsi_ValueMakeStringDup(interp, NULL, zSql);
    items[1] = Jsi_ValueMakeNumber(interp, NULL, (Jsi_Number)tm);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 2));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->zProfile, vpargs, &ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    Jsi_DecrRefCount(interp, ret);
    if (rc != JSI_OK)
        pDb->errorCnt++;
}
#endif

#ifndef OMIT_SQLITE_HOOK_COMMANDS
/*
** This routine is called when a transaction is committed.  The
** JSI script in pDb->zCommit is executed.  If it returns non-zero or
** if it throws an exception, the transaction is rolled back instead
** of being committed.
*/
static int DbCommitHandler(void *cd) {
    int rc = 0;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Value *ret = Jsi_ValueNew1(pDb->interp);

    assert(pDb->zCommit);
    if( JSI_OK!=Jsi_FunctionInvoke(pDb->interp, pDb->zCommit, NULL, &ret, NULL) ) {
        pDb->errorCnt++;
        rc = 1;
    } else
        rc = getIntBool(pDb->interp, ret);
    Jsi_DecrRefCount(pDb->interp, ret);

    return rc;
}

/*
** This procedure handles wal_hook callbacks.
*/
static int DbWalHandler( void *cd, _SQL_LITE_N_() *db, const char *zDb, int nEntry ){
    int rc;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Interp *interp = pDb->interp;
    Jsi_Value *vpargs, *items[3] = {}, *ret;

    items[0] = Jsi_ValueMakeStringDup(interp, NULL, zDb);
    items[1] = Jsi_ValueMakeNumber(interp, NULL, (Jsi_Number)nEntry);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 2));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->pWalHook, vpargs, &ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    if (rc != JSI_OK) {
        pDb->errorCnt++;
        rc = 1;
    } else
        rc = getIntBool(pDb->interp, ret);
    Jsi_DecrRefCount(interp, ret);
    return rc;
}
 
static void DbRollbackHandler(void *cd) {
    Jsi_Db *pDb = (Jsi_Db*)cd;
    assert(pDb->pRollbackHook);
    if( JSI_OK!=Jsi_FunctionInvoke(pDb->interp, pDb->pRollbackHook, NULL, NULL, NULL) ) {
        pDb->errorCnt++;
    }
}
#endif

#if defined(SQLITE_TEST) && defined(SQLITE_ENABLE_UNLOCK_NOTIFY)
static void setTestUnlockNotifyVars(Jsi_Interp *interp, int iArg, int nArg) {
    char zBuf[64];
    sprintf(zBuf, "%d", iArg);
    Jsi_SetVar(interp, "sqlite_unlock_notify_arg", zBuf, JSI_GLOBAL_ONLY);
    sprintf(zBuf, "%d", nArg);
    Jsi_SetVar(interp, "sqlite_unlock_notify_argcount", zBuf, JSI_GLOBAL_ONLY);
}
#else
# define setTestUnlockNotifyVars(x,y,z)
#endif

#ifdef SQLITE_ENABLE_UNLOCK_NOTIFY
//TODO: unimpl
static void DbUnlockNotify(void **apArg, int nArg) {
    int i;
    for(i=0; i<nArg; i++) {
        const int flags = (JSI_EVAL_GLOBAL);
        Jsi_Db *pDb = (Jsi_Db *)apArg[i];
        setTestUnlockNotifyVars(pDb->interp, i, nArg);
        assert( pDb->pUnlockNotify);
        Jsi_EvalObjEx(pDb->interp, pDb->pUnlockNotify, flags);
        Jsi_ObjDecrRefCount(pDb->pUnlockNotify);
        pDb->pUnlockNotify = 0;
    }
}
#endif

#ifndef OMIT_SQLITE_HOOK_COMMANDS
static void DbUpdateHandler(
    void *p,
    int op,
    const char *zDb,
    const char *zTbl,
    sqlite_int64 rowid
) {
    Jsi_Db *pDb = (Jsi_Db *)p;
    Jsi_Interp *interp = pDb->interp;
    int rc, i = 0;
    Jsi_Value *vpargs, *items[5] = {}, *ret;

    assert( pDb->pUpdateHook );
    assert( op==_SQLITEN_(INSERT) || op==_SQLITEN_(UPDATE) || op==_SQLITEN_(DELETE) );
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, (op==_SQLITEN_(INSERT))?"INSERT":(op==_SQLITEN_(UPDATE))?"UPDATE":"DELETE");
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zDb);
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zTbl);
    items[i++] = Jsi_ValueMakeNumber(interp, NULL, (Jsi_Number)rowid);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 1));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->pUpdateHook, vpargs, &ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    Jsi_DecrRefCount(interp, ret);
    if (rc != JSI_OK)
        pDb->errorCnt++;
}
#endif

#ifndef OMIT_SQLITE_COLLATION

static void jsiCollateNeeded(
    void *cd,
    _SQL_LITE_N_() *db,
    int enc,
    const char *zName
) {
    int rc;
    Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Interp *interp = pDb->interp;
    Jsi_Value *vpargs, *items[2], *ret;
    items[0] = Jsi_ValueMakeStringDup(interp, NULL, zName);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 1));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->pCollateNeeded, vpargs,& ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    Jsi_DecrRefCount(interp, ret);
    if (rc != JSI_OK)
        pDb->errorCnt++;

}

/*
** This routine is called to evaluate an SQL collation function implemented
** using JSI script.
*/
static int jsiSqlCollate(
    void *pCtx,
    int nA,
    const void *zA,
    int nB,
    const void *zB
) {
    SqlCollate *p = (SqlCollate *)pCtx;
    Jsi_Interp *interp = p->interp;

    int rc;
    //Jsi_Db *pDb = (Jsi_Db*)cd;
    Jsi_Value *vpargs, *items[3], *ret;

    items[0] = Jsi_ValueMakeStringDup(interp, NULL, zA);
    items[1] = Jsi_ValueMakeStringDup(interp, NULL, zB);
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 2));
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, p->zScript, vpargs, &ret, NULL);
    if( JSI_OK!=rc ) {
        //pDb->errorCnt++;
        rc = 0;
    } else
        rc = getIntBool(interp, ret);
    Jsi_DecrRefCount(interp, vpargs);
    Jsi_DecrRefCount(interp, ret);
    return rc;
}
#endif

static Jsi_Value* dbGetValueGet(Jsi_Interp *interp, _SQL_LITE_N_(_value) *pIn)
{
    switch (_SQL_LITE_N_(_value_type)(pIn)) {
    case _SQLITEN_(BLOB): {
        int bytes;
#ifdef USE_SQLITE_V4
        const char *zBlob = _SQL_LITE_N_(_value_blob)(pIn, &bytes);
#else
        bytes = _SQL_LITE_N_(_value_bytes)(pIn);
        const char *zBlob = _SQL_LITE_N_(_value_blob)(pIn);
#endif
        if( bytes == 0 || !zBlob ) {
            return Jsi_ValueMakeNull(interp, NULL);
        }
        return Jsi_ValueMakeBlob(interp, NULL, (unsigned char*)zBlob, bytes);
    }
    case _SQLITEN_(INTEGER): {
        sqlite_int64 v = _SQL_LITE_N_(_value_int64)(pIn);
        if( v>=-2147483647 && v<=2147483647 ) {
            return Jsi_ValueMakeNumber(interp, NULL, v);
        } else {
            return Jsi_ValueMakeNumber(interp, NULL, v);
        }
    }
    case _SQLITEN_(FLOAT): {
        return Jsi_ValueMakeNumber(interp, NULL, (Jsi_Number)_SQL_LITE_N_(_value_double)(pIn));
    }
    case _SQLITEN_(NULL): {
        return Jsi_ValueMakeNull(interp, NULL);
    }
    default:
#ifdef USE_SQLITE_V4
        return Jsi_ValueMakeStringDup(interp, NULL, (char *)_SQL_LITE_N_(_value_text)(pIn, 0));
#else
        return Jsi_ValueMakeStringDup(interp, NULL, (char *)_SQL_LITE_N_(_value_text)(pIn));
#endif
    }
    return Jsi_ValueNew(interp);
}

static void jsiSqlFunc(_SQL_LITE_N_(_context) *context, int argc, _SQL_LITE_N_(_value)**argv) {
#ifdef USE_SQLITE_V4
    SqlFunc *p = _SQL_LITE_N_(_context_appdata)(context);
#else
    SqlFunc *p = _SQL_LITE_N_(_user_data)(context);
#endif
    int i;
    int rc;
    Jsi_Interp *interp = p->interp;
    Jsi_Value *vpargs, *itemsStatic[100], **items = itemsStatic, *ret;
    if (argc>100)
        items = Jsi_Calloc(argc, sizeof(Jsi_Value*));

    for(i=0; i<argc; i++) {
        items[i] = dbGetValueGet(interp, argv[i]);
    }
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, argc));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew1(interp);
    rc = Jsi_FunctionInvoke(interp, p->tocall, vpargs, &ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);
    if (items != itemsStatic)
        Jsi_Free(items);

    if( rc != JSI_OK) {
        char buf[250];
        snprintf(buf, sizeof(buf), "error in function: %.200s", p->zName);
        _SQL_LITE_N_(_result_error)(context, buf, -1);

    } else if (Jsi_ValueIsBoolean(interp, ret)) {
        Jsi_GetBoolFromValue(interp, ret, &i);
        _SQL_LITE_N_(_result_int)(context, i);
    } else if (Jsi_ValueIsNumber(interp, ret)) {
        Jsi_Number d;
        // if (Jsi_GetIntFromValueBase(interp, ret, &i, 0, JSI_NO_ERRMSG);
        // _SQL_LITE_N_(_result_int64)(context, v);
        Jsi_GetNumberFromValue(interp, ret, &d);
        _SQL_LITE_N_(_result_double)(context, (double)d);
    } else {
        const char * data;
        if (!(data = Jsi_ValueGetStringLen(interp, ret, &i))) {
            //TODO: handle objects???
            data = Jsi_ValueToString(interp, ret);
            i = Jsi_Strlen(data);
        }
        _SQL_LITE_N_(_result_text)(context, (char *)data, i, _SQLITEN_(TRANSIENT) _SQLBIND_END_);
    }
    Jsi_DecrRefCount(interp, ret);
}

#ifndef SQLITE_OMIT_AUTHORIZATION
/*
** This is the authentication function.  It appends the authentication
** type code and the two arguments to zCmd[] then invokes the result
** on the interpreter.  The reply is examined to determine if the
** authentication fails or succeeds.
*/
static int auth_callback(
    void *pArg,
    int code,
    const char *zArg1,
    const char *zArg2,
    const char *zArg3,
    const char *zArg4
) {
    char *zCode;
    int rc;
    const char *zReply;
    Jsi_Db *pDb = (Jsi_Db*)pArg;
    Jsi_Interp *interp = pDb->interp;
    if( pDb->disableAuth ) return _SQLITEN_(OK);

    switch( code ) {
    case _SQLITEN_(COPY)              :
        zCode="_SQLITEN_(COPY)";
        break;
    case _SQLITEN_(CREATE_INDEX)      :
        zCode="_SQLITEN_(CREATE_INDEX)";
        break;
    case _SQLITEN_(CREATE_TABLE)      :
        zCode="_SQLITEN_(CREATE_TABLE)";
        break;
    case _SQLITEN_(CREATE_TEMP_INDEX) :
        zCode="_SQLITEN_(CREATE_TEMP_INDEX)";
        break;
    case _SQLITEN_(CREATE_TEMP_TABLE) :
        zCode="_SQLITEN_(CREATE_TEMP_TABLE)";
        break;
    case _SQLITEN_(CREATE_TEMP_TRIGGER):
        zCode="_SQLITEN_(CREATE_TEMP_TRIGGER)";
        break;
    case _SQLITEN_(CREATE_TEMP_VIEW)  :
        zCode="_SQLITEN_(CREATE_TEMP_VIEW)";
        break;
    case _SQLITEN_(CREATE_TRIGGER)    :
        zCode="_SQLITEN_(CREATE_TRIGGER)";
        break;
    case _SQLITEN_(CREATE_VIEW)       :
        zCode="_SQLITEN_(CREATE_VIEW)";
        break;
    case _SQLITEN_(DELETE)            :
        zCode="_SQLITEN_(DELETE)";
        break;
    case _SQLITEN_(DROP_INDEX)        :
        zCode="_SQLITEN_(DROP_INDEX)";
        break;
    case _SQLITEN_(DROP_TABLE)        :
        zCode="_SQLITEN_(DROP_TABLE)";
        break;
    case _SQLITEN_(DROP_TEMP_INDEX)   :
        zCode="_SQLITEN_(DROP_TEMP_INDEX)";
        break;
    case _SQLITEN_(DROP_TEMP_TABLE)   :
        zCode="_SQLITEN_(DROP_TEMP_TABLE)";
        break;
    case _SQLITEN_(DROP_TEMP_TRIGGER) :
        zCode="_SQLITEN_(DROP_TEMP_TRIGGER)";
        break;
    case _SQLITEN_(DROP_TEMP_VIEW)    :
        zCode="_SQLITEN_(DROP_TEMP_VIEW)";
        break;
    case _SQLITEN_(DROP_TRIGGER)      :
        zCode="_SQLITEN_(DROP_TRIGGER)";
        break;
    case _SQLITEN_(DROP_VIEW)         :
        zCode="_SQLITEN_(DROP_VIEW)";
        break;
    case _SQLITEN_(INSERT)            :
        zCode="_SQLITEN_(INSERT)";
        break;
    case _SQLITEN_(PRAGMA)            :
        zCode="_SQLITEN_(PRAGMA)";
        break;
    case _SQLITEN_(READ)              :
        zCode="_SQLITEN_(READ)";
        break;
    case _SQLITEN_(SELECT)            :
        zCode="_SQLITEN_(SELECT)";
        break;
    case _SQLITEN_(TRANSACTION)       :
        zCode="_SQLITEN_(TRANSACTION)";
        break;
    case _SQLITEN_(UPDATE)            :
        zCode="_SQLITEN_(UPDATE)";
        break;
    case _SQLITEN_(ATTACH)            :
        zCode="_SQLITEN_(ATTACH)";
        break;
    case _SQLITEN_(DETACH)            :
        zCode="_SQLITEN_(DETACH)";
        break;
    case _SQLITEN_(ALTER_TABLE)       :
        zCode="_SQLITEN_(ALTER_TABLE)";
        break;
    case _SQLITEN_(REINDEX)           :
        zCode="_SQLITEN_(REINDEX)";
        break;
    case _SQLITEN_(ANALYZE)           :
        zCode="_SQLITEN_(ANALYZE)";
        break;
    case _SQLITEN_(CREATE_VTABLE)     :
        zCode="_SQLITEN_(CREATE_VTABLE)";
        break;
    case _SQLITEN_(DROP_VTABLE)       :
        zCode="_SQLITEN_(DROP_VTABLE)";
        break;
    case _SQLITEN_(FUNCTION)          :
        zCode="_SQLITEN_(FUNCTION)";
        break;
    case _SQLITEN_(SAVEPOINT)         :
        zCode="_SQLITEN_(SAVEPOINT)";
        break;
    default                       :
        zCode="????";
        break;
    }
    int i = 0;
    Jsi_Value *vpargs, *items[6] = {}, *ret;

    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zCode);
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zArg1 ? zArg1 : "");
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zArg2 ? zArg2 : "");
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zArg3 ? zArg3 : "");
    items[i++] = Jsi_ValueMakeStringDup(interp, NULL, zArg4 ? zArg4 : "");
    vpargs = Jsi_ValueMakeObject(interp, NULL, Jsi_ObjNewArray(interp, items, 1));
    Jsi_IncrRefCount(interp, vpargs);
    ret = Jsi_ValueNew(interp);
    rc = Jsi_FunctionInvoke(interp, pDb->zAuth, vpargs, &ret, NULL);
    Jsi_DecrRefCount(interp, vpargs);

    if (rc == JSI_OK && (zReply = Jsi_ValueGetStringLen(interp, ret, NULL)))
    {
        if( Jsi_Strcmp(zReply,"_SQLITEN_(OK)")==0 ) {
            rc = _SQLITEN_(OK);
        } else if( Jsi_Strcmp(zReply,"_SQLITEN_(DENY)")==0 ) {
            rc = _SQLITEN_(DENY);
        } else if( Jsi_Strcmp(zReply,"_SQLITEN_(IGNORE)")==0 ) {
            rc = _SQLITEN_(IGNORE);
        } else {
            rc = 999;
        }
    }
    Jsi_DecrRefCount(interp, ret);
    return rc;
}
#endif /* _SQLITEN_(OMIT_AUTHORIZATION) */

/*
** This routine reads a line of text from FILE in, stores
** the text in memory obtained from malloc() and returns a pointer
** to the text.  NULL is returned at end of file, or if malloc()
** fails.
**
** The interface is like "readline" but no command-line editing
** is done.
**
** copied from shell.c from '.import' command
*/
static char *local_getline(char *zPrompt, Jsi_Channel in) {
    char *zLine;
    int nLine;
    int n;
    int eol;

    nLine = 100;
    zLine = Jsi_Malloc( nLine );
    if( zLine==0 ) return 0;
    n = 0;
    eol = 0;
    while( !eol ) {
        if( n+100>nLine ) {
            nLine = nLine*2 + 100;
            zLine = Jsi_Realloc(zLine, nLine);
            if( zLine==0 ) return 0;
        }
        if( Jsi_Gets(in, &zLine[n], nLine - n)==0 ) {
            if( n==0 ) {
                Jsi_Free(zLine);
                return 0;
            }
            zLine[n] = 0;
            eol = 1;
            break;
        }
        while( zLine[n] ) {
            n++;
        }
        if( n>0 && zLine[n-1]=='\n' ) {
            n--;
            zLine[n] = 0;
            eol = 1;
        }
    }
    zLine = Jsi_Realloc( zLine, n+1 );
    return zLine;
}


/*
** This function is part of the implementation of the command:
**
**   $db transaction [-deferred|-immediate|-exclusive] SCRIPT
**
** It is invoked after evaluating the script SCRIPT to commit or rollback
** the transaction or savepoint opened by the [transaction] command.
*/
static int DbTransPostCmd(
    Jsi_Db *pDb,                       /* Sqlite3Db for $db */
    Jsi_Interp *interp,                  /* Jsi interpreter */
    int result                           /* Result of evaluating SCRIPT */
) {
    static const char *azEnd[] = {
        "RELEASE _jsi_transaction",        /* rc==JSI_ERROR, nTransaction!=0 */
        "COMMIT",                          /* rc!=JSI_ERROR, nTransaction==0 */
        "ROLLBACK TO _jsi_transaction ; RELEASE _jsi_transaction",
        "ROLLBACK"                         /* rc==JSI_ERROR, nTransaction==0 */
    };
    int rc = result;
    const char *zEnd;

    pDb->nTransaction--;
    zEnd = azEnd[(rc==JSI_ERROR)*2 + (pDb->nTransaction==0)];

    pDb->disableAuth++;
    if( _SQL_LITE_N_(_exec)(pDb->db, zEnd, 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
    )) {
        /* This is a tricky scenario to handle. The most likely cause of an
        ** error is that the exec() above was an attempt to commit the
        ** top-level transaction that returned _SQLITEN_(BUSY). Or, less likely,
        ** that an IO-error has occured. In either case, throw a Jsi exception
        ** and try to rollback the transaction.
        **
        ** But it could also be that the user executed one or more BEGIN,
        ** COMMIT, SAVEPOINT, RELEASE or ROLLBACK commands that are confusing
        ** this method's logic. Not clear how this would be best handled.
        */
        if( rc!=JSI_ERROR ) {
            Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
            rc = JSI_ERROR;
        }
        _SQL_LITE_N_(_exec)(pDb->db, "ROLLBACK", 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
    );
    }
    pDb->disableAuth--;

    return rc;
}



#if 0
static void dbEvalRowInfo(
    DbEvalContext *p,               /* Evaluation context */
    int *pnCol,                     /* OUT: Number of column names */
    char ***papColName,           /* OUT: Array of column names */
    int **papColType
) {
    /* Compute column names */
    // Jsi_Interp *interp = p->pDb->interp;

    if( 0==p->apColName ) {
        _SQL_LITE_N_(_stmt) *pStmt = p->pPreStmt->pStmt;
        int i;                        /* Iterator variable */
        int nCol;                     /* Number of columns returned by pStmt */
        char **apColName = 0;      /* Array of column names */
        int *apColType = 0;
        const char *zColName;         /* Column name */
        int numRid = 0;               /* Number of times rowid seen. */

        p->nCol = nCol = _SQL_LITE_N_(_column_count)(pStmt);
        if( nCol>0 && (papColName || p->pArray) ) {
            int cnLen = sizeof(char*)*nCol, cnStart = cnLen;
            for(i=0; i<nCol && cnLen<sizeof(p->staticColNames); i++)
                cnLen += Jsi_Strlen(_SQL_LITE_N_(_column_name)(pStmt,i))+1;
            if (cnLen>=sizeof(p->staticColNames)) {
                apColName = (char**)Jsi_Calloc(nCol, sizeof(char*) );
                cnStart = 0;
            } else {
                apColName = (char**)p->staticColNames;
            }
            if (papColType) {
                if (nCol < SQL_MAX_STATIC_TYPES)
                    apColType = p->staticColTypes;
                else
                    apColType = (int*)Jsi_Calloc(nCol, sizeof(int));
            }
            for(i=0; i<nCol; i++) {
                zColName = _SQL_LITE_N_(_column_name)(pStmt,i);
                if (cnStart==0)
                    apColName[i] = Jsi_Strdup(zColName);
                else {
                    apColName[i] = p->staticColNames+cnStart;
                    Jsi_Strcpy(apColName[i], zColName);
                    cnStart += Jsi_Strlen(zColName)+1;
                }
                if (apColType)
                    apColType[i] = _SQL_LITE_N_(_column_type)(pStmt,i);
                /* Check if rowid appears first, and more than once. */
                if ((i == 0 || numRid>0) &&
                        (zColName[0] == 'r' && Jsi_Strcmp(zColName,"rowid") == 0)) {
                    numRid++;
                }
            }
            /* Change first rowid to oid. */
            if (numRid > 1) {
                if (apColName != (char**)p->staticColNames) {
                    Jsi_Free(apColName[0]);
                    apColName[0] = Jsi_Strdup("oid");
                } else {
                    Jsi_Strcpy(apColName[0], "oid");
                }
            }
            p->apColName = apColName;
            p->apColType = apColType;
        }
    }
    if( papColName ) {
        *papColName = p->apColName;
    }
    if( papColType ) {
        *papColType = p->apColType;
    }
    if( pnCol ) {
        *pnCol = p->nCol;
    }
}
#endif

/*
** Return a JSON formatted value for the iCol'th column of the row currently pointed to by
** the DbEvalContext structure passed as the first argument.
*/
static void dbEvalSetColumnJSON(DbEvalContext *p, int iCol, Jsi_DString *dStr) {
    Jsi_Interp *interp = p->jdb->interp;
    char nbuf[200];

    _SQL_LITE_N_(_stmt) *pStmt = p->pPreStmt->pStmt;

    switch( _SQL_LITE_N_(_column_type)(pStmt, iCol) ) {
    case _SQLITEN_(BLOB): {
#ifndef USE_SQLITE_V4
        int bytes = _SQL_LITE_N_(_column_bytes)(pStmt, iCol);
        const char *zBlob = _SQL_LITE_N_(_column_blob)(pStmt, iCol);
#else
        int bytes;
        const char *zBlob = _SQL_LITE_N_(_column_blob)(pStmt, iCol, &bytes);
#endif
        if( !zBlob ) {
            bytes = 0;
            Jsi_DSAppend(dStr, "null", NULL);
            return;
        }
        Jsi_JSONQuote(interp, zBlob, bytes, dStr);
        return;
    }
    case _SQLITEN_(INTEGER): {
        sqlite_int64 v = _SQL_LITE_N_(_column_int64)(pStmt, iCol);
        sprintf(nbuf, "%lld", v);
        Jsi_DSAppend(dStr, nbuf, NULL);
        return;
    }
    case _SQLITEN_(FLOAT): {
        sprintf(nbuf, "%lg", _SQL_LITE_N_(_column_double)(pStmt, iCol));
        Jsi_DSAppend(dStr, nbuf, NULL);
        return;
    }
    case _SQLITEN_(NULL): {
        Jsi_DSAppend(dStr, "null", NULL);
        return;
    }
    }
    const char *str = (char*)_SQL_LITE_N_(_column_text)(pStmt, iCol _SQLBIND_END_);
    if (!str)
        str = p->jdb->execOpts.nullvalue;
    Jsi_JSONQuote(interp, str?str:"", -1, dStr);
}

static void dbEvalSetColumn(DbEvalContext *p, int iCol, Jsi_DString *dStr) {
    //Jsi_Interp *interp = p->pDb->interp;
    char nbuf[200];

    _SQL_LITE_N_(_stmt) *pStmt = p->pPreStmt->pStmt;

    switch( _SQL_LITE_N_(_column_type)(pStmt, iCol) ) {
    case _SQLITEN_(BLOB): {
#ifndef USE_SQLITE_V4
        int bytes = _SQL_LITE_N_(_column_bytes)(pStmt, iCol);
        const char *zBlob = _SQL_LITE_N_(_column_blob)(pStmt, iCol);
#else
        int bytes;
        const char *zBlob = _SQL_LITE_N_(_column_blob)(pStmt, iCol, &bytes);
#endif
        if( !zBlob ) {
            bytes = 0;
            return;
        }
        Jsi_DSAppendLen(dStr, zBlob, bytes);
        return;
    }
    case _SQLITEN_(INTEGER): {
        sqlite_int64 v = _SQL_LITE_N_(_column_int64)(pStmt, iCol);
        sprintf(nbuf, "%lld", v);
        Jsi_DSAppend(dStr, nbuf, NULL);
        return;
    }
    case _SQLITEN_(FLOAT): {
        sprintf(nbuf, "%lg", _SQL_LITE_N_(_column_double)(pStmt, iCol));
        Jsi_DSAppend(dStr, nbuf, NULL);
        return;
    }
    case _SQLITEN_(NULL): {
        return;
    }
    }
    const char *str = (char*)_SQL_LITE_N_(_column_text)(pStmt, iCol _SQLBIND_END_);
    if (!str)
        str = p->jdb->execOpts.nullvalue;
    Jsi_DSAppend(dStr, str?str:"", NULL);
}


static Jsi_Value* dbEvalSetColumnValue(DbEvalContext *p, int iCol, Jsi_Value *val) {
    Jsi_Interp *interp = p->jdb->interp;

    _SQL_LITE_N_(_stmt) *pStmt = p->pPreStmt->pStmt;
    const char *str;
    
    switch( _SQL_LITE_N_(_column_type)(pStmt, iCol) ) {
    case _SQLITEN_(BLOB): {
#ifndef USE_SQLITE_V4
        int bytes = _SQL_LITE_N_(_column_bytes)(pStmt, iCol);
        const char *zBlob = _SQL_LITE_N_(_column_blob)(pStmt, iCol);
#else
        int bytes;
        const char *zBlob = _SQL_LITE_N_(_column_blob)(pStmt, iCol, &bytes);
#endif
        if( !zBlob ) {
            bytes = 0;
            Jsi_ValueMakeNull(interp, val);
            break;
        }
        return Jsi_ValueMakeBlob(interp, val, (unsigned char*)zBlob, bytes);
        break;
    }
    case _SQLITEN_(INTEGER): {
        sqlite_int64 v = _SQL_LITE_N_(_column_int64)(pStmt, iCol);
        if( v>=-2147483647 && v<=2147483647 ) {
            return Jsi_ValueMakeNumber(interp, val, v);
        } else {
            return Jsi_ValueMakeNumber(interp, val, v);
        }
        break;
    }
    case _SQLITEN_(FLOAT): {
        return Jsi_ValueMakeNumber(interp, val, (Jsi_Number)_SQL_LITE_N_(_column_double)(pStmt, iCol));
        break;
    }
    case _SQLITEN_(NULL): {
        return Jsi_ValueMakeNull(interp, val);
        break;;
    }
    default:
        str = (char*)_SQL_LITE_N_(_column_text)(pStmt, iCol _SQLBIND_END_);
        if (!str)
            str = p->jdb->execOpts.nullvalue;
        return Jsi_ValueMakeStringDup(interp, val, str?str:"");
    }
    return Jsi_ValueNew1(interp);;
}


# define SQLITE_JSI_NRE 0
# define DbUseNre() 0
# define Jsi_NRAddCallback(a,b,c,d,e,f) 0
# define Jsi_NREvalObj(a,b,c) 0
# define Jsi_NRCreateCommand(a,b,c,d,e,f) 0

#include <stdio.h>

static int DbEvalCallCmd( DbEvalContext *p, Jsi_Interp *interp, int result)
{
    int cnt = 0, rc = result;
    Jsi_Value *varg1;
    Jsi_Obj *argso;
    char **apColName = NULL;
    int *apColType = NULL;
    if (p->jdb->trace & TMODE_EVAL)
        fprintf(stderr, "TRACE: eval\n");

    while( (rc==JSI_OK) && JSI_OK==(rc = dbEvalStep(p)) ) {
        int i;
        int nCol;

        cnt++;
        dbEvalRowInfo(p, &nCol, &apColName, &apColType);
        if (nCol<=0)
            continue;
        if (Jsi_ValueIsNull(interp,p->tocall))
            continue;
        /* Single object containing sql result members. */
        varg1 = Jsi_ValueMakeObject(interp, NULL, argso = Jsi_ObjNew(interp));
        for(i=0; i<nCol; i++) {
            Jsi_Value *nnv = dbEvalSetColumnValue(p, i, NULL);
            Jsi_ObjInsert(interp, argso, apColName[i], nnv, 0);
        }
        Jsi_IncrRefCount(interp, varg1);
        rc = Jsi_FunctionInvokeBool(interp, p->tocall, varg1);
        Jsi_DecrRefCount(interp, varg1);
        if (rc)
            break;
    }
    //dbEvalFinalize(p);

    if( rc==JSI_OK || rc==JSI_BREAK ) {
        //Jsi_ResetResult(interp);
        rc = JSI_OK;
    }
    return rc;
}

static Jsi_Db *getDb(Jsi_Interp *interp, Jsi_Value *_this, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb = Jsi_UserObjGetData(interp, _this, funcPtr);
    if (!pDb) {
        Jsi_LogError("Sqlite call to a non-sqlite object\n");
        return NULL;
    }
    if (!pDb->db)
    {
        Jsi_LogError("Sqlite db closed");
        return NULL;
    }
    return pDb;
}

static void sqliteObjErase(Jsi_Db *pDb)
{
    DbDeleteCmd(pDb);
    pDb->db = NULL;
}

static int sqliteObjFree(Jsi_Interp *interp, void *data)
{
    Jsi_Db *fo = data;
    SQLSIGASSERT(fo,DB);
    sqliteObjErase(fo);
    MEMCLEAR(fo);
    Jsi_Free(fo);
    return JSI_OK;
}

static int sqliteObjIsTrue(void *data)
{
    Jsi_Db *fo = data;
    SQLSIGASSERT(fo,DB);
    if (!fo->db) return 0;
    else return 1;
}

static int sqliteObjEqual(void *data1, void *data2)
{
    //SQLSIGASSERT(data1,DB);
    //SQLSIGASSERT(data2,DB);
    return (data1 == data2);
}

/**   new Sqlite(FILENAME,?-vfs VFSNAME?,?-key KEY?,?-readonly BOOLEAN?,
**                           ?-create BOOLEAN?,?-nomutex BOOLEAN?)
**
** This is the sqlite constructior called  using "new Sqlite".
**
** The first argument is the name of the database file.
**
*/

static int SqliteConstructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                             Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc = JSI_OK;
    Jsi_Value *toacc = NULL;
    if (Jsi_FunctionIsConstructor(funcPtr)) {
        toacc = _this;
    } else {
        Jsi_Obj *o = Jsi_ObjNew(interp);
        Jsi_PrototypeObjSet(interp, "Sqlite", o);
        Jsi_ValueMakeObject(interp, *ret, o);
        toacc = *ret;
    }
    /* void *cd = clientData; */
    int  flags;
    char *zErrMsg;
    const char *zFile = NULL, *vfs = 0;
    
    /* In normal use, each JSI interpreter runs in a single thread.  So
    ** by default, we can turn of mutexing on SQLite database connections.
    ** However, for testing purposes it is useful to have mutexes turned
    ** on.  So, by default, mutexes default off.  But if compiled with
    ** _SQLITEN_(JSI_DEFAULT_FULLMUTEX) then mutexes default on.
    */
    flags = _SQLITEN_(OPEN_READWRITE) | _SQLITEN_(OPEN_CREATE);
#ifdef USE_SQLITE_V4
    vfs = vfs; // Gets rid of warning.
#else
#ifdef SQLITE_JSI_DEFAULT_FULLMUTEX
    flags |= _SQLITEN_(OPEN_FULLMUTEX);
#else
    flags |= _SQLITEN_(OPEN_NOMUTEX);
#endif
#endif

    Jsi_Value *vFile = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 1);
    Jsi_DString dStr = {};

    if (!vFile)
        zFile = ":memory:";
    else {
        zFile = Jsi_ValueNormalPath(interp, vFile, &dStr);
        if (zFile == NULL) {
            Jsi_LogError("bad or missing file name");
            return JSI_ERROR;
        }
    }
    zErrMsg = 0;
    Jsi_Db *db = (Jsi_Db*)Jsi_Calloc(1, sizeof(*db) );
    if( db==0 ) {
        Jsi_DSFree(&dStr);
        Jsi_LogError("malloc failed");
        return JSI_ERROR;
    }
    db->sig = SQLITE_SIG_DB;
    db->maxStmts = NUM_PREPARED_STMTS;
    db->hasOpts = (arg != NULL && !Jsi_ValueIsNull(interp,arg));
    if (db->hasOpts && Jsi_OptionsProcess(interp, SqlOptions, arg, db, 0) < 0) {
        Jsi_DSFree(&dStr);
        return JSI_ERROR;

    }
    if (vFile && Jsi_SafeAccess(interp, vFile, db->readonly==0) != JSI_OK) {
        Jsi_LogError("Safe accces denied");
        goto bail;
    }

    if (db->maxStmts<0 || db->maxStmts>MAX_PREPARED_STMTS) {
        Jsi_LogError("option maxStmts value %d is not in range 0..%d", db->maxStmts, MAX_PREPARED_STMTS);
        goto bail;
    }
    if (db->readonly) {
        flags &= ~(_SQLITEN_(OPEN_READWRITE)|_SQLITEN_(OPEN_CREATE));
        flags |= _SQLITEN_(OPEN_READONLY);
    } else {
        flags &= ~_SQLITEN_(OPEN_READONLY);
        flags |= _SQLITEN_(OPEN_READWRITE);
        if (db->nocreate) {
            flags &= ~_SQLITEN_(OPEN_CREATE);
        }
    }
    if (db->vfs)
        vfs = Jsi_ValueToString(interp, db->vfs);
#ifndef USE_SQLITE_V4
    if(db->mutex == MUTEX_NONE) {
        flags |= _SQLITEN_(OPEN_NOMUTEX);
        flags &= ~_SQLITEN_(OPEN_FULLMUTEX);
    } else {
        flags &= ~_SQLITEN_(OPEN_NOMUTEX);
    }
    if(db->mutex ==MUTEX_FULL) {
        flags |= _SQLITEN_(OPEN_FULLMUTEX);
        flags &= ~_SQLITEN_(OPEN_NOMUTEX);
    } else {
        flags &= ~_SQLITEN_(OPEN_FULLMUTEX);
    }
  
    if (_SQLITEN_(OK) != _SQL_LITE_N_(_open_v2)(zFile, &db->db, flags, vfs)) {
        Jsi_LogError("db open failed");
        goto bail;
    }
#else
    if (_SQLITEN_(OK) != _SQL_LITE_N_(_open)(db->pEnv, zFile, &db->db, NULL)) {
        Jsi_LogError("db open failed");
        goto bail;
    }
#endif
    //Jsi_DSFree(&translatedFilename);

    if( _SQLITEN_(OK)!=_SQL_LITE_N_(_errcode)(db->db) ) {
        zErrMsg = _SQL_LITE_N_(_mprintf)(_SQLITE_PENV_(db) "%s", _SQL_LITE_N_(_errmsg)(db->db));
        DbClose(db->db);
        db->db = 0;
    }
#ifdef SQLITE_HAS_CODEC
    if( db->db && db->key) {
        const char *key = 0;
        if (db->key)
            key = Jsi_ValueString(interp, db->key, NULL);
        if (key)
            _SQL_LITE_N_(_key)(db->db, key, strlen(key));
    }
#endif
    if( db->db==0 ) {
#ifdef USE_SQLITE_V4
        _SQL_LITE_N_(_free)(db->pEnv, zErrMsg);
#else
        _SQL_LITE_N_(_free)(zErrMsg);
#endif
        goto bail;
    }
    Jsi_Obj *userObjPtr = Jsi_ValueGetObj(interp, toacc /* constructor obj*/);
    if ((db->objId = Jsi_UserObjNew(interp, &sqliteobject, userObjPtr, db))<0)
        goto bail;
    db->stmtHash = Jsi_HashNew(interp, JSI_KEYS_STRING, NULL);
    db->userObjPtr = userObjPtr;
    //dbSys->cnt = Jsi_UserObjCreate(interp, sqliteobject.name /*dbSys*/, userObjPtr, db);
    db->interp = interp;
    rc = JSI_OK;
    
bail:
    if (rc != JSI_OK) {
        if (db->hasOpts)
            Jsi_OptionsFree(interp, SqlOptions, db, 0);
        Jsi_Free(db);
    }
    Jsi_DSFree(&dStr);
    return rc;
}

#ifndef OMIT_SQLITE_HOOK_COMMANDS

#define FN_busy JSI_INFO("\
Invoke the given callback when an SQL statement attempts to open \
a locked database file. Call with null to disable, or no arguments, \
to return the current busy function. ")

static int SqliteBusyCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                         Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->zBusy ) {
            ValueCopy(interp, ret, pDb->zBusy);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_busy_handler)(pDb->db, 0, 0);
        if( pDb->zBusy ) {
            Jsi_DecrRefCount(interp, pDb->zBusy);
        }
        pDb->zBusy = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->zBusy ) {
            Jsi_DecrRefCount(interp, pDb->zBusy);
        }
        pDb->zBusy = func;
        Jsi_IncrRefCount(interp, func);
        _SQL_LITE_N_(_busy_handler)(pDb->db, DbBusyHandler, pDb);
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}
#endif

#ifndef OMIT_SQLITE_COLLATION
static int SqliteCollateCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                            Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    Jsi_Value *func;

    SqlCollate *pCollate;
    char *zName;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    zName = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    func = Jsi_ValueArrayIndex(interp, args, 1);
    pCollate = (SqlCollate*)Jsi_Calloc(1, sizeof(*pCollate));
    if( pCollate==0 ) return JSI_ERROR;
    pCollate->interp = interp;
    pCollate->pNext = pDb->pCollate;
    pCollate->zScript = func; /*(char*)&pCollate[1];*/
    pDb->pCollate = pCollate;

#ifdef USE_SQLITE_V4
    if( _SQL_LITE_N_(_create_collation)(pDb->db, zName, pCollate, jsiSqlCollate, 0 ))
#else
    if( _SQL_LITE_N_(_create_collation)(pDb->db, zName, _SQLITEN_(UTF8), pCollate, jsiSqlCollate) )
#endif
    {
        Jsi_LogError("%s", (char *)_SQL_LITE_N_(_errmsg)(pDb->db));
        return JSI_ERROR;
    }
    return JSI_OK;
}

static int SqliteCollationNeededCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    Jsi_Value *func;
    int argc = Jsi_ValueGetLength(interp, args);

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->pCollateNeeded ) {
            ValueCopy(interp, ret, pDb->pCollateNeeded);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_collation_needed)(pDb->db, 0, 0);
        if( pDb->zCommit ) {
            Jsi_DecrRefCount(interp, pDb->pCollateNeeded);
        }
        pDb->zCommit = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->pCollateNeeded ) {
            Jsi_DecrRefCount(interp, pDb->pCollateNeeded);
        }
        pDb->pCollateNeeded = func;
        Jsi_DecrRefCount(interp, func);
        _SQL_LITE_N_(_collation_needed)(pDb->db, pDb, jsiCollateNeeded);
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;

}
#endif

#ifndef OMIT_SQLITE_HOOK_COMMANDS

#define FN_commithook JSI_INFO("\
Invoke the given callback just before committing every SQL transaction. \
If the callback throws an exception or returns non-zero, then the \
transaction is aborted.  If CALLBACK is an empty string, the callback \
is disabled.")

static int SqliteCommitHookCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                               Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->zCommit ) {
            ValueCopy(interp, ret, pDb->zCommit);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_commit_hook)(pDb->db, 0, 0);
        if( pDb->zCommit ) {
            Jsi_DecrRefCount(interp, pDb->zCommit);
        }
        pDb->zCommit = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->zCommit ) {
            Jsi_DecrRefCount(interp, pDb->zCommit);
        }
        pDb->zCommit = func;
        Jsi_IncrRefCount(interp, func);
        _SQL_LITE_N_(_commit_hook)(pDb->db, DbCommitHandler, pDb);
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}

static int SqliteProgressCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                             Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int n, argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func, *nVal;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->zProgress ) {
            ValueCopy(interp, ret, pDb->zProgress);
        }
        return JSI_OK;
    }
    if (argc != 2) {
        Jsi_LogError("expected 0 or 2 args");
        return JSI_ERROR;
    }
    func = Jsi_ValueArrayIndex(interp, args, 1);
    nVal = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_GetIntFromValue(interp, nVal, &n) != JSI_OK)
        return JSI_ERROR;
    if (Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_progress_handler)(pDb->db, 0, 0, 0);
        if( pDb->zProgress ) {
            Jsi_DecrRefCount(interp, pDb->zProgress);
        }
        pDb->zProgress = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->zProgress ) {
            Jsi_DecrRefCount(interp, pDb->zProgress);
        }
        pDb->zProgress = func;
        Jsi_IncrRefCount(interp, func);
        _SQL_LITE_N_(_progress_handler)(pDb->db, n, DbProgressHandler, pDb);
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}
#endif

#define FN_profile JSI_INFO("\
Make arrangements to invoke the CALLBACK routine after each SQL statement \
that has run.  The text of the SQL and the amount of elapse time are \
arguments to CALLBACK.")

static int SqliteProfileCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                            Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->zProfile ) {
            ValueCopy(interp, ret, pDb->zProfile);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
#ifndef SQLITE_OMIT_TRACE
        _SQL_LITE_N_(_profile)(pDb->db, 0, 0 _SQLBIND_END_);
#endif
        if( pDb->zProfile ) {
            Jsi_DecrRefCount(interp, pDb->zProfile);
        }
        pDb->zProfile = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->zProfile ) {
            Jsi_DecrRefCount(interp, pDb->zProfile);
        }
        pDb->zProfile = func;
        Jsi_IncrRefCount(interp, func);
#ifndef SQLITE_OMIT_TRACE
#ifdef USE_SQLITE_V4
        _SQL_LITE_N_(_profile)(pDb->db, pDb->db, DbProfileHandler, 0);
#else
        _SQL_LITE_N_(_profile)(pDb->db, DbProfileHandler, pDb);
#endif
#endif
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}

static int SqliteRekeyCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                          Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    Jsi_Value *val = Jsi_ValueArrayIndex(interp, args, 0);
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    int nKey, rc = JSI_OK;
    void *pKey = Jsi_ValueString(interp, val, &nKey);
    
    if (!pKey) {
        Jsi_LogError("key must be a non-null string");
        return JSI_ERROR;
    }
#ifdef SQLITE_HAS_CODEC
    rc = _SQL_LITE_N_(_rekey)(pDb->db, pKey, nKey);
    if( rc ) {
#if defined(SQLITE3_AMALGAMATION) || defined(SQLITE4_AMALGAMATION)
        Jsi_LogError("Rekey: %s", _SQL_LITE_N_(ErrStr(rc)));
#else
        Jsi_LogError("Rekey error");
#endif
        rc = JSI_ERROR;
    } else {
        Jsi_ValueMakeBool(interp, *ret, 1);
    }
#endif
    return rc;
}

#define FN_trace JSI_INFO("\
Make arrangements to invoke the callback routine for each SQL statement\
that is executed.  The text of the SQL is an argument to callback.")

static int SqliteTraceCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                          Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->zTrace ) {
            ValueCopy(interp, ret, pDb->zTrace);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
#ifndef SQLITE_OMIT_TRACE
        _SQL_LITE_N_(_trace)(pDb->db, 0, 0 _SQLBIND_END_);
#endif
        if( pDb->zTrace ) {
            Jsi_DecrRefCount(interp, pDb->zTrace);
        }
        pDb->zTrace = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->zTrace ) {
            Jsi_DecrRefCount(interp, pDb->zTrace);
        }
        pDb->zTrace = func;
        Jsi_IncrRefCount(interp, func);
#ifndef SQLITE_OMIT_TRACE
#ifdef USE_SQLITE_V4
        _SQL_LITE_N_(_trace)(pDb->db, DbTraceHandler, 0, 0);
#else
        _SQL_LITE_N_(_trace)(pDb->db, DbTraceHandler, pDb);
#endif
#endif
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}

#ifndef OMIT_SQLITE_HOOK_COMMANDS

static int SqliteUnlockNotifyCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                 Jsi_Value **ret, Jsi_Func *funcPtr)
{

#ifndef SQLITE_ENABLE_UNLOCK_NOTIFY
    Jsi_LogError("unlock_notify not available in this build");
    return JSI_ERROR;
#else
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->zUnlockNotify ) {
            ValueCopy(interp, ret, pDb->zUnlockNotify);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (func == NULL || Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_unlock_notify)(pDb->db, 0, 0);
        if( pDb->pUnlockNotify ) {
            Jsi_DecrRefCount(interp, pDb->pUnlockNotify);
        }
        pDb->pUnlockNotify = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->pUnlockNotify ) {
            Jsi_DecrRefCount(interp, pDb->pUnlockNotify);
        }
        pDb->pUnlockNotify = func;
        Jsi_IncrRefCount(interp, func);
        if( _SQL_LITE_N_(_unlock_notify)(pDb->db, DbUnlockNotify, (void*)pDb)) {
            Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
            return JSI_ERROR;
        }

    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
#endif
}

static int SqliteUpdateHookCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                               Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->pUpdateHook ) {
            ValueCopy(interp, ret, pDb->pUpdateHook);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (func == NULL || Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_update_hook)(pDb->db, 0, 0);
        if( pDb->pUpdateHook ) {
            Jsi_DecrRefCount(interp, pDb->pUpdateHook);
        }
        pDb->pUpdateHook = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->pUpdateHook ) {
            Jsi_DecrRefCount(interp, pDb->pUpdateHook);
        }
        pDb->pUpdateHook = func;
        Jsi_IncrRefCount(interp, func);
        _SQL_LITE_N_(_update_hook)(pDb->db, DbUpdateHandler, pDb);

    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}

static int SqliteRollbackHookCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                 Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->pRollbackHook ) {
            ValueCopy(interp, ret, pDb->pRollbackHook);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_rollback_hook)(pDb->db, 0, 0);
        if( pDb->pRollbackHook ) {
            Jsi_DecrRefCount(interp, pDb->pRollbackHook);
        }
        pDb->pRollbackHook = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->pRollbackHook ) {
            Jsi_DecrRefCount(interp, pDb->pRollbackHook);
        }
        pDb->pRollbackHook = func;
        Jsi_IncrRefCount(interp, func);
        _SQL_LITE_N_(_rollback_hook)(pDb->db, DbRollbackHandler, pDb);
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}


static int SqliteWalHookCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                 Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *func;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    if( argc==0 ) {
        if( pDb->pWalHook ) {
            ValueCopy(interp, ret, pDb->pWalHook);
        }
        return JSI_OK;
    }
    func = Jsi_ValueArrayIndex(interp, args, 0);
    if (Jsi_ValueIsNull(interp, func)) {
        _SQL_LITE_N_(_rollback_hook)(pDb->db, 0, 0);
        if( pDb->pWalHook ) {
            Jsi_DecrRefCount(interp, pDb->pWalHook);
        }
        pDb->pWalHook = NULL;
    } else if(Jsi_ValueIsFunction(interp, func)) {
        if( pDb->pWalHook ) {
            Jsi_DecrRefCount(interp, pDb->pWalHook);
        }
        pDb->pWalHook = func;
        Jsi_IncrRefCount(interp, func);
        _SQL_LITE_N_(_wal_hook)(pDb->db, DbWalHandler, pDb);
    } else {
        Jsi_LogError("expected null or function");
        return JSI_ERROR;
    }
    return JSI_OK;
}
#endif

#define FN_authorizer JSI_INFO("\
  db.authorizer(FUNC) \
\n\
Invoke the given callback to authorize each SQL operation as it is \
compiled.  5 arguments are appended to the callback before it is \
invoked: \
\n\
  (1) The authorization type (ex: SQLITE_CREATE_TABLE, SQLITE_INSERT, ...) \
  (2) First descriptive name (depends on authorization type) \
  (3) Second descriptive name \
  (4) Name of the database (ex: 'main', 'temp') \
  (5) Name of trigger that is doing the access \
\n\
The callback should return on of the following strings: SQLITE_OK, \
SQLITE_IGNORE, or SQLITEN_DENY.  Any other return value is an error. \
\n\
If this method is invoked with no arguments, the current authorization \
callback string is returned.")

#ifndef SQLITE_OMIT_AUTHORIZATION
static int SqliteAuthorizorCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                               Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    Jsi_Value *auth;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    auth = Jsi_ValueArrayIndex(interp, args, 0);
    if(Jsi_ValueIsFunction(interp, auth)) {
        if (pDb->zAuth)
            Jsi_DecrRefCount(interp, pDb->zAuth);
        pDb->zAuth = auth;
        Jsi_IncrRefCount(interp, auth);
    } else if (Jsi_ValueIsUndef(interp, auth)) {
        if (pDb->zAuth)
            Jsi_DecrRefCount(interp, pDb->zAuth);
        pDb->zAuth = 0;
    } else {
        Jsi_LogError("expected function or undefined");
        return JSI_ERROR;
    }
    if( pDb->zAuth ) {
        pDb->interp = interp;
        _SQL_LITE_N_(_set_authorizer)(pDb->db, auth_callback, pDb);
    } else {
        _SQL_LITE_N_(_set_authorizer)(pDb->db, 0, 0);
    }
    return JSI_OK;
}
#endif


static const char *copyConflictStrs[] = {
    "ROLLBACK", "ABORT", "FAIL", "IGNORE", "REPLACE", 0
};
enum { CC_ROLLBACK, CC_ABORT, CC_FAIL, CC_IGNORE, CC_REPLACE, CC_NONE };

typedef struct ImportData {
    int limit;
    int conflict;
    char csv;
    char headers;
    const char *separator;
    const char *nullvalue;
} ImportData;

static Jsi_OptionSpec ImportOptions[] =
{
    JSI_OPT(BOOL,   ImportData, headers, .help="First row contains column labels"),
    JSI_OPT(BOOL,   ImportData, csv, .help="Treat input values as CSV"),
    JSI_OPT(CUSTOM, ImportData, conflict, .custom=Jsi_Opt_SwitchEnum,  .data=copyConflictStrs, .help="Set conflict resolution"),
    JSI_OPT(INT,    ImportData, limit, .help="Maximum number of lines to load"),
    JSI_OPT(STRKEY, ImportData, nullvalue, .help="Null string"),
    JSI_OPT(STRKEY, ImportData, separator, .help="Separator string; default is comma if csv, else tabs"),
    JSI_OPT_END(ImportData)
};

#define FN_import JSI_INFO("\
Import data from a file into table. SqlOptions include the 'separator' \
to use, which defaults to commas for csv, or tabs otherwise.\
If a column contains a null string, or the \
value of 'nullvalue', a null is inserted for the column. \
A 'conflict' is one of the sqlite conflict algorithms: \
   rollback, abort, fail, ignore, replace \
On success, return the number of lines processed, not necessarily same \
as 'db.changes' due to the conflict algorithm selected. \
")

static int SqliteImportCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                         Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int rc;
    char *zTable;               /* Insert data into this table */
    char *zFile;                /* The file from which to extract data */
    const char *zConflict;            /* The conflict algorithm to use */
    _SQL_LITE_N_(_stmt) *pStmt;        /* A statement */
    int nCol;                   /* Number of columns in the table */
    int nByte;                  /* Number of bytes in an SQL string */
    int i, j;                   /* Loop counters */
    int nSep;                   /* Number of bytes in zSep[] */
    int nNull;                  /* Number of bytes in zNull[] */
    char *zSql;                 /* An SQL statement */
    char *zLine;                /* A single line of input from the file */
    char **azCol;               /* zLine[] broken up into columns */
    char *zCommit;              /* How to commit changes */
    Jsi_Channel in;                   /* The input file */
    int lineno = 0;             /* Line number of input file */
    int created = 0;
    const char *zSep;
    const char *zNull;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 2);
    ImportData opts = {};

    if (arg) {
        if (Jsi_OptionsProcess(interp, ImportOptions, arg, &opts, 0) < 0)
            return JSI_ERROR;
    }
    zConflict = copyConflictStrs[opts.conflict];
    
    if(opts.separator ) {
        zSep = opts.separator;
    } else {
        zSep = (opts.csv ? "," : "\t");
    }
    if(opts.nullvalue ) {
        zNull = opts.nullvalue;
    } else {
        zNull = "";
    }
    zTable = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    Jsi_Value *fname = Jsi_ValueArrayIndex(interp, args, 1);
    zFile = Jsi_ValueArrayIndexToStr(interp, args, 1, NULL);
    nSep = strlen30(zSep);
    nNull = strlen30(zNull);
    if( nSep==0 ) {
        Jsi_LogError("Error: non-null separator required for copy");
        return JSI_ERROR;
    }


    zSql = _SQL_LITE_N_(_mprintf)(_SQLITE_PENV_(pDb) "SELECT * FROM '%q'", zTable);
    if (zSql==0) {
        Jsi_LogError("Error: bad table: %s", zTable);
        return JSI_ERROR;
    }
    
    if (opts.headers) {
        in = Jsi_Open(interp, fname, "rb");
        if( in==0 ) {
            Jsi_LogError("Error: cannot open file: %s", zFile);
            return JSI_ERROR;
        }
        if ((zLine = local_getline(0, in))==0 ) {
            Jsi_Close(in);
            return JSI_ERROR;
        }
        Jsi_Close(in);
        char *zn, *ze, *z = zLine;
        Jsi_DString cStr = {};
        int zlen = 0, icnt = 0;
        Jsi_DSAppend(&cStr, "CREATE TABLE IF NOT EXISTS '", zTable, "' (", NULL);
        while (1) {
            zn = strstr(z, zSep);
            if (!zn) zlen = strlen30(z);
            else zlen = zn-z;
            if (zlen<=0) break;
            ze = z+zlen-1;
            Jsi_DSAppend(&cStr, (icnt?",":""), "'", NULL);
            icnt++;
            if (opts.csv && *z=='"' && zn>z && *ze == '"')
                Jsi_DSAppendLen(&cStr, z+1, zlen-2);
            else
                Jsi_DSAppendLen(&cStr, z, zlen);
            Jsi_DSAppend(&cStr, "'", NULL);
            if (!zn) break;
            z = zn+nSep;
        }
        Jsi_DSAppend(&cStr, ");", NULL);
        Jsi_Free(zLine);
        if (zlen<=0) {
            Jsi_DSFree(&cStr);
            Jsi_LogError("null header problem");
            return JSI_ERROR;
        }
        rc = _SQL_LITE_N_(_exec)(pDb->db, Jsi_DSValue(&cStr), 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
        );
        Jsi_DSFree(&cStr);
        if (rc) {
            Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
            return JSI_ERROR;
        }
        created = 1;
    }
    
    nByte = strlen30(zSql);
    rc = _SQL_LITE_N_(_prepare)(pDb->db, zSql, -1, &pStmt, 0);
#ifdef USE_SQLITE_V4
        _SQL_LITE_N_(_free)(pDb->pEnv, zSql);
#else
        _SQL_LITE_N_(_free)(zSql);
#endif
    if( rc ) {
        Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
        nCol = 0;
    } else {
        nCol = _SQL_LITE_N_(_column_count)(pStmt);
    }
    _SQL_LITE_N_(_finalize)(pStmt);
    if( nCol==0 ) {
        rc = JSI_ERROR;
        goto bail;
    }
    zSql = Jsi_Malloc( nByte + 50 + nCol*2 );
    if( zSql==0 ) {
        Jsi_LogError("Error: can't malloc()");
        rc = JSI_ERROR;
        goto bail;
    }
    _SQL_LITE_N_(_snprintf)(
#ifdef USE_SQLITE_V4
    zSql, nByte+50, 
#else
    nByte+50, zSql, 
#endif
        "INSERT OR %q INTO '%q' VALUES(?",
                     zConflict, zTable);
    j = strlen30(zSql);
    for(i=1; i<nCol; i++) {
        zSql[j++] = ',';
        zSql[j++] = '?';
    }
    zSql[j++] = ')';
    zSql[j] = 0;
    rc = _SQL_LITE_N_(_prepare)(pDb->db, zSql, -1, &pStmt, 0);
    Jsi_Free(zSql);
    if( rc ) {
        Jsi_LogError("Error: %s", _SQL_LITE_N_(_errmsg)(pDb->db));
        _SQL_LITE_N_(_finalize)(pStmt);
        return JSI_ERROR;
    }
    in = Jsi_Open(interp, fname, "rb");
    if( in==0 ) {
        Jsi_LogError("Error: cannot open file: %s", zFile);
        _SQL_LITE_N_(_finalize)(pStmt);
        return JSI_ERROR;
    }
    azCol = Jsi_Malloc( sizeof(azCol[0])*(nCol+1) );
    if( azCol==0 ) {
        Jsi_LogError("Error: can't malloc()");
        Jsi_Close(in);
        rc = JSI_ERROR;
        goto bail;
    }
    (void)_SQL_LITE_N_(_exec)(pDb->db, "BEGIN", 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
    );
    zCommit = "COMMIT";
    while ((zLine = local_getline(0, in))!=0 ) {
        char *z;
        i = 0;
        lineno++;
        if (opts.limit>0 && lineno > opts.limit) {
            Jsi_Free(zLine);
            break;
        }
        if (lineno == 1 && opts.headers) {
            Jsi_Free(zLine);
            continue;
        }
        if (opts.csv && strchr(zLine,'"')) 
        {
            char *zn, *z = zLine;
            Jsi_DString sStr = {};
            int qcnt = 0;
            i = -1;
            while (*z) if (*z++ == '"') qcnt++;
            z = zLine;
            if (qcnt%2) { /* aggregate quote spanning newlines */
                Jsi_DSAppend(&sStr, zLine, NULL);
                do {
                    lineno++;
                    Jsi_DSAppend(&sStr, "\n", NULL);
                    Jsi_Free(zLine);
                    if (((zLine = local_getline(0, in)))==0)
                        break;
                    Jsi_DSAppend(&sStr, zLine, NULL);
                    z = zLine;
                    while (*z) if (*z++ == '"') qcnt++;
                } while (qcnt%2);
                z = Jsi_DSValue(&sStr);
            }
            if (qcnt%2) {
                Jsi_DSFree(&sStr);
                Jsi_Free(zLine);
                Jsi_Close(in);
                Jsi_LogError("unterminated string at line: %d", lineno);
                break;
            }
            while (z) {
                if (*z != '\"') { /* Handle un-quoted value */
                    zn = strstr(z, zSep);
                    azCol[++i] = z;
                    if (!zn)
                        break;
                    *zn = 0;
                    z = zn+nSep;
                    continue;
                }
                /* Handle quoted value */
                zn = ++z;
                Jsi_DString cStr = {};
                while (1) {
                    if (!zn)
                        break;
                    if (*zn != '"')
                        Jsi_DSAppendLen(&cStr, zn, 1);
                    else {
                        if (zn[1] == '"') {
                            zn++;
                            Jsi_DSAppendLen(&cStr, "\"", 1);
                        } else if (zn[1] == 0) {
                            break;
                        } else if (strncmp(zn+1,zSep, nSep)==0) {
                            *zn = 0;
                            zn += (nSep + 1);
                            break;
                        } else {
                            /* Invalid, comma should be right after close quote, so just eat quote. */
                            Jsi_DSAppendLen(&cStr, zn, 1);
                        }
                    }
                    zn++;
                }
                strcpy(z, Jsi_DSValue(&cStr));
                Jsi_DSFree(&cStr);
                azCol[++i] = z;
                z = zn;
            }
        } else {
            azCol[0] = zLine;
            for(i=0, z=zLine; *z; z++) {
                if( *z==zSep[0] && strncmp(z, zSep, nSep)==0 ) {
                    *z = 0;
                    i++;
                    if( i<nCol ) {
                        azCol[i] = &z[nSep];
                        z += nSep-1;
                    }
                }
            }
        }
        if( i+1!=nCol ) {
            Jsi_LogError("%s line %d: expected %d columns of data but found %d",
                 zFile, lineno, nCol, i+1);
            zCommit = "ROLLBACK";
            break;
        }
        for(i=0; i<nCol; i++) {
            /* check for null data, if so, bind as null */
            if( (nNull>0 && Jsi_Strcmp(azCol[i], zNull)==0)
                    || strlen30(azCol[i])==0
              ) {
                _SQL_LITE_N_(_bind_null)(pStmt, i+1);
            } else {
                _SQL_LITE_N_(_bind_text)(pStmt, i+1, azCol[i], -1, _SQLITEN_(STATIC) _SQLBIND_END_);
            }
        }
        _SQL_LITE_N_(_step)(pStmt);
        rc = _SQL_LITE_N_(_reset)(pStmt);
        if (zLine)
            Jsi_Free(zLine);
        if( rc!=_SQLITEN_(OK) ) {
            Jsi_LogError("%s at line: %d", _SQL_LITE_N_(_errmsg)(pDb->db), lineno);
            zCommit = "ROLLBACK";
            break;
        }
    }
    Jsi_Free(azCol);
    Jsi_Close(in);
    _SQL_LITE_N_(_finalize)(pStmt);
    (void)_SQL_LITE_N_(_exec)(pDb->db, zCommit, 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
    );

    if( zCommit[0] == 'C' ) {
        /* success, set result as number of lines processed */
        Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)lineno);
        rc = JSI_OK;
    } else {
        rc = JSI_ERROR;
    }
    
bail:
    if (rc != JSI_OK && created && opts.conflict == CC_ROLLBACK) {
        Jsi_DString cStr = {};
        Jsi_DSAppend(&cStr, "DROP TABLE IF EXISTS '", zTable, "';", NULL);
        (void)_SQL_LITE_N_(_exec)(pDb->db, Jsi_DSValue(&cStr), 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
        );
        Jsi_DSFree(&cStr);
    }
    return rc;
}

/*
** Make sure we have a PACKAGE_VERSION macro defined.  This will be
** defined automatically by the TEA makefile.  But other makefiles
** do not define it.
*/
#ifndef PACKAGE_VERSION
# define PACKAGE_VERSION _SQLITEN_(VERSION)
#endif

/*
int Sqlite3_Init(Jsi_Interp *interp){
  Jsi_InitStubs(interp, "8.4", 0);
  Jsi_CommandCreate(interp, "_SQL_LITE_N_()", DbMain, 0, 0);
  Jsi_PkgProvide(interp, "_SQL_LITE_N_()", PACKAGE_VERSION);
  Jsi_CommandCreate(interp, "sqlite", DbMain, 0, 0);
  Jsi_PkgProvide(interp, "sqlite", PACKAGE_VERSION);
  return JSI_OK;
}*/

static int SqliteCloseCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                          Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *p;
    if (!(p = getDb(interp, _this, funcPtr)))
        return JSI_ERROR;
    DbClose(p->db);
    p->db = NULL;
    return JSI_OK;
}

#define FN_evaluate JSI_INFO("\
Execute one or more comma seperated sql statments. \
Variable binding is NOT performed, results are discarded, and  \
no value is returned")
static int SqliteEvaluateCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                         Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc = _SQLITEN_(OK), rc2;
    Jsi_Db *pDb;
    _SQL_LITE_N_(_stmt) *pStmt = NULL;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    _SQL_LITE_N_() *db = pDb->db;
    const char *zSql = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    const char *zLeftover = NULL, *zErrMsg = NULL;

    while( zSql[0] && (_SQLITEN_(OK) == rc) ) {
        rc = _SQL_LITE_N_(_prepare_v2)(db, zSql, -1, &pStmt, &zLeftover);

        if( _SQLITEN_(OK) != rc ) {
            break;
        } else {
            if( !pStmt ) {
                /* this happens for a comment or white-space */
                zSql = zLeftover;
                while( isspace(zSql[0]) ) zSql++;
                continue;
            }

            do {
                rc = _SQL_LITE_N_(_step)(pStmt);
            } while( rc == _SQLITEN_(ROW) );
            rc2 = _SQL_LITE_N_(_finalize)(pStmt);
            if( rc!=_SQLITEN_(NOMEM) ) rc = rc2;
            if( rc==_SQLITEN_(OK) ) {
                zSql = zLeftover;
                while( isspace(zSql[0]) ) zSql++;
            } else {
            }
        }
    }
 
    if (rc == _SQLITEN_(OK))
        return JSI_OK;
    zErrMsg = _SQL_LITE_N_(_errmsg)(db);
    Jsi_LogError("sqlite error: %s", zErrMsg ? zErrMsg : "");
    return JSI_ERROR;
}

/*
** If a field contains any character identified by a 1 in the following
** array, then the string must be quoted for CSV.
*/
static const char needCsvQuote[] = {
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 0, 1, 0, 0, 0, 0, 1,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 1, 
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
};

/*
** Output a single term of CSV.  Actually, p->separator is used for
** the separator, which may or may not be a comma.  p->nullvalue is
** the null value.  Strings are quoted if necessary.
*/
static void output_csv(ExecFmt *p, const char *z, Jsi_DString *dStr, int bSep)
{
    if( z==0 ) {
        Jsi_DSAppend(dStr,  p->nullvalue?p->nullvalue:"", NULL);
    } else {
        int i;
        int nSep = strlen30(p->separator);
        for(i=0; z[i]; i++) {
            if( needCsvQuote[((unsigned char*)z)[i]] || 
                (z[i]==p->separator[0] && (nSep==1 || memcmp(z, p->separator, nSep)==0)) ) {
                i = 0;
                break;
            }
        }
        if( i==0 ) {
            Jsi_DSAppend(dStr, "\"", NULL);
            for(i=0; z[i]; i++) {
                if( z[i]=='"' ) Jsi_DSAppend(dStr, "\"", NULL);
                Jsi_DSAppendLen(dStr, z+i, 1);
            }
            Jsi_DSAppend(dStr, "\"", NULL);
        } else {
            Jsi_DSAppend(dStr, z, NULL);
        }
    }
    if( bSep ) {
        Jsi_DSAppend(dStr, p->separator, NULL);
    }
}

static void output_html_string(ExecFmt *p, const char *z, Jsi_DString *dStr)
{
    while( *z ) {
        switch (*z) {
        case '<':
            Jsi_DSAppend(dStr, "&lt;", NULL);
            break;
        case '>':
            Jsi_DSAppend(dStr, "&gt;", NULL);
            break;
        case '&':
            Jsi_DSAppend(dStr, "&amp;", NULL);
            break;
        case '\"':
            Jsi_DSAppend(dStr, "&quot;", NULL);
            break;
        case '\'':
            Jsi_DSAppend(dStr, "&#39;", NULL);
            break;
        default:
            Jsi_DSAppendLen(dStr, z, 1);
            break;
        }
        z++;
    }
}
/*
** Output the given string as a quoted string using SQL quoting conventions.
*/
static void output_quoted_string(Jsi_DString *dStr, const char *z) {
    int i;
    int nSingle = 0;
    for(i=0; z[i]; i++) {
        if( z[i]=='\'' ) nSingle++;
    }
    if( nSingle==0 ) {
        Jsi_DSAppend(dStr,"'", z, "'", NULL);
    } else {
        Jsi_DSAppend(dStr,"'", NULL);
        while( *z ) {
            for(i=0; z[i] && z[i]!='\''; i++) {}
            if( i==0 ) {
                Jsi_DSAppend(dStr,"''", NULL);
                z++;
            } else if( z[i]=='\'' ) {
                Jsi_DSAppendLen(dStr,z, i);
                Jsi_DSAppend(dStr,"''", NULL);
                z += i+1;
            } else {
                Jsi_DSAppend(dStr, z, NULL);
                break;
            }
        }
        Jsi_DSAppend(dStr,"'", NULL);
    }
}
/*
** Output the given string as a hex-encoded blob (eg. X'1234' )
*/
static void output_hex_blob(Jsi_DString *dStr, const void *pBlob, int nBlob){
  int i;
  char out[100], *zBlob = (char *)pBlob;
  Jsi_DSAppend(dStr, "X'", NULL);
  for(i=0; i<nBlob; i++){ sprintf(out,"%02x",zBlob[i]&0xff);Jsi_DSAppend(dStr, out, NULL); }
  Jsi_DSAppend(dStr, "'", NULL);
}

#define FN_sqlexec JSI_INFO("\
Return values in formatted as JSON, HTML, etc. \
, optionally calling function with a result object")
static int SqliteExecCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                             Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc = JSI_OK;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *vSql = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_DString eStr;
#ifdef JSI_DB_DSTRING_SIZE
    JSI_DSTRING_VAR(dStr, JSI_DB_DSTRING_SIZE);
#else
    Jsi_DString ddStr, *dStr = &ddStr;
    Jsi_DSInit(dStr);
#endif
    const char *zSql = Jsi_ValueGetDString(interp, vSql, &eStr, 0);
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 1);
    int cnt = 0;
    char **apColName = NULL;
    int *apColType = NULL, isopts = 0;
    DbEvalContext sEval;
    ExecFmt opts;
    opts = pDb->execOpts;
    opts.callback = NULL;
    opts.width = NULL;
    Jsi_Value *callback = NULL, *width = NULL;
            
    if (arg) {
        if (Jsi_ValueIsNull(interp,arg)) {
        } else if (Jsi_ValueIsFunction(interp,arg)) {
            callback = opts.callback = arg;
        } else if (Jsi_ValueIsObjType(interp, arg, JSI_OT_OBJECT)) {
            isopts = 1;
        } else {
            Jsi_LogError("argument must be null, a function, or options");
            return JSI_ERROR;
        }
    }

    if (isopts) {
        if (Jsi_OptionsProcess(interp, ExecFmtOptions, arg, &opts, 0) < 0)
            return JSI_ERROR;
        callback = (opts.callback ? opts.callback : pDb->execOpts.callback);
        width = (opts.width ? opts.width : pDb->execOpts.width);
    }
    if (pDb->execOpts.cdata) {
        char *cdata = (char*)pDb->execOpts.cdata;
        Jsi_DbMultipleBind* copts = Jsi_CDataLookup(interp, cdata);
        if (!copts) {
            Jsi_LogError("unknown cdata option: %s", pDb->execOpts.cdata);
            return JSI_ERROR;
        }
        int n = Jsi_DbQuery(pDb, copts->opts, copts->data, copts->numData, zSql, copts->flags);
        Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)n);
        return JSI_OK;
    }
    if (!opts.separator) {
        switch (opts.mode) {
            case EF_LIST: opts.separator = "|"; break;
            case EF_COLUMN: opts.separator = " "; break;
            case EF_TABS: opts.separator = "\t"; break;
            default: opts.separator = ",";
        }
    }
    Jsi_DString sStr;
    Jsi_DSInit(&sStr);
    dbEvalInit(interp, &sEval, pDb, zSql, &sStr, 0, 0);
    sEval.nocache = opts.nocache;
    sEval.ret = *ret;
    if (callback) {
        sEval.tocall = callback;
        if (opts.mode != EF_ROWS) {
            Jsi_LogError("'mode' must be 'rows' with 'callback'");
            rc = JSI_ERROR;
            goto bail;
        }
        rc = DbEvalCallCmd(&sEval, interp, JSI_OK);
        goto bail;
    } else
    switch (opts.mode) {
    case EF_NONE:
        while(JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        goto bail;
        break;
    case EF_JSON:
        if (opts.headers) {
            Jsi_DSAppend(dStr, "[ ", NULL);
            while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
                int i;
                int nCol;
                dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
                if (cnt == 0) {
                    Jsi_DSAppend(dStr, "[", NULL);
                    for(i=0; i<nCol; i++) {
                        if (i)
                            Jsi_DSAppend(dStr, ", ", NULL);
                        Jsi_JSONQuote(interp, apColName[i], -1, dStr);
                    }
                    Jsi_DSAppend(dStr, "]", NULL);
                    cnt++;
                }
                if (cnt)
                    Jsi_DSAppend(dStr, ", ", NULL);
                Jsi_DSAppend(dStr, "[", NULL);
                for(i=0; i<nCol; i++) {
                    if (i)
                        Jsi_DSAppend(dStr, ", ", NULL);
                    dbEvalSetColumnJSON(&sEval, i, dStr);
                }
                Jsi_DSAppend(dStr, "]", NULL);
                cnt++;
                if (opts.limit && cnt>opts.limit) break;
            }
            Jsi_DSAppend(dStr, " ]", NULL);
            
        } else {
            Jsi_DSAppend(dStr, "[ ", NULL);
            while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
                int i;
                int nCol;
                dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
                if (cnt)
                    Jsi_DSAppend(dStr, ", ", NULL);
                Jsi_DSAppend(dStr, "{", NULL);
                for(i=0; i<nCol; i++) {
                    if (i)
                        Jsi_DSAppend(dStr, ", ", NULL);
                    Jsi_JSONQuote(interp, apColName[i], -1, dStr);
                    Jsi_DSAppend(dStr, ":", NULL);
                    dbEvalSetColumnJSON(&sEval, i, dStr);
                }
                Jsi_DSAppend(dStr, "}", NULL);
                cnt++;
                if (opts.limit && cnt>=opts.limit) break;
            }
            Jsi_DSAppend(dStr, " ]", NULL);
        }
        break;
        
    case EF_JSON2: {
            while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
                int i;
                int nCol;
                dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
                if (cnt == 0 && 1) {
                    Jsi_DSAppend(dStr, "{ \"names\": [ ", NULL);
                    for(i=0; i<nCol; i++) {
                        if (i)
                            Jsi_DSAppend(dStr, ", ", NULL);
                        Jsi_JSONQuote(interp, apColName[i], -1, dStr);
                    }
                    Jsi_DSAppend(dStr, " ], \"values\": [ ", NULL);
                }
                if (cnt)
                    Jsi_DSAppend(dStr, ", ", NULL);
                Jsi_DSAppend(dStr, "[", NULL);
                for(i=0; i<nCol; i++) {
                    if (i)
                        Jsi_DSAppend(dStr, ", ", NULL);
                    dbEvalSetColumnJSON(&sEval, i, dStr);
                }
                Jsi_DSAppend(dStr, " ]", NULL);
                cnt++;
                if (opts.limit && cnt>=opts.limit) break;
            }
            if (cnt)
                Jsi_DSAppend(dStr, " ] } ", NULL);
        }
        break;
        
    case EF_LIST:
        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0 && opts.headers) {
                for(i=0; i<nCol; i++) {
                    if (i)
                        Jsi_DSAppend(dStr, opts.separator, NULL);
                    Jsi_DSAppend(dStr, apColName[i], NULL);
                }
            }

            if (cnt || opts.headers)
                Jsi_DSAppend(dStr, "\n", NULL);
            for(i=0; i<nCol; i++) {
                if (i)
                    Jsi_DSAppend(dStr, opts.separator, NULL);
                dbEvalSetColumn(&sEval, i, dStr);
            }
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        break;
        
    case EF_COLUMN: {
        int *wids = NULL;
        Jsi_DString vStr = {};
        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i, w;
            int nCol;
            
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0 && nCol>0) {
                Jsi_DString sStr;
                wids = Jsi_Calloc(nCol, sizeof(int));
                Jsi_DSInit(&sStr);
                for(i=0; i<nCol; i++) {
                    int j = Jsi_Strlen(apColName[i]);
                    wids[i] = (j<10?10:j);
                    if (width) {
                        Jsi_Value *wv = Jsi_ValueArrayIndex(interp, width, i);
                        if (wv) {
                            Jsi_Number dv;
                            Jsi_ValueGetNumber(interp, wv, &dv);
                            if (dv>0)
                                wids[i] = (int)dv;
                        }
                    }
                    w = (j<wids[i] ? j : wids[i]);
                    Jsi_DSAppendLen(dStr, apColName[i], w);
                    w = (j<wids[i] ? wids[i]-j+1 : 0);
                    while (w-- > 0)
                        Jsi_DSAppend(dStr, " ", NULL);
                }
                for(i=0; i<nCol && opts.headers; i++) {
                    w = wids[i];
                    w -= Jsi_Strlen(apColName[i]);
                    if (i) {
                        Jsi_DSAppend(dStr, opts.separator, NULL);
                        Jsi_DSAppend(&sStr, opts.separator, NULL);
                    }
                    w = wids[i];
                    while (w-- > 0)
                        Jsi_DSAppend(&sStr, "-", NULL);
                }
                if (opts.headers)
                    Jsi_DSAppend(dStr, "\n", Jsi_DSValue(&sStr), "\n", NULL);
                Jsi_DSFree(&sStr);
            }

            if (cnt)
                Jsi_DSAppend(dStr, "\n", NULL);
            for(i=0; i<nCol; i++) {
                if (i)
                    Jsi_DSAppend(dStr, opts.separator, NULL);
                Jsi_DSSetLength(&vStr, 0);
                dbEvalSetColumn(&sEval, i, &vStr);
                int nl = Jsi_DSLength(&vStr);
                if (nl > wids[i]) {
                    Jsi_DSSetLength(&vStr, wids[i]);
                    w = 0;
                } else {
                    w = wids[i]-nl;
                }
                Jsi_DSAppend(dStr, Jsi_DSValue(&vStr), NULL);
                while (w-- > 0)
                    Jsi_DSAppend(dStr, " ", NULL);
            }
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        Jsi_DSFree(&vStr);
        if (wids)
            Jsi_Free(wids);
        break;
    }
    
    case EF_INSERT: {
        Jsi_DString vStr = {};    
        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            const char *tbl = (opts.table ? opts.table : "table");
            if (cnt)
                Jsi_DSAppend(dStr, "\n", NULL);
            Jsi_DSAppend(dStr, "INSERT INTO ", tbl, " VALUES(", NULL);
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            for(i=0; i<nCol; i++) {
                Jsi_Number dv;
                const char *azArg;
                Jsi_DSSetLength(&vStr, 0);
                dbEvalSetColumn(&sEval, i, &vStr);
                _SQL_LITE_N_(_stmt) *pStmt = sEval.pPreStmt->pStmt;
                int ptype = _SQL_LITE_N_(_column_type)(pStmt, i);
                
                azArg = Jsi_DSValue(&vStr);
                char *zSep = i>0 ? ",": "";
                if( (azArg[i]==0) || (apColType && apColType[i]==_SQLITEN_(NULL)) ) {
                  Jsi_DSAppend(dStr, zSep, "NULL", NULL);
                }else if( ptype ==_SQLITEN_(TEXT) ) {
                  if( zSep[0] ) Jsi_DSAppend(dStr,zSep, NULL);
                  output_quoted_string(dStr, azArg);
                }else if (ptype==_SQLITEN_(INTEGER) || ptype ==_SQLITEN_(FLOAT)) {
                  Jsi_DSAppend(dStr, zSep, azArg, NULL);
                }else if (ptype ==_SQLITEN_(BLOB)) {
#ifdef USE_SQLITE_V4
                  int nBlob;
                  const void *pBlob = _SQL_LITE_N_(_column_blob)(pStmt, i, &nBlob);
#else
                  const void *pBlob = _SQL_LITE_N_(_column_blob)(pStmt, i _SQLBIND_END_);
                  int nBlob = _SQL_LITE_N_(_column_bytes)(pStmt, i);
#endif
                  if( zSep[0] ) Jsi_DSAppend(dStr,zSep, NULL);
                  output_hex_blob(dStr, pBlob, nBlob);
                }else if( Jsi_GetDouble(interp, azArg, &dv) == JSI_OK ){
                  Jsi_DSAppend(dStr, zSep, azArg[i], NULL);
                }else{
                  if( zSep[0] ) Jsi_DSAppend(dStr,zSep, NULL);
                  output_quoted_string(dStr, azArg);
                }
            }
            Jsi_DSAppend(dStr, ");", NULL);
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        Jsi_DSFree(&vStr);
    }

    case EF_TABS:
    case EF_CSV: {
        Jsi_DString vStr = {};  
        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0 && opts.headers) {
                for(i=0; i<nCol; i++) {
                    if (i)
                        Jsi_DSAppend(dStr, opts.separator, NULL);
                    Jsi_DSAppend(dStr, apColName[i], NULL);
                }
            }

            if (cnt || opts.headers)
                Jsi_DSAppend(dStr, "\n", NULL);
            for(i=0; i<nCol; i++) {
                if (i)
                    Jsi_DSAppend(dStr, opts.separator, NULL);
                Jsi_DSSetLength(&vStr, 0);
                dbEvalSetColumn(&sEval, i, &vStr);
                if (opts.mode == EF_CSV)
                    output_csv(&opts, Jsi_DSValue(&vStr), dStr, 0);
                else
                    Jsi_DSAppend(dStr, Jsi_DSValue(&vStr), NULL);
            }
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        Jsi_DSFree(&vStr);
        break;
    }
        
    case EF_LINE: {
        int i, w = 5, ww;
        int nCol;
        Jsi_DString vStr = {};   
        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0) {
                for(i=0; i<nCol; i++) {
                    ww = Jsi_Strlen(apColName[i]);
                    if (ww>w)
                        w = ww;
                }
            }

            for(i=0; i<nCol; i++) {
                Jsi_DString eStr;
                Jsi_DSInit(&eStr);
                Jsi_DSSetLength(&vStr, 0);
                dbEvalSetColumn(&sEval, i, &vStr);
                Jsi_DSPrintf(&eStr, "%*s = %s", w, apColName[i], Jsi_DSValue(&vStr));
                Jsi_DSAppend(dStr, (cnt?"\n":""), Jsi_DSValue(&eStr), NULL);
                Jsi_DSFree(&eStr);
            }
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        Jsi_DSFree(&vStr);
        break;
    }
        
    case EF_HTML: {
        Jsi_DString vStr = {};   
        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0 && opts.headers) {
                Jsi_DSAppend(dStr, "<TR>", NULL);
                for(i=0; i<nCol; i++) {
                    Jsi_DSAppend(dStr, "<TH>", NULL);
                    output_html_string(&opts, apColName[i], dStr);
                    Jsi_DSAppend(dStr, "</TH>", NULL);
                }
                Jsi_DSAppend(dStr, "</TR>", NULL);
            }
            if (cnt || opts.headers)
                Jsi_DSAppend(dStr, "\n", NULL);
            Jsi_DSAppend(dStr, "<TR>", NULL);
            for(i=0; i<nCol; i++) {
                Jsi_DSAppend(dStr, "<TD>", NULL);
                Jsi_DSSetLength(&vStr, 0);
                dbEvalSetColumn(&sEval, i, &vStr);
                output_html_string(&opts, Jsi_DSValue(&vStr), dStr);
                Jsi_DSAppend(dStr, "</TD>", NULL);
            }
            Jsi_DSAppend(dStr, "</TR>", NULL);
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        Jsi_DSFree(&vStr);
        break;
    }
        
    case EF_ROWS:
    {
        Jsi_Value *vcur, *vrow;
        int cnt = 0;
        Jsi_Obj *oall, *ocur;
        Jsi_ValueMakeArrayObject(interp, *ret, oall = Jsi_ObjNewType(interp, JSI_OT_ARRAY));

        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            ocur = Jsi_ObjNewType(interp, JSI_OT_OBJECT);
            vrow = Jsi_ValueMakeObject(interp, NULL, ocur);
            for(i=0; i<nCol; i++) {
                vcur = dbEvalSetColumnValue(&sEval, i, NULL);
                Jsi_ObjInsert(interp, ocur, Jsi_Strdup(apColName[i]), vcur, 0);
            }
            Jsi_ObjArrayAdd(interp, oall, vrow);
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        dbEvalFinalize(&sEval);
        if (rc != JSI_ERROR)
            rc = JSI_OK;
        goto bail;
        break;
    }
    case EF_ARRAYS:
    {
        Jsi_Value *vcur, *vrow;
        int cnt = 0;
        Jsi_Obj *oall, *ocur;
        Jsi_ValueMakeArrayObject(interp, *ret, oall = Jsi_ObjNewType(interp, JSI_OT_ARRAY));

        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0 && opts.headers) {
                vrow = Jsi_ValueMakeArrayObject(interp, NULL, ocur = Jsi_ObjNewType(interp, JSI_OT_ARRAY));
                for(i=0; i<nCol; i++) {
                    vcur = Jsi_ValueNewStringDup(interp, apColName[i]);
                    Jsi_ObjArrayAdd(interp, ocur, vcur);
                }
                Jsi_ObjArrayAdd(interp, oall, vrow);
            }
            vrow = Jsi_ValueMakeArrayObject(interp, NULL, ocur = Jsi_ObjNewType(interp, JSI_OT_ARRAY));
            for(i=0; i<nCol; i++) {
                vcur = dbEvalSetColumnValue(&sEval, i, NULL);
                Jsi_ObjArrayAdd(interp, ocur, vcur);
            }
            Jsi_ObjArrayAdd(interp, oall, vrow);
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        dbEvalFinalize(&sEval);
        if (rc != JSI_ERROR)
            rc = JSI_OK;
        goto bail;
        break;
    }
    case EF_ARRAY1D:
    {
        Jsi_Value *vcur;
        int cnt = 0;
        Jsi_Obj *oall;
        Jsi_ValueMakeArrayObject(interp, *ret, oall = Jsi_ObjNewType(interp, JSI_OT_ARRAY));

        while( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
            int i;
            int nCol;
            dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
            if (cnt == 0 && opts.headers) {
                for(i=0; i<nCol; i++) {
                    vcur = Jsi_ValueNewStringDup(interp, apColName[i]);
                    Jsi_ObjArrayAdd(interp, oall, vcur);
                }
            }
            for(i=0; i<nCol; i++) {
                vcur = dbEvalSetColumnValue(&sEval, i, NULL);
                Jsi_ObjArrayAdd(interp, oall, vcur);
            }
            cnt++;
            if (opts.limit && cnt>=opts.limit) break;
        }
        dbEvalFinalize(&sEval);
        if (rc != JSI_ERROR)
            rc = JSI_OK;
        goto bail;
        break;
    }
    }
    dbEvalFinalize(&sEval);
    if( rc==JSI_BREAK ) {
        rc = JSI_OK;
    }
    Jsi_ValueMakeStringDup(interp, *ret, Jsi_DSValue(dStr));
bail:
    if (isopts) {
        Jsi_OptionsFree(interp, ExecFmtOptions, &opts, 0);
    }
    Jsi_DSFree(dStr);
    Jsi_DSFree(&eStr);

    return rc;
}

static int SqliteOnecolumnCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                          Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *vSql = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_DString dStr;
    Jsi_DSInit(&dStr);
    Jsi_DString sStr;
    Jsi_DSInit(&sStr);
    DbEvalContext sEval;
    const char *zSql = Jsi_ValueGetDString(interp, vSql, &dStr, 0);

    dbEvalInit(interp, &sEval, pDb, zSql, &sStr, 0, 0);
    sEval.nocache = pDb->execOpts.nocache;
    sEval.ret = *ret;
    sEval.tocall = NULL;
    int cnt = 0;


    if( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
        _SQL_LITE_N_(_stmt) *pStmt = sEval.pPreStmt->pStmt;
        int nCol = _SQL_LITE_N_(_column_count)(pStmt);
        if (nCol>0)
            dbEvalSetColumnValue(&sEval, 0, *ret);
        cnt++;
    }
    dbEvalFinalize(&sEval);
    if( rc==JSI_BREAK ) {
        rc = JSI_OK;
    }
    Jsi_DSFree(&dStr);
    return rc;
}

static int SqliteExistsCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                           Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *vSql = Jsi_ValueArrayIndex(interp, args, 0);
    const char *zSql;
    Jsi_DString dStr = {};
    DbEvalContext sEval;
    zSql = Jsi_ValueGetDString(interp, vSql, &dStr, 0);

    Jsi_DString sStr;
    Jsi_DSInit(&sStr);
    dbEvalInit(interp, &sEval, pDb, zSql, &sStr, 0, 0);
    sEval.nocache = pDb->execOpts.nocache;
    sEval.ret = *ret;
    int cnt = 0;


    if( JSI_OK==(rc = dbEvalStep(&sEval)) ) {
        _SQL_LITE_N_(_stmt) *pStmt = sEval.pPreStmt->pStmt;
        int nCol = _SQL_LITE_N_(_column_count)(pStmt);
        if (nCol>0)
            cnt++;
    }
    dbEvalFinalize(&sEval);
    if( rc==JSI_BREAK ) {
        rc = JSI_OK;
    }
    Jsi_DSFree(&dStr);
    Jsi_ValueMakeBool(interp, *ret, cnt);
    return rc;
}

static int SqliteFilenameCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                             Jsi_Value **ret, Jsi_Func *funcPtr)
{
#ifndef SQLITE_OMIT_LOAD_EXTENSION
#if (_SQLITEN_(VERSION_NUMBER)>3007016)
    const char *zName = "main";
    int argc = Jsi_ValueGetLength(interp, args);
    Jsi_Db *pDb;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    if (argc)
        zName = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    zName = _SQL_LITE_N_(_db_filename)(pDb->db, zName);
    if (zName)
        Jsi_ValueMakeStringDup(interp, *ret, zName);
#endif
#endif
    return JSI_OK;
}

/*
** Find an SqlFunc structure with the given name.  Or create a new
** one if an existing one cannot be found.  Return a pointer to the
** structure.
*/
static SqlFunc *findSqlFunc(Jsi_Db *pDb, const char *zName) {
    SqlFunc *p, *pNew;
    int i;
    pNew = (SqlFunc*)Jsi_Calloc(1, sizeof(*pNew) + strlen30(zName) + 1 );
    pNew->sig = SQLITE_SIG_FUNC;
    pNew->zName = (char*)&pNew[1];
    for(i=0; zName[i]; i++) {
        pNew->zName[i] = tolower(zName[i]);
    }
    pNew->zName[i] = 0;
    for(p=pDb->pFunc; p; p=p->pNext) {
        if( Jsi_Strcmp(p->zName, pNew->zName)==0 ) {
            Jsi_Free((char*)pNew);
            return p;
        }
    }
    pNew->interp = pDb->interp;
    pNew->pScript = 0;
    Jsi_DSInit(&pNew->dScript);
    pNew->pNext = pDb->pFunc;
    pDb->pFunc = pNew;
    return pNew;
}

static int SqliteFunctionCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                             Jsi_Value **ret, Jsi_Func *funcPtr)
{
    SqlFunc *pFunc;
    Jsi_Value *tocall, *nVal;
    char *zName;
    int rc, nArg = -1, argc;
    argc = Jsi_ValueGetLength(interp, args);
    Jsi_Db *pDb;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    zName = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    tocall = Jsi_ValueArrayIndex(interp, args, 1);
    if (zName == NULL) {
        Jsi_LogError("expected name");
        return JSI_ERROR;
    }
    if (!Jsi_ValueIsFunction(interp, tocall)) {
        Jsi_LogError("expected function");
        return JSI_ERROR;
    }
    if (argc == 3) {
        nVal = Jsi_ValueArrayIndex(interp, args, 2);
        if (Jsi_GetIntFromValue(interp, nVal, &nArg) != JSI_OK)
            return JSI_ERROR;
    } else {
        Jsi_FunctionArguments(interp, tocall, &nArg);
    }
    if (nArg > _SQLITEN_(LIMIT_FUNCTION_ARG)) {
        Jsi_LogError("to many args");
        return JSI_ERROR;
    }
    /*  if( argc==6 ){
        const char *z = Jsi_GetString(objv[3]);
        int n = strlen30(z);
        if( n>2 && strncmp(z, "-argcount",n)==0 ){
          if( Jsi_GetIntFromObj(interp, objv[4], &nArg) ) return JSI_ERROR;
          if( nArg<0 ){
            Jsi_LogError( "number of arguments must be non-negative");
            return JSI_ERROR;
          }
        }
        pScript = objv[5];
      }else if( argc!=4 ){
        Jsi_WrongNumArgs(interp, 2, objv, "NAME [-argcount N] SCRIPT");
        return JSI_ERROR;
      }else{
        pScript = objv[3];
      }*/
    pFunc = findSqlFunc(pDb, zName);
    if( pFunc==0 ) return JSI_ERROR;
    SQLSIGASSERT(pFunc,FUNC);

    pFunc->tocall = tocall;
    Jsi_IncrRefCount(interp, tocall);
#ifdef USE_SQLITE_V4
    rc = _SQL_LITE_N_(_create_function)(pDb->db, zName, nArg,
                                 pFunc, jsiSqlFunc, 0, 0, 0);
#else
    rc = _SQL_LITE_N_(_create_function)(pDb->db, zName, nArg, _SQLITEN_(UTF8),
                                 pFunc, jsiSqlFunc, 0, 0);
#endif
    if( rc!=_SQLITEN_(OK) ) {
        rc = JSI_ERROR;
        Jsi_LogError("function create error: %s", (char *)_SQL_LITE_N_(_errmsg)(pDb->db));
    }
    return JSI_OK;
}

static int SqliteLastInsertRowidCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Wide rowid;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    rowid = DbLastInsertRowid(pDb);
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)rowid);
    return JSI_OK;
}

static int SqliteInterruptCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                              Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    _SQL_LITE_N_(_interrupt)(pDb->db);
    return JSI_OK;
}


static int SqliteCompleteCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                             Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *s = Jsi_ValueArrayIndex(interp, args, 0);
    const char *str =  Jsi_ValueString(interp, s, NULL);
    int isComplete = 0;
    if (str)
        isComplete = _SQL_LITE_N_(_complete)( str );
    Jsi_ValueMakeBool(interp, *ret, isComplete);
    return JSI_OK;
}

static int SqliteEnableLoadCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                               Jsi_Value **ret, Jsi_Func *funcPtr)
{
#ifndef SQLITE_OMIT_LOAD_EXTENSION
    Jsi_Db *pDb;
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    int onoff;
    if( Jsi_GetBoolFromValue(interp, arg, &onoff) != JSI_OK) {
        return JSI_ERROR;
    }
    _SQL_LITE_N_(_enable_load_extension)(pDb->db, onoff);
    return JSI_OK;
#else
    Jsi_LogError("extension loading is turned off at compile-time");
    return JSI_ERROR;
#endif
}

static int SqliteErrorCodeCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                              Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int n;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    n = _SQL_LITE_N_(_errcode)(pDb->db);
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)n);
    return JSI_OK;
}


static int SqliteChangesCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                            Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int n;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    n = _SQL_LITE_N_(_changes)(pDb->db);
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)n);
    return JSI_OK;
}


static int SqliteTotalChangesCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                 Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int n;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    n = _SQL_LITE_N_(_total_changes)(pDb->db);
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)n);
    return JSI_OK;
}

#ifndef OMIT_SQLITE_HOOK_COMMANDS

#define FN_restore JSI_INFO("\
   db.restore(FILENAME, ?,DATABASE? ) \
\n\
Open a database file named FILENAME.  Transfer the content \
of FILENAME into the local database DATABASE (default: 'main').")

static int SqliteRestoreCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                            Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    const char *zSrcFile;
    const char *zDestDb;
    _SQL_LITE_N_() *pSrc;
    _SQL_LITE_N_(_backup) *pBackup;
    int nTimeout = 0, rc;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *vFile = Jsi_ValueArrayIndex(interp, args, 0);
    int argc = Jsi_ValueGetLength(interp, args);
    if( argc==1 ) {
        zDestDb = "main";
    } else {
        zDestDb = Jsi_ValueArrayIndexToStr(interp, args, 1, NULL);
    }
    Jsi_DString dStr = {};
    if (!vFile)
        zSrcFile = ":memory:";
    else {
        zSrcFile = Jsi_ValueNormalPath(interp, vFile, &dStr);
        if (zSrcFile == NULL) {
            Jsi_LogError("bad or missing file name");
            return JSI_ERROR;
        }
    }
#ifdef USE_SQLITE_V4
    rc = _SQL_LITE_N_(_open)(pDb->pEnv, zSrcFile, &pSrc, _SQLITEN_(OPEN_READONLY), NULL);
#else
    rc = _SQL_LITE_N_(_open_v2)(zSrcFile, &pSrc, _SQLITEN_(OPEN_READONLY), 0);
#endif
    if( rc!=_SQLITEN_(OK) ) {
        Jsi_LogError("cannot open source database: %s", _SQL_LITE_N_(_errmsg)(pSrc));
        DbClose(pSrc);
        Jsi_DSFree(&dStr);
        return JSI_ERROR;
    }
    pBackup = _SQL_LITE_N_(_backup_init)(pDb->db, zDestDb, pSrc, "main");
    if( pBackup==0 ) {
        Jsi_LogError("restore failed: %s", _SQL_LITE_N_(_errmsg)(pDb->db));
        DbClose(pSrc);
        Jsi_DSFree(&dStr);
        return JSI_ERROR;
    }
    while( (rc = _SQL_LITE_N_(_backup_step)(pBackup,100))==_SQLITEN_(OK)
            || rc==_SQLITEN_(BUSY) ) {
        if( rc==_SQLITEN_(BUSY) ) {
            if( nTimeout++ >= 3 ) break;
            _SQL_LITE_N_(_sleep)(100);
        }
    }
    _SQL_LITE_N_(_backup_finish)(pBackup);
    if( rc==_SQLITEN_(DONE) ) {
        rc = JSI_OK;
    } else if( rc==_SQLITEN_(BUSY) || rc==_SQLITEN_(LOCKED) ) {
        Jsi_LogError("restore failed: source database busy");
        rc = JSI_ERROR;
    } else {
        Jsi_LogError("restore failed: %s", _SQL_LITE_N_(_errmsg)(pDb->db));
        rc = JSI_ERROR;
    }
    Jsi_DSFree(&dStr);
    DbClose(pSrc);
    return rc;
}
#endif

#define FN_status JSI_INFO("\
    db.status('steps'|'sorts') \
\n\
Display _SQLITEN_(STMTSTATUS_FULLSCAN_STEP) or \
_SQLITEN_(STMTSTATUS_SORT) for the most recent eval.")

static int SqliteStatusCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                           Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int v;
    const char *zOp;
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    zOp = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    if(zOp &&  Jsi_Strcmp(zOp, "steps")==0 ) {
        v = pDb->nStep;
    } else if(zOp && Jsi_Strcmp(zOp, "sorts")==0 ) {
        v = pDb->nSort;
    } else {
        Jsi_LogError("bad argument: should be steps or sorts");;
        return JSI_ERROR;
    }
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)v);
    return JSI_OK;
}

#ifndef OMIT_SQLITE_HOOK_COMMANDS

static int SqliteTimeoutCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                            Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    Jsi_Number n;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *s = Jsi_ValueArrayIndex(interp, args, 0);
    Jsi_GetNumberFromValue(interp, s, &n);
    _SQL_LITE_N_(_busy_timeout)( pDb->db, (int)n );
    return JSI_OK;
}
#endif

#define FN_transaction JSI_INFO("\
   db.transaction(FUNC ?,'deferred'|'immediate'|'exclusive'?)\
\n\
Start a new transaction (if we are not already in the midst of a \
transaction) and execute the JS function FUNC.  After FUNC \
completes, either commit the transaction or roll it back if FUNC \
throws an exception.  Or if no new transation was started, do nothing. \
pass the exception on up the stack.")
static int SqliteTransactionCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                                Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int rc;
    Jsi_Db *pDb;

    int argc = Jsi_ValueGetLength(interp, args);
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;

    Jsi_Value *pScript;
    const char *zBegin = "SAVEPOINT _jsi_transaction";

    if( pDb->nTransaction==0 && argc==2 ) {
        Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);
        static const char *TTYPE_strs[] = {
            "deferred",   "exclusive",  "immediate", 0
        };
        enum TTYPE_enum {
            TTYPE_DEFERRED, TTYPE_EXCLUSIVE, TTYPE_IMMEDIATE
        };
        int ttype;
        if( Jsi_ValueGetIndex(interp, arg, TTYPE_strs, "transaction type",
                              0, &ttype) ) {
            return JSI_ERROR;
        }
        switch( (enum TTYPE_enum)ttype ) {
        case TTYPE_DEFERRED:    /* no-op */
            ;
            break;
        case TTYPE_EXCLUSIVE:
            zBegin = "BEGIN EXCLUSIVE";
            break;
        case TTYPE_IMMEDIATE:
            zBegin = "BEGIN IMMEDIATE";
            break;
        }
    }
    pScript = Jsi_ValueArrayIndex(interp, args, argc-1);
    if(!Jsi_ValueIsFunction(interp, pScript)) {
        Jsi_LogError("expected function");
        return JSI_ERROR;
    }

    /* Run the SQLite BEGIN command to open a transaction or savepoint. */
    pDb->disableAuth++;
    rc = _SQL_LITE_N_(_exec)(pDb->db, zBegin, 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
    );
    pDb->disableAuth--;
    if( rc!=_SQLITEN_(OK) ) {
        Jsi_LogError("%s", _SQL_LITE_N_(_errmsg)(pDb->db));
        return JSI_ERROR;
    }
    pDb->nTransaction++;

    /* Evaluate the function , then
    ** call function DbTransPostCmd() to commit (or rollback) the transaction
    ** or savepoint.  */
    rc = Jsi_FunctionInvoke(interp, pScript, NULL, NULL, NULL);
    rc = DbTransPostCmd(pDb, interp, rc);
    return rc;
}

static int SqliteVersionCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                            Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    char *str;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    str=Jsi_Strdup((char *)_SQL_LITE_N_(_libversion)());
    Jsi_ValueMakeString(interp, *ret, str);
    return JSI_OK;
}

static int SqliteCacheSizeCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                              Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    Jsi_Number n;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *s = Jsi_ValueArrayIndex(interp, args, 0);
    if (s && Jsi_GetNumberFromValue(interp, s, &n) == JSI_OK) {
        if( n>MAX_PREPARED_STMTS)
            n = MAX_PREPARED_STMTS;
        pDb->maxStmts = n;
    }
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)pDb->maxStmts);
    return JSI_OK;
}

#ifndef OMIT_SQLITE_HOOK_COMMANDS

#define FN_backup JSI_INFO("\
    db.backup(FILENAME, ?DATABASE?) \
\n\
Open or create a database file named FILENAME.  Transfer the \
content of local database DATABASE (default: 'main') into the \
FILENAME database.")

static int SqliteBackupCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                           Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    int rc;
    const char *zDestFile;
    const char *zSrcDb;
    _SQL_LITE_N_() *pDest;
    _SQL_LITE_N_(_backup) *pBackup;

    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    Jsi_Value *vFile = Jsi_ValueArrayIndex(interp, args, 0);
    int argc = Jsi_ValueGetLength(interp, args);
    if( argc==1 ) {
        zSrcDb = "main";
    } else {
        zSrcDb = Jsi_ValueArrayIndexToStr(interp, args, 1, NULL);
    }
    Jsi_DString dStr = {};
    if (!vFile)
        zDestFile = ":memory:";
    else {
        zDestFile = Jsi_ValueNormalPath(interp, vFile, &dStr);
        if (zDestFile == NULL) {
            Jsi_LogError("bad or missing file name");
            return JSI_ERROR;
        }
    }
#ifdef USE_SQLITE_V4
    rc = _SQL_LITE_N_(_open)(pDb->pEnv, zDestFile, &pDest, NULL);
#else
    rc = _SQL_LITE_N_(_open)(zDestFile, &pDest);
#endif
    if( rc!=_SQLITEN_(OK) ) {
        Jsi_LogError("cannot open target database %s: %s", zDestFile, _SQL_LITE_N_(_errmsg)(pDest));
        DbClose(pDest);
        Jsi_DSFree(&dStr);
        return JSI_ERROR;
    }
    pBackup = _SQL_LITE_N_(_backup_init)(pDest, "main", pDb->db, zSrcDb);
    if( pBackup==0 ) {
        Jsi_LogError("backup failed: %s", _SQL_LITE_N_(_errmsg)(pDest));
        DbClose(pDest);
        Jsi_DSFree(&dStr);
        return JSI_ERROR;
    }
    while(  (rc = _SQL_LITE_N_(_backup_step)(pBackup,100))==_SQLITEN_(OK) ) {}
    _SQL_LITE_N_(_backup_finish)(pBackup);
    if( rc==_SQLITEN_(DONE) ) {
        rc = JSI_OK;
    } else {
        Jsi_LogError("backup failed: %s", _SQL_LITE_N_(_errmsg)(pDest));
        rc = JSI_ERROR;
    }
    Jsi_DSFree(&dStr);
    DbClose(pDest);
    return rc;
}
#endif

static int SqliteCacheFlushCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                               Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    flushStmtCache( pDb );
    return JSI_OK;
}

static int SqliteConfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
                         Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Db *pDb;
    if (!(pDb = getDb(interp, _this, funcPtr))) return JSI_ERROR;
    //pDb->hasOpts = 1;
    return Jsi_OptionsConf(interp, SqlOptions, Jsi_ValueArrayIndex(interp, args, 0), pDb, *ret, 0);
}

static Jsi_CmdSpec sqliteCmds[] = {
    { "Sqlite",         SqliteConstructor,      0,  2,  "?file?,options??", JSI_CMD_IS_CONSTRUCTOR, 
        .help="Create a new db connection to the named file or :memory:", .opts=SqlOptions,},
#ifndef SQLITE_OMIT_AUTHORIZATION
    { "authorizor",     SqliteAuthorizorCmd,    1,  1, "?func?", .help="Setup authorizor", .info=FN_authorizer },
#endif
#ifndef OMIT_SQLITE_HOOK_COMMANDS
    { "backup",         SqliteBackupCmd,        1,  2, "file?,dbname?", .help="Backup db to file (default db is 'main')", .info=FN_backup },
    { "busy",           SqliteBusyCmd,          0,  1, "?func?", .help="Function callback upon open busy", .info=FN_busy },
#endif
    { "cache_flush",    SqliteCacheFlushCmd,    0,  0, "", .help="Flush the prepared statement cache." },
    { "cache_size",     SqliteCacheSizeCmd,     0,  1, "?size?", .help="Set/get the size of the prepared statement cache" },
    { "changes",        SqliteChangesCmd,       0,  0, "", .help="Return the number of rows that were modified, inserted, or deleted by last command." },
    { "close",          SqliteCloseCmd,         0,  0, "", .help="Close db" },
#ifndef OMIT_SQLITE_COLLATION
    { "collate",        SqliteCollateCmd,       2,  2, "name,func?", .help="Create new SQL collation command" },
    { "collation_needed",SqliteCollationNeededCmd,          0,  1, "?func?", .help="Set/get func to call on unknown collation" },
#endif
#ifndef OMIT_SQLITE_HOOK_COMMANDS
    { "commit_hook",    SqliteCommitHookCmd,    0,  1, "?func?", .help="Set/get func to call on commit", .info=FN_commithook },
#endif
    { "complete",       SqliteCompleteCmd,      1,  1, "sql", .help="Return true if sql is complete" },
    { "conf",           SqliteConfCmd,          0,  1, "?string|options?", .help="Configure options", .opts=SqlOptions },
    { "enable_load_extension", SqliteEnableLoadCmd, 1,  1, "bool", .help="En/disable loading of extensions (default false)"},
    { "errorcode",      SqliteErrorCodeCmd,     0,  0, "", .help = "Return the numeric error code that was returned by the most recent call to sqlite3_exec()" },
    { "evaluate",       SqliteEvaluateCmd,      1,  1, "sql", .help="Execute semicolon seperated sql statments, without var substitution", .info=FN_evaluate },
    { "exec",           SqliteExecCmd,          1,  2, "sql?,func|options?", .help="Execute sql statement with bindings", .opts=ExecFmtOptions, .info=FN_sqlexec },
    { "exists",         SqliteExistsCmd,        1,  1, "sql", .help="Execute sql, and return true if there is at least one result value" },
    { "filename",       SqliteFilenameCmd,      0,  1, "?name?", .help="Return filename for named or all attached dbs. Defaults to 'main'" },
    { "func",           SqliteFunctionCmd,      2,  3, "name,func?,numArgs?", .help="Register a new function with database" },
    { "import",         SqliteImportCmd,        2,  3, "table,file?,options?", .help="Import data from file into table ", .info=FN_import, .opts=ImportOptions },
    { "interrupt",      SqliteInterruptCmd,     0,  0, "", .help="Interrupt in progress statement" },
    { "last_insert_rowid",SqliteLastInsertRowidCmd,0,  0, "", .help="Return rowid of last insert" },
    { "onecolumn",      SqliteOnecolumnCmd,         1,  1, "sql", .help="Execute sql, and return a single value" },
    { "profile",        SqliteProfileCmd,       0,  1, "?func?", .help="Set/get func to call on every SQL executed. Call args are: SQL,time", .info=FN_profile },
#ifndef OMIT_SQLITE_HOOK_COMMANDS
    { "progress",       SqliteProgressCmd,      0,  2, "?N,func?", .help="Set/get func to call on every N VM opcodes executed" },
#endif
    { "rekey",          SqliteRekeyCmd,         1,  1, "key", .help="Change the encryption key on the currently open database" },
#ifndef OMIT_SQLITE_HOOK_COMMANDS
    { "restore",        SqliteRestoreCmd,       1,  2, "file?,dbname?", .help="Restore db from file (default db is 'main')", .info=FN_restore },
    { "rollback_hook",  SqliteRollbackHookCmd,  0,  1, "?func?", .help="Set/get func to call on rollback" },
#endif
    { "status",         SqliteStatusCmd,        1,  1, "'steps'|'sorts'", .help="Return number of steps or sorts from last query", .info=FN_status},
#ifndef OMIT_SQLITE_HOOK_COMMANDS
    { "timeout",        SqliteTimeoutCmd,       1,  1, "milliseconds", .help="Delay for the number of milliseconds specified when a file is locked"},
#endif
    { "total_changes",  SqliteTotalChangesCmd,  0,  0, "", .help="Return the number of rows that were modified, inserted, or deleted since db opened" },
    { "trace",          SqliteTraceCmd,         0,  1, "?func?", .help="Set/get func to trace SQL: Call args are: SQL", .info=FN_trace },
    { "transaction",    SqliteTransactionCmd,   1,  2, "func?,type?", .help="Call function inside db tranasaction. Type is: 'deferred', 'exclusive', 'immediate'", .info=FN_transaction },
#ifndef OMIT_SQLITE_HOOK_COMMANDS
    { "update_hook",    SqliteUpdateHookCmd,    0,  1, "?func?", .help="Set/get func to call on update: Call args are: OP,db,table,rowid" },
    { "unlock_notify",  SqliteUnlockNotifyCmd,  0,  1, "?func?", .help="Set/get func to call on unlock" },
    { "wal_hook",       SqliteWalHookCmd,       0,  1, "?func?", .help="Set/get func to call on wal commit: Call args are: db,numEntries" },
#endif
    { "version",        SqliteVersionCmd,       0,  0, "", .help="Return database verion string" },
    { NULL, .help="Commands for accessing sqlite databases" }
};

#endif

typedef struct {
    Jsi_DbMultipleBind *binds;
    Jsi_OptionSpec *rowidPtr, *dirtyPtr;
    int optLen;             /* Length of binds[0].binds */
} OptionBind;

static int dbBindOptionStmt(Jsi_Db *jdb, _SQL_LITE_N_(_stmt) *pStmt, OptionBind *obPtr,
                            int dataIdx, int bmax, int flags)
{
    Jsi_DbMultipleBind *binds = obPtr->binds;
    Jsi_Interp *interp = jdb->interp;
    int j, k, cnt = 0, idx, sidx = -1, rc = 0;
    Jsi_OptionSpec *specPtr, *specs;
    void *rec;
    Jsi_DString *eStr;
    const char *bName;
    int lastBind = _SQL_LITE_N_(_bind_parameter_count)(pStmt);
    if (lastBind<=0)
        return JSI_OK;
    int structSize = 0;
    _SQL_LITE_N_(_destructor_type) statFlags = ((flags&JSI_DB_NO_STATIC)?_SQLITEN_(TRANSIENT):_SQLITEN_(STATIC));
    specPtr = binds[0].opts;
    structSize = specPtr[obPtr->optLen].size;
    
    for (j=1; j<=lastBind; j++) {
        bName = _SQL_LITE_N_(_bind_parameter_name)(pStmt, j);
        if (bName==NULL || bName[0]==0 || bName[1]==0)
            continue;
        idx = j;
        /*        if (bName[0] != '?')
                    idx = j;
                else {
                    idx = _SQL_LITE_N_(_bind_parameter_index)(pStmt, bName);
                    if (idx<=0)
                        continue;
                }*/
        if (binds[0].prefix==0)
            k = 0;
        else {
            for (k=0; binds[k].opts; k++) {
                if (bName[0] == binds[k].prefix)
                    break;
            }
            if (bmax>0 && k>=bmax)
                continue;
            if (!binds[k].opts) {
                Jsi_LogError("bad bind: %s", bName);
                continue;
            }
        }
        specs = binds[k].opts;
        rec = binds[k].data;
        if (k==0) {
            if (flags & JSI_DB_PTRS)
                rec = ((void**)rec)[dataIdx];
            else
                rec = (char*)rec + (dataIdx * structSize);
        }
        if (bName[0] == '?')
            sidx = atoi(bName+1);
        for (specPtr = specs, cnt=1; specPtr->type>JSI_OPTION_NONE && specPtr->type < JSI_OPTION_END; specPtr++, cnt++) {
            if (specPtr->flags&JSI_OPT_DB_IGNORE)
                continue;
            if (bName[0] == '?') {
                if (cnt == sidx)
                    break;
            } else {
                const char *sName = specPtr->name;
                if (bName[1] == sName[0] && !Jsi_Strcmp(bName+1, sName))
                    break;
            }
        }
        if (specPtr->type<=JSI_OPTION_NONE || specPtr->type>=JSI_OPTION_END) {
            Jsi_LogError("unknown bind: %s", bName);
            return JSI_ERROR;
        }

        char *ptr = (char *)rec + specPtr->offset;
        switch (specPtr->type) {
        case JSI_OPTION_BOOL:
            rc = _SQL_LITE_N_(_bind_int)(pStmt, idx, *(int*)ptr);
            break;
        case JSI_OPTION_INT:
            rc = _SQL_LITE_N_(_bind_int64)(pStmt, idx, *(int*)ptr);
            break;
        case JSI_OPTION_DATETIME:
        case JSI_OPTION_WIDE:
            rc = _SQL_LITE_N_(_bind_int64)(pStmt, idx, *(Jsi_Wide*)ptr);
            break;
        case JSI_OPTION_DOUBLE:
            rc = _SQL_LITE_N_(_bind_double)(pStmt, idx, *(Jsi_Number*)ptr);
            break;
        case JSI_OPTION_CUSTOM: {
            Jsi_OptionCustom* cust = Jsi_OptionCustomBuiltin(specPtr->custom);
            if (cust && cust->formatProc) {
                Jsi_DString dStr;
                Jsi_DSInit(&dStr);
                if ((*cust->formatProc)(interp, specPtr, NULL, &dStr, rec) != JSI_OK) {
                    Jsi_DSFree(&dStr);
                    return JSI_ERROR;
                }
                rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, Jsi_DSValue(&dStr), -1, _SQLITEN_(TRANSIENT) _SQLBIND_END_);
                Jsi_DSFree(&dStr);
            } else {
                Jsi_LogError("missing or invalid custom for \"%s\"", specPtr->name);
                return JSI_ERROR;
            }
            break;
        }
        case JSI_OPTION_DSTRING:
            eStr = (Jsi_DString*)ptr;
            if (jdb->execOpts.nullvalue && !Jsi_Strcmp(jdb->execOpts.nullvalue, Jsi_DSValue(eStr)))
                rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, NULL, -1, statFlags _SQLBIND_END_);
            else
                rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, Jsi_DSValue(eStr), -1, statFlags _SQLBIND_END_);
            break;
        case JSI_OPTION_STRBUF:
            if (jdb->execOpts.nullvalue && ptr && !Jsi_Strcmp(jdb->execOpts.nullvalue, (char*)ptr))
                rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, NULL, -1, statFlags _SQLBIND_END_);
            else
                rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, (char*)ptr, -1, statFlags _SQLBIND_END_);
            break;
        case JSI_OPTION_STRKEY:
            rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, (char*)ptr, -1, _SQLITEN_(STATIC) _SQLBIND_END_);
            break;
#ifndef JSI_LITE_ONLY
        case JSI_OPTION_STRING:
            rc = _SQL_LITE_N_(_bind_text)(pStmt, idx, Jsi_ValueString(interp, *((Jsi_Value **)ptr), NULL), -1, statFlags _SQLBIND_END_);
            break;
#else
        case JSI_OPTION_STRING:
#endif
        case JSI_OPTION_VALUE: /* Unsupported. */
        case JSI_OPTION_VAR:
        case JSI_OPTION_OBJ:
        case JSI_OPTION_ARRAY:
        case JSI_OPTION_FUNC:
        case JSI_OPTION_END:
        case JSI_OPTION_NONE:
        default:
            Jsi_LogError("unsupported jdb option type \"%s\" for \"%s\"", Jsi_OptionTypeStr(specPtr->type), specPtr->name);
            return JSI_ERROR;

        }
        if (rc != _SQLITEN_(OK))
            Jsi_LogError("bind failure: %s", _SQL_LITE_N_(_errmsg)(jdb->db));
    }
    cnt++;
    return JSI_OK;
}

/* Prepare, bind, then step.
 * If there are results return JSI_OK. On error return JSI_ERROR;
 */
static int dbEvalStepOption(DbEvalContext *p, OptionBind *obPtr, int *cntPtr, int didx, int bmax, int flags) {
    //Jsi_DbMultipleBind *binds = obPtr->binds;
    Jsi_Db *jdb = p->jdb;
    int cnt = 0;
    while( p->zSql[0] || p->pPreStmt ) {
        int rc;
        cnt++;
        if( p->pPreStmt==0 ) {
            rc = dbPrepareStmt(p->jdb, p->zSql, &p->zSql, &p->pPreStmt);
            if( rc!=JSI_OK ) return rc;
        }
        if (bmax!=0) {
            rc = dbBindOptionStmt(jdb, p->pPreStmt->pStmt, obPtr, didx, bmax, flags);
            if( rc!=JSI_OK ) return rc;
        }
        rc = dbEvalStepSub(p, 1);
        if (rc != JSI_BREAK)
            return rc;
        *cntPtr = cnt;
    }
    
    /* Finished */
    return JSI_BREAK;
}

static Jsi_OptionSpec* LookupSpecFromName(Jsi_OptionSpec *specs, const char *name) {
    Jsi_OptionSpec *specPtr = NULL;
    for (specPtr = specs; specPtr->type>JSI_OPTION_NONE && specPtr->type < JSI_OPTION_END; specPtr++) {
        if  (specPtr->flags&JSI_OPT_DB_IGNORE)
            continue;
        const char *cname = (specPtr->extName?specPtr->extName:specPtr->name);
        if (cname[0] == name[0] && !Jsi_Strncasecmp(cname, name, -1))
            return specPtr;
    }
    return NULL;
}

const char* Jsi_DbKeyAdd(Jsi_Db *jdb, const char *str)
{
#ifndef JSI_LITE_ONLY
    if (jdb->interp)
        return Jsi_KeyAdd(jdb->interp, str);
#endif
    Jsi_HashEntry *hPtr;
    int isNew;
    hPtr = Jsi_HashEntryCreate(jdb->strKeyTbl, str, &isNew);
    assert(hPtr) ;
    return (const char*)Jsi_HashKeyGet(hPtr);
}

static int jsiDbOptSelect(Jsi_Db *jdb, const char *cmd, OptionBind *obPtr, int flags)
{
    Jsi_DbMultipleBind *binds = obPtr->binds;
    void *rec = binds[0].data, **recPtrPtr = NULL;
    Jsi_Interp *interp = jdb->interp;
    Jsi_OptionSpec *specPtr, *specs = binds[0].opts;
    DbEvalContext sEval;
    int ccnt = 0;
    const char *cPtr = strstr(cmd, " %s");
    if (!cPtr) cPtr = strstr(cmd, "\t%s");
    Jsi_DString *eStr;
#ifdef JSI_DB_DSTRING_SIZE
    JSI_DSTRING_VAR(dStr, JSI_DB_DSTRING_SIZE);
#else
    Jsi_DString sStr, *dStr = &sStr;
    Jsi_DSInit(dStr);
#endif
    dbEvalInit(interp, &sEval, jdb, NULL, dStr, 0, 0);
    if (flags&JSI_DB_NO_CACHE)
        sEval.nocache = 1;
    Jsi_DSAppendLen(dStr, cmd, cPtr?(cPtr-cmd):-1);
    if (cPtr) {
        Jsi_DSAppend(dStr, " ", NULL);
        for (specPtr = specs; specPtr->type>JSI_OPTION_NONE && specPtr->type < JSI_OPTION_END; specPtr++) {
            if (specPtr == obPtr->dirtyPtr || (specPtr->flags&JSI_OPT_DB_IGNORE))
                continue;
            if (ccnt)
                Jsi_DSAppendLen(dStr, ",", 1);
            Jsi_DSAppend(dStr, "[", specPtr->extName?specPtr->extName:specPtr->name, "]", NULL);
            ccnt++; 
        }
        Jsi_DSAppend(dStr, cPtr+3, NULL);
    }
    sEval.zSql = Jsi_DSValue(dStr);
    sEval.nocache = jdb->execOpts.nocache;
    int rc, structSize = 0;
    int cnt = 0, dmax = ((flags&JSI_DB_PTR_PTRS)?0:1);
    int multi = ((flags&JSI_DB_PTR_PTRS)!=0);
    int dnum = binds[0].numData;
    if (dnum<=0 && !(flags&JSI_DB_PTR_PTRS)) {
        dmax = dnum = 1;
    }
    if (dnum>1) {
        multi = 1;
        dmax = binds[0].numData;
    }
    if (flags&JSI_DB_PTR_PTRS) {
        recPtrPtr = (void**)rec; /* This is really a void***, but this gets recast below. */
        rec = *recPtrPtr;
    }
    structSize = specs[obPtr->optLen].size;

    cnt = 0;
    int ncnt = 0, bmax = -1, didx = -1;
    while(1) {
        didx++;
        if (didx>=dmax) {
            if (!(flags&JSI_DB_PTR_PTRS))
                break;
            else {
            /* Handle fully dynamic allocation of memory. */
#ifndef JSI_DB_MAXDYN_SIZE
#define JSI_DB_MAXDYN_SIZE 100000000
#endif
#ifndef JSI_DB_DYN_INCR
#define JSI_DB_DYN_INCR 16
#endif
                if (dmax>=JSI_DB_MAXDYN_SIZE)
                    break;
                int olddm = dmax;
                dmax += JSI_DB_DYN_INCR;
                if (dmax>JSI_DB_MAXDYN_SIZE)
                    dmax = JSI_DB_MAXDYN_SIZE;
                if (!olddm)
                    rec = Jsi_Calloc(dmax+1, sizeof(void*));
                else {
                    rec = Jsi_Realloc(rec, (dmax+1)*sizeof(void*));
                    memset((char*)rec+olddm*sizeof(void*), 0, (dmax-olddm+1)*sizeof(void*));
                }
                *recPtrPtr = rec;
            }
        }

        rc = dbEvalStepOption(&sEval, obPtr, &ncnt, didx, bmax, flags);
        if (rc == JSI_ERROR)
            break;
        if (rc != JSI_OK)
            break;
        cnt += ncnt;
        _SQL_LITE_N_(_stmt) *pStmt = sEval.pPreStmt->pStmt;
        int idx;
        int nCol;
        char **apColName;
        const char *str;
        int *apColType;
        void *prec = rec;
        bmax = 0;

        if (flags & (JSI_DB_PTR_PTRS|JSI_DB_PTRS)) {
            prec = ((void**)rec)[didx];
            if (!prec)
                ((void**)rec)[didx] = prec = Jsi_Calloc(1, structSize);
        } else
                prec = (char*)rec + (didx * structSize);
        dbEvalRowInfo(&sEval, &nCol, &apColName, &apColType);
        for (idx=0; idx<nCol; idx++) {
            specPtr = LookupSpecFromName(specs, apColName[idx]);
            if (!specPtr) {
                Jsi_LogError("unknown column name: %s", apColName[idx]);
                goto bail;
            }          
            if (specPtr->type<=JSI_OPTION_NONE || specPtr->type>=JSI_OPTION_END) {
                Jsi_LogError("unknown option type \"%d\" for \"%s\"", specPtr->type, specPtr->name);
                goto bail;
            }
            char *ptr = (char*)prec + specPtr->offset;

            switch (specPtr->type) {
                case JSI_OPTION_BOOL:
                    *(int*)ptr = _SQL_LITE_N_(_column_int)(pStmt, idx);
                    break;
                case JSI_OPTION_INT:
                    *(int*)ptr = (int)_SQL_LITE_N_(_column_int64)(pStmt, idx);
                    break;
                case JSI_OPTION_DATETIME:
                case JSI_OPTION_WIDE:
                    *(Jsi_Wide*)ptr = (Jsi_Wide)_SQL_LITE_N_(_column_int64)(pStmt, idx);
                    break;
                case JSI_OPTION_DOUBLE:
                    *(Jsi_Number*)ptr = (Jsi_Number)_SQL_LITE_N_(_column_double)(pStmt, idx);
                    break;
                case JSI_OPTION_DSTRING:
                    eStr = (Jsi_DString*)ptr;
                    str = (char*)_SQL_LITE_N_(_column_text)(pStmt, idx _SQLBIND_END_);
                    if (!str)
                        str = jdb->execOpts.nullvalue;
                    Jsi_DSSet(eStr, str?str:"");
                    break;
                case JSI_OPTION_STRBUF:
                    str = (char*)_SQL_LITE_N_(_column_text)(pStmt, idx _SQLBIND_END_);
                    if (!str)
                        str = jdb->execOpts.nullvalue;
                    strncpy((char*)ptr, str?str:"", specPtr->size);
                    ((char*)ptr)[specPtr->size-1] = 0;
                    break;
                case JSI_OPTION_CUSTOM: {
                    Jsi_OptionCustom* cust = Jsi_OptionCustomBuiltin(specPtr->custom);
                    if (cust && cust->parseProc) {
                        str = (char*)_SQL_LITE_N_(_column_text)(pStmt, idx _SQLBIND_END_);
                        if ((*cust->parseProc)(interp, specPtr, NULL, str, prec) != JSI_OK) {
                            goto bail;
                        }
                    } else {
                        Jsi_LogError("missing or invalid custom for \"%s\"", specPtr->name);
                        goto bail;
                    }
                    break;
                }
                case JSI_OPTION_STRKEY:
                    str = (char*)_SQL_LITE_N_(_column_text)(pStmt, idx _SQLBIND_END_);
                    if (!str)
                        str = jdb->execOpts.nullvalue;
                    *(char**)ptr = (str?(char*)Jsi_DbKeyAdd(jdb, str):NULL);
                    break;
#ifndef JSI_LITE_ONLY
                case JSI_OPTION_STRING: {
                    Jsi_Value *vPtr = *((Jsi_Value **)ptr);
                    if (!(flags&JSI_OPT_NO_DUPVALUE)) {
                        if (vPtr) Jsi_DecrRefCount(interp, vPtr);
                        *((Jsi_Value **)ptr) = NULL;
                    }
                    str = (char*)_SQL_LITE_N_(_column_text)(pStmt, idx _SQLBIND_END_);
                    if (!str)
                        str = jdb->execOpts.nullvalue;
                    if (str) {
                        vPtr = Jsi_ValueNewStringDup(interp, str);
                        *((Jsi_Value **)ptr) = vPtr;
                    }
                    break;
                }
#else
                case JSI_OPTION_STRING:        
#endif
                case JSI_OPTION_VALUE: /* The rest are unsupported. */
                case JSI_OPTION_VAR:
                case JSI_OPTION_OBJ:
                case JSI_OPTION_ARRAY:
                case JSI_OPTION_FUNC:
                
                case JSI_OPTION_END:
                case JSI_OPTION_NONE:
                default:
                    fprintf(stderr, "unsupported type: %s\n", Jsi_OptionTypeStr(specPtr->type));
                    break;
            }
        }
        if (binds[0].callback)
            binds[0].callback(interp, binds, prec);
        cnt++;
        if (!multi)
            break;
    }
    dbEvalFinalize(&sEval);
    if( rc==JSI_BREAK ) {
        rc = JSI_OK;
    }
    return (rc==JSI_OK?cnt:-1);

bail:
    dbEvalFinalize(&sEval);
    return -1;
}

static int
jsi_DbExecBinds(Jsi_Db *jdb, Jsi_DbMultipleBind *binds, const char *cmd, int flags)
{
    int k, cnt;
    OptionBind ob = {.binds = binds};
    Jsi_OptionSpec *specPtr, *specs;
    Jsi_Interp *interp = jdb->interp;
    if (!cmd) cmd="";
    const char *cPtr = strstr(cmd, " %s");
    if (!cPtr) cPtr = strstr(cmd, "\t%s");
    if (!binds[0].data) {
        Jsi_LogError("data may not be null");
        return -1;
    }
    for (k=0; binds[k].opts; k++) {
        if (binds[k].numData>1 || k==0) {
            int scnt = 0;
            for (specPtr = binds[k].opts, scnt=0; specPtr->type>JSI_OPTION_NONE
                && specPtr->type < JSI_OPTION_END; specPtr++, scnt++) {
                if (specPtr->flags&JSI_OPT_DB_IGNORE)
                    continue;
                if (k==0) {
                    if (specPtr->type == JSI_OPTION_WIDE && tolower(specPtr->name[0]) == 'r' && !Jsi_Strncasecmp(specPtr->name, "rowid", -1))
                        ob.rowidPtr = specPtr;
                    if (specPtr->flags&JSI_OPT_DB_DIRTY) {
                        if (specPtr->type == JSI_OPTION_BOOL || specPtr->type == JSI_OPTION_INT) {
                            ob.dirtyPtr = specPtr;
                        } else {
                            Jsi_LogError("dirty flag is not a int/bool field: %s", specPtr->name);
                            return -1;
                        }
                    }
                            
                }
            }
            if (k==0)
                ob.optLen = scnt;
            assert(specPtr->type == JSI_OPTION_END);
        }
    }
    specs = binds[0].opts;
    int structSize = specs[ob.optLen].size;
    if (flags & (JSI_DB_MEMCLEAR|JSI_DB_MEMFREE)) {
        cnt = binds[0].numData;
        void *rec = binds[0].data, *prec = rec;
        void **recPtrPtr = NULL;
        if (flags&JSI_DB_PTR_PTRS) {
            recPtrPtr = (void**)rec; /* This is really a void***, but this gets recast below. */
            rec = *recPtrPtr;
        }
        if (cnt<=0 && rec && flags&JSI_DB_PTR_PTRS) {
            for (cnt=0; ((void**)rec)[cnt]!=NULL; cnt++);
        }
        for (k=0; k<cnt; k++) {
            if (flags & (JSI_DB_PTRS|JSI_DB_PTR_PTRS))
                prec = ((void**)rec)[k];
            else
                prec = (char*)rec + (k * structSize);
            if (!prec)
                continue;
            Jsi_OptionsFree(interp, specs, prec, 0);
            if (flags & (JSI_DB_PTRS|JSI_DB_PTR_PTRS)) {
                Jsi_Free(prec);
            }
        }
        if (recPtrPtr) {
            Jsi_Free(*recPtrPtr);
            *recPtrPtr = NULL;
        }
        if (cmd == NULL || cmd[0] == 0)
            return 0;
    }
    if (!Jsi_Strncasecmp(cmd, "SELECT", 6))
        return jsiDbOptSelect(jdb, cmd, &ob, flags);
    DbEvalContext sEval;
    int insert = 0, replace = 0, update = 0;
    char nbuf[100], *bPtr;
#ifdef JSI_DB_DSTRING_SIZE
    JSI_DSTRING_VAR(dStr, JSI_DB_DSTRING_SIZE);
#else
    Jsi_DString sStr, *dStr = &sStr;
    Jsi_DSInit(dStr);
#endif
    dbEvalInit(interp, &sEval, jdb, NULL, dStr, 0, 0);
    if (flags&JSI_DB_NO_CACHE)
        sEval.nocache = 1;
    int dmax = binds[0].numData;
    if (dmax<0) {
        Jsi_LogError("Data size of < 0 is valid only for SELECT");
        return -1;
    }
    cnt = 0;
    if (dmax==0)
        dmax = 1;
    char ch[2];

    ch[0] = binds[0].prefix;
    ch[1] = 0;
    if (!ch[0])
        ch[0] = ':';
    if ((update=(Jsi_Strncasecmp(cmd, "UPDATE", 6)==0))) {
        Jsi_DSAppendLen(dStr, cmd, cPtr?(cPtr-cmd):-1);
        if (cPtr) {
            Jsi_DSAppend(dStr, " ", NULL);
            int cidx = 0;
            int killf = (JSI_OPT_DB_IGNORE|JSI_OPT_READ_ONLY|JSI_OPT_INIT_ONLY);
            for (specPtr = specs; specPtr->type != JSI_OPTION_END; specPtr++, cidx++) {
                if (specPtr == ob.rowidPtr || specPtr == ob.dirtyPtr || (specPtr->flags&killf))
                    continue;
                const char *fname = specPtr->extName?specPtr->extName:specPtr->name;
                if (ch[0] == '?')
                    sprintf(bPtr=nbuf, "%d", cidx+1);
                else
                    bPtr = specPtr->name;
                Jsi_DSAppend(dStr, (cnt?",":""), "[", fname, "]=",
                    ch, bPtr, NULL);
                cnt++;
            }
            Jsi_DSAppend(dStr, cPtr+3, NULL);
        }
    } else if ((insert=(Jsi_Strncasecmp(cmd, "INSERT", 6)==0))
        || (replace=(Jsi_Strncasecmp(cmd, "REPLACE", 7)==0))) {
        Jsi_DSAppendLen(dStr, cmd, cPtr?(cPtr-cmd):-1);
        if (cPtr) {
            Jsi_DSAppend(dStr, " (", NULL);
            int killf = JSI_OPT_DB_IGNORE;
            if (replace)
                killf |= (JSI_OPT_READ_ONLY|JSI_OPT_INIT_ONLY);
            for (specPtr = specs; specPtr->type != JSI_OPTION_END; specPtr++) {
                if (specPtr == ob.rowidPtr || specPtr == ob.dirtyPtr || specPtr->flags&killf)
                    continue;
                const char *fname = specPtr->extName?specPtr->extName:specPtr->name;
                Jsi_DSAppend(dStr, (cnt?",":""), "[", fname, "]", NULL);
                cnt++;
            }
            Jsi_DSAppendLen(dStr,") VALUES(", -1);
            cnt = 0;
            int cidx = 0;
            for (specPtr = specs; specPtr->type != JSI_OPTION_END; specPtr++, cidx++) {
                if (specPtr == ob.rowidPtr || specPtr == ob.dirtyPtr
                    || specPtr->flags&killf)
                    continue;
                if (ch[0] == '?')
                    sprintf(bPtr=nbuf, "%d", cidx+1);
                else
                    bPtr = specPtr->name;
                Jsi_DSAppend(dStr, (cnt?",":""), ch, bPtr, NULL);
                cnt++;
            }
            Jsi_DSAppend(dStr,")", cPtr+3, NULL);
        }
    } else if (!Jsi_Strncasecmp(cmd, "DELETE", 6)) {
        Jsi_DSAppend(dStr, cmd, NULL);
    } else {
        Jsi_LogError("unrecognized cmd \"%s\": expected one of: SELECT, UPDATE, INSERT, REPLACE or DELETE", cmd);
        return -1;
    }
    sEval.zSql = Jsi_DSValue(dStr);

    int rc, bmax = -1, didx = 0;
    cnt = 0;
    int ismodify = (replace||insert||update);
    int isnew = (replace||insert);
    int didBegin = 0;
    DbEvalContext *p = &sEval;
    rc = dbPrepareStmt(p->jdb, p->zSql, &p->zSql, &p->pPreStmt);
    if( rc!=JSI_OK ) return -1;
    if (dmax>1 && !(flags&JSI_DB_NO_BEGINEND)) {
        didBegin = 1;
        if (_SQLITEN_(OK) != _SQL_LITE_N_(_exec)(jdb->db, "BEGIN;", 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
        )) {
            rc = JSI_ERROR;
            goto bail;
        }
    }
    while (didx<dmax) {
        if (ismodify && ob.dirtyPtr && (flags&JSI_DB_DIRTY_ONLY)) { /* Check to limit updates to dirty values only. */
            void *rec = binds[0].data;
            if (flags & (JSI_DB_PTRS|JSI_DB_PTR_PTRS))
                rec = ((void**)rec)[didx];
            else
                rec = (char*)rec + (didx * structSize);
            char *ptr = rec + ob.dirtyPtr->offset;
            int isDirty = *(int*)ptr;
            int bit = 0;
            if (ob.dirtyPtr->type == JSI_OPTION_BOOL)
                bit = (int)ob.dirtyPtr->data;
            if (!(isDirty&(1<<(bit)))) {
                didx++;
                continue;
            }
            isDirty &= ~(1<<(bit));
            *(int*)ptr = isDirty; /* Note that the dirty bit is cleared, even upon error.*/
        }
        rc = dbBindOptionStmt(jdb, p->pPreStmt->pStmt, &ob, didx, bmax, flags);
        if( rc!=JSI_OK )
            goto bail;
        bmax = 1;
        rc = dbEvalStepSub(p, (didx>=dmax));
        if (rc == JSI_ERROR)
            goto bail;
        cnt += _SQL_LITE_N_(_changes)(jdb->db);
        if (rc != JSI_OK && rc != JSI_BREAK)
            break;
        if (ob.rowidPtr && isnew) {
            void *rec = binds[0].data;
            if (flags & (JSI_DB_PTRS|JSI_DB_PTR_PTRS))
                rec = ((void**)rec)[didx];
            else
                rec = (char*)rec + (didx * structSize);
            char *ptr = rec + ob.rowidPtr->offset;
            *(Jsi_Wide*)ptr = DbLastInsertRowid(jdb);
        }
        didx++;
    }
    if (didBegin)
        _SQL_LITE_N_(_exec)(jdb->db, "COMMIT;", 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
        );
    dbEvalFinalize(&sEval);
    if( rc==JSI_BREAK ) {
        rc = JSI_OK;
    }
    return (rc==JSI_OK?cnt:-1);

bail:
    dbEvalFinalize(&sEval);
    if (didBegin)
        _SQL_LITE_N_(_exec)(jdb->db, "ROLLBACK;", 0, 0
#ifndef USE_SQLITE_V4
        ,0
#endif
        );
    return rc;
}

int
Jsi_DbQuery(Jsi_Db *jdb, Jsi_OptionSpec *specs, void *data, int numData, const char *query, int flags)
{
    if (!specs)
        return jsi_DbExecBinds(jdb, (Jsi_DbMultipleBind*)data, query, flags);
    Jsi_DbMultipleBind binds[2] = {
        {.opts=specs, .data=data, .numData=numData},
        {}
    };
    return jsi_DbExecBinds(jdb, binds, query, flags);
}

void *Jsi_DbHandle(Jsi_Interp *interp, Jsi_Db* jdb)
{
    SQLSIGASSERT(jdb,DB);
    return jdb->db;
}

/* This is the JSI_LITE_ONLY, non-constructor creator of Jsi_Db */
Jsi_Db* Jsi_DbNew(const char *zFile, int inFlags /* JSI_DBI_* flags */)
{
    char *zErrMsg;
#ifdef JSI_LITE_ONLY
    if (0) { /* Get rid of compiler warnings. */
        const char **ee = execFmtStrs;
        ee = mtxStrs;
        ee = trcModeStrs;
        ee = ee;
    }
#endif
    int flags = _SQLITEN_(OPEN_READWRITE) | _SQLITEN_(OPEN_CREATE);
#ifndef USE_SQLITE_V4
#ifdef SQLITE_JSI_DEFAULT_FULLMUTEX
    flags |= _SQLITEN_(OPEN_FULLMUTEX);
#else
    flags |= _SQLITEN_(OPEN_NOMUTEX);
#endif
#endif

    if (!zFile)
        zFile = ":memory:";
    zErrMsg = 0;
    Jsi_Db *db = (Jsi_Db*)Jsi_Calloc(1, sizeof(*db) );
    if( db==0 ) {
        fprintf(stderr, "malloc failed\n");
        return NULL;
    }
    db->sig = SQLITE_SIG_DB;
    db->maxStmts = NUM_PREPARED_STMTS;

    if (db->maxStmts<0 || db->maxStmts>MAX_PREPARED_STMTS) {
        fprintf(stderr, "option maxStmts value %d is not in range 0..%d", db->maxStmts, MAX_PREPARED_STMTS);
        goto bail;
    }
    if (inFlags&JSI_DBI_READONLY) {
        flags &= ~(_SQLITEN_(OPEN_READWRITE)|_SQLITEN_(OPEN_CREATE));
        flags |= _SQLITEN_(OPEN_READONLY);
    } else {
        flags &= ~_SQLITEN_(OPEN_READONLY);
        flags |= _SQLITEN_(OPEN_READWRITE);
        if (inFlags&JSI_DBI_NOCREATE) {
            flags &= ~_SQLITEN_(OPEN_CREATE);
        }
    }
#ifndef USE_SQLITE_V4
    if(inFlags&JSI_DBI_NO_MUTEX) {
        flags |= _SQLITEN_(OPEN_NOMUTEX);
        flags &= ~_SQLITEN_(OPEN_FULLMUTEX);
    } else {
        flags &= ~_SQLITEN_(OPEN_NOMUTEX);
    }
    if(inFlags&JSI_DBI_FULL_MUTEX) {
        flags |= _SQLITEN_(OPEN_FULLMUTEX);
        flags &= ~_SQLITEN_(OPEN_NOMUTEX);
    } else {
        flags &= ~_SQLITEN_(OPEN_FULLMUTEX);
    }
#endif
    char cpath[PATH_MAX];
    char *npath = Jsi_FileRealpathStr(NULL, zFile, cpath);
    
    if (_SQLITEN_(OK) != _SQL_LITE_N_(_open_v2)(npath, &db->db, flags, NULL)) {
        fprintf(stderr, "db open failed\n");
        goto bail;
    }
    //Jsi_DSFree(&translatedFilename);

    if( _SQLITEN_(OK)!=_SQL_LITE_N_(_errcode)(db->db) ) {
        zErrMsg = _SQL_LITE_N_(_mprintf)(_SQLITE_PENV_(db) "%s", _SQL_LITE_N_(_errmsg)(db->db));
        DbClose(db->db);
        db->db = 0;
    }
    if( db->db==0 ) {
        fprintf(stderr, "Db open failed %s\n", zErrMsg);
#ifdef USE_SQLITE_V4
        _SQL_LITE_N_(_free)(db->pEnv, zErrMsg);
#else
        _SQL_LITE_N_(_free)(zErrMsg);
#endif
        goto bail;
    }
    db->stmtHash = Jsi_HashNew(NULL, JSI_KEYS_STRING, NULL);
    db->strKeyTbl = Jsi_HashNew(NULL, JSI_KEYS_STRING, NULL);
    return db;
    
bail:
    return NULL;
}


#ifndef JSI_LITE_ONLY

int Jsi_DoneSqlite(Jsi_Interp *interp)
{
    Jsi_UserObjUnregister(interp, &sqliteobject);
    return JSI_OK;
}
#ifdef JSI_DB_TEST
#include "c-demos/dbdemo.c"
#endif

int Jsi_InitSqlite(Jsi_Interp *interp)
{
    Jsi_Hash* dbSys;
#ifdef JSI_USE_STUBS
  if (Jsi_StubsInit(interp, 0) != JSI_OK)
    return JSI_ERROR;
#endif
    if (!(dbSys = Jsi_UserObjRegister(interp, &sqliteobject))) {
        Jsi_LogError("Failed to init sqlite extension");
        return JSI_ERROR;
    }
    if (!Jsi_CommandCreateSpecs(interp, sqliteobject.name, sqliteCmds, dbSys, 0))
        return JSI_ERROR;
#ifdef JSI_DB_TEST
    if (getenv("RUN_DB_TEST"))
        TestSqlite(interp);
#endif
    return JSI_OK;
}
#endif

#else
/* Linking placeholders for when Sqlite is not compiled-in. */
int
Jsi_DbQuery(Jsi_Db *jdb, Jsi_OptionSpec *specs, void *data, int numData, const char *cmd, int flags)
{
    fprintf(stderr, "Jsi_DbQuery unsupported\n");
    return -1;
}

void *Jsi_DbHandle(Jsi_Interp *interp, Jsi_Db* jdb)
{
    fprintf(stderr, "Jsi_DbHandle unsupported\n");
    return NULL;
}
#endif
