#ifndef DIAG_H
#define DIAG_H

extern void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

#endif
