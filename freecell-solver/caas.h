/*
 * caas.h - the various possible implementations of the function
 * freecell_solver_check_and_add_state().
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef FC_SOLVE__CAAS_H
#define FC_SOLVE__CAAS_H

#ifdef __cplusplus
extern "C" {
#endif

/* #define FCS_USE_INLINE */
    
/*
 * check_and_add_state is defined in caas.c.
 *
 * DFS stands for Depth First Search which is the type of scan Freecell
 * Solver uses to solve a given board.
 * */

extern int freecell_solver_check_and_add_state(
    freecell_solver_soft_thread_t * soft_thread,
    fcs_state_with_locations_t * new_state,
    fcs_state_with_locations_t * * existing_state
    );

#ifdef __cplusplus
}
#endif

#endif /* #ifndef FC_SOLVE__CAAS_H */
