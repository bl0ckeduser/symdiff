#include "gc.h"

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
#include "rulefiles.h"
#include "gc.h"

int main(int argc, char** argv)
{
	token_t* tokens;
	exp_tree_t tree;
	int i;
	exp_tree_t* rules[128];
	exp_tree_t* pres_rules[128];
	int rc;
	int prc;
	char lin[1024];
	gcarray_t *setup, *iter;

	extern void fail(char*);
	extern int unwind_expos(exp_tree_t *et);

#ifdef LEAK_STRESS_TEST
	strcpy(lin, "diff(z^z^z, z)");
#endif

	cgc_set(setup = new_gca());

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

	/*
	 * 2014-04-24
	 *
	 * It's helpful to tell the user what
	 * the program actually does and what
	 * its syntax is. (thanks knipil)
	 */
	printf("\n-------------------------------------------------------\n");
	printf("Welcome to SYMDIFF, a program trained to compute \n");
	printf("symbolic derivatives. You may enter requests such as: \n\n");
	printf("\t diff(3cos(x^2)^3 + x^x, x)\n\n");
	printf("and quickly obtain the symbolic result.\n");
	printf("(This code was speed-tuned on an ancient 400 MHz machine.)\n");
	printf("\nYou will be given a prompt. As always, you can use\n");
	printf("CTRL-C to exit.\n");
	printf("\n-------------------------------------------------------\n");
	printf("\n");

	while (1) {

#ifndef LEAK_STRESS_TEST
		/* Print prompt, read line */
		printf("]=> ");
		fflush(stdout);
		if (!fgets(lin, 1024, stdin)) {
			printf("\n");	/* (stops gracefully on EOF) */
			break;
		}
#endif

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

		cgc_set(iter = new_gca());

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
#ifndef LEAK_STRESS_TEST
				printout_tree_infix(tree);
				printf("\n");
#endif
			}
		}

		gca_cleanup(iter);
	}

	gca_cleanup(setup);

	return 0;
}
