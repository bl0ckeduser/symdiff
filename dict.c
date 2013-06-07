/*
 * Data structure: symbol-matches dictionary produced
 * by the matching algorithm and used by the substitution
 * algorithm.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dict.h"

extern void fail(char*);

dict_t* new_dict()
{
	int i;

	dict_t* d = malloc(sizeof(dict_t));
	d->symbol = malloc(100 * sizeof(char *));
	for (i = 0; i < 100; ++i)
		d->symbol[i] = malloc(128);
	d->match = malloc(100 * sizeof(exp_tree_t *));
	if (!d->symbol || !d->match)
		fail("malloc dict");
	d->count = 0;
	d->alloc = 100;
	d->success = 0;
	return d;
}

void expand_dict(dict_t* d, int alloc)
{
	int i;
	int old = d->alloc;
	d->alloc = alloc;
	d->symbol = realloc(d->symbol, d->alloc * sizeof(char *));
	for (i = old; i < alloc; ++i)
		d->symbol[i] = malloc(128);
	d->match = realloc(d->match, d->alloc * sizeof(exp_tree_t *));
}

int dict_search(dict_t *d, char* var)
{
	int i;
	for (i = 0; i < d->count; ++i)
		if (!strcmp(d->symbol[i], var))
			return i;
	return -1;
}

/* for debugging purposes */
void printout_dict(dict_t *d)
{
	int i =0;
	printf("[");
	for (i = 0; i < d->count; ++i) {
		if (i)
			printf(", ");
		printf("%s: ", d->symbol[i]);
		fflush(stdout);
		printout_tree(*(d->match[i]));
		fflush(stdout);
	}
	printf("]");
	fflush(stdout);
}

void free_dict(dict_t *d)
{
	int i = 0;
	for (i = 0; i < d->alloc; ++i)
		free(d->symbol[i]);
	free(d->symbol);
	free(d->match);
	free(d);
}

