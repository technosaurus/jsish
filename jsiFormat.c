#ifndef JSI_LITE_ONLY
/*
 * Implements the internals of the format command for jsi
 *
 * The FreeBSD license
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE JSI PROJECT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * JSI OR TCL PROJECTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the Jsi Tcl Project.
 *
 * Based on code originally from Tcl 8.5:
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 *
 * See the file "tcl.license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include <ctype.h>
#include <string.h>
#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#include "jsiUtf8.h"
#endif

//#define JSI_UTF_MAX 3
#define JSI_INTEGER_SPACE 24
#define MAX_FLOAT_WIDTH 320

/**
 * Apply the printf-like format in fmtObjPtr with the given arguments.
 *
 * Returns a new object with zero reference count if OK, or NULL on error.
 */
int Jsi_FormatString(Jsi_Interp *interp, Jsi_Value *args, Jsi_DString *dStr)
{
    const char *span, *format, *formatEnd, *msg;
    int numBytes = 0, argIndex = 1, gotXpg = 0, gotSequential = 0, argCnt;
    static const char * const mixedXPG =
        "cannot mix \"%\" and \"%n$\" conversion specifiers";
    static const char * const badIndex[2] = {
        "not enough arguments for all format specifiers",
        "\"%n$\" argument index out of range"
    };
    int formatLen;
    Jsi_Value *v;
    
    /* A single buffer is used to store numeric fields (with sprintf())
     * This buffer is allocated/reallocated as necessary
     */
    char stat_buf[100], *num_buffer = stat_buf;
    int num_buffer_size = sizeof(stat_buf);
    argCnt = Jsi_ValueGetLength(interp, args);

    if (argCnt<1) {
        msg = "missing format";
        goto errorMsg;
    }
    format = Jsi_ValueArrayIndexToStr(interp, args,0, &formatLen);
    
    span = format;
    formatEnd = format + formatLen;
    Jsi_DSInit(dStr);

    while (format != formatEnd) {
        char *end;
        int gotMinus, sawFlag;
        int gotPrecision, useShort, useLong;
        long width, precision;
        int newXpg;
        int ch;
        int step;
        int doubleType;
        char pad = ' ';
        char spec[2*JSI_INTEGER_SPACE + 12];
        char *p;

        int formatted_chars;
        int formatted_bytes;
        const char *formatted_buf = NULL;

        step = jsi_utf8_tounicode(format, &ch);
        format += step;
        if (ch != '%') {
            numBytes += step;
            continue;
        }
        if (numBytes) {
            Jsi_DSAppendLen(dStr, span, numBytes);
            numBytes = 0;
        }

        /*
         * Saw a % : process the format specifier.
         *
         * Step 0. Handle special case of escaped format marker (i.e., %%).
         */

        step = jsi_utf8_tounicode(format, &ch);
        if (ch == '%') {
            span = format;
            numBytes = step;
            format += step;
            continue;
        }

        /*
         * Step 1. XPG3 position specifier
         */

        newXpg = 0;
        if (isdigit(ch)) {
            int position = strtoul(format, &end, 10);
            if (*end == '$') {
                newXpg = 1;
                argIndex = position - 1;
                format = end + 1;
                step = jsi_utf8_tounicode(format, &ch);
            }
        }
        if (newXpg) {
            if (gotSequential) {
                msg = mixedXPG;
                goto errorMsg;
            }
            gotXpg = 1;
        } else {
            if (gotXpg) {
                msg = mixedXPG;
                goto errorMsg;
            }
            gotSequential = 1;
        }
        if ((argIndex < 0) || (argIndex >= argCnt)) {
            msg = badIndex[gotXpg];
            goto errorMsg;
        }

        /*
         * Step 2. Set of flags. Also build up the sprintf spec.
         */
        p = spec;
        *p++ = '%';

        gotMinus = 0;
        sawFlag = 1;
        do {
            switch (ch) {
            case '-':
                gotMinus = 1;
                break;
            case '0':
                pad = ch;
                break;
            case ' ':
            case '+':
            case '#':
                break;
            default:
                sawFlag = 0;
                continue;
            }
            *p++ = ch;
            format += step;
            step = jsi_utf8_tounicode(format, &ch);
        } while (sawFlag);

        /*
         * Step 3. Minimum field width.
         */

        width = 0;
        if (isdigit(ch)) {
            width = strtoul(format, &end, 10);
            format = end;
            step = jsi_utf8_tounicode(format, &ch);
        } else if (ch == '*') {
            if (argIndex >= argCnt - 1) {
                msg = badIndex[gotXpg];
                goto errorMsg;
            }
            v = Jsi_ValueArrayIndex(interp, args, argIndex);
            if (Jsi_GetLongFromValue(interp, v, &width) != JSI_OK) {
                goto error;
            }
            if (width < 0) {
                width = -width;
                if (!gotMinus) {
                    *p++ = '-';
                    gotMinus = 1;
                }
            }
            argIndex++;
            format += step;
            step = jsi_utf8_tounicode(format, &ch);
        }

        /*
         * Step 4. Precision.
         */

        gotPrecision = precision = 0;
        if (ch == '.') {
            gotPrecision = 1;
            format += step;
            step = jsi_utf8_tounicode(format, &ch);
        }
        if (isdigit(ch)) {
            precision = strtoul(format, &end, 10);
            format = end;
            step = jsi_utf8_tounicode(format, &ch);
        } else if (ch == '*') {
            if (argIndex >= argCnt - 1) {
                msg = badIndex[gotXpg];
                goto errorMsg;
            }
            v = Jsi_ValueArrayIndex(interp, args, argIndex);
            if (Jsi_GetLongFromValue(interp, v, &precision) != JSI_OK) {
                goto error;
            }

            /*
             * TODO: Check this truncation logic.
             */

            if (precision < 0) {
                precision = 0;
            }
            argIndex++;
            format += step;
            step = jsi_utf8_tounicode(format, &ch);
        }

        /*
         * Step 5. Length modifier.
         */

        useShort = 0;
        useLong = 0;
        if (ch == 'h') {
            useShort = 1;
            format += step;
            step = jsi_utf8_tounicode(format, &ch);
        } else if (ch == 'l') {
            useLong = 1;
            format += step;
            step = jsi_utf8_tounicode(format, &ch);
            if (ch == 'l') {
                format += step;
                step = jsi_utf8_tounicode(format, &ch);
            }
        }

        format += step;
        span = format;

        /*
         * Step 6. The actual conversion character.
         */

        if (ch == 'i') {
            ch = 'd';
        }

        doubleType = 0;

        /* Each valid conversion will set:
         * formatted_buf   - the result to be added
         * formatted_chars - the length of formatted_buf in characters
         * formatted_bytes - the length of formatted_buf in bytes
         */
        switch (ch) {
        case '\0':
            msg = "format string ended in middle of field specifier";
            goto errorMsg;
        case 's': {
            v = Jsi_ValueArrayIndex(interp, args, argIndex);
            if (Jsi_GetStringFromValue(interp, v, &formatted_buf) != JSI_OK)
                goto error;
            formatted_bytes = 
            formatted_chars = Jsi_Strlen(formatted_buf);
            if (gotPrecision && (precision < formatted_chars)) {
                /* Need to build a (null terminated) truncated string */
                formatted_chars = precision;
                formatted_bytes = jsi_utf8_index(formatted_buf, precision);
            }
            break;
        }
        case 'c': {
            Jsi_Wide code;

            v = Jsi_ValueArrayIndex(interp, args, argIndex);
            if (Jsi_GetWideFromValue(interp, v, &code) != JSI_OK) {
                goto error;
            }
            /* Just store the value in the 'spec' buffer */
            formatted_bytes = jsi_utf8_fromunicode(spec, code);
            formatted_buf = spec;
            formatted_chars = 1;
            break;
        }

        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':
            doubleType = 1;
            /* fall through */
        case 'd':
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            Jsi_Wide w;
            Jsi_Number d;
            int length;

            /* Fill in the width and precision */
            if (width) {
                p += sprintf(p, "%ld", width);
            }
            if (gotPrecision) {
                p += sprintf(p, ".%ld", precision);
            }

            /* Now the modifier, and get the actual value here */
            if (doubleType) {
                v = Jsi_ValueArrayIndex(interp, args, argIndex);
                if (Jsi_GetDoubleFromValue(interp, v, &d) != JSI_OK) {
                    goto error;
                }
                length = MAX_FLOAT_WIDTH;
            }
            else {
                v = Jsi_ValueArrayIndex(interp, args, argIndex);
                if (Jsi_GetWideFromValue(interp, v, &w) != JSI_OK) {
                    goto error;
                }
                length = JSI_INTEGER_SPACE;
                if (useShort) {
                    *p++ = 'h';
                    if (ch == 'd') {
                        w = (short)w;
                    }
                    else {
                        w = (unsigned short)w;
                    }
                }
                else {
                    *p++ = 'l';
#ifdef HAVE_LONG_LONG
                    if (useLong && sizeof(long long) == sizeof(Jsi_Wide)) {
                        *p++ = 'l';
                    }
#endif
                }
            }

            *p++ = (char) ch;
            *p = '\0';

            /* Adjust length for width and precision */
            if (width > length) {
                length = width;
            }
            if (gotPrecision) {
                length += precision;
            }

            /* Increase the size of the buffer if needed */
            if (num_buffer_size < length + 1) {
                num_buffer_size = length + 1;
                num_buffer = Jsi_Realloc((num_buffer==stat_buf?NULL:num_buffer), num_buffer_size);
            }

            if (doubleType) {
                snprintf(num_buffer, length + 1, spec, d);
            }
            else {
                formatted_bytes = snprintf(num_buffer, length + 1, spec, w);
            }
            formatted_chars = formatted_bytes = strlen(num_buffer);
            formatted_buf = num_buffer;
            break;
        }

        default: {
            /* Just reuse the 'spec' buffer */
            spec[0] = ch;
            spec[1] = '\0';
            Jsi_LogError("bad field specifier \"%s\"", spec);
            goto error;
        }
        }

        if (!gotMinus) {
            while (formatted_chars < width) {
                Jsi_DSAppendLen(dStr, &pad, 1);
                formatted_chars++;
            }
        }

        Jsi_DSAppendLen(dStr, formatted_buf, formatted_bytes);

        while (formatted_chars < width) {
            Jsi_DSAppendLen(dStr, &pad, 1);
            formatted_chars++;
        }

        argIndex += gotSequential;
    }
    if (numBytes) {
        Jsi_DSAppendLen(dStr, span, numBytes);
    }

    if (num_buffer!=stat_buf)
        Jsi_Free(num_buffer);
    return JSI_OK;

errorMsg:
    Jsi_LogError("%s", msg);
error:
    Jsi_DSFree(dStr);
    if (num_buffer!=stat_buf)
        Jsi_Free(num_buffer);
    return JSI_ERROR;
}
#endif
