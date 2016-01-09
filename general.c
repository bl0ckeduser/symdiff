
/* 
 * General-use routines 
 */

#include "gc.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <stdio.h>

char buf[1024];

void fail(char* mesg)
{
	fprintf(stderr, "error: %s\n", mesg);
	exit(1);
}

void sanity_requires(int exp)
{
	if(!exp)
		fail("that doesn't make any sense");
}

/*
 * Extract the raw string 
 * from a token object 
 * (original source: codegen_x86.c in my wannabe c compiler)
 */
char* get_tok_str(token_t t)
{
        strncpy(buf, t.start, t.len);
        buf[t.len] = 0;
        return buf;
}
