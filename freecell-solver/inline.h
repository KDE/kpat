/*
 * inline.h - the purpose of this file is to define the GCC_INLINE
 * macro.
 *
 * Written by Shlomi Fish, 2002
 *
 * This file is in the public domain (it's uncopyrighted).
 * */

#ifndef FC_SOLVE__INLINE_H
#define FC_SOLVE__INLINE_H

#if defined(__GNUC__)
#define GCC_INLINE __inline__
#else
#define GCC_INLINE 
#endif


#endif
