#ifndef MATCH_H
#define MATCH_H

extern int treematch(exp_tree_t *a, exp_tree_t *b, dict_t* d);
extern int matchloop(exp_tree_t** rules, int rc, exp_tree_t* tree);
extern exp_tree_t* find_subtree(exp_tree_t *tree, exp_tree_t *key);

#endif
