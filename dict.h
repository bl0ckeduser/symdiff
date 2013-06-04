#ifndef DICT_H
#define DICT_H

#include "tree.h"

typedef struct dict_s {
	char** symbol;
	exp_tree_t **match;
	int count;
	int alloc;
	int success;
} dict_t;

extern dict_t* new_dict();
extern void expand_dict(dict_t* d, int alloc);
extern int dict_search(dict_t *d, char* var);
extern void printout_dict(dict_t *d);
extern void free_dict(dict_t *d);

#endif
