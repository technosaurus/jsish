#ifndef __NUMBER_H__
#define __NUMBER_H__

#include "unichar.h"

/*
 * since nan/infinity math function are not supported in var c compiler,
 * we make our own here, in well supported compiler, you may replace those
 * mass to build-in function, such as isnan, isfinite.
 * 
 * IEEE/ISO C double type(64 bits):
 * bit		Function
 * 63		Sign
 * 52 - 62	Exponent
 * 0 - 51	Fraction
 * 
 * Normal number 	(0 <= e < 2047)
 * Infinity 		e = 2047(max); f = 0 (all zeroes) 
 * NaN				e = 2047, but not Infinity
 * 
 * in little endian maching, a double should like this
 * ffffffff*6 EEEEffff SEEEEEEE
*/

/* cpu calc faster 64bit long long type, comment this */
#define _NO_LL_SUPPORT


#define MAXEXP	2047

#ifndef _NO_LL_SUPPORT
#define EXP(a) ((0x7ff0000000000000LL & (*(long long*)&a)) >> 52)
#define FRAZERO(a) ((0xfffffffffffffLL & (*(long long*)&a)) == 0LL)
#else
	#ifndef _BIGENDIAN
	/* little endian (intel) */
	typedef unsigned short u16t;
	typedef unsigned int u32t;
	
	#define EXP(a) ((((u16t *)(&a))[3] & 0x7ff0) >> 4)
	#define FRAZERO(a) (((u32t *)(&a))[0] == 0 &&			\
						(((u32t *)(&a))[1] & 0x0fffff) == 0)
	#else
	/* big endian */
	/* not support yet */
	#endif
#endif

#define ieee_isnormal(a) 	(EXP(a) != MAXEXP)
#define ieee_isnan(a)		(EXP(a) == MAXEXP && !FRAZERO(a))
#define ieee_infinity(a)	((EXP(a) == MAXEXP && FRAZERO(a)) ? (a > 0 ? 1 : -1) : 0)

double ieee_makeinf(int i);
double ieee_makenan();

void num_itoa10(int value, unichar* str);
void num_uitoa10(unsigned int value, unichar* str);
void num_dtoa2(double value, unichar* str, int prec);

#endif

