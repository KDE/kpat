/* Patsolve can be configured to solve a variety of games.  The default here
is Seahaven (same suit, not red-black-red-black) with any card allowed on
empty work piles.  Define SAME_SUIT=0 for Freecell, and define KING_ONLY=1
to only allow kings on empty work piles (see the Makefile). */

#ifndef CONFIG_H
#define CONFIG_H

#ifndef SAME_SUIT
#define SAME_SUIT 1
#endif
#ifndef KING_ONLY
#define KING_ONLY 0
#endif

#ifndef NWPILES
#define NWPILES 10      /* number of W piles */
#endif
#ifndef NTPILES
#define NTPILES 4       /* number of T cells */
#endif

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#define DEBUG 0

#ifdef WIN32
typedef unsigned char u_char;
typedef unsigned __int64 u_int64_t;
typedef unsigned __int32 u_int32_t;
#endif

#endif
