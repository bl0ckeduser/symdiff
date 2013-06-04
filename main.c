/*
==============================================================================
        This is a symbolic differentiation program

   It features an infix REPL, a tree pattern-matcher/substitutor,
   and some preloaded derivative rules (the fanciest of which is
   probably the chain rule)

   This is a toy/self-education project by "blockeduser", May 2013.
   It is very far from perfect and could be improved upon.
   It is coded in hopefully-not-too-bad C.

   Much of it is strongly based on examples given in the SICP
   ("Structure and Interpretation of Computer Programs") book and videos
==============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include "parser.h"
#include "dict.h"
#include "subst.h"
#include "optimize.h"
#include "match.h"
#include "infix-printer.h"
#include "apply-rules.h"

int main(int argc, char** argv)
{
	char* buf;
	token_t* tokens;
	exp_tree_t tree;
	int mc;
	int i;
	exp_tree_t* rules[128];
	exp_tree_t* pres_rules[128];
	int rc;
	int prc;
	char lin[256];
	int success;

	extern void fail(char*);

	/*
	 * Setup lexer (wannabe regex code)
	 */
	setup_tokenizer();

	/*
	 * Read rules stored in "rules"
	 * directory. Boring file processing
	 * stuff that calls the parser and
 	 * stores parsed trees into an array.
	 * Code is data. Data is code.
	 */
	rc = readrules(rules, "rules");

	/* There are also rules for presentation
	 * of the final result, for example
	 * the notation e^x is preferred to
	 * the notation exp(x), etc.
	 */
	prc = readrules(pres_rules, "pres-rules");

	printf("Welcome to the REPL.\n");
	printf("As always, ctrl-C to exit.\n");

	while (1) {

		/* Print prompt, read line */
		printf("]=> ");
		fflush(stdout);
		fgets(lin, 1024, stdin);

		/* (stops gracefully on EOF) */
		if (feof(stdin)) {
			printf("\n");
			break;
		}

		/* Skip empty lines. The parsing code really
		 * hates them, and by "really" I mean segfault. */
		if (!lin[1])
			continue;

		/* Check for command (!xxx) */
		if (*lin == '!') {
			/* !sr: show stored rules */
			if (!strcmp(lin, "!sr\n")) {
				printf("%d rule%s memorized%s\n",
					rc,
					rc > 1 ? "s have been" : " has been",
					rc ? ":" : "");
				for (i = 0; i < rc; ++i) {
					printf("%d: ", i);
					fflush(stdout);
					printout_tree(*rules[i]);
					putchar('\n');
				}
			}
			/* Don't parse this ! */
			continue;
		}

		/* Lex the line */
		tokens = tokenize(lin);

#ifdef DEBUG_2
		/* Display the tokens */
		for (i = 0; tokens[i].start; i++) {
			fprintf(stderr, "%d: %s: ", i, tok_nam[tokens[i].type]);
			tok_display(stderr, tokens[i]);
			fputc('\n', stderr);
		}
#endif

		/* Parse the tokens into an expression-tree */
		tree = *((parse(tokens)).child[0]);

#ifdef DEBUG_2
		/* Print-out raw parse-tree in
		 * prefix (Lisp-style) notation */
		printout_tree(tree);
		fputc('\n', stderr);
#endif

#ifdef DEBUG
		/* Printout parse in standard infix
		 * notation */
		printout_tree_infix(tree);
		fputc('\n', stderr);
#endif
		/*
		 * If the expression is an equation,
		 * it's considered a pattern-matching rule
		 * and gets added to the list of rules.
		 */
		if (tree.head_type == EQL) {
			rules[rc++] = alloc_exptree(tree);
			printf("Rule stored.\n");
		} else {
			/*
			 * If, on the other hand, the expression
			 * is not an equation, well we try to
			 * reduce it using the rules.
			 */

			/* Print final result if any reductions
			 * succeeded */
			if (apply_rules_and_optimize(rules, rc, &tree)) {
				/* Some final clean-up ... */
				while(unwind_expos(&tree))
					;
				(void)apply_rules_and_optimize(pres_rules, prc,
					&tree);
				printout_tree_infix(tree);
				printf("\n");
			}
		}
	}

	return 0;
}
