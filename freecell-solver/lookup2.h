/*
Note:
    This code was ripped and modified by Shlomi Fish. The original can
    be found at http://burtleburtle.net/bob/c/lookup2.c.
    This file is in the Public Domain.
*/
#ifndef FC_SOLVE__LOOKUP2_H
#define FC_SOLVE__LOOKUP2_H

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;

ub4 freecell_solver_lookup2_hash_function( 
    register ub1 *k,        /* the key */
    register ub4  length,   /* the length of the key */
    register ub4  initval    /* the previous hash, or an arbitrary value */
        );

#endif /* FC_SOLVE__LOOKUP2_H */
