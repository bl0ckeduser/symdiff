#ifndef TOKENS_H
#define TOKENS_H

#include "tokenizer.h"
#include <stdlib.h>
#include <stdio.h>

/* token types */

enum {
	TOK_IF = 0,
	TOK_WHILE,
	TOK_INT,
	TOK_ECHO,
	TOK_DRAW,
	TOK_WAIT,
	TOK_CMX,
	TOK_CMY,
	TOK_MX,
	TOK_MY,
	TOK_OD,
	TOK_INTEGER,
	TOK_PLUS,
	TOK_MINUS,
	TOK_DIV,
	TOK_MUL,
	TOK_ASGN,
	TOK_EQ,
	TOK_GT,
	TOK_LT,
	TOK_GTE,
	TOK_LTE,
	TOK_NEQ,
	TOK_PLUSEQ,
	TOK_MINUSEQ,
	TOK_DIVEQ,
	TOK_MULEQ,
	TOK_WHITESPACE,
	TOK_IDENT,
	TOK_PLUSPLUS,
	TOK_MINUSMINUS,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_SEMICOLON,
	TOK_ELSE,
	TOK_COMMA,
	TOK_NEWLINE,
	TOK_GOTO,
	TOK_COLON,
	TOK_LBRACK,
	TOK_RBRACK,
	TOK_PROC,
	TOK_RET,
	TOK_FOR,
	TOK_QUOT,
	TOK_NAN,
	TOK_MNUM,
	TOK_MVAR,
	TOK_DOLLAR,
	TOK_CC,
	TOK_EXP,
	TOK_NOTSYMBOL,
	TOK_PIPE,
	TOK_PREVSUB
};

static char* tok_nam[] = {
	"TOK_IF",
	"TOK_WHILE",
	"TOK_INT",
	"TOK_ECHO",
	"TOK_DRAW",
	"TOK_WAIT",
	"TOK_CMX",
	"TOK_CMY",
	"TOK_MX",
	"TOK_MY",
	"TOK_OD",
	"TOK_INTEGER",
	"TOK_PLUS",
	"TOK_MINUS",
	"TOK_DIV",
	"TOK_MUL",
	"TOK_ASGN",
	"TOK_EQ",
	"TOK_GT",
	"TOK_LT",
	"TOK_GTE",
	"TOK_LTE",
	"TOK_NEQ",
	"TOK_PLUSEQ",
	"TOK_MINUSEQ",
	"TOK_DIVEQ",
	"TOK_MULEQ",
	"TOK_WHITESPACE",
	"TOK_IDENT",
	"TOK_PLUSPLUS",
	"TOK_MINUSMINUS",
	"TOK_LBRACE",
	"TOK_RBRACE",
	"TOK_LPAREN",
	"TOK_RPAREN",
	"TOK_SEMICOLON",
	"TOK_ELSE",
	"TOK_COMMA",
	"TOK_NEWLINE",
	"TOK_GOTO",
	"TOK_COLON",
	"TOK_LBRACK",
	"TOK_RBRACK",
	"TOK_PROC",
	"TOK_RET",
	"TOK_FOR",
	"TOK_QUOT",
	"TOK_NAN",
	"TOK_MNUM",
	"TOK_MVAR",
	"TOK_DOLLAR",
	"TOK_CC",
	"TOK_EXP",
	"TOK_NOTSYMBOL",
	"TOK_PIPE",
	"TOK_PREVSUB"
};

/* routines */

extern int is_add_op(char type);
extern int is_asg_op(char type);
extern int is_mul_op(char type);
extern int is_comp_op(char type);
extern int is_instr(char type);
extern void tok_display(FILE *f, token_t t);
extern token_t* make_fake_tok(char *s);

#endif

