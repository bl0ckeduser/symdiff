#include "gc.h"

/*
 * Substitution routine
 */

#include "dict.h"
#include "tree.h"
#include "subst.h"
#include <string.h>
#include <stdio.h>

char buf[128];

extern void fail(char* mesg);

/* Find index of symbol in dictionary array */
int findkey(char *buf, dict_t* d)
{
	int i;
	for (i = 0; i < d->count; ++i)
		if (!strcmp(d->symbol[i], buf))
			return i;
	return -1;
}

void subst(exp_tree_t *tree, dict_t *d, exp_tree_t **father_ptr)
{
	int i, key;

	/* Function name substitution */
	if (tree->head_type == FUNC) {
		/* Get metasymbol */
		strncpy(buf, tree->tok->start, tree->tok->len);
		buf[tree->tok->len] = 0;

		if((key = findkey(buf, d)) != -1) {
			if (!father_ptr)
				fail("null pointer substitution....");
			/* 
			 * Don't substitute trees, but instead 
			 * change the tree head label
			 */
			(*father_ptr)->tok = d->match[key]->tok;
			/* Then go substitute the children */
			goto match_children;
		}
	}

	/* General case */
	if (tree->head_type == VARIABLE) {
		strncpy(buf, tree->tok->start, tree->tok->len);
		buf[tree->tok->len] = 0;
		if((key = findkey(buf, d)) != -1) {
			if (!father_ptr)
				fail("null pointer substitution....");
			*father_ptr = copy_tree(d->match[key]);
		}
	} else {
match_children:
		/* Children recursion */
		for (i = 0; i < tree->child_count; ++i) {
			if(tree->child[i]->head_type == VARIABLE
			|| tree->child[i]->head_type == FUNC
			|| tree->child[i]->child_count)
				subst(tree->child[i], d, &(tree->child[i]));
		}
	}
}
