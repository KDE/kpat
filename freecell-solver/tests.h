/*
 * fcs.h - header file of the test functions for Freecell Solver.
 *
 * The test functions code is found in freecell.c
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __TESTS_H
#define __TESTS_H

#include "fcs.h"

typedef int (*freecell_solver_solve_for_state_test_t)(
        freecell_solver_instance_t *,
        fcs_state_with_locations_t *,
        int,
        int,
        int,
        int
        );

extern freecell_solver_solve_for_state_test_t freecell_solver_sfs_tests[FCS_TESTS_NUM];

/*
 * This macro determines if child can be placed above parent.
 *
 * instance has to be defined.
 *
 * */
#define fcs_is_parent_card(child, parent) \
    ((fcs_card_card_num(child)+1 == fcs_card_card_num(parent)) && \
    ((instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_RANK) ?   \
        1 :                                                          \
        ((instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_SUIT) ?   \
            (fcs_card_deck(child) == fcs_card_deck(parent)) :     \
            ((fcs_card_deck(child) & 0x1) != (fcs_card_deck(parent)&0x1))   \
        ))                \
    )



#endif /* __TESTS_H */
