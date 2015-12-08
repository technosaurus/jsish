#ifndef JSI_LITE_ONLY
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif
#include <math.h>

#define MFUNC1(fname, func)  \
static int fname (Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,\
    Jsi_Value **ret, Jsi_Func *funcPtr)\
{\
    Jsi_Value *val = Jsi_ValueArrayIndex(interp, args, 0);\
    Jsi_Number num;\
    if (Jsi_GetNumberFromValue(interp, val, &num) != JSI_OK)\
        return JSI_ERROR;\
    Jsi_ValueMakeNumber(interp, *ret, func (num));\
    return JSI_OK;\
}

#define MFUNC2(fname, func)  \
static int fname (Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,\
    Jsi_Value **ret, Jsi_Func *funcPtr)\
{\
    Jsi_Value *val1 = Jsi_ValueArrayIndex(interp, args, 0);\
    Jsi_Number num1;\
    if (Jsi_GetNumberFromValue(interp, val1, &num1) != JSI_OK)\
        return JSI_ERROR;\
    Jsi_Value *val2 = Jsi_ValueArrayIndex(interp, args, 1);\
    Jsi_Number num2;\
    if (Jsi_GetNumberFromValue(interp, val2, &num2) != JSI_OK)\
        return JSI_ERROR;\
    Jsi_ValueMakeNumber(interp, *ret, func (num1,num2));\
    return JSI_OK;\
}

static int MathMinCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int i, argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *val;
    Jsi_Number n, num;
    if (argc<=0)
        return JSI_OK;
    for (i=0; i<argc; i++) {
        val = Jsi_ValueArrayIndex(interp, args, i);
        if (Jsi_GetNumberFromValue(interp, val, &num) != JSI_OK)
            return JSI_ERROR;
        if (i==0)
            n = num;
        else
            n =  (num>n ? n : num);
    }
    Jsi_ValueMakeNumber(interp, *ret, n);
    return JSI_OK;
}


static int MathMaxCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    int i, argc = Jsi_ValueGetLength(interp, args);
    Jsi_Value *val;
    Jsi_Number n, num;
    if (argc<=0)
        return JSI_OK;
    for (i=0; i<argc; i++) {
        val = Jsi_ValueArrayIndex(interp, args, i);
        if (Jsi_GetNumberFromValue(interp, val, &num) != JSI_OK)
            return JSI_ERROR;
        if (i==0)
            n = num;
        else
            n =  (num<n ? n : num);
    }
    Jsi_ValueMakeNumber(interp, *ret, n);
    return JSI_OK;
}

static int MathRandomCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    static int init = 0;
#ifndef __WIN32
    if (!init) {
        init = 1;
        srand48((long)time(NULL));
    }
    Jsi_ValueMakeNumber(interp, *ret, drand48());
#else
    Jsi_ValueMakeNumber(interp, *ret, (Jsi_Number)rand());
#endif
    return JSI_OK;
}

MFUNC1(MathAbsCmd,  abs)
MFUNC1(MathAcosCmd, acos)
MFUNC1(MathAsinCmd, asin)
MFUNC1(MathAtanCmd, atan)
MFUNC1(MathCeilCmd, ceil)
MFUNC1(MathCosCmd,  cos)
MFUNC1(MathExpCmd,  exp)
MFUNC1(MathFloorCmd,floor)
MFUNC1(MathLogCmd,  log)
MFUNC1(MathRoundCmd,round)
MFUNC1(MathSinCmd,  sin)
MFUNC1(MathSqrtCmd, sqrt)
MFUNC1(MathTanCmd,  tan)
MFUNC2(MathAtan2Cmd,atan2)
MFUNC2(MathPowCmd,  pow)

static Jsi_CmdSpec mathCmds[] = {
    { "abs",    MathAbsCmd,     1, 1, "num", .help="Returns the absolute value of x" },
    { "acos",   MathAcosCmd,    1, 1, "num", .help="Returns the arccosine of x, in radians" },
    { "asin",   MathAsinCmd,    1, 1, "num", .help="Returns the arcsine of x, in radians" },
    { "atan",   MathAtanCmd,    1, 1, "num", .help="Returns the arctangent of x as a numeric value between -PI/2 and PI/2 radians" },
    { "atan2",  MathAtan2Cmd,   2, 2, "x,y", .help="Returns the arctangent of the quotient of its arguments" },
    { "ceil",   MathCeilCmd,    1, 1, "num", .help="Returns x, rounded upwards to the nearest integer" },
    { "cos",    MathCosCmd,     1, 1, "num", .help="Returns the cosine of x (x is in radians)" },
    { "exp",    MathExpCmd,     1, 1, "num", .help="Returns the value of Ex" },
    { "floor",  MathFloorCmd,   1, 1, "num", .help="Returns x, rounded downwards to the nearest integer" },
    { "log",    MathLogCmd,     1, 1, "num", .help="Returns the natural logarithm (base E) of x" },
    { "max",    MathMaxCmd,     1,-1, "x,y,z,...,n", .help="Returns the number with the highest value" },
    { "min",    MathMinCmd,     1,-1, "x,y,z,...,n", .help="Returns the number with the lowest value" },
    { "pow",    MathPowCmd,     2, 2, "x,y", .help="Returns the value of x to the power of y" },
    { "random", MathRandomCmd,  0, 0, "", .help="Returns a random number between 0 and 1" },
    { "round",  MathRoundCmd,   1, 1, "num", .help="Rounds x to the nearest integer" },
    { "sin",    MathSinCmd,     1, 1, "num", .help="Returns the sine of x (x is in radians)" },
    { "sqrt",   MathSqrtCmd,    1, 1, "num", .help="Returns the square root of x" },
    { "tan",    MathTanCmd,     1, 1, "num", .help="Returns the tangent of an angle" },
    { NULL, .help="Commands performing math operations on numbers" }
};

    
int jsi_MathInit(Jsi_Interp *interp)
{
    Jsi_Value *val = Jsi_CommandCreateSpecs(interp, "Math",    mathCmds,    NULL, JSI_CMDSPEC_NOTOBJ);
#define MCONST(name,v) Jsi_ValueInsert(interp, val, name, Jsi_ValueNewNumber(interp, v), JSI_OM_READONLY)
    MCONST("PI", M_PI);
    MCONST("LN2", M_LN2);
    MCONST("LN10", M_LN10);
    MCONST("LOG2E", M_LOG2E);
    MCONST("LOG10E", M_LOG10E);
    MCONST("SQRT2", M_SQRT2);
    MCONST("SQRT1_2", M_SQRT1_2);
    MCONST("E", M_E);
    return JSI_OK;
}

#endif
