#include <stdio.h>
#include "number.h"

#ifndef _NO_LL_SUPPORT
static const long long _numnan = 0x7ff8000000000000LL;
static const long long *NAN = &_numnan;
static const long long _numinf = 0x7ff0000000000000LL;
static const long long *INF = &_numinf;

#else
	#ifndef _BIGENDIAN
	static const unsigned char _numnan[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xF8, 0x7F };
	static const unsigned char _numinf[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xF0, 0x7F };
	#else
	static const unsigned char _numnan[] = { 0x7F, 0xF8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	static const unsigned char _numinf[] = { 0x7F, 0xF0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	#endif
	static const char *NAN = _numnan;
	static const char *INF = _numinf;
#endif

double ieee_makeinf(int i)
{
	double r = *(double *)INF;
	if (i < 0) r = -r;
	return r;
}

double ieee_makenan()
{
	return *(double *)NAN;
}

/*
 * below numtoa function modify from :
 * http://code.google.com/p/stringencoders/
 * Copyright &copy; 2007, Nick Galbreath -- nickg [at] modp [dot] com
 * All rights reserved.
 * Released under the MIT license.
 */

static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000,
							   10000000, 100000000, 1000000000};

static void strreverse(unichar* begin, unichar* end)
{
	unichar aux;
	while (end > begin)
		aux = *end, *end-- = *begin, *begin++ = aux;
}

void num_itoa10(int value, unichar* str)
{
	unichar* wstr = str;
	/* Take care of sign */
	unsigned int uvalue = (value < 0) ? -value : value;
	/* Conversion. Number is reversed. */
	do *wstr++ = (unichar)(48 + (uvalue % 10)); while(uvalue /= 10);
	if (value < 0) *wstr++ = '-';

	unistrlen(str) = wstr - str;
	/* Reverse string */
	strreverse(str,wstr-1);
}

void num_uitoa10(unsigned int value, unichar* str)
{
	unichar* wstr=str;
	/* Conversion. Number is reversed. */
	do *wstr++ = (unichar)(48 + (value % 10)); while (value /= 10);
	
	unistrlen(str) = wstr - str;
	/* Reverse string */
	strreverse(str, wstr-1);
}

/* This is near identical to modp_dtoa above */
/*   The differnce is noted below */
void num_dtoa2(double value, unichar* str, int prec)
{
	/* Hacky test for NaN
	 * under -fast-math this won't work, but then you also won't
	 * have correct nan values anyways.  The alternative is
	 * to link with libmath (bad) or hack IEEE double bits (bad)
	 */
	if (! (value == value)) {
		str[0] = 'N'; str[1] = 'a'; str[2] = 'N';
		unistrlen(str) = 3;
		return;
	}

	/* if input is larger than thres_max, revert to exponential */
	const double thres_max = (double)(0x7FFFFFFF);

	int count;
	double diff = 0.0;
	unichar* wstr = str;

	if (prec < 0) {
		prec = 0;
	} else if (prec > 9) {
		/* precision of >= 10 can lead to overflow errors */
		prec = 9;
	}

	/* we'll work in positive values and deal with the
	   negative sign issue later */
	int neg = 0;
	if (value < 0) {
		neg = 1;
		value = -value;
	}

	int whole = (int) value;
	double tmp = (value - whole) * pow10[prec];
	unsigned int frac = (unsigned int)(tmp);
	diff = tmp - frac;

	if (diff > 0.5) {
		++frac;
		/* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
		if (frac >= pow10[prec]) {
			frac = 0;
			++whole;
		}
	} else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
		/* if halfway, round up if odd, OR
		   if last digit is 0.  That last part is strange */
		++frac;
	}

	/* for very large numbers switch back to native sprintf for exponentials.
	   anyone want to write code to replace this? */
	/*
	  normal printf behavior is to print EVERY whole number digit
	  which can be 100s of characters overflowing your buffers == bad
	*/
	if (value > thres_max) {
		char bstr[64];
		int i;
		sprintf(bstr, "%e", neg ? -value : value);
		for (i = 0; bstr[i]; ++i)
			str[i] = bstr[i];

		unistrlen(str) = i;
		return;
	}

	if (prec == 0) {
		diff = value - whole;
		if (diff > 0.5) {
			/* greater than 0.5, round up, e.g. 1.6 -> 2 */
			++whole;
		} else if (diff == 0.5 && (whole & 1)) {
			/* exactly 0.5 and ODD, then round up */
			/* 1.5 -> 2, but 2.5 -> 2 */
			++whole;
		}

		/*vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2 */
	} else if (frac) {
		count = prec;
		/* now do fractional part, as an unsigned number */
		/* we know it is not 0 but we can have leading zeros, these */
		/* should be removed */
		while (!(frac % 10)) {
			--count;
			frac /= 10;
		}
		/*^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2 */

		/* now do fractional part, as an unsigned number */
		do {
			--count;
			*wstr++ = (unichar)(48 + (frac % 10));
		} while (frac /= 10);
		/* add extra 0s */
		while (count-- > 0) *wstr++ = '0';
		/* add decimal */
		*wstr++ = '.';
	}

	/* do whole part */
	/* Take care of sign */
	/* Conversion. Number is reversed. */
	do *wstr++ = (unichar)(48 + (whole % 10)); while (whole /= 10);
	if (neg) {
		*wstr++ = '-';
	}
	unistrlen(str) = wstr - str;
	strreverse(str, wstr-1);
}

