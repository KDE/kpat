/*
 * freecell.c - The various movement tests performed by Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000-2001
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#if FCS_STATE_STORAGE==FCS_STATE_STORAGE_LIBREDBLACK_TREE
#include <search.h>
#endif


#include "config.h"
#include "state.h"
#include "card.h"
#include "fcs_dm.h"
#include "fcs.h"

#include "fcs_isa.h"
#include "tests.h"

/*
 * The number of cards that can be moved is 
 * (freecells_number + 1) * 2 ^ (free_stacks_number)
 *
 * See the Freecell FAQ and the source code of PySol
 * 
 * */
#define calc_max_sequence_move(fc_num, fs_num)        \
    ((instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD) ? \
        (                                                 \
            (instance->unlimited_sequence_move) ?         \
                INT_MAX :                                 \
                (((fc_num)+1)<<(fs_num))                  \
        ) :                                               \
        ((fc_num)+1)                                      \
    )
            

         

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

/*
 * check_and_add_state is defined in dfs.c.
 *
 * DFS stands for Depth First Search which is the type of scan Freecell 
 * Solver uses to solve a given board.
 * */
extern int freecell_solver_check_and_add_state(
    freecell_solver_instance_t * instance, 
    fcs_state_with_locations_t * new_state, 
    fcs_state_with_locations_t * * existing_state,
    int depth);


/*
 * This macro is used by sfs_check_state_begin() which is in turn
 * used in all the tests functions.
 *
 * What it does is allocate some space for the state that is derived from 
 * the current one. The derived state will be manipulated to create the 
 * board after the test was done. 
 * */
#define sfs_check_state_init() \
    ptr_new_state_with_locations = fcs_state_ia_alloc(instance);

/*
 * This macro is used by _all_ the tests to handle some check_and_add_state()
 * return codes. Notice the "else if" at its beginning.
 * */
#define sfs_check_state_finish() \
else if ((check == FCS_STATE_IS_NOT_SOLVEABLE) && ((instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR)||(instance->method == FCS_METHOD_OPTIMIZE))) \
{  \
    /* In A* or BFS, check_and_add_state() returns STATE_IS_NOT_SOLVEABLE \
     * if the state was just found to be unencountered so far, and was    \
     * placed in the queue or priority queue of the scan.                 \
     *                                                                    \
     * If it was indeed placed in the queue, then the move stack leading  \
     * to it was also recorded inside the fcs_state_with_locations_t      \
     * which encapsulates it. Thus, we need a fresh one here.             \
     */           \
    moves = fcs_move_stack_create();  \
} \
else if ((check == FCS_STATE_ALREADY_EXISTS) || (check == FCS_STATE_OPTIMIZED)) \
{             \
    /* The state already exists so we can release its pointer */    \
    fcs_state_ia_release(instance);          \
    if (check == FCS_STATE_OPTIMIZED)  \
    {               \
        moves = fcs_move_stack_create();  \
    }          \
}           \
else if ((check == FCS_STATE_EXCEEDS_MAX_NUM_TIMES) || \
         (check == FCS_STATE_SUSPEND_PROCESS) || \
         (check == FCS_STATE_BEGIN_SUSPEND_PROCESS) || \
         (check == FCS_STATE_EXCEEDS_MAX_DEPTH)) \
{          \
    if ((check == FCS_STATE_SUSPEND_PROCESS)) \
    {              \
        /* Record the moves so far in a move stack in the stack of  \
         * move stacks that belong to the scan.            \
         * */            \
        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_CANONIZE);      \
        fcs_move_stack_push(moves, temp_move);           \
        instance->proto_solution_moves[depth] = moves;    \
    }          \
    else if ( (check == FCS_STATE_BEGIN_SUSPEND_PROCESS) &&  ((instance->method == FCS_METHOD_BFS)||(instance->method == FCS_METHOD_A_STAR)||(instance->method == FCS_METHOD_OPTIMIZE)) )    \
    {             \
        /* Do nothing because those methods expect us to have a move stack present in every state */  \
    }              \
    else           \
    {             \
        fcs_move_stack_destroy(moves);    \
    }         \
    return check;   \
}    


/*
 * A macro that manages the Soft-DFS stuff. It is placed inside
 * sfs_check_state_end() or in move_top_stack_cards_to_founds or
 * move_freecell_cards_to_founds.
 * */
#define sfs_check_state_handle_soft_dfs() \
if (instance->method == FCS_METHOD_SOFT_DFS)         \
{                                                    \
    /* Record the state  */                         \
    instance->soft_dfs_states_to_check[depth][       \
        instance->soft_dfs_num_states_to_check[depth]     \
        ] = ptr_new_state_with_locations;            \
    /* Record its move stack */                        \
    instance->soft_dfs_states_to_check_move_stacks[depth][  \
        instance->soft_dfs_num_states_to_check[depth]    \
        ] = moves;                                        \
    /* Increment the number of recorded states */            \
    instance->soft_dfs_num_states_to_check[depth]++;         \
    /* Create a fresh move stack because the current one was   \
     * recorded and must not be changed at this point */       \
    moves = fcs_move_stack_create();                        \
    /* We are going to exceed the maximum number of recorderd states  \
     * so we should resize it */    \
    if (instance->soft_dfs_num_states_to_check[depth] ==     \
        instance->soft_dfs_max_num_states_to_check[depth])   \
    {                                  \
        instance->soft_dfs_max_num_states_to_check[depth] += FCS_SOFT_DFS_STATES_TO_CHECK_GROW_BY;    \
        instance->soft_dfs_states_to_check[depth] = realloc(     \
            instance->soft_dfs_states_to_check[depth],           \
            sizeof(instance->soft_dfs_states_to_check[depth][0]) *    \
                instance->soft_dfs_max_num_states_to_check[depth]      \
            );          \
        instance->soft_dfs_states_to_check_move_stacks[depth] = realloc(     \
            instance->soft_dfs_states_to_check_move_stacks[depth],           \
            sizeof(instance->soft_dfs_states_to_check_move_stacks[depth][0]) *    \
                instance->soft_dfs_max_num_states_to_check[depth]      \
            );          \
    }        \
    \
}


/*
 * sfs_check_state_begin() is now used by all the tests.
 *
 * */
#define sfs_check_state_begin() \
sfs_check_state_init() \
/* Initialize the derived state from the source state */    \
fcs_duplicate_state(new_state_with_locations, state_with_locations); \
ptr_new_state_with_locations->moves_to_parent = NULL;         \
if ( (instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR) || (instance->method == FCS_METHOD_OPTIMIZE))     \
{       \
    /*  Some A* and BFS parameters that need to be initialized in the derived \
     *  state.               \
     * */        \
    ptr_new_state_with_locations->parent = ptr_state_with_locations;    \
    ptr_new_state_with_locations->moves_to_parent = moves; \
    ptr_new_state_with_locations->depth = depth+1; \
} \
ptr_new_state_with_locations->visited = 0; 



/*
 * This macro is not used by move_top_stack_cards_to_founds and
 * move_freecell_cards_to_founds because of their deducing of 
 * dead-end operations.
 * */
#define sfs_check_state_end()                        \
/* The last move in a move stack should be FCS_MOVE_TYPE_CANONIZE \
 * because it indicates that the order of the stacks and freecells \
 * need to be recalculated \ 
 * */ \
fcs_move_set_type(temp_move,FCS_MOVE_TYPE_CANONIZE); \
fcs_move_stack_push(moves, temp_move);               \
    \
{                                                    \
    fcs_state_with_locations_t * existing_state;     \
    check = freecell_solver_check_and_add_state(     \
        instance,                                    \
        ptr_new_state_with_locations,                \
        &existing_state,                             \
        depth);                                      \
    /* In Soft-DFS I'd like to save memory by pointing each state  \
     * to its cached state */   \
    if ((instance->method == FCS_METHOD_SOFT_DFS) && \
        (check == FCS_STATE_ALREADY_EXISTS))         \
    {                                                    \
        ptr_new_state_with_locations = existing_state;    \
    }                                                    \
}                                                    \
sfs_check_state_handle_soft_dfs()                    \
                                                     \
                                                     \
/* This is for Hard-DFS: we put all the intermediate \
 * move stacks in proto_solution_moves */            \
                                                     \
if (check == FCS_STATE_WAS_SOLVED)                   \
{                                                    \
    instance->proto_solution_moves[depth] = moves;   \
    return FCS_STATE_WAS_SOLVED;                     \
}                                                    \
sfs_check_state_finish();


/*
 *  Those are some macros to make it easier for the programmer.
 * */
#define state_with_locations (*ptr_state_with_locations)
#define state (ptr_state_with_locations->s)
#define new_state_with_locations (*ptr_new_state_with_locations)
#define new_state (ptr_new_state_with_locations->s)


/*
    This macro checks if the top card in the stack is a flipped card
    , and if so flips it so its face is up.
  */
#define fcs_flip_top_card(stack) \
                    {            \
                        int cards_num;         \
                        cards_num = fcs_stack_len(new_state,stack);    \
                                                                \
                        if (cards_num > 0)              \
                        {                \
                            if (fcs_card_get_flipped(fcs_stack_card(new_state,stack,cards_num-1)) == 1)        \
                            {               \
                                fcs_flip_stack_card(new_state,stack,cards_num-1);            \
                                fcs_move_set_type(temp_move, FCS_MOVE_TYPE_FLIP_CARD);        \
                                fcs_move_set_src_stack(temp_move, stack);           \
                                    \
                                fcs_move_stack_push(moves, temp_move);              \
                            }          \
                        }          \
                    }          \


/*
 * This function tries to move stack cards that are present at the
 * top of stacks to the foundations.
 * */
int freecell_solver_sfs_move_top_stack_cards_to_founds(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int stack;
    int cards_num;
    int deck;
    int d;
    fcs_card_t card;
    fcs_card_t temp_card;
    int check;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;
    fcs_state_with_locations_t * existing_state;

    moves = fcs_move_stack_create();

    for(stack=0;stack<instance->stacks_num;stack++)
    {
        cards_num = fcs_stack_len(state, stack);
        if (cards_num)
        {
            /* Get the top card in the stack */
            card = fcs_stack_card(state,stack,cards_num-1);
            for(deck=0;deck<instance->decks_num;deck++)
            {
                if (fcs_foundation_value(state, deck*4+fcs_card_suit(card)) == fcs_card_card_num(card) - 1)
                {
                    /* We can put it there */

                    sfs_check_state_begin()

                    fcs_pop_stack_card(new_state, stack, temp_card);

                    fcs_increment_foundation(new_state, deck*4+fcs_card_suit(card));

                    fcs_move_stack_reset(moves);

                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FOUNDATION);
                    fcs_move_set_src_stack(temp_move,stack);
                    fcs_move_set_foundation(temp_move,deck*4+fcs_card_suit(card));

                    fcs_move_stack_push(moves, temp_move);

                    fcs_flip_top_card(stack);

                    /* The last move needs to be FCS_MOVE_TYPE_CANONIZE
                     * because it indicates that the internal order of the 
                     * stacks
                     * and freecells may have changed. */
                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_CANONIZE);
                    fcs_move_stack_push(moves, temp_move);

                    check = freecell_solver_check_and_add_state(
                        instance, 
                        &new_state_with_locations,
                        &existing_state,
                        depth);

                    if ((instance->method == FCS_METHOD_SOFT_DFS) &&
                        (check == FCS_STATE_ALREADY_EXISTS))
                    {
                        ptr_new_state_with_locations = existing_state;
                    }
                    
                    sfs_check_state_handle_soft_dfs()

                    if (check == FCS_STATE_WAS_SOLVED)
                    {
                        instance->proto_solution_moves[depth] = moves;

                        return FCS_STATE_WAS_SOLVED;
                    }
                    else if (
                        ((check == FCS_STATE_IS_NOT_SOLVEABLE) || 
                        (check == FCS_STATE_ALREADY_EXISTS)) &&
                        (!ignore_osins)
                        )
                    {
                        if (check == FCS_STATE_ALREADY_EXISTS)
                        {
                            fcs_state_ia_release(instance);
                        }
                        /*  If the decks of the other colour are higher than the card number  *
                         *  of this card + 2, it means that if th sub-state is not solveable, *
                         *  then this state is not solveable either.                          */

                        if (instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_SUIT)
                        {
                            fcs_move_stack_destroy(moves);
                            return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                        }
                        
                        if (instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_RANK)
                        {
                            for(d=0;d<instance->decks_num*4;d++)
                            {
                                if (fcs_card_card_num(card) - 2 > fcs_foundation_value(state, d))
                                {
                                    break;
                                }
                            }

                            if (d == instance->decks_num*4)
                            {
                                fcs_move_stack_destroy(moves);
                                return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                            }
                        }
                        else
                        {
                            /* instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_ALTERNATE_COLOR */
                            for(d=0;d<instance->decks_num;d++)
                            {
                                if (!((fcs_card_card_num(card) - 2 <= fcs_foundation_value(state, d*4+(fcs_card_suit(card)^0x1))) &&
                                    (fcs_card_card_num(card) - 2 <= fcs_foundation_value(state, d*4+(fcs_card_suit(card)^0x3)))))
                                {
                                    break;
                                }
                            }
                        
                            if (d == instance->decks_num)
                            {
                                fcs_move_stack_destroy(moves);
                                return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                            }
                        }
                    }
                    sfs_check_state_finish();                    
                    break;
                }
            }
        }
    }

    fcs_move_stack_destroy(moves);
    
    return FCS_STATE_IS_NOT_SOLVEABLE;
}


/*
 * This test moves single cards that are present in the freecells to 
 * the foundations.
 * */
int freecell_solver_sfs_move_freecell_cards_to_founds(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int fc;
    int deck;
    int d;
    fcs_card_t card;
    int check;
    fcs_move_stack_t * moves;
    fcs_move_t temp_move;
    fcs_state_with_locations_t * existing_state;

    moves = fcs_move_stack_create();

    
    /* Now check the same for the free cells */
    for(fc=0;fc<instance->freecells_num;fc++)
    {
        card = fcs_freecell_card(state, fc);
        if (fcs_card_card_num(card) != 0)
        {
            for(deck=0;deck<instance->decks_num;deck++)
            {
                if (fcs_foundation_value(state, deck*4+fcs_card_suit(card)) == fcs_card_card_num(card) - 1)
                {
                    /* We can put it there */
                    sfs_check_state_begin()
                        
                    fcs_empty_freecell(new_state, fc);

                    fcs_increment_foundation(new_state, deck*4+fcs_card_suit(card));

                    fcs_move_stack_reset(moves);
                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION);
                    fcs_move_set_src_freecell(temp_move,fc);
                    fcs_move_set_foundation(temp_move,deck*4+fcs_card_suit(card));

                    fcs_move_stack_push(moves, temp_move);
                    
                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_CANONIZE);
                    fcs_move_stack_push(moves, temp_move);                    

                    check = freecell_solver_check_and_add_state(
                            instance, 
                            ptr_new_state_with_locations,
                            &existing_state,
                            depth);
                    if ((instance->method == FCS_METHOD_SOFT_DFS) &&
                        (check == FCS_STATE_ALREADY_EXISTS))
                    {
                        ptr_new_state_with_locations = existing_state;
                    }
                    
                    sfs_check_state_handle_soft_dfs()

                    if (check == FCS_STATE_WAS_SOLVED)
                    {
                        instance->proto_solution_moves[depth] = moves;

                        return FCS_STATE_WAS_SOLVED;
                    }
                    else if (
                        ((check == FCS_STATE_IS_NOT_SOLVEABLE) || 
                        (check == FCS_STATE_ALREADY_EXISTS)) &&
                        (!ignore_osins)
                        )
                    {
                        if (check == FCS_STATE_ALREADY_EXISTS)
                        {
                            fcs_state_ia_release(instance);
                        }
                        /*  If the decks of the other colour are higher than the card number  *
                         *  of this card + 2, it means that if th sub-state is not solveable, *
                         *  then this state is not solveable either.                          */

                        if (instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_SUIT)
                        {
                            fcs_move_stack_destroy(moves);
                            return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                        }
                        
                        /*  If the decks of the other colour are higher than the card number  *
                         *  of this card + 2, it means that if the sub-state is not solveable, *
                         *  then this state is not solveable either.                          */

                        if (instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_RANK)
                        {
                            for(d=0;d<instance->decks_num*4;d++)
                            {
                                if (fcs_card_card_num(card) - 2 > fcs_foundation_value(state, d))
                                {
                                    break;
                                }
                            }

                            if (d == instance->decks_num*4)
                            {
                                fcs_move_stack_destroy(moves);
                                return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                            }
                        }
                        else
                        {
                            /* instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_ALTERNATE_COLOR */
                            for(d=0;d<instance->decks_num;d++)
                            {
                                if (!((fcs_card_card_num(card) - 2 <= fcs_foundation_value(state, d*4+(fcs_card_suit(card)^0x1))) &&
                                    (fcs_card_card_num(card) - 2 <= fcs_foundation_value(state, d*4+(fcs_card_suit(card)^0x3)))))
                                {
                                    break;
                                }
                            }
                        
                            if (d == instance->decks_num)
                            {
                                fcs_move_stack_destroy(moves);
                                return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                            }
                        }
                    }
                    sfs_check_state_finish()
                    break;
                }
            }
        }
    }
    
    fcs_move_stack_destroy(moves);

    return FCS_STATE_IS_NOT_SOLVEABLE;
}

int freecell_solver_sfs_move_freecell_cards_on_top_of_stacks(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int dest_cards_num;
    int ds, fc, dc;
    fcs_card_t dest_card, src_card, temp_card, dest_below_card;
    int check;
    fcs_move_stack_t * moves;
    fcs_move_t temp_move;
    int is_seq_in_dest;
    int num_cards_to_relocate;
    int freecells_to_fill, freestacks_to_fill;
    int a,b;

    moves = fcs_move_stack_create();

    /* Let's try to put cards in the freecells on top of stacks */

    /* ds stands for destination stack */
    for(ds=0;ds<instance->stacks_num;ds++)
    {
        dest_cards_num = fcs_stack_len(state, ds);

        /* If the stack is not empty we can proceed */
        if (dest_cards_num > 0)
        {
            /*
             * Let's search for a suitable card in the stack
             * */
            for(dc=dest_cards_num-1;dc>=0;dc--)
            {
                dest_card = fcs_stack_card(state, ds, dc);

                /* Scan the freecells */
                for(fc=0;fc<instance->freecells_num;fc++)
                {
                    src_card = fcs_freecell_card(state, fc);
    
                    /* If the freecell is not empty and dest_card is its parent
                     * */
                    if ( (fcs_card_card_num(src_card) != 0) && 
                         fcs_is_parent_card(src_card,dest_card)     )
                    {
                        /* Let's check if we can put it there */

                        /* Check if the destination card is already below a
                         * suitable card */
                        is_seq_in_dest = 0;
                        if (dest_cards_num - 1 > dc)
                        {
                            dest_below_card = fcs_stack_card(state, ds, dc+1);
                            if (fcs_is_parent_card(dest_below_card, dest_card))
                            {
                                is_seq_in_dest = 1;
                            }
                        }


                        if (! is_seq_in_dest)
                        {
                            num_cards_to_relocate = dest_cards_num - dc - 1;

                            freecells_to_fill = min(num_cards_to_relocate, num_freecells);

                            num_cards_to_relocate -= freecells_to_fill;

                            if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD)
                            {
                                freestacks_to_fill = min(num_cards_to_relocate, num_freestacks);

                                num_cards_to_relocate -= freestacks_to_fill;
                            }
                            else
                            {
                                freestacks_to_fill = 0;
                            }

                            if (num_cards_to_relocate == 0)
                            {
                                /* We can move it */

                                sfs_check_state_begin()

                                fcs_move_stack_reset(moves);

                                /* Fill the freecells with the top cards */

                                for(a=0 ; a<freecells_to_fill ; a++)
                                {
                                    /* Find a vacant freecell */
                                    for(b=0;b<instance->freecells_num;b++)
                                    {
                                        if (fcs_freecell_card_num(new_state, b) == 0)
                                        {
                                            break;
                                        }
                                    }
                                    fcs_pop_stack_card(new_state, ds, temp_card);

                                    fcs_put_card_in_freecell(new_state, b, temp_card);

                                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                    fcs_move_set_src_stack(temp_move,ds);
                                    fcs_move_set_dest_freecell(temp_move,b);
                                    fcs_move_stack_push(moves, temp_move);
                                }

                                /* Fill the free stacks with the cards below them */
                                for(a=0; a < freestacks_to_fill ; a++)
                                {
                                    /* Find a vacant stack */
                                    for(b=0;b<instance->stacks_num;b++)
                                    {
                                        if (fcs_stack_len(new_state, b) == 0)
                                        {
                                            break;
                                        }
                                    }

                                    fcs_pop_stack_card(new_state, ds, temp_card);
                                    fcs_push_card_into_stack(new_state, b, temp_card);
                                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                    fcs_move_set_src_stack(temp_move,ds);
                                    fcs_move_set_dest_stack(temp_move,b);
                                    fcs_move_set_num_cards_in_seq(temp_move,1);
                                    fcs_move_stack_push(moves, temp_move);
                                }

                                /* Now put the freecell card on top of the stack */
                                fcs_push_card_into_stack(new_state, ds, src_card);
                                fcs_empty_freecell(new_state, fc);
                                fcs_move_set_type(temp_move,FCS_MOVE_TYPE_FREECELL_TO_STACK);
                                fcs_move_set_src_freecell(temp_move,fc);
                                fcs_move_set_dest_stack(temp_move,ds);
                                fcs_move_stack_push(moves, temp_move);

                                sfs_check_state_end()
                            }
                        }
                    }
                }
            }
        }
    }

    fcs_move_stack_destroy(moves);    

    return FCS_STATE_IS_NOT_SOLVEABLE;
}



int freecell_solver_sfs_move_non_top_stack_cards_to_founds(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    int stack;
    int cards_num;
    int c, a, b;
    fcs_card_t temp_card, card;
    int deck;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    moves = fcs_move_stack_create();
    

    /* Now let's check if a card that is under some other cards can be placed 
     * in the foundations. */

    for(stack=0;stack<instance->stacks_num;stack++)
    {
        cards_num = fcs_stack_len(state, stack);
        /*
         * We starts from cards_num-2 because the top card is already covered
         * by move_top_stack_cards_to_founds.
         * */
        for(c=cards_num-2 ; c >= 0 ; c--)
        {
            card = fcs_stack_card(state, stack, c);
            for(deck=0;deck<instance->decks_num;deck++)
            {
                if (fcs_foundation_value(state, deck*4+fcs_card_suit(card)) == fcs_card_card_num(card)-1)
                {
                    /* The card is foundation-able. Now let's check if we 
                     * can move the cards above it to the freecells and 
                     * stacks */

                    if ((num_freecells + 
                        ((instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD) ? 
                            num_freestacks : 
                            0
                        )) 
                            >= cards_num-(c+1))
                    {
                        /* We can move it */
                        
                        sfs_check_state_begin()

                        fcs_move_stack_reset(moves);
                        
                        /* Fill the freecells with the top cards */
                        for(a=0 ; a<min(num_freecells, cards_num-(c+1)) ; a++)
                        {
                            /* Find a vacant freecell */
                            for(b=0; b<instance->freecells_num; b++)
                            {
                                if (fcs_freecell_card_num(new_state, b) == 0)
                                {
                                    break;
                                }
                            }
                            fcs_pop_stack_card(new_state, stack, temp_card);

                            fcs_put_card_in_freecell(new_state, b, temp_card);

                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                            fcs_move_set_src_stack(temp_move,stack);
                            fcs_move_set_dest_freecell(temp_move,b);

                            fcs_move_stack_push(moves, temp_move);

                            fcs_flip_top_card(stack);
                        }

                        /* Fill the free stacks with the cards below them */
                        for(a=0; a < cards_num-(c+1) - min(num_freecells, cards_num-(c+1)) ; a++)
                        {
                            /* Find a vacant stack */
                            for(b=0;b<instance->stacks_num;b++)
                            {
                                if (fcs_stack_len(new_state, b) == 0)
                                {
                                    break;
                                }
                            }

                            fcs_pop_stack_card(new_state, stack, temp_card);
                            fcs_push_card_into_stack(new_state, b, temp_card);

                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                            fcs_move_set_src_stack(temp_move,stack);
                            fcs_move_set_dest_stack(temp_move,b);
                            fcs_move_set_num_cards_in_seq(temp_move,1);

                            fcs_move_stack_push(moves, temp_move);

                            fcs_flip_top_card(stack);
                        }

                        fcs_pop_stack_card(new_state, stack, temp_card);
                        fcs_increment_foundation(new_state, deck*4+fcs_card_suit(temp_card));

                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FOUNDATION);
                        fcs_move_set_src_stack(temp_move,stack);
                        fcs_move_set_foundation(temp_move,deck*4+fcs_card_suit(temp_card));

                        fcs_move_stack_push(moves, temp_move);

                        fcs_flip_top_card(stack);
                        
                        sfs_check_state_end()
                    }
                    break;
                }
            }
        }
    }

    fcs_move_stack_destroy(moves);

    return FCS_STATE_IS_NOT_SOLVEABLE;
}




int freecell_solver_sfs_move_stack_cards_to_a_parent_on_the_same_stack(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    int stack, c, cards_num, a, dc,b;
    int is_seq_in_dest;
    fcs_card_t card, temp_card, prev_card;
    fcs_card_t dest_below_card, dest_card;
    int freecells_to_fill, freestacks_to_fill;
    int dest_cards_num, num_cards_to_relocate;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    moves = fcs_move_stack_create();


    /*
     * Now let's try to move a stack card to a parent card which is found
     * on the same stack.
     * */
    for (stack=0;stack<instance->stacks_num;stack++)
    {
        cards_num = fcs_stack_len(state, stack);

        for (c=0 ; c<cards_num ; c++)
        {   
            /* Find a card which this card can be put on; */

            card = fcs_stack_card(state, stack, c);


            /* Do not move cards that are already found above a suitable parent */
            a = 1;
            if (c != 0)
            {
                prev_card = fcs_stack_card(state, stack, c-1);
                if ((fcs_card_card_num(prev_card) == fcs_card_card_num(card)+1) &&
                    ((fcs_card_suit(prev_card) & 0x1) != (fcs_card_suit(card) & 0x1)))
                {
                   a = 0;  
                }
            }
            if (a)
            {
#define ds stack
                /* Check if it can be moved to something on the same stack */
                dest_cards_num = fcs_stack_len(state, ds);
                for(dc=0;dc<c-1;dc++)
                {
                    dest_card = fcs_stack_card(state, ds, dc);
                    if (fcs_is_parent_card(card, dest_card))
                    {
                        /* Corresponding cards - see if it is feasible to move
                           the source to the destination. */

                        is_seq_in_dest = 0;
                        dest_below_card = fcs_stack_card(state, ds, dc+1);
                        if (fcs_is_parent_card(dest_below_card, dest_card))
                        {
                            is_seq_in_dest = 1;
                        }

                        if (!is_seq_in_dest)
                        {
                            num_cards_to_relocate = dest_cards_num - dc - 1;

                            freecells_to_fill = min(num_cards_to_relocate, num_freecells);

                            num_cards_to_relocate -= freecells_to_fill;
                            
                            if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD)
                            {
                                freestacks_to_fill = min(num_cards_to_relocate, num_freestacks);

                                num_cards_to_relocate -= freestacks_to_fill;
                            }
                            else
                            {
                                freestacks_to_fill = 0;
                            }

                            if (num_cards_to_relocate == 0)
                            {
                                /* We can move it */
                                
                                sfs_check_state_begin()

                                fcs_move_stack_reset(moves);
                                
                                {
                                    int i_card_pos;
                                    fcs_card_t moved_card;
                                    int source_type, source_index;
                                    
                                    i_card_pos = fcs_stack_len(new_state,stack)-1;
                                    a = 0;
                                    while(i_card_pos>c)
                                    {
                                        if (a < freecells_to_fill)
                                        {
                                            for(b=0;b<instance->freecells_num;b++)
                                            {
                                                if (fcs_freecell_card_num(new_state, b) == 0)
                                                {
                                                    break;
                                                }
                                            }
                                            fcs_pop_stack_card(new_state, ds, temp_card);
                                            fcs_put_card_in_freecell(new_state, b, temp_card);

                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                            fcs_move_set_src_stack(temp_move,ds);
                                            fcs_move_set_dest_freecell(temp_move,b);

                                            fcs_move_stack_push(moves, temp_move);

                                        }
                                        else
                                        {

                                            /*  Find a vacant stack */
                                            for(b=0;b<instance->stacks_num;b++)
                                            {
                                                if (fcs_stack_len(new_state, b) == 0)
                                                {
                                                    break;
                                                }
                                            }

                                            fcs_pop_stack_card(new_state, ds, temp_card);
                                            fcs_push_card_into_stack(new_state, b, temp_card);

                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                            fcs_move_set_src_stack(temp_move,ds);
                                            fcs_move_set_dest_stack(temp_move,b);
                                            fcs_move_set_num_cards_in_seq(temp_move,1);

                                            fcs_move_stack_push(moves, temp_move);
                                            
                                        }
                                        a++;

                                        i_card_pos--;
                                    }
                                    fcs_pop_stack_card(new_state, ds, moved_card);
                                    if (a < freecells_to_fill)
                                    {
                                        for(b=0;b<instance->freecells_num;b++)
                                        {
                                            if (fcs_freecell_card_num(new_state, b) == 0)
                                            {
                                                break;
                                            }
                                        }
                                        fcs_put_card_in_freecell(new_state, b, moved_card);
                                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                        fcs_move_set_src_stack(temp_move,ds);
                                        fcs_move_set_dest_freecell(temp_move,b);
                                        fcs_move_stack_push(moves, temp_move);

                                        source_type = 0;
                                        source_index = b;
                                    }
                                    else
                                    {
                                        for(b=0;b<instance->stacks_num;b++)
                                        {
                                            if (fcs_stack_len(new_state, b) == 0)
                                            {
                                                break;
                                            }
                                        }
                                        fcs_push_card_into_stack(new_state, b, moved_card);
                                        
                                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                        fcs_move_set_src_stack(temp_move,ds);
                                        fcs_move_set_dest_stack(temp_move,b);
                                        fcs_move_set_num_cards_in_seq(temp_move,1);
                                        fcs_move_stack_push(moves, temp_move);

                                        source_type = 1;
                                        source_index = b;
                                    }
                                    i_card_pos--;
                                    a++;
                                    
                                    while(i_card_pos>dc)
                                    {
                                        if (a < freecells_to_fill)
                                        {
                                            for(b=0;b<instance->freecells_num;b++)
                                            {
                                                if (fcs_freecell_card_num(new_state, b) == 0)
                                                {
                                                    break;
                                                }
                                            }
                                            fcs_pop_stack_card(new_state, ds, temp_card);
                                            fcs_put_card_in_freecell(new_state, b, temp_card);

                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                            fcs_move_set_src_stack(temp_move,ds);
                                            fcs_move_set_dest_freecell(temp_move,b);
                                            
                                            fcs_move_stack_push(moves, temp_move);
                                        }
                                        else
                                        {

                                            /*  Find a vacant stack */
                                            for(b=0;b<instance->stacks_num;b++)
                                            {
                                                if (fcs_stack_len(new_state, b) == 0)
                                                {
                                                    break;
                                                }
                                            }

                                            fcs_pop_stack_card(new_state, ds, temp_card);
                                            fcs_push_card_into_stack(new_state, b, temp_card);

                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                            fcs_move_set_src_stack(temp_move,ds);
                                            fcs_move_set_dest_stack(temp_move,b);
                                            fcs_move_set_num_cards_in_seq(temp_move,1);
                                            
                                            fcs_move_stack_push(moves, temp_move);
                                            
                                        }
                                        a++;

                                        i_card_pos--;
                                    }

                                    if (source_type == 0)
                                    {
                                        moved_card = fcs_freecell_card(new_state, source_index);
                                        fcs_empty_freecell(new_state, source_index);

                                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_FREECELL_TO_STACK);
                                        fcs_move_set_src_freecell(temp_move,source_index);
                                        fcs_move_set_dest_stack(temp_move,ds);
                                        fcs_move_stack_push(moves, temp_move);
                                    }
                                    else
                                    {
                                        fcs_pop_stack_card(new_state, source_index, moved_card);
                                        
                                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                        fcs_move_set_src_stack(temp_move,source_index);
                                        fcs_move_set_dest_stack(temp_move,ds);
                                        fcs_move_set_num_cards_in_seq(temp_move,1);
                                        fcs_move_stack_push(moves, temp_move);
                                    }

                                    fcs_push_card_into_stack(new_state, ds, moved_card);
                                }

                                sfs_check_state_end()
                            } 
                        }
                    }
                }
            }
        }
    }

    fcs_move_stack_destroy(moves);
    
    return FCS_STATE_IS_NOT_SOLVEABLE;
}
#undef ds


int freecell_solver_sfs_move_stack_cards_to_different_stacks(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    int stack, c, cards_num, a, dc, ds,b;
    int is_seq_in_dest;
    fcs_card_t card, temp_card, this_card, prev_card;
    fcs_card_t dest_below_card, dest_card;
    int freecells_to_fill, freestacks_to_fill;
    int dest_cards_num, num_cards_to_relocate;
    int seq_end;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    moves = fcs_move_stack_create();


    /* Now let's try to move a card from one stack to the other     *
     * Note that it does not involve moving cards lower than king   *
     * to empty stacks                                              */
        
    for (stack=0;stack<instance->stacks_num;stack++)
    {
        cards_num = fcs_stack_len(state, stack);

        for (c=0 ; c<cards_num ; c=seq_end+1)
        {   
            /* Check if there is a sequence here. */
            for(seq_end=c ; seq_end<cards_num-1 ; seq_end++)
            {
                this_card = fcs_stack_card(state, stack, seq_end+1);
                prev_card = fcs_stack_card(state, stack, seq_end);

                if (fcs_is_parent_card(this_card,prev_card))
                {
                }
                else
                {
                    break;
                }
            }

            /* Find a card which this card can be put on; */

            card = fcs_stack_card(state, stack, c);

            /* Make sure the card is not flipped or else we can't move it */
            if (fcs_card_get_flipped(card) == 0)
            {
                for(ds=0 ; ds<instance->stacks_num; ds++)
                {
                    if (ds != stack)
                    {
                        dest_cards_num = fcs_stack_len(state, ds);
                        for(dc=0;dc<dest_cards_num;dc++)
                        {
                            dest_card = fcs_stack_card(state, ds, dc);
                            
                            if (fcs_is_parent_card(card, dest_card))
                            {
                                /* Corresponding cards - see if it is feasible to move
                                   the source to the destination. */

                                is_seq_in_dest = 0;
                                if (dest_cards_num - 1 > dc)
                                {
                                    dest_below_card = fcs_stack_card(state, ds, dc+1);
                                    if (fcs_is_parent_card(dest_below_card, dest_card))
                                    {
                                        is_seq_in_dest = 1;
                                    }
                                }

                                if (! is_seq_in_dest)
                                {
                                    num_cards_to_relocate = dest_cards_num - dc - 1 + cards_num - seq_end - 1;

                                    freecells_to_fill = min(num_cards_to_relocate, num_freecells);

                                    num_cards_to_relocate -= freecells_to_fill;
                                      
                                    if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD)
                                    {
                                        freestacks_to_fill = min(num_cards_to_relocate, num_freestacks);

                                        num_cards_to_relocate -= freestacks_to_fill;
                                    }
                                    else
                                    {
                                        freestacks_to_fill = 0;
                                    }

                                    if ((num_cards_to_relocate == 0) &&
                                       (calc_max_sequence_move(num_freecells-freecells_to_fill, num_freestacks-freestacks_to_fill) >=
                                        seq_end - c + 1))
                                    {
                                        /* We can move it */
                                        int from_which_stack;
                                        
                                        sfs_check_state_begin()

                                        fcs_move_stack_reset(moves);
                                        
                                        /* Fill the freecells with the top cards */
                                        
                                        for(a=0 ; a<freecells_to_fill ; a++)
                                        {
                                            /* Find a vacant freecell */
                                            for(b=0;b<instance->freecells_num;b++)
                                            {
                                                if (fcs_freecell_card_num(new_state, b) == 0)
                                                {
                                                    break;
                                                }
                                            }

                                            if (fcs_stack_len(new_state, ds) == dc+1)
                                            {
                                                from_which_stack = stack;
                                            }
                                            else
                                            {
                                                from_which_stack = ds;
                                            }
                                            fcs_pop_stack_card(new_state, from_which_stack, temp_card);

                                            fcs_put_card_in_freecell(new_state, b, temp_card);

                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                            fcs_move_set_src_stack(temp_move,from_which_stack);
                                            fcs_move_set_dest_freecell(temp_move,b);
                                            fcs_move_stack_push(moves, temp_move);

                                            fcs_flip_top_card(from_which_stack);
                                        }

                                        /* Fill the free stacks with the cards below them */
                                        for(a=0; a < freestacks_to_fill ; a++)
                                        {
                                            /*  Find a vacant stack */
                                            for(b=0;b<instance->stacks_num;b++)
                                            {
                                                if (fcs_stack_len(new_state, b) == 0)
                                                {
                                                    break;
                                                }
                                            }

                                            if (fcs_stack_len(new_state, ds) == dc+1)
                                            {
                                                from_which_stack = stack;
                                            }
                                            else
                                            {
                                                from_which_stack = ds;
                                            }

                                            
                                            fcs_pop_stack_card(new_state, from_which_stack, temp_card);
                                            fcs_push_card_into_stack(new_state, b, temp_card);
                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                            fcs_move_set_src_stack(temp_move,from_which_stack);
                                            fcs_move_set_dest_stack(temp_move,b);
                                            fcs_move_set_num_cards_in_seq(temp_move,1);
                                            fcs_move_stack_push(moves, temp_move);

                                            fcs_flip_top_card(from_which_stack);
                                        }

                                        for(a=c ; a <= seq_end ; a++)
                                        {
                                            fcs_push_stack_card_into_stack(new_state, ds, stack, a);
                                        }

                                        for(a=0; a < seq_end-c+1 ;a++)
                                        {
                                            fcs_pop_stack_card(new_state, stack, temp_card);
                                        }
                                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                        fcs_move_set_src_stack(temp_move,stack);
                                        fcs_move_set_dest_stack(temp_move,ds);
                                        fcs_move_set_num_cards_in_seq(temp_move,seq_end-c+1);
                                        fcs_move_stack_push(moves, temp_move);

                                        fcs_flip_top_card(stack);
                                        
                                        sfs_check_state_end()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    fcs_move_stack_destroy(moves);    

    return FCS_STATE_IS_NOT_SOLVEABLE;
}



int freecell_solver_sfs_move_sequences_to_free_stacks(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    int stack, cards_num, c, ds, a, b, seq_end;
    fcs_card_t this_card, prev_card, temp_card;
    int max_sequence_len;
    int num_cards_to_relocate, freecells_to_fill, freestacks_to_fill;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;
    
    if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_NONE)
    {
        return FCS_STATE_IS_NOT_SOLVEABLE;
    }

    moves = fcs_move_stack_create();    

    max_sequence_len = calc_max_sequence_move(num_freecells, num_freestacks-1);

    /* Now try to move sequences to empty stacks */

    if (num_freestacks > 0)
    {
        for(stack=0;stack<instance->stacks_num;stack++)
        {
            cards_num = fcs_stack_len(state, stack);

            for(c=0; c<cards_num; c=seq_end+1)
            {
                /* Check if there is a sequence here. */
                for(seq_end=c ; seq_end<cards_num-1; seq_end++)
                {
                    this_card = fcs_stack_card(state, stack, seq_end+1);
                    prev_card = fcs_stack_card(state, stack, seq_end);

                    if (! fcs_is_parent_card(this_card, prev_card))
                    {
                        break;
                    }
                }

                if ((fcs_stack_card_num(state, stack, c) != 13) &&
                    (instance->empty_stacks_fill == FCS_ES_FILLED_BY_KINGS_ONLY))
                {
                    continue;
                }

                if (seq_end == cards_num -1)
                {
                    /* One stack is the destination stack, so we have one     *
                     * less stack in that case                                */
                    while ((max_sequence_len < cards_num -c) && (c > 0))
                    {                    
                        c--;
                    }                
            
                    if (
                        (c > 0) && 
                        ((instance->empty_stacks_fill == FCS_ES_FILLED_BY_KINGS_ONLY) ? 
                            (fcs_card_card_num(fcs_stack_card(state, stack, c)) == 13) : 
                            1
                        )
                       )
                    {
                        sfs_check_state_begin();

                        fcs_move_stack_reset(moves);
                    
                        for(ds=0;ds<instance->stacks_num;ds++)
                        {
                            if (fcs_stack_len(state, ds) == 0)
                                break;
                        }

                        for(a=c ; a <= cards_num-1 ; a++)
                        {
                            fcs_push_stack_card_into_stack(new_state, ds, stack, a);
                        }

                        for(a=0;a<cards_num-c;a++)
                        {
                            fcs_pop_stack_card(new_state, stack, temp_card);
                        }

                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                        fcs_move_set_src_stack(temp_move,stack);
                        fcs_move_set_dest_stack(temp_move,ds);
                        fcs_move_set_num_cards_in_seq(temp_move,cards_num-c);

                        fcs_move_stack_push(moves, temp_move);
                  
                
                        sfs_check_state_end()
                    }
                }
                else
                {
                    num_cards_to_relocate = cards_num - seq_end - 1;

                    freecells_to_fill = min(num_cards_to_relocate, num_freecells);

                    num_cards_to_relocate -= freecells_to_fill;

                    if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD)
                    {
                        freestacks_to_fill = min(num_cards_to_relocate, num_freestacks);

                        num_cards_to_relocate -= freestacks_to_fill;
                    }
                    else
                    {
                        freestacks_to_fill = 0;
                    }

                    if ((num_cards_to_relocate == 0) && (num_freestacks-freestacks_to_fill > 0))
                    {
                        /* We can move it */
                        int seq_start = c;
                        while (
                            (calc_max_sequence_move(
                                num_freecells-freecells_to_fill,                             
                                num_freestacks-freestacks_to_fill-1) < seq_end-seq_start+1)
                                &&
                            (seq_start <= seq_end)
                            )
                        {
                            seq_start++;
                        }
                        if ((seq_start <= seq_end) && 
                            ((instance->empty_stacks_fill == FCS_ES_FILLED_BY_KINGS_ONLY) ? 
                                (fcs_card_card_num(fcs_stack_card(state, stack, seq_start)) == 13) : 
                                1
                            )
                        )
                        {
                            sfs_check_state_begin();

                            fcs_move_stack_reset(moves);

                            /* Fill the freecells with the top cards */

                            for(a=0; a<freecells_to_fill ; a++)
                            {
                                /* Find a vacant freecell */
                                for(b=0;b<instance->freecells_num;b++)
                                {
                                    if (fcs_freecell_card_num(new_state, b) == 0)
                                    {
                                        break;
                                    }
                                }
                                fcs_pop_stack_card(new_state, stack, temp_card);
                                fcs_put_card_in_freecell(new_state, b, temp_card);

                                fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                fcs_move_set_src_stack(temp_move,stack);
                                fcs_move_set_dest_freecell(temp_move,b);
                                fcs_move_stack_push(moves, temp_move);
                            }

                            /* Fill the free stacks with the cards below them */
                            for(a=0; a < freestacks_to_fill ; a++)
                            {
                                /* Find a vacant stack */
                                for(b=0; b<instance->stacks_num; b++)
                                {
                                    if (fcs_stack_len(new_state, b) == 0)
                                    {
                                        break;
                                    }
                                }
                                fcs_pop_stack_card(new_state, stack, temp_card);
                                fcs_push_card_into_stack(new_state, b, temp_card);
                                fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                fcs_move_set_src_stack(temp_move,stack);
                                fcs_move_set_dest_stack(temp_move,b);
                                fcs_move_set_num_cards_in_seq(temp_move,1);
                                fcs_move_stack_push(moves, temp_move);
                            }

                            /* Find a vacant stack */
                            for(b=0; b<instance->stacks_num; b++)
                            {
                                if (fcs_stack_len(new_state, b) == 0)
                                {
                                    break;
                                }
                            }

                            for(a=seq_start ; a <= seq_end ; a++)
                            {
                                fcs_push_stack_card_into_stack(new_state, b, stack, a);
                            }
                            for(a=seq_start ; a <= seq_end ; a++)
                            {
                                fcs_pop_stack_card(new_state, stack, temp_card);
                            }

                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                            fcs_move_set_src_stack(temp_move,stack);
                            fcs_move_set_dest_stack(temp_move,b);
                            fcs_move_set_num_cards_in_seq(temp_move,seq_end-seq_start+1);
                            fcs_move_stack_push(moves, temp_move);

                            sfs_check_state_end();
                        }
                    }
                }
            }
        }
    }
    
    fcs_move_stack_destroy(moves);

    return FCS_STATE_IS_NOT_SOLVEABLE;
}



int freecell_solver_sfs_move_freecell_cards_to_empty_stack(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;
    int fc, stack;
    fcs_card_t card;    

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    /* Let's try to put cards that occupy freecells on an empty stack */

    if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_NONE)
    {
        return FCS_STATE_IS_NOT_SOLVEABLE;
    }

    moves = fcs_move_stack_create();

    for(fc=0;fc<instance->freecells_num;fc++)
    {
        card = fcs_freecell_card(state, fc);
        if (
            (instance->empty_stacks_fill == FCS_ES_FILLED_BY_KINGS_ONLY) ? 
                (fcs_card_card_num(card) == 13) :
                (fcs_card_card_num(card) != 0)
           )
        {
            for(stack=0;stack<instance->stacks_num;stack++)
            {
                if (fcs_stack_len(state, stack) == 0)
                {
                    break;
                }
            }
            if (stack != instance->stacks_num)
            {
                /* We can move it */
                
                sfs_check_state_begin();

                fcs_move_stack_reset(moves);
                
                fcs_push_card_into_stack(new_state, stack, card);
                fcs_empty_freecell(new_state, fc);

                fcs_move_set_type(temp_move,FCS_MOVE_TYPE_FREECELL_TO_STACK);
                fcs_move_set_src_freecell(temp_move,fc);
                fcs_move_set_dest_stack(temp_move,stack);
                fcs_move_stack_push(moves, temp_move);
                
                sfs_check_state_end()
            }
        }
    }

    fcs_move_stack_destroy(moves);
    
    return FCS_STATE_IS_NOT_SOLVEABLE;
}

int freecell_solver_sfs_move_cards_to_a_different_parent(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    int stack, cards_num, c, a, b, ds, dc;
    int is_seq_in_src, is_seq_in_dest;
    int num_cards_to_relocate;
    int dest_cards_num;
    fcs_card_t card, this_card, prev_card, temp_card;
    fcs_card_t dest_card, dest_below_card;
    int freecells_to_fill, freestacks_to_fill;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    moves = fcs_move_stack_create();


    /* This time try to move cards that are already on top of a parent to a different parent */

    for (stack=0;stack<instance->stacks_num;stack++)
    {
        cards_num = fcs_stack_len(state, stack);

        for (c=0 ; c<cards_num ; c++)
        {   
            /* Check if there is a sequence here. */
            is_seq_in_src = 1;
            for(a=c+1 ; a<cards_num ; a++)
            {
                this_card = fcs_stack_card(state, stack, a);
                prev_card = fcs_stack_card(state, stack, a-1);
                
                if (fcs_is_parent_card(this_card,prev_card))
                {
                }
                else
                {
                    is_seq_in_src = 0;
                    break;
                }
            }

            /* Find a card which this card can be put on; */

            card = fcs_stack_card(state, stack, c);


            /* Do not move cards that are already found above a suitable parent */
            a = 1;
            if (c != 0)
            {
                prev_card = fcs_stack_card(state, stack, c-1);
                if (fcs_is_parent_card(card,prev_card))
                {
                   a = 0;  
                }
            }
            /* And do not move cards that are flipped */
            if (!a)
            {
                this_card = fcs_stack_card(state,stack,c);
                if (fcs_card_get_flipped(this_card))
                {
                    a = 0;
                }
            }
            if (!a)
            {
                for(ds=0 ; ds<instance->stacks_num; ds++)
                {
                    if (ds != stack)
                    {
                        dest_cards_num = fcs_stack_len(state, ds);
                        for(dc=0;dc<dest_cards_num;dc++)
                        {
                            dest_card = fcs_stack_card(state, ds, dc);
                            
                            if (fcs_is_parent_card(card,dest_card))
                            {
                                /* Corresponding cards - see if it is feasible to move
                                   the source to the destination. */

                                is_seq_in_dest = 0;
                                if (dest_cards_num - 1 > dc)
                                {
                                    dest_below_card = fcs_stack_card(state, ds, dc+1);
                                    if (fcs_is_parent_card(dest_below_card,dest_card))
                                    {
                                        is_seq_in_dest = 1;
                                    }
                                }

                                if (! is_seq_in_dest)
                                {
                                    if (is_seq_in_src)
                                    {
                                        num_cards_to_relocate = dest_cards_num - dc - 1;

                                        freecells_to_fill = min(num_cards_to_relocate, num_freecells);

                                        num_cards_to_relocate -= freecells_to_fill;

                                        if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD)
                                        {
                                            freestacks_to_fill = min(num_cards_to_relocate, num_freestacks);

                                            num_cards_to_relocate -= freestacks_to_fill;
                                        }
                                        else
                                        {
                                            freestacks_to_fill = 0;
                                        }

                                        if ((num_cards_to_relocate == 0) &&
                                           (calc_max_sequence_move(num_freecells-freecells_to_fill, num_freestacks-freestacks_to_fill) >=
                                            cards_num - c))
                                        {
                                            /* We can move it */
                                            
                                            sfs_check_state_begin()

                                            fcs_move_stack_reset(moves);
                                            
                                            /* Fill the freecells with the top cards */
                                            for(a=0 ; a<freecells_to_fill ; a++)
                                            {
                                                /* Find a vacant freecell */
                                                for(b=0;b<instance->freecells_num;b++)
                                                {
                                                    if (fcs_freecell_card_num(new_state, b) == 0)
                                                    {
                                                        break;
                                                    }
                                                }

                                                fcs_pop_stack_card(new_state, ds, temp_card);

                                                fcs_put_card_in_freecell(new_state, b, temp_card);

                                                fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                                                fcs_move_set_src_stack(temp_move,ds);
                                                fcs_move_set_dest_freecell(temp_move,b);
                                                fcs_move_stack_push(moves, temp_move);
                                            }

                                            /* Fill the free stacks with the cards below them */
                                            for(a=0; a < freestacks_to_fill ; a++)
                                            {
                                                /*  Find a vacant stack */
                                                for(b=0;b<instance->stacks_num;b++)
                                                {
                                                    if (fcs_stack_len(new_state, b) == 0)
                                                    {
                                                        break;
                                                    }
                                                }

                                                fcs_pop_stack_card(new_state, ds, temp_card);
                                                fcs_push_card_into_stack(new_state, b, temp_card);

                                                fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                                fcs_move_set_src_stack(temp_move,ds);
                                                fcs_move_set_dest_stack(temp_move,b);
                                                fcs_move_set_num_cards_in_seq(temp_move,1);
                                                fcs_move_stack_push(moves, temp_move);
                                            }

                                            for(a=c ; a <= cards_num-1 ; a++)
                                            {
                                                fcs_push_stack_card_into_stack(new_state, ds, stack, a);
                                            }

                                            for(a=0;a<cards_num-c;a++)
                                            {
                                                fcs_pop_stack_card(new_state, stack, temp_card);
                                            }

                                            fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_STACK);
                                            fcs_move_set_src_stack(temp_move,stack);
                                            fcs_move_set_dest_stack(temp_move,ds);
                                            fcs_move_set_num_cards_in_seq(temp_move,cards_num-c);
                                            fcs_move_stack_push(moves, temp_move);

                                            sfs_check_state_end()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    fcs_move_stack_destroy(moves);    
    
    return FCS_STATE_IS_NOT_SOLVEABLE;
}




int freecell_solver_sfs_empty_stack_into_freecells(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    int stack, cards_num, c, b;
    fcs_card_t temp_card;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_NONE)
    {
        return FCS_STATE_IS_NOT_SOLVEABLE;
    }

    moves = fcs_move_stack_create();
    


    /* Now, let's try to empty an entire stack into the freecells, so other cards can 
     * inhabit it */

    if (num_freestacks == 0)
    {
        for(stack=0;stack<instance->stacks_num;stack++)
        {
            cards_num = fcs_stack_len(state, stack);
            if (cards_num <= num_freecells)
            {
                /* We can empty it */
                
                sfs_check_state_begin()

                fcs_move_stack_reset(moves);
                
                for(c=0;c<cards_num;c++)
                {
                    /* Find a vacant freecell */
                    for(b=0; b<instance->freecells_num; b++)
                    {
                        if (fcs_freecell_card_num(new_state, b) == 0)
                        {
                            break;
                        }
                    }
                    fcs_pop_stack_card(new_state, stack, temp_card);

                    fcs_put_card_in_freecell(new_state, b, temp_card);

                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FREECELL);
                    fcs_move_set_src_stack(temp_move,stack);
                    fcs_move_set_dest_freecell(temp_move,b);
                    fcs_move_stack_push(moves, temp_move);
                }
                
                sfs_check_state_end()
            }
        }
    }

    fcs_move_stack_destroy(moves);    
    
    return FCS_STATE_IS_NOT_SOLVEABLE;

}

#ifdef FCS_WITH_TALONS
/*
    Let's try to deal the Gypsy-type Talon.
    
  */
int freecell_solver_sfs_deal_gypsy_talon(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{
    fcs_state_with_locations_t * ptr_new_state_with_locations;
    int check;

    fcs_card_t temp_card;
    int a;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    if (instance->talon_type != FCS_TALON_GYPSY)
    {
        return FCS_STATE_IS_NOT_SOLVEABLE;
    }

    moves = fcs_move_stack_create();
    
    if (fcs_talon_pos(state) < fcs_talon_len(state))
    {
        sfs_check_state_begin()
        for(a=0;a<instance->stacks_num;a++)
        {
            temp_card = fcs_get_talon_card(new_state, fcs_talon_pos(new_state)+a);
            fcs_push_card_into_stack(new_state,a,temp_card);
        }
        fcs_talon_pos(new_state) += instance->stacks_num;
        fcs_move_set_type(temp_move, FCS_MOVE_TYPE_DEAL_GYPSY_TALON);
        fcs_move_stack_push(moves, temp_move);

        sfs_check_state_end()
    }

    fcs_move_stack_destroy(moves);    
    
    return FCS_STATE_IS_NOT_SOLVEABLE;
}


int freecell_solver_sfs_get_card_from_klondike_talon(
        freecell_solver_instance_t * instance,
        fcs_state_with_locations_t * ptr_state_with_locations,
        int depth,
        int num_freestacks,
        int num_freecells,
        int ignore_osins
        )
{

    fcs_state_with_locations_t * ptr_new_state_with_locations;
    fcs_state_with_locations_t * talon_temp;

    fcs_move_stack_t * moves;
    fcs_move_t temp_move;

    int check;
    int num_redeals_left, num_redeals_done, num_cards_moved[2];
    int first_iter;
    fcs_card_t card_to_check, top_card;
    int s;
    int cards_num;
    int a;

    if (instance->talon_type != FCS_TALON_KLONDIKE)
    {
        return FCS_STATE_IS_NOT_SOLVEABLE;
    }

    moves = fcs_move_stack_create();

    /* Duplicate the talon and its parameters into talon_temp */
    talon_temp = malloc(sizeof(fcs_state_with_locations_t));
    talon_temp->s.talon = malloc(fcs_klondike_talon_len(state)+1);
    memcpy(
        talon_temp->s.talon, 
        ptr_state_with_locations->s.talon, 
        fcs_klondike_talon_len(state)+1
        );
    memcpy(
        talon_temp->s.talon_params, 
        ptr_state_with_locations->s.talon_params, 
        sizeof(ptr_state_with_locations->s.talon_params)
        );

    /* Make sure we redeal the talon only once */
    num_redeals_left = fcs_klondike_talon_num_redeals_left(state);
    if ((num_redeals_left > 0) || (num_redeals_left < 0))
    {
        num_redeals_left = 1;
    }
    num_redeals_done = 0;
    num_cards_moved[0] = 0;
    num_cards_moved[1] = 0;

    first_iter = 1;
    while (num_redeals_left >= 0)
    {
        if ((fcs_klondike_talon_stack_pos(talon_temp->s) == -1) &&
            (fcs_klondike_talon_queue_pos(talon_temp->s) == fcs_klondike_talon_len(talon_temp->s)))
        {
            break;
        }
        if ((!first_iter) || (fcs_klondike_talon_stack_pos(talon_temp->s) == -1))
        {
            if (fcs_klondike_talon_queue_pos(talon_temp->s) == fcs_klondike_talon_len(talon_temp->s))
            {
                if (num_redeals_left > 0)
                {
                    fcs_klondike_talon_len(talon_temp->s) = fcs_klondike_talon_stack_pos(talon_temp->s);
                    fcs_klondike_talon_redeal_bare(talon_temp->s);
                    
                    num_redeals_left--;
                    num_redeals_done++;
                }
                else
                {
                    break;
                }
            }
            fcs_klondike_talon_queue_to_stack(talon_temp->s);
            num_cards_moved[num_redeals_done]++;
        }
        first_iter = 0;

        card_to_check = fcs_klondike_talon_get_top_card(talon_temp->s);
        for(s=0 ; s<instance->stacks_num ; s++)
        {
            cards_num = fcs_stack_len(state,s);
            top_card = fcs_stack_card(state,s,cards_num-1);
            if (fcs_is_parent_card(card_to_check, top_card))
            {
                /* We have a card in the talon that we can move 
                to the stack, so let's move it */
                sfs_check_state_begin()
                
                new_state.talon = malloc(fcs_klondike_talon_len(talon_temp->s)+1);
                memcpy(
                    new_state.talon,
                    talon_temp->s.talon,
                    fcs_klondike_talon_len(talon_temp->s)+1
                    );

                memcpy(
                    ptr_new_state_with_locations->s.talon_params, 
                    talon_temp->s.talon_params,
                    sizeof(ptr_state_with_locations->s.talon_params)
                );

                for(a=0;a<=num_redeals_done;a++)
                {
                    fcs_move_set_type(temp_move, FCS_MOVE_TYPE_KLONDIKE_FLIP_TALON);
                    fcs_move_set_num_cards_flipped(temp_move, num_cards_moved[a]);
                    fcs_move_stack_push(moves, temp_move);
                    if (a != num_redeals_done)
                    {
                        fcs_move_set_type(temp_move, FCS_MOVE_TYPE_KLONDIKE_REDEAL_TALON);
                        fcs_move_stack_push(moves,temp_move);
                    }
                }
                fcs_push_card_into_stack(new_state, s, fcs_klondike_talon_get_top_card(new_state));
                fcs_move_set_type(temp_move, FCS_MOVE_TYPE_KLONDIKE_TALON_TO_STACK);
                fcs_move_set_dest_stack(temp_move, s);
                fcs_klondike_talon_decrement_stack(new_state);

                sfs_check_state_end()
            }            
        }
    }



#if 0
 cleanup:
#endif
    free(talon_temp->s.talon);
    free(talon_temp);
    
    fcs_move_stack_destroy(moves);

    return FCS_STATE_IS_NOT_SOLVEABLE;

}
#endif

#undef state
#undef new_state_with_locations
#undef state_with_locations
#undef new_state

freecell_solver_solve_for_state_test_t freecell_solver_sfs_tests[FCS_TESTS_NUM] = 
{
    freecell_solver_sfs_move_top_stack_cards_to_founds,
    freecell_solver_sfs_move_freecell_cards_to_founds,
    freecell_solver_sfs_move_freecell_cards_on_top_of_stacks,
    freecell_solver_sfs_move_non_top_stack_cards_to_founds,
    freecell_solver_sfs_move_stack_cards_to_different_stacks,
    freecell_solver_sfs_move_stack_cards_to_a_parent_on_the_same_stack,
    freecell_solver_sfs_move_sequences_to_free_stacks,
    freecell_solver_sfs_move_freecell_cards_to_empty_stack,
    freecell_solver_sfs_move_cards_to_a_different_parent,
    freecell_solver_sfs_empty_stack_into_freecells
#ifdef FCS_WITH_TALONS        
        ,
    freecell_solver_sfs_deal_gypsy_talon,
    freecell_solver_sfs_get_card_from_klondike_talon
#endif        
};


