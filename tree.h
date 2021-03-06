#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include "tokenizer.h"

enum {		/* head_type */
	BLOCK = 0,
	ADD,
	SUB,
	MULT,
	DIV,
	ASGN,
	IF,
	WHILE,
	NUMBER,
	VARIABLE,
	GT,
	LT,
	GTE,
	LTE,
	EQL,
	NEQL,
	INT_DECL,
	BPF_INSTR,
	NEGATIVE,
	INC,
	DEC,
	POST_INC,
	POST_DEC,
	LABEL,
	GOTO,
	ARRAY,
	ARRAY_DECL,
	ARG_LIST,
	PROC,
	RET,
	FUNC,
	SYMBOL,
	MATCH_NUM,
	MATCH_VAR,
	MATCH_IND,
	MATCH_CONT,
	MATCH_FUNC,
	MATCH_CONTX,
	MATCH_NAN,
	MATCH_IND_SYM,
	EXP,
	/* special */
	NULL_TREE
};

static char* tree_nam[] = {
	"BLOCK",
	"ADD",
	"SUB",
	"MULT",
	"DIV",
	"ASGN",
	"IF",
	"WHILE",
	"NUMBER",
	"VARIABLE",
	"GT",
	"LT",
	"GTE",
	"LTE",
	"EQL",
	"NEQL",
	"INT_DECL",
	"BPF_INSTR",
	"NEGATIVE",
	"INC",
	"DEC",
	"POST_INC",
	"POST_DEC",
	"LABEL",
	"GOTO",
	"ARRAY",
	"ARRAY_DECL",
	"ARG_LIST",
	"PROC",
	"RET",
	"FUNC",
	"SYMBOL",
	"MATCH_NUM",
	"MATCH_VAR",
	"MATCH_IND",
	"MATCH_CONT",
	"MATCH_FUNC",
	"MATCH_CONTX",
	"MATCH_NAN",
	"MATCH_IND_SYM",
	"EXP",
	"NULL_TREE"
};


typedef struct exp_tree {
	char head_type;
	token_t* tok;
	unsigned int child_count;
	unsigned int child_alloc;
	struct exp_tree **child;
} exp_tree_t;

static exp_tree_t null_tree = { NULL_TREE, NULL, 0, 0, NULL };

extern void add_child(exp_tree_t *dest, exp_tree_t* src);
extern exp_tree_t new_exp_tree(unsigned int type, token_t* tok);
extern int valid_tree(exp_tree_t et);
extern exp_tree_t *alloc_exptree(exp_tree_t et);
extern void printout_tree(exp_tree_t et);
extern exp_tree_t* copy_tree(exp_tree_t *);
extern int is_special_tree(int ht);

#endif
