#include "tree.h"
#include "hashtable.h"
#include "tokenizer.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

char buf[1024];

extern char* get_tok_str(token_t t);

double my_sec(double x)
{
	return 1 / cos(x);
}

double my_cot(double x)
{
	return 1 / tan(x);
}

double my_csc(double x)
{
	return 1 / sin(x);
}

double give_a_nan(double x) {
	return (float)1.0/0.0;
}

double float_eval(exp_tree_t *t, double val)
{
	int i;
	double result = 0.0;
	double (*fp)(double);
	int v;
	hashtab_t *function_dispatch_table = new_hashtab();
	struct fts {
		char *key;
		double (*val)(double);
	} function_table[] = {
		{ "arccos", &acos },
		{ "arcsin", &asin },
		{ "arctan", &atan },
		/* XXX: arccsc, arcsec, arccot */
		{ "cos", &cos},
		{ "cot", &my_cot },
		{ "csc", &my_csc },
		{ "ln", &log },
		/* n.b. this is by design; base-10 logarithms = shit */
		{ "log", &log },
		{ "sin", &sin },
		{ "sqrt", &sqrt },
		{ "tan", &tan },
		{ "sec", &my_sec },
		/*
		 * D is the local notation for the differentiation operator.
		 * Definitely things are wrong if it didn't succesfully reduce 
		 * differentiation operators to numerical operators, so then
		 * it is (presumably) appropriate to return a NaN.
		 */
		{ "D", &give_a_nan }
	};

	/*
 	 * Register functions
	 */
	for (i = 0; i < sizeof(function_table) / sizeof(struct fts); ++i) {
		hashtab_insert(function_dispatch_table, 
			       function_table[i].key,
			       (void *)function_table[i].val);
	}
	

	#define children_reduce_infix(operator) \
		for (i = 1, result = float_eval(t->child[0], val); i < t->child_count; ++i) \
			result = result operator float_eval(t->child[i], val);

	#define children_reduce_prefix(operator) \
		for (i = 1, result = float_eval(t->child[0], val); i < t->child_count; ++i) \
			result = operator ( result , float_eval(t->child[i], val) );

	switch (t->head_type) {
		case NEGATIVE: return -float_eval(t->child[0], val);
		case ADD: children_reduce_infix(+); break;
		case SUB: children_reduce_infix(-); break;
		case MULT: children_reduce_infix(*); break;
		case DIV: children_reduce_infix(/); break;

		case EXP: children_reduce_prefix(powf); break;

		case FUNC: {
			fp = hashtab_lookup(function_dispatch_table,
				            get_tok_str(*(t->tok)));
			if (!fp) {
				fprintf(stderr, "error: FLOATEVAL: uncoded function: %s\n", get_tok_str(*(t->tok)));
				exit(1);
			}
			result = fp(float_eval(t->child[0], val));
		}
		break;

		case VARIABLE: {
			if (!strcmp(get_tok_str(*(t->tok)), "e"))
				result = 2.71828182845904523536028747135266;
			else
				/* 
				 * Provided value + some variation based
				 * on the first letter of the variable
				 */
				result = val + (float)(1.0 * (*get_tok_str(*(t->tok)) % 5));
		}
		break;

		case NUMBER: {
			sscanf(get_tok_str(*(t->tok)), "%d", &v);
			result = (double)v;
		}
		break;
	}

	return result;
}

double float_diff(exp_tree_t *t, double val)
{
	double dx = 0.000001;
	/*
	 * This formula is actually better than the more obvious naive one.
	 * This is explained in the book "Numerical computing with IEEE floating point arithmetic"
	 * by Michael L. Overton, SIAM 2001, pages 73-75.
	 */
	double dy = float_eval(t, val + dx) - float_eval(t, val - dx);
	return dy/(2.0*dx);
}



