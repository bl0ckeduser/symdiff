#include "gc.h"

/*
 * Print out an expression tree in standard infix notation,
 * e.g. 1 + 2 * 3, omitting unnecessary parentheses.
 */

#include "tree.h"
#include "tokenizer.h"
#include "tokens.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char name[128];

/*
 * Helper routine: check precedence of an
 * operator. (Used by the unnecessary-
 * parenthesis-omission code below).
 */
int op_prec(int head_type)
{
	switch (head_type) {
		case EQL:	return 1;
		case EXP:	return 2;
		case DIV:	return 3;
		case MULT:	return 3;
		case SUB:	return 4;
		case ADD:	return 4;
		default:	return 0;
	}
}

/* Helper: tree type -> operator symbol */
char opsym(int head_type)
{
	switch (head_type) {
		case MULT:	return '*';
		case SUB:	return '-';
		case ADD:	return '+';
		case DIV:	return '/';
		case EQL:	return '=';
		case EXP:	return '^';
	}
}

void printout_tree_infix(exp_tree_t et)
{
	extern void printout_tree_infix_derp(exp_tree_t et, int pp);
	printout_tree_infix_derp(et, 0);
}

void printout_tree_infix_derp(exp_tree_t et, int pp)
{
	int i;
	int parens_a = 0;
	int parens_b = 0;

	/* 'foo(bar, donald, baker)' */
	if (et.head_type == FUNC) {
		strncpy(name, (et.tok)->start, (et.tok)->len);
		name[(et.tok)->len] = 0;
		printf("%s(", name);
		fflush(stdout);
		for (i = 0; i < et.child_count; i++) {
			if (i) {
				printf(", ");
				fflush(stdout);
			}
			printout_tree_infix_derp(*(et.child[i]), 0);
		}
		printf(")");
		fflush(stdout);
		return;
	}

	/* 'bar' */
	/* '123' */
	if (et.head_type == VARIABLE
	|| et.head_type == NUMBER) {
		strncpy(name, (et.tok)->start, (et.tok)->len);
		name[(et.tok)->len] = 0;
		printf("%s", name);
		fflush(stdout);
		return;
	}

	/* '-X' */
	if (et.head_type == NEGATIVE) {
		printf("-");
		fflush(stdout);
		printout_tree_infix_derp(**et.child, 0);
		return;
	}

	/* 'A + B + C' */
	if (et.head_type == MULT
	|| et.head_type == ADD
	|| et.head_type == SUB
	|| et.head_type == DIV
	|| et.head_type == EQL
	|| et.head_type == EXP) {
		/* 
		 * Need parentheses if child has lower precedence.
		 * e.g. (1 + 2) * 3
		 */
		parens_a = pp && pp < op_prec(et.head_type);
		if (parens_a) {
			printf("(");
			fflush(stdout);
		}

		for (i = 0; i < et.child_count; i++) {
			/* 
			 * Check for the second kind of need-of-parentheses,
			 * for example in expressions like 1 / (2 / 3) 
			 */
			parens_b = op_prec(et.head_type) == 
					op_prec(et.child[i]->head_type)
			    && et.child[i]->child_count
				&& op_prec(et.head_type)
				&& et.head_type != MULT
				&& et.head_type != ADD;

			if (parens_b)
				printf("(");

			/* print child */
			printout_tree_infix_derp(*(et.child[i]), op_prec(et.head_type));

			if (parens_b) {
				printf(")");
				fflush(stdout);
			}

			/* 
			 * In infix notation, an operator symbol comes
			 * after every child except the last
			 */
			if (i < et.child_count - 1) {
				printf(" %c ", opsym(et.head_type));
				fflush(stdout);
			}
		}

		/* close first type of parentheses-need */
		if (parens_a) {
			printf(")");
			fflush(stdout);
		}
		return;
	}
}


