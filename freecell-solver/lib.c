/*
 * lib.c - library interface functions of Freecell Solver.
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */
#include <stdlib.h>
#include <string.h>

#include "fcs.h"
#include "preset.h"
#include "fcs_user.h"

struct fcs_user_struct
{
    freecell_solver_instance_t * instance;
    fcs_state_with_locations_t state;
    int ret;
};

typedef struct fcs_user_struct fcs_user_t;

void * freecell_solver_user_alloc(void)
{
    fcs_user_t * ret;

    ret = (fcs_user_t *)malloc(sizeof(fcs_user_t));
    ret->instance = freecell_solver_alloc_instance();
    ret->ret = FCS_STATE_NOT_BEGAN_YET;
    return (void*)ret;
}

int freecell_solver_user_apply_preset(
    void * user_instance,
    const char * preset_name)
{
    fcs_user_t * user;

    user = (fcs_user_t*)user_instance;
    
    return fcs_apply_preset_by_name(
        user->instance,
        preset_name
        );
}

void freecell_solver_user_limit_iterations(
    void * user_instance,
    int max_iters
    )
{
    fcs_user_t * user;

    user = (fcs_user_t*)user_instance;
    
    user->instance->max_num_times = max_iters;
}

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

void freecell_solver_user_set_tests_order(
    void * user_instance,
    const char * tests_order
    )
{
    fcs_user_t * user;
    int a;

    user = (fcs_user_t*)user_instance;
    
    user->instance->tests_order_num = min(strlen(tests_order), FCS_TESTS_NUM);
    for(a=0;a<user->instance->tests_order_num;a++)
    {
        user->instance->tests_order[a] = (tests_order[a]-'0')%FCS_TESTS_NUM;
    }
}

int freecell_solver_user_solve_board(
    void * user_instance,
    const char * state_as_string
    )
{
    fcs_user_t * user;
    int ret;
    fcs_card_t card;

    user = (fcs_user_t*)user_instance;

    user->state = fcs_initial_user_state_to_c(
        state_as_string,
        user->instance->freecells_num, 
        user->instance->stacks_num,
        user->instance->decks_num
        );

    ret = fcs_check_state_validity(
        &user->state, 
        user->instance->freecells_num, 
        user->instance->stacks_num, 
        user->instance->decks_num,
        &card);

    if (ret != 0)
    {
        return FCS_STATE_INVALID_STATE;
    }

    fcs_canonize_state(
        &user->state,
        user->instance->freecells_num,
        user->instance->stacks_num
        );

    freecell_solver_init_instance(user->instance);

    user->ret = freecell_solver_solve_instance(user->instance, &user->state);

    if (user->ret == FCS_STATE_WAS_SOLVED)
    {
        int a;
        
        for(a=0;a<user->instance->num_solution_states;a++)
        {
            free((void*)user->instance->solution_states[a]);
        }
        free((void*)user->instance->solution_states);
        user->instance->solution_states = NULL;

        fcs_move_stack_normalize(
            user->instance->solution_moves,
            &(user->state),
            user->instance->freecells_num,
            user->instance->stacks_num,
            user->instance->decks_num
            );
    }

    return user->ret;
}

int freecell_solver_user_resume_solution(
    void * user_instance
    )
{
    fcs_user_t * user;

    user = (fcs_user_t *)user_instance;

    user->ret = freecell_solver_resume_instance(user->instance);
    if (user->ret == FCS_STATE_WAS_SOLVED)
    {
        int a;
        
        for(a=0;a<user->instance->num_solution_states;a++)
        {
            free((void*)user->instance->solution_states[a]);
        }
        free((void*)user->instance->solution_states);
        user->instance->solution_states = NULL;

        fcs_move_stack_normalize(
            user->instance->solution_moves,
            &(user->state),
            user->instance->freecells_num,
            user->instance->stacks_num,
            user->instance->decks_num
            );
    }

    return user->ret;
}

int freecell_solver_user_get_next_move(
    void * user_instance,
    fcs_move_t * move
    )
{
    fcs_user_t * user;
    
    user = (fcs_user_t*)user_instance;    
    if (user->ret == FCS_STATE_WAS_SOLVED)
    {
        return fcs_move_stack_pop(
            user->instance->solution_moves,
            move
            );
    }
    else
    {
        return 1;
    }
}

void freecell_solver_user_free(
    void * user_instance
    )
{
    fcs_user_t * user;

    user = (fcs_user_t *)user_instance;

    if (user->ret == FCS_STATE_WAS_SOLVED)
    {
        fcs_move_stack_destroy(user->instance->solution_moves);        
    }
    else if (user->ret == FCS_STATE_SUSPEND_PROCESS)
    {
        freecell_solver_unresume_instance(user->instance);
    }

    if (user->ret != FCS_STATE_NOT_BEGAN_YET)
    {
        freecell_solver_finish_instance(user->instance);
    }

    freecell_solver_free_instance(user->instance);

    free(user);
}

int freecell_solver_user_get_current_depth(
    void * user_instance
    )
{
    fcs_user_t * user;
    
    user = (fcs_user_t *)user_instance;
    
    return (user->instance->num_solution_states - 1);
}

void freecell_solver_user_set_solving_method(
    void * user_instance,
    int method
    )
{
    fcs_user_t * user;
    
    user = (fcs_user_t *)user_instance;
    
    user->instance->method = method;
}

int freecell_solver_user_set_game(void * user_instance,
                                  int freecells_num,
                                  int stacks_num,
                                  int decks_num,
                                  int sequences_are_built_by,
                                  int unlimited_sequence_move,
                                  int empty_stacks_fill)
{
    fcs_user_t * user;
    
    user = (fcs_user_t *)user_instance;

    if (freecells_num < 0 || freecells_num > MAX_NUM_FREECELLS)
        return 1;
    if (stacks_num < 1 || stacks_num > MAX_NUM_STACKS)
        return 2;
    if (decks_num < 1 || decks_num > MAX_NUM_DECKS)
        return 3;
    if (sequences_are_built_by < 0 || sequences_are_built_by > 2)
        return 4;
    if (unlimited_sequence_move < 0 || unlimited_sequence_move > 1)
        return 5;
    if (empty_stacks_fill < 0 || empty_stacks_fill > 2)
        return 6;

    user->instance->freecells_num = freecells_num;
    user->instance->stacks_num = stacks_num;
    user->instance->decks_num = decks_num;
    
    user->instance->sequences_are_built_by = sequences_are_built_by;
    user->instance->unlimited_sequence_move = unlimited_sequence_move;
    user->instance->empty_stacks_fill = empty_stacks_fill;
    
    return 0;
}

int freecell_solver_user_get_num_times(void * user_instance)
{
fcs_user_t * user;
    
    user = (fcs_user_t *)user_instance;
    
    return user->instance->num_times;
}

int freecell_solver_user_get_limit_iterations(void * user_instance)
{
fcs_user_t * user;
    
    user = (fcs_user_t *)user_instance;
    
    return user->instance->max_num_times;
}

int freecell_solver_user_get_moves_left(void * user_instance)
{
    fcs_user_t * user;
    
    user = (fcs_user_t *)user_instance;
    if (user->ret == FCS_STATE_WAS_SOLVED)
        return user->instance->solution_moves->num_moves;
    else 
        return 0;
}
