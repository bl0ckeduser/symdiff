#include "gc.h"

/* Token helper routines */

#include "tokens.h"
#include <string.h>
#include <stdio.h>

extern void fail(char* mesg);

/* Print out a token's raw string */
void tok_display(FILE *f, token_t t)
{
	char buf[1024];
	strncpy(buf, t.start, t.len);
	buf[t.len] = 0;
	fprintf(f, "%s", buf);
}

/* Convert token's string to integer */
int tok2int(token_t *t)
{
	char buf[1024];
	int i;
	strncpy(buf, t->start, t->len);
	buf[t->len] = 0;
	sscanf(buf, "%d", &i);
	return i;
}

/* For hacks */
token_t* make_fake_tok(char *s)
{
	token_t *t = cgc_malloc(sizeof(token_t));
	if (!t)
		fail("cgc_malloc(sizeof(token_t))");
	t->start = cgc_malloc(strlen(s) + 1);
	strcpy(t->start, s);
	t->len = strlen(s);
	return t;
}


int is_add_op(char type)
{
	return type == TOK_PLUS || type == TOK_MINUS;
}

int is_mul_op(char type)
{
	return type == TOK_MUL || type == TOK_DIV;
}


