#include "gc.h"

/*
 * Pretty compiler-failure error.
 * (same code as in my wannabe compiler
 * from december 2012, from which this
 * was taken from)
 */

#include "tokenizer.h"
#include "tokens.h"
#include <string.h>

void compiler_fail(char *message, token_t *token,
        int in_line, int in_chr)
{
	char buf[1024];
	int line;
	int chr;
	int i;

	/* 
	 * Check for specific line/chr override
	 * (used for special cases like end of line)
	 */
	line = in_line ? in_line : token->from_line;
	chr = in_line ? in_chr : token->from_char;

	fflush(stdout);
	fflush(stderr);

	/*
	 * Copy the originating code line to a buffer,
	 * then print this to stderr 
	 */
	strncpy(buf, code_lines[line], 1024);
	buf[1023] = 0;
	for (i = 0; code_lines[line]; ++i)
		if (buf[i] == '\n') {
			buf[i] = 0;
			break;
		}
	fprintf(stderr, "%s\n", buf);

	/*
	 * Put a little arrow under the offending
	 * character
	 */
	for (i = 1; i < chr; ++i)
		if (buf[i - 1] == '\t')
			fputc(buf[i - 1], stderr);
		else
			fputc(' ', stderr);
	fputc('^', stderr);
	fputc('\n', stderr);

	/*
	 * Finally display line number and error
	 * description
	 */
	fprintf(stderr, "line %d: %s\n",
		line,
		message);
	exit(1);
}
