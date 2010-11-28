#ifndef __REGEXP_H__
#define __REGEXP_H__

#include <regex.h>
#include "unichar.h"

regex_t *regex_new(const char *str, int compflag);

#endif

