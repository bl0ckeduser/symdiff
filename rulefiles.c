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
#include <dirent.h>
#include <unistd.h>
#include "rulefiles.h"
#include "optimize.h"

extern void fail(char*);


int readrules(exp_tree_t** rules, char *dir)
{
	DIR *dirp;
	struct dirent *dp;
	FILE* f;
	char lin[1024];
	int rc = 0;

	printf("Reading rules from '%s'...", dir);
	fflush(stdout);
	if ((dirp = opendir(dir))) {
		if (chdir(dir) == -1)
			fail("directory change failed");
		for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
			if (*(dp->d_name) != '.') {
#ifdef DEBUG_2
				printf("Reading rule-file '%s'...\n", dp->d_name);
#endif
				if ((f = fopen(dp->d_name, "r"))) {
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
					printf("\nCouldn't open rule-file '%s'\n", dp->d_name);
			}
		}
		if (chdir("..") == -1)
			fail("directory change failed");
		closedir(dirp);
		printf("Done.\n");
	} else
		printf("\nCouldn't open rules directory '%s'\n", dir);

	return rc;	/* rule count */
}

void rule(char *r, exp_tree_t **rules, int* rc)
{
	char* buf;
	token_t* tokens;
	exp_tree_t tree;
	int mc;
	int i;

	extern void fail(char*);

	/* lexing */
	tokens = tokenize(r);

#ifdef DEBUG_2
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
	optimize(&tree);

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


