#ifndef GC_H
#define GC_H

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
	int alloc;
	int memb;
	void **pile;
} gcarray_t;

gcarray_t* new_gca(void);
void *cgc_malloc(size_t size);

#endif
