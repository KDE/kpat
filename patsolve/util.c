/* Standard utilities. */

#include "util.h"
#include "pat.h"

/* Like strcpy() but return the length of the string. */

int strecpy(char *dest, char *src)
{
	int i;
	u_char *d = (u_char *)dest;
	u_char *s = (u_char *)src;

	i = 0;
	while ((*d++ = *s++) != '\0') {
		i++;
	}

	return i;
}

/* Allocate some space and return a pointer to it.  See new() in util.h. */

void *new_(size_t s)
{
	void *x;

	if (s > Mem_remain) {
		Status = FAIL;
		return NULL;
	}

	if ((x = (void *)malloc(s)) == NULL) {
		Status = FAIL;
		return NULL;
	}

	Mem_remain -= s;
	return x;
}
