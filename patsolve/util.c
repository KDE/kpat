/* Standard utilities. */

#include "util.h"
#include "pat.h"

char *Progname = NULL;

/* Just print a message. */

void msg(char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
}

/* Open a file or exit on failure. */

FILE *fileopen(char *name, char *mode)
{
    FILE *f;
    
    if ((f = fopen(name, mode)) == NULL) {
	fprintf(stderr, "can't %s '%s'", *mode == 'r' ? "open" : "write", name);
	exit(1);
    }
    return f;
}

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
