/*
 * fcs_enums.h - header file for various Freecell Solver Enumertaions. Common
 * to the main program headers and to the library headers.
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __FCS_ENUMS_H
#define __FCS_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

enum FCS_EMPTY_STACKS_FILL_T
{
    FCS_ES_FILLED_BY_ANY_CARD,
    FCS_ES_FILLED_BY_KINGS_ONLY,
    FCS_ES_FILLED_BY_NONE
};

enum FCS_SEQUENCES_ARE_BUILT_BY_T
{
    FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
    FCS_SEQ_BUILT_BY_SUIT,
    FCS_SEQ_BUILT_BY_RANK
};

enum freecell_solver_state_solving_return_codes
{
    FCS_STATE_WAS_SOLVED,
    FCS_STATE_IS_NOT_SOLVEABLE,
    FCS_STATE_ALREADY_EXISTS,
    FCS_STATE_EXCEEDS_MAX_NUM_TIMES,
    FCS_STATE_BEGIN_SUSPEND_PROCESS,
    FCS_STATE_SUSPEND_PROCESS,
    FCS_STATE_EXCEEDS_MAX_DEPTH,
    FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE,
    FCS_STATE_INVALID_STATE,
    FCS_STATE_NOT_BEGAN_YET,
    FCS_STATE_DOES_NOT_EXIST
};


#ifdef __cplusplus
}
#endif

#endif /* __FCS_ENUMS_H */
