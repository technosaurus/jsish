#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "value.h"
#include "regexp.h"
#include "func.h"
#include "error.h"

regex_t *regex_new(const char *str, int compflag)
{
	regex_t *reg = malloc(sizeof(regex_t));
	if (!reg) die("Out of memory\n");
	
	if (regcomp(reg, str, compflag)) 
		die("Invalid regex string '%s'\n", str);

	return reg;
}


