#ifndef JSI_LITE_ONLY
#ifndef JSI_OMIT_STUBS
#include "jsiInt.h"
#include "jsiStubs.h"
  
static int Jsi_Stubs__initialize(Jsi_Interp *interp, double version, const char* name, int flags, 
    const char *md5, int bldFlags, int stubSize, void **ptr);
    
Jsi_Stubs jsiStubsTbl = { __JSI_STUBS_INIT__ };
Jsi_Stubs *jsiStubsTblPtr = &jsiStubsTbl;

int Jsi_Stubs__initialize(Jsi_Interp *interp, double version, const char* name, int flags, 
    const char *md5, int bldFlags, int stubSize, void **ptr)
{
    if (strcmp(name,"jsi")) { /* Sub-stub support */
        int rc = Jsi_StubLookup(interp, name, ptr);
        if (rc != JSI_OK)
            return JSI_ERROR;
        Jsi_Stubs *sp = *ptr;
        if (sp->_Jsi_Stubs__initialize && sp->sig == JSI_STUBS_SIG &&
            sp->_Jsi_Stubs__initialize != jsiStubsTbl._Jsi_Stubs__initialize)
            return (*sp->_Jsi_Stubs__initialize)(interp, version, name, flags, md5, bldFlags, stubSize, ptr);
        Jsi_LogError("failed to find stub for %s", name);
        return JSI_ERROR;
    }
    int strict = (flags & JSI_STUBS_STRICT);
    int sizediff = (sizeof(Jsi_Stubs) - stubSize);
    assert(jsiStubsTbl.sig == JSI_STUBS_SIG);
    if (sizediff<0 || (strict && (strcmp(md5, JSI_STUBS_MD5) || sizediff))) {
        fprintf(stderr, "%s: extension from incompatible build: %s\n", name, (sizediff ? "size changed": md5));
        return JSI_ERROR;
    }
    if (bldFlags != jsiStubsTbl.bldFlags) {
        fprintf(stderr, "%s: extension build flags mismatch (different libc?)\n", name);
        if (strict)
            return JSI_ERROR;
    }
    if (version > JSI_VERSION) {
        fprintf(stderr, "%s: extension version newer than jsish (%g > %g)\n", name, version, JSI_VERSION);
        if (strict)
            return JSI_ERROR;
    }
    return JSI_OK;
}
#endif
#endif
