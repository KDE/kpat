/*
 * move.h - main header file for the Freecell Solver library.
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */
#ifndef __FCS_USER_H
#define __FCS_USER_H

#include "fcs_enums.h"
#include "fcs_move.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void * freecell_solver_user_alloc(void);

extern int freecell_solver_user_apply_preset(
    void * instance,
    const char * preset_name
    );

extern void freecell_solver_user_limit_iterations(
    void * user_instance,
    int max_iters
    );

extern void freecell_solver_user_set_tests_order(
    void * user_instance,
    const char * tests_order
    );

extern int freecell_solver_user_solve_board(
    void * user_instance,
    const char * state_as_string
    );

extern int freecell_solver_user_resume_solution(
    void * user_instance
    );

extern int freecell_solver_user_get_next_move(
    void * user_instance,
    fcs_move_t * move
    );

extern void freecell_solver_user_free(
    void * user_instance
    );

extern int freecell_solver_user_get_current_depth(
    void * user_instance
    );

extern void freecell_solver_user_set_solving_method(
    void * user_instance,
    int method
    );

extern int freecell_solver_user_get_num_times(
    void * user_instance
    );

extern int freecell_solver_user_get_limit_iterations(
    void * user_instance
    );

extern int freecell_solver_user_get_moves_left(
    void * user_instance
    );

extern int freecell_solver_user_set_game(
    void * user_instance,
    int freecells_num,
    int stacks_num,
    int decks_num,
    int sequences_are_built_by,
    int unlimited_sequence_move,
    int empty_stacks_fill
    );

extern void freecell_solver_user_set_solution_optimization(
    void * user_instance, 
    int optimize
);

#ifdef __cplusplus
}
#endif

#endif /* __FCS_USER_H */
