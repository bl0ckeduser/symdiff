#ifndef RULEFILE_H
#define RULEFILE_H

extern void rule(char *r, exp_tree_t **rules, int* rc);
extern int readrules(exp_tree_t** rules, char *dir);

#endif
