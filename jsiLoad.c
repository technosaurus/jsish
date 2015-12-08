#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif
#ifndef JSI_OMIT_LOAD

#ifdef __WIN32
#define dlsym(l,s) GetProcAddress(l,s)
#define dlclose(l) FreeLibrary(l)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifndef RTLD_NOW
    #define RTLD_NOW 0
#endif
#ifndef RTLD_LOCAL
    #define RTLD_LOCAL 0
#endif

#endif

typedef struct LoadData {
    const char *name;
    Jsi_Value *fname;
#ifdef __WIN32
    HMODULE handle;
#else
    void *handle;
#endif
} LoadData;

#ifndef JSI_OMIT_LOAD

/**
 * Note that jsi_LoadLibrary() requires a path to an existing file.
 *
 * If it is necessary to search JSI_LIBPATH, use Jsi_PkgRequire() instead.
 */
int jsi_LoadLibrary(Jsi_Interp *interp, const char *pathName, Jsi_Value *fval)
{
#ifdef __WIN32
    HMODULE handle = LoadLibrary(pathName);
#else
    void *handle = dlopen(pathName, RTLD_NOW | RTLD_LOCAL);
#endif
    if (handle == NULL) {
        Jsi_LogError("loading extension \"%s\": %s", pathName, dlerror());
        return JSI_ERROR;
    }
    /* We use a unique init symbol depending on the extension name.
     * This is done for compatibility between static and dynamic extensions.
     * For extension readline.so, the init symbol is "Jsi_readlineInit"
     */
    const char *pt;
    const char *pkgname;
    int pkgnamelen, n;
    char initsym[100], *cp = "";
    Jsi_InitProc *onload = NULL;
    struct Jsi_Stubs **vp = NULL;
    Jsi_HashEntry *hPtr;
    LoadData *d;

    pt = strrchr(pathName, '/');
    if (pt) {
        pkgname = pt + 1;
    }
    else {
        pkgname = pathName;
    }
    pt = strchr(pkgname, '.');
    if (pt) {
        pkgnamelen = pt - pkgname;
    }
    else {
        pkgnamelen = strlen(pkgname);
    }
    for (n = 0; n<4 && onload==NULL; n++)
    {
        char ch;
        if (n==2) { /* Skip lib prefix. */
            if (pkgnamelen<4 || strncmp(pkgname,"lib",3))
                break;
            pkgname += 3;
            pkgnamelen -= 3;
        }
        ch = ((n%2) ? pkgname[0] : toupper(pkgname[0]));
        snprintf(initsym, sizeof(initsym), "Jsi_Init%c%.*s", ch, pkgnamelen-1, pkgname+1);
        cp = initsym+8;
        hPtr = Jsi_HashEntryFind(interp->loadTbl, cp);
        if (hPtr) { /* Already loaded? */
            d = Jsi_HashValueGet(hPtr);
            if (d && d->handle)
                return JSI_OK;
        }
        onload = (Jsi_InitProc *)dlsym(handle, initsym);
#ifndef JSI_OMIT_STUBS
        if (onload) {
            /* Handle stubs. TODO: handle other libs like sqliteStubsPtr, websocketStubsPtr, etc... */
            vp = (struct Jsi_Stubs**)dlsym(handle, "jsiStubsPtr");
            if (vp) {
                *vp = jsiStubsTblPtr;
            }
        }
#endif
    }
    
    if (onload == NULL) {
        Jsi_LogError("No %s symbol found in extension %s", initsym, pathName);
    }
    else if (onload(interp) != JSI_ERROR) {
        int isNew;
        hPtr = Jsi_HashEntryCreate(interp->loadTbl, cp, &isNew);
        if (hPtr && isNew) {
            d = Jsi_Calloc(1, sizeof(*d));
            d->handle = handle;
            d->name = Jsi_HashKeyGet(hPtr);
            d->fname = fval;
            Jsi_IncrRefCount(interp, fval);
            Jsi_HashValueSet(hPtr, d);
            return JSI_OK;
        }
        Jsi_LogError("duplicate load");
    }

    if (handle) {
        dlclose(handle);
    }
    return JSI_ERROR;
}
#else
int jsi_LoadLibrary(Jsi_Interp *interp, const char *pathName, Jsi_Value *fval)
{
    Jsi_LogError("load not supported");
    return JSI_ERROR;
}
#endif

int jsi_FreeOneLoadHandle(Jsi_Interp *interp, void *ptr) {
    LoadData *d = ptr;
#ifndef JSI_OMIT_LOAD
    dlclose(d->handle);
#endif
    Jsi_IncrRefCount(interp, d->fname);
    Jsi_Free(d);
    return JSI_OK;
}

int Jsi_StubLookup(Jsi_Interp *interp, const char *name, void **ptr)
{
    Jsi_HashEntry *hPtr = Jsi_HashEntryFind(interp->loadTbl, name);
    if (!hPtr)
        return JSI_ERROR;
    LoadData *d = Jsi_HashValueGet(hPtr);
    if (d==NULL || !d->handle)
        return JSI_ERROR;
    char initsym[100];
    snprintf(initsym, sizeof(initsym), "%sStubsTblPtr", name);
    void **vp = NULL;
#ifndef JSI_OMIT_LOAD
    vp = (void**)dlsym(d->handle, initsym);
#endif
    if (vp == NULL || !*vp)
        return JSI_ERROR;
    *ptr = *vp;
    return JSI_OK;
}

#define FN_load JSI_INFO("\
Load a shared libary and call the _Init function.\
")
static int LoadLoadCmd(Jsi_Interp *interp, Jsi_Value *args, 
    Jsi_Value *_this, Jsi_Value **ret, Jsi_Func *funcPtr)
{
    Jsi_Value *fval = Jsi_ValueArrayIndex(interp, args, 0);
    char *v = Jsi_Realpath(interp, fval, NULL);
    if (v == NULL)
        return JSI_ERROR;
    int rc = jsi_LoadLibrary(interp, v, fval);
    Jsi_Free(v);
    return rc;
}

static Jsi_CmdSpec loadCmds[] = {
    { "load",   LoadLoadCmd, 1, 1, "shlib.so", .help="Load a shared executable and invoke its _Init call", .info=FN_load },
//TODO: { "unload", LoadUnloadCmd,  NULL,   1, 1, "shlib.so", .help="Unload a shared executable and invoke its _Done call", .info=FN_load },
    { NULL, .help="Commands for managing shared libraries" }
};

int jsi_LoadInit(Jsi_Interp *interp)
{
    Jsi_CommandCreateSpecs(interp, "", loadCmds, NULL, JSI_CMDSPEC_NOTOBJ);
    return JSI_OK;
}


#ifdef __WIN32
#undef dlsym
#undef dlclose
#endif
#endif
