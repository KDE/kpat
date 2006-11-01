/* Standard utilities. */

#include "util.h"
#include "pat.h"

char *Progname = NULL;

/* Print a message and exit. */

void fatalerr(char *msg, ...)
{
	va_list ap;

	if (Progname) {
		fprintf(stderr, "%s: ", Progname);
	}
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);

	exit(1);
}

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
		fatalerr("can't %s '%s'", *mode == 'r' ? "open" : "write", name);
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
#if 0
		POSITION *pos;

		/* Try to get some space back from the freelist. A vain hope. */

		while (Freepos) {
			pos = Freepos->queue;
			free_array(Freepos, u_char, sizeof(POSITION) + Ntpiles);
			Freepos = pos;
		}
		if (s > Mem_remain) {
			Status = FAIL;
			return NULL;
		}
#else
		Status = FAIL;
		return NULL;
#endif
	}

	if ((x = (void *)malloc(s)) == NULL) {
		Status = FAIL;
		return NULL;
	}

	Mem_remain -= s;
	return x;
}
