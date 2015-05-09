#include "gc.h"

/*
 * Optimizations:
 *
 * - constant folding for simple arithmetic
 * - canonicalization (flatten nesting)
 * - some algebraic simplifications
 *   (which get quite ugly to code)
 *
 * It has come at a point where almost
 * the bulk of the complete program is in
 * algebraic simplifications.
 *
 * Maybe it would be possible to come up with
 * a domain-language that simplifies the coding
 * of the algebraic simplifications...
 */

#include "tree.h"
#include "tokens.h"
#include <stdio.h>
#include <string.h>	/* memcpy */

#ifdef DEBUG_OPT
	#define return(X) { /* fprintf(stderr, "[DEBUG] leaving optimizer at line %d\n", \
				    __LINE__); \
			    fprintf(stderr, "[DEBUG] fe1: %f, fe2: %f\n", fe_init, float_eval(et, 2.0)); */ \
			    if (fabs(fe_init - float_eval(et, 2.0)) > 1.0) { \
				 fprintf(stderr, "\n\n\n**************************************\n"\
				 " [DEBUG] probably bad code at line %d of optimize.c\n" \
				 "**************************************\n\n\n", __LINE__); \
				 exit(1); \
			    } \
			    return(X); \
			  }
#endif

extern int tok2int(token_t *t);
extern int sametree(exp_tree_t *a, exp_tree_t *b);

char buf[1024];
exp_tree_t new, *new_ptr;
exp_tree_t new2, *new2_ptr;
exp_tree_t num, *num_ptr;
exp_tree_t *cancel;

void make_tree_number(exp_tree_t *t, int n)
{
	char buffer[128];
	sprintf(buffer, "%d", n);
	t->tok = make_fake_tok(buffer);
	t->head_type = NUMBER;
	t->child_count = 0;
}

exp_tree_t* new_tree_number(int n)
{
	exp_tree_t t = new_exp_tree(NUMBER, NULL);
	make_tree_number(&t, n);
	return alloc_exptree(t);
}

/* check for an arithmetic operation tree */
int arith_type(int ty)
{
	return 	ty == ADD
			|| ty == SUB
			|| ty == MULT
			|| ty == DIV;
}

/*
 * Performs the conversion:
 * e^(a + b + c + ...) => e^a * e^b * e^c * ...
 */
int unwind_expos(exp_tree_t *et)
{
	int i, q;
	int res = 0;
	exp_tree_t *derp, *below;

	for (i = 0; i < et->child_count; ++i)
		res |= unwind_expos(et->child[i]);
			
	if (et->head_type == EXP
		&& et->child_count == 2
		&& et->child[1]->head_type == ADD
		&& et->child[1]->child_count >= 2) {

		if (et->child[0]->head_type == VARIABLE) {
			strncpy(buf, et->child[0]->tok->start, et->child[0]->tok->len);
			buf[et->child[0]->tok->len] = 0;

			if (!strcmp(buf, "e")) {
				/* yes */

				/* derp = e^  */
				derp = copy_tree(et);
				derp->child_count = 1;

				new = new_exp_tree(MULT, NULL);
				new_ptr = alloc_exptree(new);
				
				below = et->child[1];
				for (q = 0; q < below->child_count; ++q) {
					new2_ptr = copy_tree(derp);
					add_child(new2_ptr, below->child[q]);
					add_child(new_ptr, new2_ptr);
				}

				/* transpose */
				memcpy(et, new_ptr, sizeof(exp_tree_t));

				return 1;
			}
		}
	}
	
	return res;
}

int optimize(exp_tree_t *et)
{
	int i = 0;
	int q, w, e, r, t, y;
	int did = 0;
	int chk, chk2, chk3;
	int a, b;
	int val;
	exp_tree_t *below;
	exp_tree_t *herp;
	exp_tree_t *derp;
	exp_tree_t *right_child;
	double fe_init;
	extern token_t* make_fake_tok(char *s);
	extern double float_eval(exp_tree_t *, double val);

	#ifdef DEBUG_OPT
		fe_init = float_eval(et, 2.0);
	#endif

	for (i = 0; i < et->child_count; i++) {
		did |= optimize(et->child[i]);
	}

	/* e.g. (DIV 6 3) -> (NUMBER 2) */
	if (et->head_type == DIV
	&& et->child_count == 2
	&& et->child[0]->head_type == NUMBER
	&& et->child[1]->head_type == NUMBER) {
		a = tok2int(et->child[0]->tok);
		b = tok2int(et->child[1]->tok);
		if (b != 0 && a % b == 0) {
			et->child_count = 0;
			et->head_type = NUMBER;
			sprintf(buf, "%d", a / b);
			et->tok = (token_t *)make_fake_tok(buf);
			return(1);
		}
	}

	/*
	 * Fold trivial arithmetic constants
	 */
	if (et->head_type == MULT
	|| et->head_type == ADD
	|| et->head_type == SUB
	|| et->head_type == NEGATIVE) {
		chk = 1;
		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type != NUMBER) {
				chk = 0;
				break;
			}

		if (chk) {
			did = 1;
			val = tok2int(et->child[0]->tok);
			for (i = 1; i < et->child_count; ++i) {
				switch(et->head_type) {
					case MULT:
					val *= tok2int(et->child[i]->tok);
					break;

					case ADD:
					val += tok2int(et->child[i]->tok);
					break;

					case SUB:
					val -= tok2int(et->child[i]->tok);
					break;
				}
			}

			if (et->head_type == NEGATIVE)
				val *= -1;

			et->head_type = NUMBER;
			sprintf(buf, "%d", val);
			et->tok = (token_t *)make_fake_tok(buf);
			et->child_count = 0;
		} else {
			/* 
			 * Simplify mixed number/non-number
			 * to one number and the rest 
			 */
			chk = 0;
			chk2 = 0;
			for (i = 0; i < et->child_count; ++i) {
				if (et->child[i]->head_type == NUMBER)
					chk++;
				if (et->child[i]->head_type != NUMBER)
					chk2 = 1;
			}

			if ((chk > 1 || (chk == 1 && et->child[0]->head_type != NUMBER))
				&& chk2
				&& (et->head_type == ADD || et->head_type == MULT)) {
				new = new_exp_tree(et->head_type, et->tok);
				val = et->head_type == ADD ? 0 : 1;
				for (i = 0; i < et->child_count; ++i) {
					if (et->child[i]->head_type == NUMBER) {
						switch(et->head_type) {
							case MULT:
							val *= tok2int(et->child[i]->tok);
							break;

							case ADD:
							val += tok2int(et->child[i]->tok);
							break;
						}
					}
				}
				sprintf(buf, "%d", val);
				num = new_exp_tree(NUMBER, make_fake_tok(buf));
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, alloc_exptree(num));
				
				for (i = 0; i < et->child_count; ++i)
					if (et->child[i]->head_type != NUMBER)
						add_child(new_ptr, copy_tree(et->child[i]));

				/* overwrite tree data */
				memcpy(et, new_ptr, sizeof(exp_tree_t));

				return(1);
			}
		}
	}

	/* 
	 * Simplify nested binary trees,
	 * e.g. (SUB (SUB (NUMBER:3) (NUMBER:2)) (NUMBER:1))
	 * becomes (SUB (NUMBER:3) (NUMBER:2) (NUMBER:1))
	 */
	if (et->child_count == 2
		&& et->child[0]->head_type == et->head_type
		&& arith_type(et->head_type)) {
		below = et->child[0];
		right_child = et->child[1];
		et->child_count = 0;
		for (i = 0; i < below->child_count; i++)
			add_child(et, below->child[i]);
		add_child(et, right_child);
		return(1);
	}

	/* (MUL (MUL xxx) yyy zzz) => (MUL xxx yyy zzz) */
	if (et->child_count >= 2
		&& et->child[0]->head_type == et->head_type
		&& et->child[0]->child_count
		&& (et->head_type == MULT || et->head_type == ADD)) {

		/*
		 * 2014-06-01
		 *
		 * There was a pretty stupid bug here
		 * where it was always making MULT nodes
		 * even if the original was an ADD
		 *
		 * The GUI somehow managed to avoid it
		 */
		new = new_exp_tree(et->head_type, NULL);
		new_ptr = alloc_exptree(new);

		below = et->child[0];	

		for (q = 0; q < below->child_count; ++q)
			add_child(new_ptr, below->child[q]);
			
		for (q = 1; q < et->child_count; ++q)
			add_child(new_ptr, et->child[q]);

		memcpy(et, new_ptr, sizeof(exp_tree_t));

		return(1);
	} 

	/* (MUL xxx (MUL yyy)) => (MUL xxx yyy) */
	if (et->child_count >= 2
		&& et->child[et->child_count - 1]->head_type
		== et->head_type
		&& (et->head_type == MULT || et->head_type == ADD)) {

		below = et->child[et->child_count - 1];
		if (below->child_count) {
			--(et->child_count);
			for (i = 0; i < below->child_count; i++)
				add_child(et, below->child[i]);

			return(1);
		}
	}

	/* (A * 123 * C) / 456 => (123/456) * A * C */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == NUMBER) {
		below = et->child[0];
		for (i = 0; i < below->child_count; ++i) {
			if (below->child[i]->head_type == NUMBER) {
				new = new_exp_tree(DIV, NULL);
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, copy_tree(below->child[i]));
				add_child(new_ptr, copy_tree(et->child[1]));
				below->child[i] = new_ptr;
				et->child_count--;
				return(1);
			}
		}
	}

	/* (A * B * C) / C => A * B */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT) {		
		below = et->child[0];
		for (i = 0; i < below->child_count; ++i) {
			if (sametree(below->child[i], et->child[1])) {
				et->child_count--;
				make_tree_number(below->child[i], 1);
				return(1);
			}
		}
	}

	/* 
	 * (A * B * C) / (X * Y * B * Z)
	 * = (A * C) / (X * Y * Z)
	 */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == MULT) {
		
		for (q = 0; q < et->child[0]->child_count; ++q) {
			cancel = et->child[0]->child[q];
			for (e = 0; e < et->child[1]->child_count; ++e) {
				below = et->child[1]->child[e];
				if (sametree(below, cancel)) {
					make_tree_number(below, 1);
					make_tree_number(cancel, 1);
					return(1);
				}
			}
		}
	}

	/*
	 * (2/(2 * foo * bar)) => 1/ (foo * bar)
	 */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type != MULT
		&& et->child[1]->head_type == MULT) {
		
		cancel = et->child[0];
		for (e = 0; e < et->child[1]->child_count; ++e) {
			below = et->child[1]->child[e];
			if (sametree(below, cancel)) {
				make_tree_number(below, 1);
				make_tree_number(cancel, 1);
				return(1);
			}
		}
	}

	/*
	 * Helper for cases like  6 * 1/x^2 * x
	 */
	if (et->head_type == MULT) {
		chk = 0;
		for (q = 0; q < et->child_count; ++q) {
			if (et->child[q]->head_type == DIV
			   && et->child[q]->child_count == 2
			   && et->child[q]->child[0]->head_type == NUMBER
			   && tok2int(et->child[q]->child[0]->tok) == 1
			   && et->child[q]->child[1]->head_type == EXP
			   && et->child[q]->child[1]->child_count == 2
			   && et->child[q]->child[1]->child[1]->head_type == NUMBER) {
				cancel = et->child[q]->child[1]->child[0];
				for (w = 0; w < et->child_count; ++w) {
					if (w != q) {
						if (sametree(cancel, et->child[w])
						 || (et->child[w]->head_type == EXP
							&& et->child[w]->child_count == 2
							&& sametree(et->child[w]->child[0], cancel))) {
							
							new_ptr = copy_tree(cancel);
							chk = tok2int(et->child[q]->child[1]->child[1]->tok);

							et->child[q]->head_type = EXP;
							et->child[q]->child_count = 2;
							et->child[q]->child[0] = new_ptr;
							make_tree_number(et->child[q]->child[1], -chk);

							return(1);
						}
					}
				}
			}
		}
	}

	/* 
	 * 2014-06-11
	 * 
	 * x ^ 2 * x ^3 * x^A * x
	 * messed up here prior to fix
	 */

	/* A * B * A * XYZ * A => A^3 * B * XYZ */
	if (et->child_count >= 2
		&& et->head_type == MULT) {
		for (q = 0; q < et->child_count; ++q) {
			chk = 0;
			for (w = 0; w < et->child_count; ++w) {
				if (sametree(et->child[w], et->child[q]))
					++chk;
				if (et->child[w]->head_type == EXP
				&& et->child[w]->child_count == 2
				&& sametree(et->child[w]->child[0], et->child[q])) {
					if (et->child[w]->child[1]->head_type == NUMBER) {
						chk += tok2int(et->child[w]->child[1]->tok);
					} else {
						goto non_number_power;
					}
				}
			}

			if (et->child[q]->head_type == NUMBER)
				chk = 0;

			if (chk > 1 || chk < 0) {
				chk2 = 0;
				cancel = copy_tree(et->child[q]);
				for (q = 0; q < et->child_count; ++q) {
					if (sametree(cancel, et->child[q])
					|| (et->child[q]->head_type == EXP
						&& et->child[q]->child_count == 2
						&& sametree(et->child[q]->child[0], cancel))) {
						if(!chk2) {
							chk2 = 1;
							new = new_exp_tree(EXP, NULL);
							new_ptr = alloc_exptree(new);
							add_child(new_ptr, cancel);
							add_child(new_ptr, new_tree_number(chk));
							et->child[q] = new_ptr;
						} else {
							make_tree_number(et->child[q], 1);
						}
					}
				}
				return(1);
			}
		}
	}

non_number_power: ;

	/* 
	 * Remove 1's from products, 0's from additions,
	 * 0's from subtractions 
	 */
filter_zeroes:
	if ((et->head_type == MULT
	|| et->head_type == ADD
	|| et->head_type == SUB)
	&& et->child_count >= 2) {
		chk = 0;
		q = et->head_type == MULT;
		for (i = 0; i < et->child_count; ++i) {
			if (et->child[i]->head_type == NUMBER
			&& tok2int(et->child[i]->tok) == q) {
				++chk;
			}
		}
		if (chk) {
			new = new_exp_tree(et->head_type, NULL);
			new_ptr = alloc_exptree(new);
			for (i = 0; i < et->child_count; ++i)
				if (!(et->child[i]->head_type == NUMBER
				&& tok2int(et->child[i]->tok) == q))
					add_child(new_ptr, copy_tree(et->child[i]));

			/* transpose */
			memcpy(et, new_ptr, sizeof(exp_tree_t));

			return(1);
		}
	}

	/*
	 * 2014-04-09
	 *
	 * There was a bug here.
	 *	 ]=> C + B * A + A
	 *	 A * (1 + C + B)
	 *
	 * should be fixed now:
	 *	 ]=> C + B * A + A
	 *	 C + A * (1 + B)
	 */

	/* A + B * A => A * (1 + B) */
	if (et->child_count >= 2
		&& et->head_type == ADD) {
		chk = 0;

		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type == MULT) {
				chk = 1;
				break;
			}
		if (chk) {
			/* iterate thru terms */
			for (q = 0; q < et->child_count; ++q) {
				cancel = et->child[q];
				/* try to cancel term q from another term */
				for (w = 0; w < et->child_count; ++w) {
					below = et->child[w];
					if (below->head_type == MULT) {
						for (e = 0; e < below->child_count; ++e) {
							if (sametree(cancel, below->child[e])) {
								/* there ! cancel factor from term */

								exp_tree_t *new_sum = alloc_exptree(new_exp_tree(ADD, NULL));

								make_tree_number(below->child[e], 1);
								add_child(new_sum, copy_tree(below));
								make_tree_number(below, 0);

								/* cancel identical terms => 1 */
								cancel = copy_tree(cancel);
								for (r = 0; r < et->child_count; ++r)
									if (sametree(et->child[r], cancel)) {
										make_tree_number(et->child[r], 1);
										add_child(new_sum, copy_tree(et->child[r]));
										make_tree_number(et->child[r], 0);
									}

								/* make new product tree */
								new = new_exp_tree(MULT, NULL);
								new_ptr = alloc_exptree(new);
								add_child(new_ptr, cancel);
								add_child(new_ptr, copy_tree(new_sum));
								
								add_child(et, new_ptr);		

								/*
								 * The above makes lots of useless 0 terms
								 * as a side effect, so get rid of them.
								 * Oterhwise, the GUI has weird-looking steps.
								 */				
								goto filter_zeroes;
							}
						}
					}
				}
			}
		}
	}

	/* (DIV foo), (ADD foo), etc. -> foo */
	if (et->child_count == 1
	&& arith_type(et->head_type)) {
		below = et->child[0];
		memcpy(et, below, sizeof(exp_tree_t));
		return(1);
	}

	/* (A * B * C) + (X * Y * B) = B * [(A*C) + (X*Y)] */
	if (et->child_count >= 2
		&& et->head_type == ADD) {
		chk = 1;
		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type != MULT) {
				chk = 0;
				break;
			}
		
		if (chk) {
			/* iterate through terms */
			for (q = 0; q < et->child_count; ++q) {
				/* iterate through factors of a term */
				for (w = 0; w < et->child[q]->child_count; ++w) {
					/* check that this factor occurs in all terms */
					chk = 1;
					cancel = copy_tree(et->child[q]->child[w]);
					for (e = 0; e < et->child_count; ++e) {
						chk2 = 0;
						for (r = 0; r < et->child[e]->child_count; ++r) {
							if (sametree(et->child[e]->child[r], cancel)) {
								chk2 = 1;
								break;
							}
						}
						chk &= chk2;
					}

					/* don't factor-out numbers */
					if (cancel->head_type == NUMBER)
						chk = 0;

					if (is_special_tree(cancel->head_type))
						chk = 0;

					if (chk) {
						/* factor => 1 */
						for (e = 0; e < et->child_count; ++e)
							for (r = 0; r < et->child[e]->child_count; ++r)
								if (sametree(et->child[e]->child[r], cancel)) {
									make_tree_number(et->child[e]->child[r], 1);
									/*
								 	 * 2014-06-10
									 *
									 * another dumb bug fixed:
									 * only cancel once!!!
									 *
									 * ]=> (A * B * C) + (X * Y * B * B) + (B * S)
									 * B * (A * C + X * Y * B + S)
									 *
									 */
									break;
								}

						/* make new product tree */
						new = new_exp_tree(MULT, NULL);
						new_ptr = alloc_exptree(new);
						add_child(new_ptr, cancel);
						add_child(new_ptr, copy_tree(et));
						
						/* transpose */
						memcpy(et, new_ptr, sizeof(exp_tree_t));

						return(1);
					}
				}
			}
		}
	}

	/* A * B * (C / D) = (A * B * C) / D */
	if (et->child_count > 2
		&& et->head_type == MULT
		&& et->child[et->child_count - 1]->head_type == DIV
		&& et->child[et->child_count - 1]->child_count == 2

		/*
		 * 2015-05-04
		 *
		 * Prevent a conflict with 
		 * 	 (A * 123 * C) / 456 => (123/456) * A * C 
		 */
		&& !(et->child[et->child_count - 1]->child[0]->head_type == NUMBER
			&& et->child[et->child_count - 1]->child[1]->head_type == NUMBER)
		) {

		below = copy_tree(et->child[et->child_count - 1]);
		et->child_count--;
		add_child(et,
			below->child[0]);
		new = new_exp_tree(DIV, NULL);
		new_ptr = alloc_exptree(new);
		add_child(new_ptr, copy_tree(et));
		add_child(new_ptr, 
			below->child[1]);
		memcpy(et, new_ptr, sizeof(exp_tree_t));
		return(1);
	} 

	/* 
	 * (A * B * C ^ 2) / C ^ 6 
	 * => A * B * C ^ -4 
	 */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == EXP
		&& et->child[1]->child_count == 2) {

		cancel = copy_tree(et->child[1]->child[0]);
		below = et->child[0];

		/*
		 * 2014-06-10
		 *
		 * ensure all the ocurrences of C
		 * in the numerator are of the 2-child EXP kind
		 * before proceding !!
		 */
		chk = 1;
		for (i = 0; i < below->child_count; ++i) {
			if (!indep(below->child[i], cancel)) {
				if (! (below->child[i]->child_count == 2
					&& below->child[i]->head_type == EXP)) {
					chk = 0;
					break;
				}
			}
		}

		if (chk) {
			chk = 0;
			for (i = 0; i < below->child_count; ++i) {
				if (below->child[i]->child_count == 2
				&& below->child[i]->head_type == EXP) {
					if (sametree(below->child[i]->child[0], cancel)) {
						chk = 1;
						new = new_exp_tree(SUB, NULL);
						new_ptr = alloc_exptree(new);
						add_child(new_ptr, copy_tree(below->child[i]->child[1]));
						add_child(new_ptr, copy_tree(et->child[1]->child[1]));
						below->child[i]->child[1] = new_ptr;
					}
				}
			}
			if (chk) {
				et->child_count--;
				return(1);
			}
		}

	}

	/*
	 * sin(x) / (sin(x)^2 * cos(x))
	 * => 1 / (sin(x) * cos(x))
	 */
	if (et->head_type == DIV
		&& et->child_count == 2	
		&& et->child[1]->head_type == MULT) {

		cancel = et->child[0];
		below = et->child[1];
	
		for (i = 0; i < below->child_count; ++i) {
			if (below->child[i]->head_type == EXP
				&& below->child[i]->child_count == 2) {
					if (sametree(cancel, below->child[i]->child[0])) {
					
						num = new_exp_tree(SUB, NULL);
						num_ptr = alloc_exptree(num);
						add_child(num_ptr, new_tree_number(1));
						add_child(num_ptr, 
							copy_tree(below->child[i]->child[1]));

						new = new_exp_tree(EXP, NULL);
						new_ptr = alloc_exptree(new);
						add_child(new_ptr, et->child[0]);
						add_child(new_ptr, num_ptr);

						make_tree_number(below->child[i], 1);

						et->child[0] = new_ptr;
						return(1);
					}
			}
		}
	}

	/*
	 * x ^ 2 * x ^3 * x^A = x ^ (5 + A)
	 */
	if (et->head_type == MULT
		&& et->child_count >= 2) {
		for (i = 0; i < et->child_count; ++i) {
			if (et->child[i]->head_type == EXP
				&& et->child[i]->child_count == 2
				&& et->child[i]->child[0]->head_type == VARIABLE) {
				cancel = et->child[i]->child[0];

				/* look for other exponentiations in this variable */
				chk = 0;
				for (q = 0; q < et->child_count; ++q) {
					if (et->child[q]->head_type == EXP
						&& et->child[q]->child_count == 2
						&& et->child[q]->child[0]->head_type == VARIABLE
						&& sametree(cancel, et->child[q]->child[0])) {
						++chk;
					}
				}
			
				/* don't do this on e^x */
				strncpy(buf, cancel->tok->start, cancel->tok->len);
				buf[cancel->tok->len] = 0;
				if (!strcmp(buf, "e"))
					chk = 0;

				/* yes there are several. first sum up the exponents */
				if (chk > 1) {
					new = new_exp_tree(ADD, NULL);
					new_ptr = alloc_exptree(new);
					for (q = 0; q < et->child_count; ++q) {
						if (et->child[q]->head_type == EXP
							&& et->child[q]->child_count == 2
							&& et->child[q]->child[0]->head_type == VARIABLE
							&& sametree(cancel, et->child[q]->child[0])) {
							add_child(new_ptr, 
								copy_tree(et->child[q]->child[1]));
						}
					}


					/* now stick them in the first exponentiation,
					 * and delete the others */
					r = 0;
					for (q = 0; q < et->child_count; ++q) {
						if (et->child[q]->head_type == EXP
							&& et->child[q]->child_count == 2
							&& et->child[q]->child[0]->head_type == VARIABLE
							&& sametree(cancel, et->child[q]->child[0])) {
							if (!r) {
								r = 1;
								et->child[q]->child[1] = new_ptr;
							} else {
								make_tree_number(et->child[q], 1);
							}
						}
					}
					return(1);
				}
			}
		}
	}

	/*
	 *	(A * B^n * C) / B = (A * B ^ (n - 1) * C)
	 */
	/* A * A * A^3 * U/A  * FOO * BAR * XYZ * A * U/A */
	if (et->head_type == DIV
		&& et->child_count == 2 
		&& et->child[0]->head_type == MULT) {

		cancel = et->child[1];
		below = et->child[0];

		chk = 0;
		for (q = 0; q < below->child_count; ++q) {
			if (below->child[q]->head_type == EXP
				&& below->child[q]->child_count == 2
				&& sametree(below->child[q]->child[0], cancel)) {
				
				chk = 1;
				new = new_exp_tree(SUB, NULL);
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, copy_tree(below->child[q]->child[1]));
				add_child(new_ptr, new_tree_number(1));
		
				/* transpose */
				memcpy(below->child[q]->child[1], 
					new_ptr, sizeof(exp_tree_t));

				/* 
				 * 2014-07-01
				 * do this only once!!!
				 */
				break;
			}
		}

		if (chk) {
			et->child_count--;
			return(1);
		}
	}

	/*
	 * A^n * (B + C/A)
	 * = A^n * B + A^(n-1) * C
	 */
	if (et->head_type == MULT
		&& et->child_count == 2
		&& et->child[0]->head_type == EXP
		&& et->child[0]->child_count == 2) {
	
		cancel = et->child[0]->child[0];
		below = et->child[1];
		chk = 0;

		for (q = 0; q < below->child_count; ++q)
			if (below->child[q]->head_type == DIV
				&& below->child[q]->child_count == 2
				&& sametree(below->child[q]->child[1], cancel)) {
					++chk; 
					derp = below->child[q];
				}
	
		if (chk == 1 && below->head_type == ADD) {
			herp = copy_tree(et->child[0]);

			new2 = new_exp_tree(ADD, NULL);
			new2_ptr = alloc_exptree(new2);

			for (q = 0; q < below->child_count; ++q) {
				if (below->child[q] != derp) {
					new = new_exp_tree(MULT, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, copy_tree(herp));
					add_child(new_ptr, copy_tree(below->child[q]));
					add_child(new2_ptr, new_ptr);
				} else {
					num_ptr = copy_tree(herp);
					new = new_exp_tree(SUB, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, 
						copy_tree(et->child[0]->child[1]));
					add_child(new_ptr, new_tree_number(1));
					num_ptr->child[1] = new_ptr;

					new = new_exp_tree(MULT, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, 
						copy_tree(below->child[q]->child[0]));
					add_child(new_ptr, num_ptr);

					add_child(new2_ptr, new_ptr);
				}
			}

			memcpy(et, new2_ptr, sizeof(exp_tree_t));

			return(1);
		}
	}

	/*
	 * A^n * B * (C + D/A)
	 * = C * A ^ n * B + D * A ^ (n - 1) * B
	 */
	if (et->head_type == MULT) {
		for (i = 0; i < et->child_count; ++i) {
			if (et->child[i]->head_type == EXP
			&& et->child[i]->child_count == 2) {
				cancel = et->child[i];
				
				chk = 0;
				for (q = 0; q < et->child_count; ++q) {
					if (et->child[q]->head_type == ADD) {
						below = et->child[q];
						for (w = 0; w < below->child_count; ++w) {
							if (below->child[w]->head_type == DIV
								&& below->child[w]->child_count == 2
								&& sametree(below->child[w]->child[1],
									cancel->child[0])) {
								++chk;
								chk2 = w;	/* divison subchild */
								chk3 = q;	/* addition child */
							}
						}
					}
				}
			
				if (chk == 1) {
					below = et->child[chk3];
					new = new_exp_tree(ADD, NULL);
					new_ptr = alloc_exptree(new);
					for (q = 0; q < below->child_count; ++q) {
						num = new_exp_tree(MULT, NULL);
						num_ptr = alloc_exptree(num);
						if (q == chk2) {
							derp = copy_tree(below->child[q]);
							derp->child_count--;
							add_child(num_ptr, derp);
						}
						else
							add_child(num_ptr, copy_tree(below->child[q]));

						for (w = 0; w < et->child_count; ++w) {
							if (et->child[w] != below) {
								if (q == chk2 && et->child[w] == cancel) {
									/* bump down power */
									derp = copy_tree(et->child[w]);
									new2 = new_exp_tree(SUB, NULL);
									new2_ptr = alloc_exptree(new2);
									add_child(new2_ptr, copy_tree(cancel->child[1]));
									add_child(new2_ptr, new_tree_number(1));
									derp->child[1] = new2_ptr;

									add_child(num_ptr, derp);
								} else
									add_child(num_ptr, copy_tree(et->child[w]));
							}
						}
						add_child(new_ptr, num_ptr);
					}

					/* transpose and return happily */
					memcpy(et, new_ptr, sizeof(exp_tree_t));
					return 1;
				}
			}
		}
	}

	/* 
	 * 		(A + W + (B * C * X) / U + (C * D * Z) / E) / C
	 * 	=>	(A + W) / C + (B * X) / U + (D * Z) / E
	 *
	 *		((B*C*X)/U + (C*D*Z)/E)/C
	 *	=>	(B * X) / U + (D * Z) / E
	 *
	 * 		(A + B + (C * D * Z) / E) / C
	 * 	=>	(A + B) / C + (D * Z) / E
	 *
	 *		((B*C+X*C)/U + (C*D+Z*C+C)/E)/C
	 * 	=>	(B + X) / U + (1 + D + Z) / E
	 *
	 *		((B*C+X*C)/U + (C*D+Z*C+C-C+C*C)/E)/C
	 *	=>	(C * (1 + D + Z) + -1 * C + C ^ 2) / E / C + (B + X) / U
	 *
	 *		((B*C+X*C)/U + (C*D+Z*C+C)/E)/C
	 *	=>	(B + X) / U + (1 + D + Z) / E
	 */

	if (et->head_type == DIV
		&& et->child_count == 2
		&& et->child[0]->head_type == ADD) {

		chk = 0;

		below = et->child[0];
		cancel = et->child[1];

		for (q = 0; q < below->child_count; ++q)
			if (below->child[q]->head_type == DIV
				&& below->child[q]->child_count == 2
				&& below->child[q]->child[0]->head_type == MULT) {
				chk2 = 0;
				for (w = 0; w < below->child[q]->child[0]->child_count; ++w) {
					if (sametree(cancel, 
						below->child[q]->child[0]->child[w]))
					{
						++chk2;
					}
				}

				/* only makes sense to fold if there is exactly one occurence */
				if(chk2 == 1)
					++chk;
			}

		if (chk >= 1) {
			/* "A + W" part */
			new = new_exp_tree(ADD, NULL);
			new_ptr = alloc_exptree(new);

			chk2 = 0;
			for (q = 0; q < below->child_count; ++q) {
				chk3 = 0;

				/*
				 * 2014-07-01
				 * this deals with some crazy rare cases
				 */
				if( (  below->child[q]->child[0]->head_type == ADD
				    || below->child[q]->child[0]->head_type == SUB
				    || below->child[q]->child[0]->head_type == DIV)
				    && below->child[q]->child[0]->child_count == 1) {
					below->child[q]->child[0] = below->child[q]->child[0]->child[0];
				}

				if (below->child[q]->head_type == DIV
					&& below->child[q]->child_count == 2
					&& below->child[q]->child[0]->head_type == MULT) {
					for (w = 0; w < below->child[q]->child[0]->child_count; ++w)
						if (sametree(cancel, 
							below->child[q]->child[0]->child[w])) {
							chk3++;
						}
				}

				/* only makes sense to fold if there is exactly one occurence */
				chk = chk3 == 1;

				if (!chk) {
					++chk2;
					add_child(new_ptr, copy_tree(below->child[q]));
				}
			}

			/* divide by "C" to make "(A + W) / C" */
			if (chk2) {
				new2 = new_exp_tree(DIV, NULL);
				new2_ptr = alloc_exptree(new2);
				add_child(new2_ptr, new_ptr);
				add_child(new2_ptr, copy_tree(et->child[1]));
			}

			/* =============================== */

			/* make [(A+W)/C + ...] */
			num = new_exp_tree(ADD, NULL);
			num_ptr = alloc_exptree(num);
			if (chk2)
				add_child(num_ptr, new2_ptr);

			/* add cancelled-out terms to the sum */
			below = et->child[0];
			cancel = et->child[1];

			for (q = 0; q < below->child_count; ++q) {
				/*
				 * 2014-07-01
				 * this deals with some crazy rare cases
				 */
				if( (  below->child[q]->child[0]->head_type == ADD
				    || below->child[q]->child[0]->head_type == SUB
				    || below->child[q]->child[0]->head_type == DIV)
				    && below->child[q]->child[0]->child_count == 1) {
					below->child[q]->child[0] = below->child[q]->child[0]->child[0];
				}

				if (below->child[q]->head_type == DIV
					&& below->child[q]->child_count == 2
					&& below->child[q]->child[0]->head_type == MULT) {
						chk3 = 0;
						for (w = 0; w < below->child[q]->child[0]->child_count; ++w) {
							if (sametree(cancel, 
								below->child[q]->child[0]->child[w]))
							{
								++chk3;
							}
						}

						/* only makes sense to fold if there is exactly one occurence */
						if (chk3 == 1) {
							derp = below->child[q];

							for (e = 0; e < derp->child[0]->child_count; ++e)
								if (sametree(derp->child[0]->child[e], cancel))
									make_tree_number(derp->child[0]->child[e], 1);

							add_child(num_ptr, copy_tree(derp));
						}
				}
			}

			memcpy(et, num_ptr, sizeof(exp_tree_t));
			return(1);
		}
	}

	/* 
	 *    (A * B^n) / (B * C * D)	
	 * => (A * B ^ (n - 1)) / (C * D)
	 */
	if (et->head_type == DIV
		&& et->child_count == 2
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == MULT) {
		below = et->child[0];
		derp = et->child[1];
		for (q = 0; q < below->child_count; ++q) {
			if (below->child[q]->head_type == EXP
				&& below->child[q]->child_count == 2) {
				cancel = below->child[q];
				chk = 0;
				for (w = 0; w < derp->child_count; ++w)
					if (sametree(derp->child[w], cancel->child[0])) {
						chk2 = w;
						if (++chk > 1)
							break;
					}

				if (chk == 1) {
					make_tree_number(derp->child[chk2], 1);
					num = new_exp_tree(SUB, NULL);
					num_ptr = alloc_exptree(num);
					add_child(num_ptr, copy_tree(cancel->child[1]));
					add_child(num_ptr, new_tree_number(1));
					cancel->child[1] = num_ptr;
					return(1);
				}
			}
		}
	}

	/*
	 * --A = A
	 */
	if (et->head_type == NEGATIVE
		&& et->child_count == 1
		&& et->child[0]->head_type == NEGATIVE
		&& et->child[0]->child_count == 1) {

		memcpy(et, et->child[0]->child[0], sizeof(exp_tree_t));
		return 1;
	}

	/*
	 * A * U/A  * FOO * BAR * XYZ
	 * => U * FOO * BAR * XYZ
	 *
	 * When applied repeatedly, it can do e.g.
	 * 		A * U/A * BAR/A * A * A * XYZ
	 * 		=> U * BAR * A * XYZ
	 */
	if (et->head_type == MULT
		&& et->child_count >= 2) {

		for (q = 0; q < et->child_count; ++q) {
			cancel = et->child[q];

			/* A must be unique */
			chk = 0;
			for (w = 0; w < et->child_count; ++w)
				if (sametree(et->child[w], cancel))
					if (++chk > 1)
						break;
			if (chk != 1)
				break;

			/* and there must be one instance of U / A */
			chk = 0;
			for (w = 0; w < et->child_count; ++w) {
				if (et->child[w]->head_type == DIV
					&& et->child[w]->child_count == 2
					&& sametree(et->child[w]->child[1], cancel)) {
					if (++chk > 1)
						break;
					else
						chk2 = w;
				}				
			}

			/* all conditions fulfilled, do it */
			if (chk == 1) {
				make_tree_number(et->child[q], 1);
				et->child[chk2]->child_count--;
				return(1);
			}
		}
	}

	/*
	 * A * (B * C * ... * F) / G 
	 * => (A * B * C .. * F) / G
	 */
	if (et->head_type == MULT
		&& et->child_count == 2
		&& et->child[1]->head_type == DIV
		&& et->child[1]->child_count == 2
		&& et->child[1]->child[0]->head_type == MULT) {

		add_child(et->child[1]->child[0], copy_tree(et->child[0]));
		make_tree_number(et->child[0], 1);
		return 1;
	}

	/*
	 * 	   	donald / (u * x ^ n) * x * foo
	 * =>  	donald / (u * x ^ (n - 1)) * foo
	 */
	if (et->head_type == MULT) {
		for (q = 0; q < et->child_count; ++q) {
			cancel = et->child[q];
			for (w = 0; w < et->child_count; ++w) {
				if (et->child[w]->head_type == DIV
					&& et->child[w]->child_count == 2
					&& et->child[w]->child[1]->head_type == MULT) {
					below = et->child[w]->child[1];
					chk = 0;
					for (e = 0; e < below->child_count; ++e) {
						if(below->child[e]->head_type == EXP
							&& below->child[e]->child_count == 2
							&& sametree(below->child[e]->child[0], cancel)) {
							chk = 1;
							break;
						}
					}
					if (chk) {
						make_tree_number(cancel, 1);
						num = new_exp_tree(SUB, NULL);
						num_ptr = alloc_exptree(num);
						add_child(num_ptr, copy_tree(below->child[e]->child[1]));
						add_child(num_ptr, new_tree_number(1));
						below->child[e]->child[1] = num_ptr;
						return(1);
					}
				}
			}
		}
	}

	/* 
	 * N-term sum rule
	 * D(u, a + b + c + ...) = D(u, a) + D(u, b) + D(u, c) + ...
	 */
	if (et->head_type == FUNC
		&& *(et->tok->start) == 'D'
		&& et->tok->len == 1
		&& et->child_count == 2
		&& et->child[1]->head_type == ADD
		&& et->child[1]->child_count > 1) {

		new = new_exp_tree(ADD, NULL);
		new_ptr = alloc_exptree(new);

		for (q = 0; q < et->child[1]->child_count; ++q) {
			num_ptr = copy_tree(et);
			num_ptr->child[1] = copy_tree(et->child[1]->child[q]);
			add_child(new_ptr, num_ptr);
		}

		memcpy(et, new_ptr, sizeof(exp_tree_t));
		return(1);
	}

	/* 
	 * help product rule
	 *
	 * 2014-06-01:
	 * this has been extracted from the closed-source GUI code,
	 * version 1.0, June 13, 2013
	 *
	 * Well I forgot to cross-pollinate it back into
	 * the CLI opensource version for a while
	 */
	if (et->head_type == FUNC
		&& *(et->tok->start) == 'D'
		&& et->tok->len == 1
		&& et->child_count == 2
		&& et->child[1]->head_type == MULT
		&& et->child[1]->child_count > 2) {

		new_ptr = alloc_exptree(new_exp_tree(MULT, NULL));

		for (q = 0; q < 2; ++q) {
			add_child(new_ptr, copy_tree(et->child[1]->child[q]));
		}

		new2_ptr =alloc_exptree(new_exp_tree(MULT, NULL));

		for (q = 2; q < et->child[1]->child_count; ++q) {
			add_child(new2_ptr, copy_tree(et->child[1]->child[q]));
		}

		et->child[1]->child_count = 2;
		et->child[1]->child[0] = new_ptr;
		et->child[1]->child[1] = new2_ptr;
		
		return(1);
	}

	/*
	 * number * -expression => -number * expression
	 */
	if (et->head_type == MULT) {
		/*
		 * Look for a number
		 */
		chk = 0;
		for (q = 0; q < et->child_count; ++q) {
			if (et->child[q]->head_type == NUMBER) {
				chk = 1;
				break;
			}
		}

		/*
		 * We found a number, now look for a negative
		 */
		if (chk) {
			for (w = 0; w < et->child_count; ++w) {
				if (q != w && et->child[w]->head_type == NEGATIVE
				    && et->child[w]->child_count == 1) {
					/* bust off the negative from ``-expression'' */
					et->child[w] = et->child[w]->child[0];
					
					/* remake the number as -number */
					r = tok2int(et->child[q]->tok);
					make_tree_number(et->child[q], -r);
	
					return (1);
				}
			}
		}
	}

	/* anything times zero is 0 */
	if (et->head_type == MULT
		&& et->child_count > 1) {
		for (q = 0; q < et->child_count; ++q) {
			if (et->child[q]->head_type == NUMBER
				&& tok2int(et->child[q]->tok) == 0) {
				memcpy(et, new_tree_number(0), sizeof(exp_tree_t));
				return(1);
			}
		}
	}

	return did;
}

#ifdef DEBUG_OPT
	#define return(X) return(X)
#endif

