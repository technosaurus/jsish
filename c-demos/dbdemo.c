#define HAVE_SQLITE 1
#include <sqlite3.h>
//#define JSI_LITE_ONLY
#include "../jsi.c"

typedef enum { MARK_NONE, MARK_A, MARK_B, MARK_C, MARK_D, MARK_F } MarkType;
typedef struct {
    char name[16];
    Jsi_DString desc;
    Jsi_Number max;
    int id;
    Jsi_Wide myTime;
    Jsi_Wide rowid;
    char isdirty;
    MarkType mark;
    int markSet;
} MyData;

static const char *markStrs[] = {"","A","B","C","D","F", NULL};

static Jsi_OptionSpec MyOptions[] = {
    JSI_OPT(STRBUF,     MyData, name,   .help="Fixed size char buf" ),
    JSI_OPT(DSTRING,    MyData, desc,   .help="Description field of arbitrary length"),
    JSI_OPT(INT,        MyData, id,     .help="Int id"),
    JSI_OPT(DOUBLE,     MyData, max,    .help="Max value"),
    JSI_OPT(DATETIME,   MyData, myTime, .help="A unix/javascript time field in milliseconds (64 bits)" ),
    JSI_OPT(WIDE,       MyData, rowid,  .help="DB rowid for update/insert; not stored in db"),
    JSI_OPT(BOOL,       MyData, isdirty,  .help="Dirty bit flag: not stored in db", .flags=JSI_OPT_DB_DIRTY),
    JSI_OPT(CUSTOM,     MyData, mark,   .help="Marks", .custom=Jsi_Opt_SwitchEnum,   .data=markStrs ),
    JSI_OPT(CUSTOM,     MyData, markSet,.help="A set", .custom=Jsi_Opt_SwitchBitset, .data=markStrs ),
    JSI_OPT_END(        MyData)
};
/*
for (var i in lst) {
    var l = lst[i],
        typ='';
    if (l.name == 'rowid') continue;
    if (l.name == 'dirty') continue;
    switch (l.type) {
        case 'dstring': case 'string': case 'strkey': case 'strbuf':
            typ='text'; break;
        case 'datetime': case 'wide': case 'bool': case 'boolbit': case 'int':
            typ='int'; break;
        case 'double': typ='float'; break;
        case 'custom':
        break;
        default: ;
    }
}
 */
 
static Jsi_Db *jdbPtr;

static int ExecMyData(MyData *data, int num, const char *query, int flags) {
    return Jsi_DbQuery(jdbPtr, MyOptions, data, num, query, flags);
}

static int ExecMySemi(MyData **data, int num, const char *query, int flags) {
    return Jsi_DbQuery(jdbPtr, MyOptions, data, 0, query, JSI_DB_PTRS|flags);
}

static int ExecMyDyn(MyData ***data, int num, const char *query, int flags) {
    return Jsi_DbQuery(jdbPtr, MyOptions, data, 0, query, JSI_DB_PTR_PTRS|flags);
}

static int TestSqlite(Jsi_Interp *interp, int level)
{
    /* SETUP AND DB CREATION */
    Jsi_Db *jdb;
    if (!interp)
        jdb = Jsi_DbNew("~/mytest.db", 0);
    else {
#ifndef JSI_LITE_ONLY
        if (JSI_OK != Jsi_EvalString(interp, "var mydb = new Sqlite('/tmp/mytest.db');", 0))
            return JSI_ERROR;
        jdb = Jsi_UserObjDataFromVar(interp, "mydb");
        if (!jdb)
            return JSI_ERROR;
#endif
    }
    sqlite3 *db = Jsi_DbHandle(interp, jdb);
    sqlite3_exec(db, "DROP TABLE IF EXISTS mytable;", 0, 0, 0);
    sqlite3_exec(db, "CREATE TABLE mytable (max FLOAT, name, desc, id INT, myTime INT, mark, markSet);", 0, 0, 0);

    /* NOW STAGE DATA TO AND FROM STRUCT. */
    int i, n, res = JSI_OK;
    MyData mydata = {.id=99, .max=100.0, .mark=MARK_A, .markSet=6, .name="maryjane"};
    mydata.myTime = time(NULL)*1000LL; // or use Jsi_DateTime() for milliseconds;
    Jsi_DSSet(&mydata.desc, "Some stuff");

    /* Quickly store 10 rows into the database */
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    for (i=0; i<10; i++) {
        mydata.id++;
        mydata.max--;
        n = Jsi_DbQuery(jdb, MyOptions, &mydata, 1, "INSERT INTO mytable %s", 0); assert(n==1);
    }
    sqlite3_exec(db, "COMMIT", 0, 0, 0);

    /* Read the last inserted row into mydata2. */
    MyData mydata2 = {};
    mydata2.rowid = mydata.rowid;
    n = Jsi_DbQuery(jdb, MyOptions, &mydata2, 1, "SELECT id,name FROM mytable WHERE rowid=:rowid", 0); assert(n==1);
    n = Jsi_DbQuery(jdb, MyOptions, &mydata2, 1, "SELECT %s FROM mytable WHERE rowid=:rowid", 0); assert(n==1);

    /* Modify all fields of last inserted row. */
    mydata.max = -1;
    mydata.myTime = Jsi_DateTime();
    strcpy(mydata.name, "billybob");
    n = Jsi_DbQuery(jdb, MyOptions, &mydata, 1, "UPDATE mytable SET %s WHERE id=:id", 0); assert(n==1);

    /* Modify specific columns for half of inserted rows. */
    mydata.id = 105;
    n = Jsi_DbQuery(jdb, MyOptions, &mydata, 1, "UPDATE mytable SET name=:name, max=:max WHERE id<:id", 0); assert(n==5);

    /* Delete item id 105. */
    n = Jsi_DbQuery(jdb, MyOptions, &mydata, 1, "DELETE FROM mytable WHERE id=:id", 0); assert(n==1);

    /* ARRAY OF STRUCTS. */
    int num = 10, cnt;
    MyData mydatas[10] = {};
    cnt = Jsi_DbQuery(jdb, MyOptions, mydatas, num, "SELECT %s FROM mytable", 0); assert(cnt==9);
    
    for (i=0; i<cnt; i++)
        mydatas[i].id += i;
    n = Jsi_DbQuery(jdb, MyOptions, mydatas, cnt, "UPDATE mytable SET %s WHERE rowid = :rowid", 0); assert(n==9);
    
    /* Update only the dirty rows. */
    for (i=1; i<=3; i++) {
        mydatas[i].isdirty = 1;
        mydatas[i].id += 100*i;
    }
    n = Jsi_DbQuery(jdb, MyOptions, mydatas, cnt, "UPDATE mytable SET %s WHERE rowid = :rowid", JSI_DB_DIRTY_ONLY); assert(n==3);
 
    /* ARRAY OF POINTERS TO STRUCTS. */
    num = 3;
    static MyData *mdPtr[3] = {};  /* Fixed length */
    n = Jsi_DbQuery(jdb, MyOptions, mdPtr, num, "SELECT %s FROM mytable", JSI_DB_PTRS); assert(n==3);
    printf("%f\n", mdPtr[0]->max);

    MyData **dynPtr = NULL;  /* Variable length */
    n = Jsi_DbQuery(jdb, MyOptions, &dynPtr, 0, "SELECT %s FROM mytable WHERE rowid < 5", JSI_DB_PTR_PTRS); assert(n==4);
    n = Jsi_DbQuery(jdb, MyOptions, &dynPtr, n, "SELECT %s FROM mytable LIMIT 1000", JSI_DB_PTR_PTRS);  assert(n==9);
    n = Jsi_DbQuery(jdb, MyOptions, &dynPtr, n, NULL, JSI_DB_PTR_PTRS|JSI_DB_MEMFREE);
    assert(!dynPtr);
    
    /* Multi-bind interface, to bind vars from other struct(s). */
    Jsi_DbMultipleBind binds[] = {
        { ':', MyOptions, mdPtr, num },
        { '$', MyOptions, &mydata },
        {}
    };
    mydata.max = -1;
    n = Jsi_DbQuery(jdb, NULL, binds, 0, "SELECT %s FROM mytable WHERE max=$max", JSI_DB_PTRS); assert(n==3);

    /* Make mdPtr available as "mydata" to Javascript info.cdata() */
#ifndef JSI_LITE_ONLY
    Jsi_CDataRegister(interp, "mydata", MyOptions, mdPtr, num, JSI_DB_PTRS);
#endif
    jdbPtr = jdb;
    if (level==2) {
        ExecMyData(mydatas, n, "SELECT %s FROM mytable;", 0);
        ExecMySemi(mdPtr,   n, "SELECT %s FROM mytable;", 0);
        ExecMyDyn(&dynPtr, n, "SELECT %s FROM mytable;", 0);
    }
    
    /* Load test: insert/select/update 1,000,000 rows. */
    if (level==1) {
        Jsi_Wide stim, etim;
        int bnum = 1000000;
        MyData *big = Jsi_Calloc(bnum, sizeof(MyData)), *b = big;
        //b->max = 2; b->id=99; b->max=100.0; b->mark=MARK_A; b->markSet=6;
        *b = mydata;
        printf("TESTING %d ROWS\n", bnum);

        stim = Jsi_DateTime();
        b[0].id = 0;
        for (i=1; i<bnum; i++) {
            big[i] = *b;
            big[i].id = i;
        }
        etim = Jsi_DateTime();
        printf("INIT C: %8.3f secs\n", ((etim-stim)/1000.0));
        sqlite3_exec(db, "DELETE FROM mytable", 0, 0, 0);

        stim=etim;
        n = ExecMyData(big, bnum, "INSERT INTO mytable %s", 0); assert(n==bnum);
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    INSERT %d ROWS\n", i/1000.0, bnum*1000/i, n);

        stim=etim;
        memset(big, 0, num*sizeof(MyData));
        n = ExecMyData(big, bnum, "SELECT %s FROM mytable", 0); assert(n==bnum);
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    SELECT %d ROWS \n", i/1000.0, bnum*1000/i, bnum);
        for (i=0; i<bnum; i++) {
            if (b[i].id != i)
                printf("FAILED: Data[%d].id: %d\n", i, b[i].id);
            b[i].id = i+1;
        }

        stim=etim;
        n = ExecMyData(big, bnum, "UPDATE mytable SET id=:id where rowid=:rowid", 0); assert(n==bnum);
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    UPDATE %d ROWS, 1 FIELD\n", i/1000.0, bnum*1000/i, n);
        for (i=0; i<bnum; i++) {
            if (b[i].id != (i+1))
                printf("FAILED: Data[%d].id: %d\n", i, b[i].id);
            b[i].id = i+2;
            if ((i%10)==0)
                b[i].isdirty = 1;
        }

        stim=etim;
        n = ExecMyData(big, bnum, "UPDATE mytable SET %s where rowid=:rowid", 0); assert(n==bnum);
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    UPDATE %d ROWS, ALL FIELDS\n", i/1000.0, n*1000/i, n);

        stim=etim;
        n = ExecMyData(big, bnum, "UPDATE mytable SET %s where rowid=:rowid", JSI_DB_DIRTY_ONLY); assert(n==(bnum/10));
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    UPDATE %d DIRTY ROWS\n", i/1000.0, (int)(n*1000.0/i), n);

        stim=etim;
        for (i=0; i<bnum; i++) {
            if ((i%1000)==0)
                b[i].isdirty = 1;
        }
        n = ExecMyData(big, bnum, "UPDATE mytable SET %s where rowid=:rowid", JSI_DB_DIRTY_ONLY); assert(n==(bnum/1000));
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    UPDATE %d DIRTY ROWS\n", i/1000.0, n*1000/i, n);

        stim=etim;
        for (i=0; i<bnum; i++) {
            if ((i%100000)==0)
                b[i].isdirty = 1;
        }
        n = ExecMyData(big, bnum, "UPDATE mytable SET %s where rowid=:rowid", JSI_DB_DIRTY_ONLY); assert(n==(bnum/100000));
        etim = Jsi_DateTime();
        i=(int)(etim-stim);
        printf("%8.3f sec, %8d rows/sec    UPDATE %d DIRTY ROWS\n", i/1000.0, n*1000/i, n);

        Jsi_Free(big);
    }
    // Why we need wrappers: following is an error that the compiler does not detect!
    if (0) Jsi_DbQuery(jdb, MyOptions, "a bad struct", n, "", 0); 

    return res;
}

int main(int argc, char *argv[]) {
#ifdef JSI_LITE_ONLY
    TestSqlite(NULL, argc>1?atoi(argv[1]):0);
#else
    Jsi_Interp *interp = Jsi_InterpCreate(NULL, argc, argv, 0);
    TestSqlite(interp, argc>1?atoi(argv[1]):0);
    Jsi_Interactive(interp, JSI_OUTPUT_QUOTE|JSI_OUTPUT_NEWLINES);
#endif
    return 0;
}
