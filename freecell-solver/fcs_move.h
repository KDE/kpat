/*
 * fcs_move.h - header file for the move structure and enums of 
 * Freecell Solver. This file is common to the main code and to the 
 * library headers.
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __FCS_MOVE_H
#define __FCS_MOVE_H

#ifdef __cplusplus
extern "C" {
#endif

enum fcs_move_types
{
    FCS_MOVE_TYPE_STACK_TO_STACK,
    FCS_MOVE_TYPE_STACK_TO_FREECELL,
    FCS_MOVE_TYPE_FREECELL_TO_STACK,
    FCS_MOVE_TYPE_FREECELL_TO_FREECELL,
    FCS_MOVE_TYPE_STACK_TO_FOUNDATION,
    FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION,
    FCS_MOVE_TYPE_CANONIZE,
    FCS_MOVE_TYPE_SEPARATOR,
    FCS_MOVE_TYPE_NULL
};

struct fcs_move_struct
{
    /* The index of the foundation, in case there are more than one decks */
    int foundation;
    /* Used in the case of a stack to stack move */
    int num_cards_in_sequence;  
    /* There are two freecells, one for the source and the other
     * for the destination */
    int src_freecell;
    int dest_freecell;
    /* Ditto for the stacks */
    int src_stack;
    int dest_stack;
    /* The type of the move see the enum fcs_move_types */
    int type;
};

typedef struct fcs_move_struct fcs_move_t;

#ifdef __cplusplus
}
#endif

#endif
