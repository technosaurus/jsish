#ifndef __SCOPE_H__
#define __SCOPE_H__

#include "unichar.h"

typedef struct strs {
	unichar **strings;
	int count;
	int _size;
} strs;

strs *strs_new();
void strs_push(strs *ss, const unichar *string);
void strs_free(strs *ss);
const unichar *strs_get(strs *ss, int i);

/* what lexical scope means:
 * --------------
 * var a;						// this is first level of scope
 * function foo() {				// parsing function make lexical scope to push
 *     var b;					// this is the second level
 *     var c = function() {		// push again
 *         var d;				// third level
 *     }						// end of an function, pop scope
 * }							// return to first scope
 * --------------
 */
void scope_push();
void scope_pop();
void scope_add_var(const unichar *str);
strs *scope_get_varlist();

#endif
