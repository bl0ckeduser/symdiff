#include "gc.h"

/*
 * This is pretty boring code which reads
 * all the lines in all the files in the
 * directory "rules", parses them and
 * adds the resulting trees into the
 * rule-list.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include "parser.h"
#include "dict.h"
#include "rulefiles.h"
#include "optimize.h"

extern void fail(char*);

int readrules(exp_tree_t** rules, char *dir)
{
	FILE* f  = NULL;
	FILE* index = NULL;
	char lin[1024];
	char path[1024];
	char filename[1024];
	int rc = 0;

	#ifndef FLOATEVAL
		printf("Reading rules from '%s'...", dir);
		fflush(stdout);
		#ifdef DEBUG_2
			printf("\n");
		#endif
	#endif

	/*
	 * Originally, this code relied on the OS
	 * to list the files in the rulefile directories.
	 * This worked, however there was a problem:
	 * it seems depending on the OS the files were shown in
	 * varying orders, leading to differing results.
	 * So now (2014-01-26) there are "_index" 
	 * files that explicitly hardcode the loading order.
	 *
	 * For testing purposes, the following should hold:
	 *		]=> diff(x^x^x, x)
	 * 		x ^ (x ^ x + x) * log(x) * (1 + log(x)) + x ^ ((x ^ x + x) - 1)
	 */

	sprintf(path, "%s/_index", dir);
	if ((index = fopen(path, "r"))) {
		for (;;) {
			if (feof(index))
				break;
			fscanf(index, "%s", filename);

			if (!*filename)
				break;

			sprintf(path, "%s/%s", dir, filename);
			#ifdef DEBUG_2
				printf("Reading rule-file '%s'...\n", path);
			#endif
			if ((f = fopen(path, "r"))) {
				while (1) {
					if (!fgets(lin, 1024, f))
						break;
					/* skips comments and blanks ... */
					if (!lin[1] || !*lin || *lin == '%')
						continue;
					else {
						lin[strlen(lin) - 1] = 0;
						rule(lin, rules, &rc);
					}
				}
				fclose(f);
			} else
				printf("\nCouldn't open rule-file '%s'\n", path);
		}
		#ifndef FLOATEVAL
			printf("Done.\n");
		#endif
		fclose(index);
	} else
		printf("\nCouldn't open index file for rules directory '%s'\n", dir);

	return rc;	/* rule count */
}

void rule(char *r, exp_tree_t **rules, int* rc)
{
	token_t* tokens;
	exp_tree_t tree;

	extern void fail(char*);

	/* lexing */
	tokens = tokenize(r);

	#ifdef DEBUG_2
		int i;
		/* Display the tokens */
		for (i = 0; tokens[i].start; i++) {
			fprintf(stderr, "%d: %s: ", i, tok_nam[tokens[i].type]);
			tok_display(stderr, tokens[i]);
			fputc('\n', stderr);
		}

		printout_tree(parse(tokens));
		printf("\n");
	#endif

	/* parsing */
	tree = *((parse(tokens)).child[0]);
	/* optimize(&tree); */

	/* some preventive checking */
	if (tree.head_type != EQL) {
		printf("\n");
		fail("you asked me to store, as a rule, a non-rule expression :(");
		printout_tree(tree);
		printf("\n");
	}

	rules[(*rc)++] = alloc_exptree(tree);
	#ifdef DEBUG_2
		printf("rule '%s' stored as %d\n", r, *rc - 1);
	#endif
}


