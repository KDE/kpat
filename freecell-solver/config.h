/*
    config.h - Configuration file for Freecell Solver

    Written by Shlomi Fish, 2000

    This file is distributed under the public domain.
    (It is not copyrighted).
*/

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* #define DEBUG_STATES */
#define COMPACT_STATES
/* #define INDIRECT_STACK_STATES */

/* #define INDIRECT_STATE_STORAGE */


/* #define DEBUG */

/* #define CARD_DEBUG_PRES */

/* Make sure one and only one of DEBUG_STATES and COMPACT_STATES is defined. 
 * The preferred is COMPACT_STATES because it occupies less memory and is 
 * faster.
 */
#if (!defined(DEBUG_STATES)) && (!defined(COMPACT_STATES)) && (!defined(INDIRECT_STACK_STATES))
#define COMPACT_STATES
#elif defined(COMPACT_STATES) && defined(DEBUG_STATES)
#undef DEBUG_STATES
#endif


/*
    The sort margin size for the previous states array.
*/
#define PREV_STATES_SORT_MARGIN 32
/*
    The amount prev_states grow by each time it each resized.
    Should be greater than 0 and in order for the program to be
    efficient, should be much bigger than 
    PREV_STATES_SORT_MARGIN.
*/
#define PREV_STATES_GROW_BY 128

/*
    The amount the pack pointers array grows by. Shouldn't be too high
    because it doesn't happen too often.
*/
#define IA_STATE_PACKS_GROW_BY 32

/*
 * The maximal number of Freecells. For efficiency's sake it should be a
 * multiple of 4.
 * */
#define MAX_NUM_FREECELLS 8

/*
 * The maximal number of Stacks. For efficiency's sake it should be a 
 * multiple of 4.
 * */
#define MAX_NUM_STACKS 12


/*
 * The maximal number of initial cards that can be found in a stack. The rule
 * of the thumb is that it plus 13 should be a multiple of 4.
 * */
#define MAX_NUM_INITIAL_CARDS_IN_A_STACK 91

#define MAX_NUM_DECKS 2

/* #define FCS_NON_DFS */

    
#define FCS_STATE_STORAGE_INDIRECT 0
#define FCS_STATE_STORAGE_INTERNAL_HASH 1
#define FCS_STATE_STORAGE_LIBAVL_AVL_TREE 2
#define FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE 3
#define FCS_STATE_STORAGE_LIBREDBLACK_TREE 4
#define FCS_STATE_STORAGE_GLIB_TREE 5
#define FCS_STATE_STORAGE_GLIB_HASH 6
#define FCS_STATE_STORAGE_DB_FILE 7

#define FCS_STACK_STORAGE_INTERNAL_HASH 0
#define FCS_STACK_STORAGE_LIBAVL_AVL_TREE 1
#define FCS_STACK_STORAGE_LIBAVL_REDBLACK_TREE 2
#define FCS_STACK_STORAGE_LIBREDBLACK_TREE 3
#define FCS_STACK_STORAGE_GLIB_TREE 4
#define FCS_STACK_STORAGE_GLIB_HASH 5
/* #define FCS_STACK_STORAGE FCS_STACK_STORAGE_INTERNAL_HASH */ 

#ifdef __cplusplus
}
#endif
    
#endif
