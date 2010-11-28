#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"
#include "scope.h"

#define MAX_SCOPE	4096

strs *strs_new()
{
	strs *ret = malloc(sizeof(strs));
	memset(ret, 0, sizeof(strs));
	return ret;
}

void strs_push(strs *ss, const unichar *string)
{
	if (ss->count >= ss->_size) {
		ss->_size += 5;
		ss->strings = realloc(ss->strings, (ss->_size) * sizeof(unichar *));
	}
	ss->strings[ss->count] = unistrdup(string);
	ss->count++;
}

strs *strs_dup(strs *ss)
{
	strs *n = strs_new();
	int i;
	if (!ss) return n;
	for (i = 0; i < ss->count; ++i) {
		strs_push(n, ss->strings[i]);
	}
	return n;
}

const unichar *strs_get(strs *ss, int i)
{
	if (i < 0 || i >= ss->count) return NULL;
	return ss->strings[i];
}

void strs_free(strs *ss)
{
	if (!ss) return;
	
	int i;
	for (i = 0; i < ss->count; ++i) {
		unifree(ss->strings[i]);
	}
	free(ss->strings);
	free(ss);
}

/* lexical scope */
static strs *scopes[MAX_SCOPE];
static int cur_scope;

void scope_push()
{
	if (cur_scope >= MAX_SCOPE - 1) bug("Scope chain to short\n");
	cur_scope++;
}

void scope_pop()
{
	if (cur_scope <= 0) bug("No more scope to pop\n");
	strs_free(scopes[cur_scope]);
	scopes[cur_scope] = NULL;
	cur_scope--;
}

void scope_add_var(const unichar *str)
{
	int i;
	if (scopes[cur_scope] == NULL) scopes[cur_scope] = strs_new();
	
	for (i = 0; i < scopes[cur_scope]->count; ++i) {
		if (unistrcmp(str, scopes[cur_scope]->strings[i]) == 0) return;
	}
	strs_push(scopes[cur_scope], str);
}

strs *scope_get_varlist()
{
	return strs_dup(scopes[cur_scope]);
}

