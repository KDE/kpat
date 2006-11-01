/* Standard utilities. */

#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* A function and some macros for allocating memory. */

extern void *new_(size_t s);
extern size_t Mem_remain;

#define new(type) (type *)new_(sizeof(type))
#define free_ptr(ptr, type) free(ptr); Mem_remain += sizeof(type)

#define new_array(type, size) (type *)new_((size) * sizeof(type))
#define free_array(ptr, type, size) free(ptr); \
				    Mem_remain += (size) * sizeof(type)

extern void free_buckets(void);
extern void free_clusters(void);
extern void free_blocks(void);

/* Error messages. */

extern char *Progname;
extern void msg(char *msg, ...);

/* Files. */

extern FILE *fileopen(char *name, char *mode);

/* Misc. */

extern int strecpy(char *dest, char *src);

#ifdef  __cplusplus
}
#endif

#endif  /* _UTIL_H */
