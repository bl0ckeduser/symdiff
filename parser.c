#include "gc.h"

/*
 * Parser (tokens -> tree)
 *
 * (this code was copied and adapted from 
 * my other project, the wannabe compiler)
 *
 * BUGS: seems to get into infinite recursions
 * or segfaults on certain occurences of 
 * invalid syntax
 */

#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include "diagnostics.h"
#include <string.h>

token_t *tokens;
int indx;
int tok_count;

exp_tree_t block();
exp_tree_t expr();
exp_tree_t pow_expr(int);
exp_tree_t sum_expr();
exp_tree_t mul_expr();
exp_tree_t unary_expr(int);
exp_tree_t signed_expr();
void printout(exp_tree_t et);
extern void fail(char* mesg);

/* give the current token */
token_t peek()
{
	if (indx >= tok_count) {
		return tokens[tok_count - 1];
	}
	else
		return tokens[indx];
}

/* generate a pretty error when parsing fails.
 * this is a hack-wrapper around the routine
 * in diagnostics.c that deals with certain
 * special cases special to the parser. */
void parse_fail(char *message)
{
	token_t tok;
	int line, chr;

	if (tokens[indx].from_line
		!= tokens[indx - 1].from_line) {
		/* end of the line */
		line = tokens[indx - 1].from_line;
		chr = strstr(code_lines[line], "\n")
			- code_lines[line] + 1;
	} else {
		line = tokens[indx].from_line;
		chr = tokens[indx].from_char;
	}

	compiler_fail(message, NULL, line, chr);
}

/*
 * Fail if the next token isn't of the desired type.
 * Otherwise, return it and advance.
 */
token_t need(char type)
{
	char buf[1024];
	if (tokens[indx++].type != type) {
		fflush(stdout);
		--indx;
		sprintf(buf, "%s expected", tok_nam[type]);
		parse_fail(buf);
	} else
		return tokens[indx - 1];
}

/* main parsing loop */
exp_tree_t parse(token_t *t)
{
	exp_tree_t program;
	exp_tree_t *subtree;
	program = new_exp_tree(BLOCK, NULL);
	int i;

	/* count the tokens */
	for (i = 0; t[i].start; ++i)
		;
	tok_count = i;

	/* the loop */
	tokens = t;
	indx = 0;
	while (tokens[indx].start) {
		subtree = alloc_exptree(block());
		/* parse fails at EOF */
		if (!valid_tree(*subtree))
			break;
		add_child(&program, subtree);
	}

	return program;
}

/* 
 * Now the real fun starts. Each routine is
 * named after the part of the grammar that
 * it deals with. When it doesn't find anything
 * that it's been trained to find, a routine
 * gives back "null_tree".
 */


/*
 *	variables (any text symbol)
 * 	quoted symbols : ' text
 */
exp_tree_t lval()
{
	token_t tok = peek();
	token_t tok2;
	exp_tree_t tree, subtree;
	int cc = 0;

	if (tok.type == TOK_NAN) {
		++indx;	/* eat @ */
		tok = peek();
		++indx;
		return new_exp_tree(MATCH_NAN, &tok);
	} else if (tok.type == TOK_MVAR) {
		++indx;	/* eat ? */
		tok = peek();
		++indx;	/* eat ident */
		return new_exp_tree(MATCH_VAR, &tok);
	}
	else if (tok.type == TOK_MNUM) {
		++indx;	/* eat # */
		tok = peek();
		++indx;	/* eat int */
		return new_exp_tree(MATCH_NUM, &tok);
	}
	else if (tok.type == TOK_QUOT) {
		++indx;	/* eat quote */
		tok = peek();
		++indx;	/* eat ident */
		return new_exp_tree(SYMBOL, &tok);
	}
	else if (tok.type == TOK_IDENT) {
			++indx;	/* eat the identifier */
			if (peek().type == TOK_NOTSYMBOL) {
				++indx;
				tok2 = peek();
				++indx;
				if (tok2.type != TOK_IDENT)
					parse_fail("plain symbol expected");
				tree = new_exp_tree(MATCH_IND_SYM, NULL);
				subtree = new_exp_tree(VARIABLE, &tok);
				add_child(&tree, alloc_exptree(subtree));
				subtree = new_exp_tree(VARIABLE, &tok2);
				add_child(&tree, alloc_exptree(subtree));
				return tree;
			} else if (peek().type == TOK_PIPE) {
				++indx;
				tok2 = peek();
				++indx;
				if (tok2.type != TOK_IDENT)
					parse_fail("plain symbol expected");
				tree = new_exp_tree(MATCH_IND, NULL);
				subtree = new_exp_tree(VARIABLE, &tok);
				add_child(&tree, alloc_exptree(subtree));
				subtree = new_exp_tree(VARIABLE, &tok2);
				add_child(&tree, alloc_exptree(subtree));
				return tree;
			} else 	if (peek().type == TOK_COLON
				|| peek().type == TOK_CC) {
				cc = peek().type == TOK_CC;
				++indx;
				tok2 = peek();
				++indx;
				if (tok2.type != TOK_IDENT)
					parse_fail("plain symbol expected");
				tree = new_exp_tree(cc ? MATCH_CONTX
					: MATCH_CONT, NULL);
				subtree = new_exp_tree(VARIABLE, &tok);
				add_child(&tree, alloc_exptree(subtree));
				subtree = new_exp_tree(VARIABLE, &tok2);
				add_child(&tree, alloc_exptree(subtree));
				return tree;
			} else
				return new_exp_tree(VARIABLE, &tok);
	}
	return null_tree;
}

exp_tree_t block()
{
	return expr();
}

/*
	expr :=  sum_expr ['=' sum_expr]
*/
exp_tree_t expr()
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3;
	exp_tree_t lv;
	token_t oper;
	token_t tok;
	int save = indx;

	/* sum_expr ['=' sum_expr] */
	if (valid_tree(subtree = sum_expr())) {
		if (peek().type != TOK_ASGN) {
			return subtree;
		}
		if (peek().type == TOK_ASGN) {
			tree = new_exp_tree(EQL, NULL);
			++indx;	/* eat comp-op */
			add_child(&tree, alloc_exptree(subtree));
			if (!valid_tree(subtree2 = sum_expr()))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree2));
		}
		return tree;
	}

	return null_tree;
}

/* sum_expr := mul_expr { add-op mul_expr } */
exp_tree_t sum_expr()
{
	exp_tree_t child, tree, new;
	exp_tree_t *child_ptr, *tree_ptr, *new_ptr;
	exp_tree_t *root;
	int prev;
	token_t oper;

	if (!valid_tree(child = mul_expr()))
		return null_tree;
	
	if (!is_add_op((oper = peek()).type))
		return child;
	
	child_ptr = alloc_exptree(child);
	
	switch (oper.type) {
		case TOK_PLUS:
			tree = new_exp_tree(ADD, NULL);
		break;
		case TOK_MINUS:
			tree = new_exp_tree(SUB, NULL);
		break;
	}
	prev = oper.type;
	++indx;	/* eat add-op */
	tree_ptr = alloc_exptree(tree);
	add_child(tree_ptr, child_ptr);

	while (1) {
		if (!valid_tree(child = mul_expr()))
			parse_fail("expression expected");

		/* add term as child */
		add_child(tree_ptr, alloc_exptree(child));

		/* bail out early if no more operators */
		if (!is_add_op((oper = peek()).type))
			return *tree_ptr;

		switch (oper.type) {
			case TOK_PLUS:
				new = new_exp_tree(ADD, NULL);
			break;
			case TOK_MINUS:
				new = new_exp_tree(SUB, NULL);
			break;
		}
		++indx;	/* eat add-op */

		new_ptr = alloc_exptree(new);
		add_child(new_ptr, tree_ptr);
		tree_ptr = new_ptr;
	}

	return *tree_ptr;
}

#define IMPMUL_DEBUG

/* mul_expr := pow_expr { ( mul-op signed_expr) | (pow_expr) } */
exp_tree_t mul_expr()
{
	/* this routine is mostly a repeat of sum_expr() */
	exp_tree_t child, child2, tree, new;
	exp_tree_t *child_ptr, *tree_ptr, *new_ptr;
	exp_tree_t *root;
	int prev;
	token_t oper;
	int impmul = 0;

	if (!valid_tree(child = signed_expr()))
		return null_tree;
	
	/*
	 * maybe an implicit multiplication...
	 */
	if (!is_mul_op((oper = peek()).type)) {
		if (indx < tok_count) {
			if (valid_tree(child2 = pow_expr(1))) {
				tree = new_exp_tree(MULT, NULL);
				impmul = 1;
				#ifdef IMPMUL_DEBUG
					printf("IMPMUL\n");
					printout_tree(child2);
					printf("\n");
				#endif
			} else {
				return child;
			}
		} else {
			return child;
		}
	}

	child_ptr = alloc_exptree(child);
	
	if (!impmul) {
		switch (oper.type) {
			case TOK_DIV:
				tree = new_exp_tree(DIV, NULL);
			break;
			case TOK_MUL:
				tree = new_exp_tree(MULT, NULL);
			break;
		}
		prev = oper.type;
		++indx;	/* eat add-op */
	}

	tree_ptr = alloc_exptree(tree);
	add_child(tree_ptr, child_ptr);

	if (impmul)
		child = child2;

	while (1) {
		if (!impmul) {
			if (!valid_tree(child = signed_expr()))
				parse_fail("expression expected");
		}

		/* add term as child */
		add_child(tree_ptr, alloc_exptree(child));

		/*
		 * it could be an implicit multiplication,
		 * as in x log(x) or 5x
		 * -- or the the end of this construction...
		 */
		impmul = 0;
		if (!is_mul_op((oper = peek()).type)) {
			if (indx < tok_count) {
				#ifdef IMPMUL_DEBUG
					printf("trying special (tokindx=%d/%d)...\n", indx, tok_count);
					//getchar();
				#endif
				if (valid_tree(child = pow_expr(1))) {
					oper.type = TOK_MUL;
					impmul = 1;
					#ifdef IMPMUL_DEBUG
						printf("IMPMUL\n");
					#endif
				} else {
					#ifdef IMPMUL_DEBUG
						printf("no\n");
					#endif
					return *tree_ptr;
				}
			} else {
				return *tree_ptr;
			}
		}

		if (!impmul) {
			switch (oper.type) {
				case TOK_DIV:
					new = new_exp_tree(DIV, NULL);
				break;
				case TOK_MUL:
					new = new_exp_tree(MULT, NULL);
				break;
			}
			++indx;	/* eat add-op */
		} else {
			new = new_exp_tree(MULT, NULL);
		}

		new_ptr = alloc_exptree(new);
		add_child(new_ptr, tree_ptr);
		tree_ptr = new_ptr;
	}

	return *tree_ptr;
}

/*
 	signed_expr := [-] pow_expr
*/
exp_tree_t signed_expr()
{
	token_t tok = peek();
	exp_tree_t tree, subtree;

	if (tok.type == TOK_MINUS) {
		++indx;
		tree = new_exp_tree(NEGATIVE, NULL);
		if (valid_tree(subtree = pow_expr(0)))
			add_child(&tree, alloc_exptree(subtree));
		else
			parse_fail("expression expected after sign");
		return tree;
	} else
		return pow_expr(0);
}

/*
	pow_expr :=  unary_expr ['^' expr]
*/
exp_tree_t pow_expr(int special)
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3;
	exp_tree_t lv;
	token_t oper;
	token_t tok;
	int save = indx;

	if (valid_tree(subtree = unary_expr(special))) {
		if (peek().type != TOK_EXP) {
			return subtree;
		}
		else {
			tree = new_exp_tree(EXP, NULL);
			++indx;	/* eat comp-op */
			add_child(&tree, alloc_exptree(subtree));
			if (!valid_tree(subtree2 = pow_expr(special)))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree2));
		}
		return tree;
	}

	return null_tree;
}


/*
	unary_expr :=  lvalue | integer
			|  '(' expr ')' 
			| ident '(' expr1, expr2, exprN ')'
			| - expr
*/
exp_tree_t unary_expr(int special)
{
	exp_tree_t tree = null_tree, subtree = null_tree;
	exp_tree_t subtree2 = null_tree, subtree3;
	token_t tok;
	int matchfunc = 0;

	/* $ident (expr1, ..., exprN) */
	if (peek().type == TOK_DOLLAR) {
		++indx;
		matchfunc = 1;
	}

	/* ident ( expr1, expr2, exprN ) */
	if (peek().type == TOK_IDENT) {
		tok = peek();
		++indx;
		if (peek().type == TOK_LPAREN) {
			++indx;	/* eat ( */
			subtree = new_exp_tree(matchfunc ? MATCH_FUNC :
				FUNC, &tok);
			while (peek().type != TOK_RPAREN) {
				subtree2 = expr();
				if (!valid_tree(subtree2))
					parse_fail("expression expected");
				add_child(&subtree, alloc_exptree(subtree2));
				if (peek().type != TOK_RPAREN)
					need(TOK_COMMA);
			}
			++indx;
			/* if there's already a negative sign,
			 * use that as a parent tree */
			if (valid_tree(tree))
				add_child(&tree, alloc_exptree(subtree));
			else
				tree = subtree;
			return tree;
		} else
			--indx;
	}

	/* parenthesized expression */
	if(peek().type == TOK_LPAREN) {
		++indx;	/* eat ( */
		if (!valid_tree(tree = expr()))
			parse_fail("expression expected");
		need(TOK_RPAREN);
		
		return tree;
	}

	tok = peek();
	
	if (tok.type == TOK_INTEGER) {
		tree = new_exp_tree(NUMBER, &tok);
		++indx;	/* eat number */

		return tree;
	}

	if (valid_tree(tree = lval())) {
		return tree;
	}

	if (!special && tok.type == TOK_MINUS) {
		++indx;
		tree = new_exp_tree(NEGATIVE, NULL);
		if (valid_tree(subtree = pow_expr(0)))
			add_child(&tree, alloc_exptree(subtree));
		else
			parse_fail("expression expected after sign");
		return tree;
	}

	return null_tree;
}

