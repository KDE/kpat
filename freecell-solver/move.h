/*
 * move.h - header file for the move and move stacks functions of 
 * Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __MOVE_H
#define __MOVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "state.h"
#include "fcs_move.h"

struct fcs_move_stack_struct
{
    fcs_move_t * moves;
    int max_num_moves;
    int num_moves;
};

typedef struct fcs_move_stack_struct fcs_move_stack_t;

fcs_move_stack_t * fcs_move_stack_create(void);
int fcs_move_stack_push(fcs_move_stack_t * stack, fcs_move_t move);
int fcs_move_stack_pop(fcs_move_stack_t * stack, fcs_move_t * move);
void fcs_move_stack_destroy(fcs_move_stack_t * stack);
void fcs_move_stack_swallow_stack(fcs_move_stack_t * stack, fcs_move_stack_t * src_stack);
void fcs_move_stack_reset(fcs_move_stack_t * stack);
int fcs_move_stack_get_num_moves(fcs_move_stack_t * stack);
fcs_move_stack_t * fcs_move_stack_duplicate(fcs_move_stack_t * stack);

void fcs_apply_move(fcs_state_with_locations_t * state_with_locations, fcs_move_t move, int freecells_num, int stacks_num, int decks_num);

#if 0
char * fcs_move_as_string(fcs_move_t move);
#endif

void fcs_move_stack_normalize(
    fcs_move_stack_t * moves,
    fcs_state_with_locations_t * init_state,
    int freecells_num,
    int stacks_num,
    int decks_num
    );

char * fcs_move_to_string(fcs_move_t move);

#ifdef __cplusplus
}
#endif

#endif
