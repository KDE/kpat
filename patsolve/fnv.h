/* This is a 32 bit FNV hash.  For more information, see
http://www.isthe.com/chongo/tech/comp/fnv/index.html */

#ifndef FNV_H
#define FNV_H

#include <sys/types.h>
#include "config.h"

#define FNV1_32_INIT 0x811C9DC5
#define FNV_32_PRIME 0x01000193

#define fnv_hash(x, hash) (((hash) * FNV_32_PRIME) ^ (x))

/* Hash a buffer. */

static INLINE u_int32_t fnv_hash_buf(u_char *s, int len)
{
	int i;
	u_int32_t h;

	h = FNV1_32_INIT;
	for (i = 0; i < len; i++) {
		h = fnv_hash(*s++, h);
	}

	return h;
}

/* Hash a 0 terminated string. */

static INLINE u_int32_t fnv_hash_str(u_char *s)
{
	u_int32_t h;

	h = FNV1_32_INIT;
	while (*s) {
		h = fnv_hash(*s++, h);
	}

	return h;
}

#endif
