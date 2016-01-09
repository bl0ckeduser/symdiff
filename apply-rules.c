#include "gc.h"

#include "tree.h"
#include "dict.h"
#include "match.h"
#include "optimize.h"

/*
 * We recursively iterate through all
 * rules on all the children of the tree
 * until the expression is irreducible,
 * i.e. stops changing.
 *
 * Also, we (try to) apply the optimization
 * routine.
 *
 * This goes on in a loop as long as either
 * pass succeeds.
 */

int apply_rules_and_optimize(exp_tree_t** rules, int rc, exp_tree_t *tree)
{
	int success = 0;
	while (1) {
		if (matchloop(rules, rc, tree)) {
			/*
			 * Reduction algorithm
			 * suceeded, print reduced tree
			 */
			#ifdef DEBUG_2
				printout_tree(*tree);
				printf("\n");
			#endif
			#ifdef DEBUG
				printout_tree_infix(*tree);
				printf("\n");
			#endif
			success = 1;
		}

		/*
		 * If optimization succeeds in
		 * modifying the tree, try
		 * reducing the new optimized
		 * tree.
		 */
		if (optimize(tree)) {
			#ifdef DEBUG
				printf("[optimize] \n");
				#ifdef DEBUG_2
					printout_tree(*tree);
					printf("\n");
				#endif
				printout_tree_infix(*tree);
				printf("\n");
			#endif
			success = 1;
			continue;
		}
		break;
	}
	return success;
}
