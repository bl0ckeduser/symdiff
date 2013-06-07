#include "gc.h"

/* Tree helper routines */

/* FIXME: crazy cgc_mallocs and memcpys, no garbage collection (!)
 *        a modern OS should clean up, but nevertheless, yeah */

#include "tree.h"
#include "tokenizer.h"
#include "tokens.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern void fail(char* mesg);

char buf_a[128], buf_b[128];

int sametree(exp_tree_t *a, exp_tree_t *b)
{
	int i;

	/* compare child counts and head-types ... */
	if (a->child_count != b->child_count)
		return 0;
	if (a->head_type != b->head_type)
		return 0;

	if (a->tok && b->tok) {
		/* extract token strings */
		strncpy(buf_a, a->tok->start, a->tok->len);
		buf_a[a->tok->len] = 0;
		strncpy(buf_b, b->tok->start, b->tok->len);
		buf_b[b->tok->len] = 0;

		/* compare 'em */
		if (strcmp(buf_a, buf_b))
			return 0;
	}

	/* recurse onto children */
	for (i = 0; i < a->child_count; ++i)
		if (!sametree(a->child[i], b->child[i]))
			return 0;

	/* none of all these tests failed ?
	 * the trees are the same, then ! */
	return 1;
}

exp_tree_t* copy_tree(exp_tree_t *src)
{
	exp_tree_t et = new_exp_tree(src->head_type, src->tok);
	int i;

	for (i = 0; i < src->child_count; ++i)
		if (src->child[i]->child_count == 0)
			add_child(&et, alloc_exptree(*src->child[i]));
		else
			add_child(&et, copy_tree(src->child[i]));

	return alloc_exptree(et);
}

exp_tree_t *alloc_exptree(exp_tree_t et)
{
	exp_tree_t *p = cgc_malloc(sizeof(exp_tree_t));
	if (!p)
		fail("cgc_malloc et");
	*p = et;
	return p;
}

int valid_tree(exp_tree_t et)
{
	return et.head_type != NULL_TREE;
}

exp_tree_t new_exp_tree(unsigned int type, token_t* tok)
{
	token_t* tok_copy = NULL;

	if (tok) {
		tok_copy = cgc_malloc(sizeof(token_t));
		if (!tok_copy)
			fail("cgc_malloc tok_copy");
		memcpy(tok_copy, tok, sizeof(token_t));
	}

	exp_tree_t tr;
	tr.head_type = type;
	tr.tok = tok_copy;
	tr.child_count = 0;
	tr.child_alloc = 64;
	tr.child = cgc_malloc(64 * sizeof(exp_tree_t *));
	if (!tr.child)
		fail("cgc_malloc tree children");
	tr.child_count = 0;

	return tr;
}

void add_child(exp_tree_t *dest, exp_tree_t* src)
{
	if (++dest->child_count >= dest->child_alloc) {
		dest->child_alloc += 64;
		dest->child = realloc(dest->child,
			dest->child_alloc * sizeof(exp_tree_t *));
		if (!dest->child)
			fail("realloc tree children");
	}
	dest->child[dest->child_count - 1] = src;
	dest->child[dest->child_count] = NULL;
}

/* 
 * Lisp-like prefix tree printout, 
 * useful for debugging the parser
 */
void printout_tree(exp_tree_t et)
{
	int i;
	fprintf(stderr, "(%s", tree_nam[et.head_type]);
	if (et.tok && et.tok->start) {
		fprintf(stderr, ":");
		tok_display(stderr, *et.tok);
	}
	for (i = 0; i < et.child_count; i++) {
		fprintf(stderr, " ");
		fflush(stderr);
		printout_tree(*(et.child[i]));
	}
	fprintf(stderr, ")");
	fflush(stderr);
}

int is_special_tree(int ht)
{
	return ((ht == MATCH_NUM)
	|| (ht == MATCH_VAR)
	|| (ht == MATCH_IND)
	|| (ht == MATCH_CONT)
	|| (ht == MATCH_FUNC)
	|| (ht == MATCH_CONTX)
	|| (ht == MATCH_NAN));
}

