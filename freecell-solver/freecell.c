/*
 * freecell.c - algorithm of Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#include <stdlib.h>
#include <search.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include "config.h"
#include "state.h"
#include "card.h"
#include "fcs_dm.h"
#include "fcs.h"

#include "fcs_isa.h"

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


#define sfs_check_state_init() \
    ptr_new_state_with_locations = fcs_state_ia_alloc(instance);        


#define sfs_check_state_finish() \
else if ((check == FCS_STATE_IS_NOT_SOLVEABLE) && ((instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR))) \
{  \
    moves = fcs_move_stack_create();  \
} \
else if ((check == FCS_STATE_ALREADY_EXISTS)) \
{             \
    fcs_state_ia_release(instance);          \
}           \
else if ((check == FCS_STATE_EXCEEDS_MAX_NUM_TIMES) || \
         (check == FCS_STATE_SUSPEND_PROCESS) || \
         (check == FCS_STATE_BEGIN_SUSPEND_PROCESS) || \
         (check == FCS_STATE_EXCEEDS_MAX_DEPTH)) \
{          \
    if ((check == FCS_STATE_SUSPEND_PROCESS)) \
    {              \
        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_CANONIZE);      \
        fcs_move_stack_push(moves, temp_move);           \
        instance->proto_solution_moves[depth] = moves;    \
    }          \
    else if ( (check == FCS_STATE_BEGIN_SUSPEND_PROCESS) &&  ((instance->method == FCS_METHOD_BFS)||(instance->method == FCS_METHOD_A_STAR)) )    \
    {             \
        /* Do nothing because those methods expect us to have a move stack present in every state */  \
    }              \
    else           \
    {             \
        fcs_move_stack_destroy(moves);    \
    }         \
    return check;   \
}    


#if 0
printf("%i,%i\n", depth, instance->soft_dfs_num_states_to_check[depth]);                                                 
#endif

#define sfs_check_state_handle_soft_dfs() \
if (instance->method == FCS_METHOD_SOFT_DFS)         \
{                                                    \
    instance->soft_dfs_states_to_check[depth][       \
        instance->soft_dfs_num_states_to_check[depth]     \
        ] = ptr_new_state_with_locations;            \
    instance->soft_dfs_states_to_check_move_stacks[depth][  \
        instance->soft_dfs_num_states_to_check[depth]    \
        ] = moves;                                        \
    instance->soft_dfs_num_states_to_check[depth]++;         \
    moves = fcs_move_stack_create();                        \
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
}                                                    \

#define sfs_check_state_begin() \
sfs_check_state_init() \
fcs_duplicate_state(new_state_with_locations, state_with_locations); \
if ( (instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR) )     \
{       \
    ptr_new_state_with_locations->parent = ptr_state_with_locations;    \
    ptr_new_state_with_locations->moves_to_parent = moves; \
    ptr_new_state_with_locations->depth = depth+1; \
} \
ptr_new_state_with_locations->visited = 0; 

#define sfs_check_state_end()                        \
fcs_move_set_type(temp_move,FCS_MOVE_TYPE_CANONIZE); \
fcs_move_stack_push(moves, temp_move);               \
    \
{                                                    \
    fcs_state_with_locations_t * existing_state;     \
check = freecell_solver_check_and_add_state(         \
    instance,                                        \
    ptr_new_state_with_locations,                    \
    &existing_state,                                  \
    depth);                                          \
    if ((instance->method == FCS_METHOD_SOFT_DFS) && \
        (check == FCS_STATE_ALREADY_EXISTS))         \
    {                                                    \
        ptr_new_state_with_locations = existing_state;    \
    }                                                    \
}                                                    \
sfs_check_state_handle_soft_dfs()                    \
                                                     \
if (check == FCS_STATE_WAS_SOLVED)                   \
{                                                    \
    instance->proto_solution_moves[depth] = moves;   \
    return FCS_STATE_WAS_SOLVED;                     \
}                                                    \
sfs_check_state_finish();



#define state_with_locations (*ptr_state_with_locations)
#define state (ptr_state_with_locations->s)
#define new_state_with_locations (*ptr_new_state_with_locations)
#define new_state (ptr_new_state_with_locations->s)


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
            card = fcs_stack_card(state,stack,cards_num-1);
            for(deck=0;deck<instance->decks_num;deck++)
            {
                if (fcs_deck_value(state, deck*4+fcs_card_deck(card)) == fcs_card_card_num(card) - 1)
                {
                    /* We can put it there */

                    sfs_check_state_begin()

                    fcs_pop_stack_card(new_state, stack, temp_card);

                    fcs_increment_deck(new_state, deck*4+fcs_card_deck(card));

                    fcs_move_stack_reset(moves);

                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FOUNDATION);
                    fcs_move_set_src_stack(temp_move,stack);
                    fcs_move_set_foundation(temp_move,deck*4+fcs_card_deck(card));

                    fcs_move_stack_push(moves, temp_move);
                    
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
                            return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                        }
                        
                        if (instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_RANK)
                        {
                            for(d=0;d<instance->decks_num*4;d++)
                            {
                                if (fcs_card_card_num(card) - 2 > fcs_deck_value(state, d))
                                {
                                    break;
                                }
                            }

                            if (d == instance->decks_num*4)
                            {
                                return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                            }
                        }
                        else
                        {
                            /* instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_ALTERNATE_COLOR */
                            for(d=0;d<instance->decks_num;d++)
                            {
                                if (!((fcs_card_card_num(card) - 2 <= fcs_deck_value(state, d*4+(fcs_card_deck(card)^0x1))) &&
                                    (fcs_card_card_num(card) - 2 <= fcs_deck_value(state, d*4+(fcs_card_deck(card)^0x3)))))
                                {
                                    break;
                                }
                            }
                        
                            if (d == instance->decks_num)
                            {
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
                if (fcs_deck_value(state, deck*4+fcs_card_deck(card)) == fcs_card_card_num(card) - 1)
                {
                    /* We can put it there */
                    sfs_check_state_begin()
                        
                    fcs_empty_freecell(new_state, fc);

                    fcs_increment_deck(new_state, deck*4+fcs_card_deck(card));

                    fcs_move_stack_reset(moves);
                    fcs_move_set_type(temp_move,FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION);
                    fcs_move_set_src_freecell(temp_move,fc);
                    fcs_move_set_foundation(temp_move,deck*4+fcs_card_deck(card));

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
                            return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                        }
                        
                        /*  If the decks of the other colour are higher than the card number  *
                         *  of this card + 2, it means that if the sub-state is not solveable, *
                         *  then this state is not solveable either.                          */

                        if (instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_RANK)
                        {
                            for(d=0;d<instance->decks_num*4;d++)
                            {
                                if (fcs_card_card_num(card) - 2 > fcs_deck_value(state, d))
                                {
                                    break;
                                }
                            }

                            if (d == instance->decks_num*4)
                            {
                                return FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE;
                            }
                        }
                        else
                        {
                            /* instance->sequences_are_built_by == FCS_SEQ_BUILT_BY_ALTERNATE_COLOR */
                            for(d=0;d<instance->decks_num;d++)
                            {
                                if (!((fcs_card_card_num(card) - 2 <= fcs_deck_value(state, d*4+(fcs_card_deck(card)^0x1))) &&
                                    (fcs_card_card_num(card) - 2 <= fcs_deck_value(state, d*4+(fcs_card_deck(card)^0x3)))))
                                {
                                    break;
                                }
                            }
                        
                            if (d == instance->decks_num)
                            {
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

    for(ds=0;ds<instance->stacks_num;ds++)
    {
        dest_cards_num = fcs_stack_len(state, ds);

        if (dest_cards_num > 0)
        {
            for(dc=dest_cards_num-1;dc>=0;dc--)
            {
                dest_card = fcs_stack_card(state, ds, dc);

                for(fc=0;fc<instance->freecells_num;fc++)
                {
                    src_card = fcs_freecell_card(state, fc);
    
                    if ( (fcs_card_card_num(src_card) != 0) && 
                         fcs_is_parent_card(src_card,dest_card)     )
                    {
                        /* Let's check if we can put it there */

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
     * in the decks. */

    for(stack=0;stack<instance->stacks_num;stack++)
    {
        cards_num = fcs_stack_len(state, stack);
        for(c=cards_num-2 ; c >= 0 ; c--)
        {
            card = fcs_stack_card(state, stack, c);
            for(deck=0;deck<instance->decks_num;deck++)
            {
                if (fcs_deck_value(state, deck*4+fcs_card_deck(card)) == fcs_card_card_num(card)-1)
                {
                    /* The card is deckable. Now let's check if we can move the
                     * cards above it to the freecells and stacks */

                    if (num_freecells + ((instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD) ? num_freestacks : 0) >= cards_num-(c+1))
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
                        }

                        fcs_pop_stack_card(new_state, stack, temp_card);
                        fcs_increment_deck(new_state, deck*4+fcs_card_deck(temp_card));

                        fcs_move_set_type(temp_move,FCS_MOVE_TYPE_STACK_TO_FOUNDATION);
                        fcs_move_set_src_stack(temp_move,stack);
                        fcs_move_set_foundation(temp_move,deck*4+fcs_card_deck(temp_card));

                        fcs_move_stack_push(moves, temp_move);
                        
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
     * in the same stack.
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
                    ((fcs_card_deck(prev_card) & 0x1) != (fcs_card_deck(card) & 0x1)))
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
#if 0
    int is_seq_in_src, is_seq_in_dest;
#else
    int is_seq_in_dest;
#endif
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
                                    
                                    sfs_check_state_end()
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



#undef state
#undef new_state_with_locations
#undef state_with_locations
#undef new_state

typedef int (*freecell_solver_solve_for_state_test_t)(
        freecell_solver_instance_t *,
        fcs_state_with_locations_t *,
        int,
        int,
        int,
        int
        );

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
};




#define state (ptr_state_with_locations->s)

    
int freecell_solver_solve_for_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations,
    int depth,
    int ignore_osins
    )

{
    
    int a;
    int check;

    int num_freestacks, num_freecells;

#ifdef DEBUG
    int iter_num = instance->num_times;
#endif
    instance->num_times++;


#ifdef DEBUG
    if (instance->debug_iter_output)
    {
        instance->debug_iter_output_func(
                (void*)instance->debug_iter_output_context,
                iter_num,
                depth,
                (void*)instance,
                ptr_state_with_locations
                );
    }
    
#endif        

    /* Count the free-cells */
    num_freecells = 0;
    for(a=0;a<instance->freecells_num;a++)
    {
        if (fcs_freecell_card_num(state, a) == 0)
        {
            num_freecells++;
        }
    }

    /* Count the number of unoccupied stacks */
    
    num_freestacks = 0;    
    for(a=0;a<instance->stacks_num;a++)
    {
        if (fcs_stack_len(state, a) == 0)
        {
            num_freestacks++;
        }
    }

   
    /* Let's check if this state is finished, and if so return 0; */
    if ((num_freestacks == instance->stacks_num) && (num_freecells == instance->freecells_num))
    {      
        instance->num_solution_states = depth+1;

        instance->solution_states = malloc(sizeof(fcs_state_with_locations_t*) * instance->num_solution_states);
        instance->proto_solution_moves = malloc(sizeof(fcs_move_stack_t *) * (instance->num_solution_states-1));

        instance->solution_states[depth] = malloc(sizeof(fcs_state_with_locations_t)); 
        fcs_duplicate_state(*(instance->solution_states[depth]), *ptr_state_with_locations); 

        instance->solution_moves = fcs_move_stack_create();
         
        return FCS_STATE_WAS_SOLVED;
    }

    
    for(a=0 ;
        a < instance->tests_order_num;
        a++)
    {
        check = freecell_solver_sfs_tests[instance->tests_order[a]] (
                instance,
                ptr_state_with_locations,
                depth,
                num_freestacks,
                num_freecells,
                ignore_osins
                );

        if (check == FCS_STATE_WAS_SOLVED)
        {
            instance->solution_states[depth] =
                malloc(sizeof(fcs_state_with_locations_t));
            fcs_duplicate_state(
                *(instance->solution_states[depth]), 
                *ptr_state_with_locations
                ); 
        
            return FCS_STATE_WAS_SOLVED;
        }
        else if ((check == FCS_STATE_ORIGINAL_STATE_IS_NOT_SOLVEABLE))
        {
            return FCS_STATE_IS_NOT_SOLVEABLE;
        }
        else if ((check == FCS_STATE_BEGIN_SUSPEND_PROCESS) ||
                 (check == FCS_STATE_SUSPEND_PROCESS))
        {
            if (check == FCS_STATE_BEGIN_SUSPEND_PROCESS)
            {
                instance->num_solution_states = depth+1;
                
                instance->solution_states = malloc(sizeof(fcs_state_with_locations_t*) * instance->num_solution_states);
                instance->proto_solution_moves = malloc(sizeof(fcs_move_stack_t *) * (instance->num_solution_states-1));
            }

            instance->solution_states[depth] = malloc(sizeof(fcs_state_with_locations_t));
            fcs_duplicate_state(*(instance->solution_states[depth]), *ptr_state_with_locations);

            return FCS_STATE_SUSPEND_PROCESS;
        }
    }

    return FCS_STATE_IS_NOT_SOLVEABLE;
}


static int freecell_solver_solve_for_state_resume_solution(
    freecell_solver_instance_t * instance,
    int depth
    )
{
    fcs_state_with_locations_t * ptr_state_with_locations;
    fcs_move_stack_t * moves = NULL;
    int check;
    int use_own_moves = 1;

    ptr_state_with_locations = instance->solution_states[depth];

    if (depth < instance->num_solution_states-1)
    {
        moves = instance->proto_solution_moves[depth];
        check = freecell_solver_solve_for_state_resume_solution(
            instance,
            depth+1
            );        
    }
    else
    {
        free(instance->solution_states);
        free(instance->proto_solution_moves);
        check = FCS_STATE_IS_NOT_SOLVEABLE;
    }

    if (check == FCS_STATE_IS_NOT_SOLVEABLE)
    {
        check = freecell_solver_solve_for_state(
            instance,
            ptr_state_with_locations,
            depth,
            1);
        use_own_moves = 0;
        if (moves != NULL)
        {
            fcs_move_stack_destroy(moves);
        }
    }
    else if ((check == FCS_STATE_SUSPEND_PROCESS) || (check == FCS_STATE_WAS_SOLVED))
    {
        instance->solution_states[depth] = malloc(sizeof(fcs_state_with_locations_t));
        fcs_duplicate_state(*(instance->solution_states[depth]), *ptr_state_with_locations);
    }

    if (use_own_moves)
    {
        instance->proto_solution_moves[depth] = moves;
    }

    free(ptr_state_with_locations);

    return check;
}

#undef state





static void freecell_solver_increase_dfs_max_depth(
    freecell_solver_instance_t * instance
    )
{
    int new_dfs_max_depth = instance->dfs_max_depth + 16;
    int d;

#define MYREALLOC(what) \
    instance->what = realloc( \
        instance->what,       \
        sizeof(instance->what[0])*new_dfs_max_depth \
        ); \

    MYREALLOC(solution_states);
    MYREALLOC(proto_solution_moves);
    MYREALLOC(soft_dfs_states_to_check);
    MYREALLOC(soft_dfs_states_to_check_move_stacks);
    MYREALLOC(soft_dfs_num_states_to_check);
    MYREALLOC(soft_dfs_current_state_indexes);
    MYREALLOC(soft_dfs_test_indexes);
    MYREALLOC(soft_dfs_max_num_states_to_check)
#undef MYREALLOC
    
#if 0
    instance->solution_states = realloc(
        instance->solution_states,
        sizeof(instance->solution_states[0])*new_dfs_max_depth
        );

    instance->proto_solution_moves= realloc(
        instance->proto_solution_moves,
        sizeof(instance->proto_solution_moves[0])*new_dfs_max_depth
        );
#endif


    for(d=instance->dfs_max_depth ; d<new_dfs_max_depth; d++)
    {
        instance->solution_states[d] = NULL;
        instance->proto_solution_moves[d] = NULL;
        instance->soft_dfs_max_num_states_to_check[d] = 0;
        instance->soft_dfs_test_indexes[d] = 0;
        instance->soft_dfs_current_state_indexes[d] = 0;
        instance->soft_dfs_num_states_to_check[d] = 0;
    }

    instance->dfs_max_depth = new_dfs_max_depth;
}


#define state (ptr_state_with_locations->s)

static int freecell_solver_soft_dfs_do_solve_or_resume(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig,
    int resume
    )
{
    int depth;
    int num_freestacks, num_freecells;
    fcs_state_with_locations_t * ptr_state_with_locations, 
        * ptr_recurse_into_state_with_locations;
    int a;
    int check;

    if (!resume)
    {
        depth=0;
    
        freecell_solver_increase_dfs_max_depth(instance);

        instance->soft_dfs_max_num_states_to_check[depth] = FCS_SOFT_DFS_STATES_TO_CHECK_GROW_BY;
        instance->soft_dfs_states_to_check[depth] = malloc(
            sizeof(instance->soft_dfs_states_to_check[depth][0]) *
            instance->soft_dfs_max_num_states_to_check[depth]
            );
        instance->soft_dfs_states_to_check_move_stacks[depth] = malloc(
            sizeof(instance->soft_dfs_states_to_check_move_stacks[depth][0]) *
            instance->soft_dfs_max_num_states_to_check[depth]
            );
        instance->solution_states[0] = ptr_state_with_locations_orig;
    }
    else
    {
        depth = instance->num_solution_states -1;
    }

    while (depth >= 0)
    {
        if (depth+1 >= instance->dfs_max_depth)
        {
            freecell_solver_increase_dfs_max_depth(instance);
        }
        
        if (instance->soft_dfs_test_indexes[depth] >= instance->tests_order_num)
        {
            depth--;
            continue; /* Just to make sure depth is not -1 now */    
        }

        if (instance->soft_dfs_current_state_indexes[depth] == instance->soft_dfs_num_states_to_check[depth])
        {
        
            instance->soft_dfs_num_states_to_check[depth] = 0;
        
            ptr_state_with_locations = instance->solution_states[depth];
        
#ifdef DEBUG
            if (instance->soft_dfs_test_indexes[depth] == 0)
            {
                if (instance->debug_iter_output)
                {
                    instance->debug_iter_output_func(
                        (void*)instance->debug_iter_output_context,
                        instance->num_times,
                        depth,
                        (void*)instance,
                        ptr_state_with_locations
                        );
                }
            }
#endif
        

            /* Count the free-cells */
            num_freecells = 0;
            for(a=0;a<instance->freecells_num;a++)
            {
                if (fcs_freecell_card_num(state, a) == 0)
                {
                    num_freecells++;
                }
            }

            /* Count the number of unoccupied stacks */

            num_freestacks = 0;    
            for(a=0;a<instance->stacks_num;a++)
            {
                if (fcs_stack_len(state, a) == 0)
                {
                    num_freestacks++;
                }
            }

            if ((num_freestacks == instance->stacks_num) && (num_freecells == instance->freecells_num))
            {
                int d;
    
                instance->num_solution_states = depth+1;
                for(d=0;d<=depth;d++)
                {
                    fcs_state_with_locations_t * ptr_state;
                
                    ptr_state = (fcs_state_with_locations_t *)malloc(sizeof(fcs_state_with_locations_t));
                    fcs_duplicate_state(*ptr_state, *(instance->solution_states[d]));
                    instance->solution_states[d] = ptr_state;

                }
                return FCS_STATE_WAS_SOLVED;
            }

        
            check = freecell_solver_sfs_tests[instance->tests_order[
                    instance->soft_dfs_test_indexes[depth]
                ]] (
                   instance,
                   ptr_state_with_locations,
                depth,
                num_freestacks,
                num_freecells,
                1
                );
        
            if ((check == FCS_STATE_BEGIN_SUSPEND_PROCESS) ||
                (check == FCS_STATE_EXCEEDS_MAX_NUM_TIMES) ||
                (check == FCS_STATE_SUSPEND_PROCESS))
            {
                instance->num_solution_states = depth+1;
                return FCS_STATE_SUSPEND_PROCESS;
            }
        
            instance->soft_dfs_test_indexes[depth]++;
        
            instance->soft_dfs_current_state_indexes[depth] = 0;
        }
        
        while (instance->soft_dfs_current_state_indexes[depth] < 
               instance->soft_dfs_num_states_to_check[depth])
        {
            ptr_recurse_into_state_with_locations = (instance->soft_dfs_states_to_check[depth][ 
                    instance->soft_dfs_current_state_indexes[depth]
                ]);
            
            instance->soft_dfs_current_state_indexes[depth]++;
            if (!ptr_recurse_into_state_with_locations->visited)
            {
                ptr_recurse_into_state_with_locations->visited = 1;
                instance->num_times++;
                instance->solution_states[depth+1] = ptr_recurse_into_state_with_locations;
                instance->proto_solution_moves[depth] = 
                    instance->soft_dfs_states_to_check_move_stacks[depth][
                        instance->soft_dfs_current_state_indexes[depth]-1
                    ];
                /*
                    I'm using current_state_indexes[depth]-1 because we already
                    increaded it by one, so now it refers to the next state.
                */
                depth++;
                instance->soft_dfs_test_indexes[depth] = 0;
                instance->soft_dfs_current_state_indexes[depth] = 0;
                instance->soft_dfs_num_states_to_check[depth] = 0;
                
                if (instance->soft_dfs_max_num_states_to_check[depth] == 0)
                {
                    instance->soft_dfs_max_num_states_to_check[depth] = FCS_SOFT_DFS_STATES_TO_CHECK_GROW_BY;
                    instance->soft_dfs_states_to_check[depth] = malloc(
                        sizeof(instance->soft_dfs_states_to_check[depth][0]) *
                        instance->soft_dfs_max_num_states_to_check[depth]
                        );
                    instance->soft_dfs_states_to_check_move_stacks[depth] = malloc(
                        sizeof(instance->soft_dfs_states_to_check_move_stacks[depth][0]) *
                        instance->soft_dfs_max_num_states_to_check[depth]
                        );
                }
                break;
            }
        }    
    }

    instance->num_solution_states = 0;
    
    return FCS_STATE_IS_NOT_SOLVEABLE;
}

#undef state

static int freecell_solver_soft_dfs_solve_for_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig
    )
{
    return freecell_solver_soft_dfs_do_solve_or_resume(
        instance,
        ptr_state_with_locations_orig,
        0
        );
}

static int freecell_solver_soft_dfs_solve_for_state_resume_solution(
    freecell_solver_instance_t * instance
    )
{
    return freecell_solver_soft_dfs_do_solve_or_resume(
        instance,
        NULL,
        1
        );
}


void freecell_solver_bfs_enqueue_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * state
    )
{
    instance->bfs_queue_last_item->next = (fcs_states_linked_list_item_t*)malloc(sizeof(fcs_states_linked_list_item_t));
    instance->bfs_queue_last_item->s = state;
    instance->bfs_queue_last_item->next->next = NULL;
    instance->bfs_queue_last_item = instance->bfs_queue_last_item->next;
}


#define FCS_A_STAR_CARDS_UNDER_SEQUENCES_EXPONENT 1.3
#define FCS_A_STAR_SEQS_OVER_RENEGADE_CARDS_EXPONENT 1.3

#define state (ptr_state_with_locations->s)

void freecell_solver_a_star_initialize_rater(
    freecell_solver_instance_t * instance, 
    fcs_state_with_locations_t * ptr_state_with_locations
    )
{
    int a, c, cards_num;
    fcs_card_t this_card, prev_card;
    double cards_under_sequences;
    cards_under_sequences = 0;
    for(a=0;a<instance->stacks_num;a++)
    {
        cards_num = fcs_stack_len(state, a);
        if (cards_num <= 1)
        {
            continue;
        }

        c = cards_num-2;
        this_card = fcs_stack_card(state, a, c+1);
        prev_card = fcs_stack_card(state, a, c);
        while (fcs_is_parent_card(this_card,prev_card) && (c >= 0))
        {
            c--;
            this_card = prev_card;
            if (c>=0)
            {
                prev_card = fcs_stack_card(state, a, c);
            }
        }
        cards_under_sequences += pow(c+1, FCS_A_STAR_CARDS_UNDER_SEQUENCES_EXPONENT);
    }
    instance->a_star_initial_cards_under_sequences = cards_under_sequences;
}


#if 0
#define FCS_A_STAR_WEIGHT_CARDS_IN_FOUNDATIONS 0.33
#define FCS_A_STAR_WEIGHT_MAX_SEQUENCE_MOVE 0
#define FCS_A_STAR_WEIGHT_CARDS_UNDER_SEQUENCES 0.33
#endif

pq_rating_t freecell_solver_a_star_rate_state(
    freecell_solver_instance_t * instance, 
    fcs_state_with_locations_t * ptr_state_with_locations)
{
    double ret=0;
    int a, c, cards_num, num_cards_in_founds;
    int num_freestacks, num_freecells;
    fcs_card_t this_card, prev_card;
    double cards_under_sequences, temp;
    double seqs_over_renegade_cards;

    cards_under_sequences = 0;
    num_freestacks = 0;
    seqs_over_renegade_cards = 0;
    for(a=0;a<instance->stacks_num;a++)
    {
        cards_num = fcs_stack_len(state, a);
        if (cards_num == 0)
        {
            num_freestacks++;
        }
        
        if (cards_num <= 1)
        {
            continue;
        }

        c = cards_num-2;
        this_card = fcs_stack_card(state, a, c+1);
        prev_card = fcs_stack_card(state, a, c);
        while ((c >= 0) && fcs_is_parent_card(this_card,prev_card))
        {
            c--;
            this_card = prev_card;
            if (c>=0)
            {
                prev_card = fcs_stack_card(state, a, c);
            }
        }
        cards_under_sequences += pow(c+1, FCS_A_STAR_CARDS_UNDER_SEQUENCES_EXPONENT);
        if (c >= 0)
        {
            seqs_over_renegade_cards += 
                ((instance->unlimited_sequence_move) ? 
                    1 :
                    pow(cards_num-c-1, FCS_A_STAR_SEQS_OVER_RENEGADE_CARDS_EXPONENT)
                    );
        }
    }

    ret += ((instance->a_star_initial_cards_under_sequences - cards_under_sequences) 
            / instance->a_star_initial_cards_under_sequences) * instance->a_star_weights[FCS_A_STAR_WEIGHT_CARDS_UNDER_SEQUENCES];

    ret += (seqs_over_renegade_cards / 
               pow(instance->decks_num*52, FCS_A_STAR_SEQS_OVER_RENEGADE_CARDS_EXPONENT) )
           * instance->a_star_weights[FCS_A_STAR_WEIGHT_SEQS_OVER_RENEGADE_CARDS];

    num_cards_in_founds = 0;
    for(a=0;a<instance->decks_num*4;a++)
    {
        num_cards_in_founds += fcs_deck_value(state, a);
    }

    ret += ((double)num_cards_in_founds/(instance->decks_num*52)) * instance->a_star_weights[FCS_A_STAR_WEIGHT_CARDS_OUT];

    num_freecells = 0;
    for(a=0;a<instance->freecells_num;a++)
    {
        if (fcs_freecell_card_num(state,a) == 0)
        {
            num_freecells++;
        }
    }

    if (instance->empty_stacks_fill == FCS_ES_FILLED_BY_ANY_CARD)
    {
        if (instance->unlimited_sequence_move)
        {
            temp = (((double)num_freecells+num_freestacks)/(instance->freecells_num+instance->stacks_num));
        }
        else
        {
            temp = (((double)((num_freecells+1)<<num_freestacks)) / ((instance->freecells_num+1)<<(instance->stacks_num)));
        }
    }
    else
    {
        if (instance->unlimited_sequence_move)
        {
            temp = (((double)num_freecells)/instance->freecells_num);
        }
        else
        {
            temp = 0;
        }
    }

    ret += (temp * instance->a_star_weights[FCS_A_STAR_WEIGHT_MAX_SEQUENCE_MOVE]);
    
    if (ptr_state_with_locations->depth <= 20000)
    {
        ret += ((20000 - ptr_state_with_locations->depth)/20000.0) * instance->a_star_weights[FCS_A_STAR_WEIGHT_DEPTH];
    }

    return (int)(ret*INT_MAX);
}


void freecell_solver_a_star_enqueue_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations
    )
{
    PQueuePush(
        instance->a_star_pqueue,
        ptr_state_with_locations,
        freecell_solver_a_star_rate_state(instance, ptr_state_with_locations)
        );
}

static int freecell_solver_a_star_or_bfs_do_solve_or_resume(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig,
    int resume
    )
{
    fcs_state_with_locations_t * ptr_state_with_locations;
    int num_freestacks, num_freecells;
    fcs_states_linked_list_item_t * save_item;
    int a;
    int check;

    if (!resume)
    {
        ptr_state_with_locations_orig->parent = NULL;
        ptr_state_with_locations_orig->moves_to_parent = NULL;
        ptr_state_with_locations_orig->depth = 0;
    }

    ptr_state_with_locations = ptr_state_with_locations_orig;
    
    while ( ptr_state_with_locations != NULL)
    {
        /* Count the free-cells */
        num_freecells = 0;
        for(a=0;a<instance->freecells_num;a++)
        {
            if (fcs_freecell_card_num(state, a) == 0)
            {
                num_freecells++;
            }
        }

        /* Count the number of unoccupied stacks */

        num_freestacks = 0;    
        for(a=0;a<instance->stacks_num;a++)
        {
            if (fcs_stack_len(state, a) == 0)
            {
                num_freestacks++;
            }
        }

#ifdef DEBUG
        if (instance->debug_iter_output)
        {
            instance->debug_iter_output_func(
                    (void*)instance->debug_iter_output_context,
                    instance->num_times,
                    ptr_state_with_locations->depth,
                    (void*)instance,
                    ptr_state_with_locations
                    );
        }    
#endif        


        if ((num_freestacks == instance->stacks_num) && (num_freecells == instance->freecells_num))
        {
            /*
                Trace the solution.
            */
            int num_solution_states;
            fcs_state_with_locations_t * s1;

            s1 = ptr_state_with_locations;

            num_solution_states = instance->num_solution_states = s1->depth+1;
            instance->solution_states = malloc(sizeof(fcs_state_with_locations_t *)*num_solution_states);
            instance->proto_solution_moves = malloc(sizeof(fcs_move_stack_t *) * (num_solution_states-1));
            while (s1->parent != NULL)
            {
                instance->proto_solution_moves[s1->depth-1] = fcs_move_stack_duplicate(s1->moves_to_parent);
                instance->solution_states[s1->depth] = (fcs_state_with_locations_t*)malloc(sizeof(fcs_state_with_locations_t));
                fcs_duplicate_state(*(instance->solution_states[s1->depth]), *s1);
        
                s1 = s1->parent;
            }
            instance->solution_states[0] = (fcs_state_with_locations_t*)malloc(sizeof(fcs_state_with_locations_t));
            fcs_duplicate_state(*(instance->solution_states[0]), *s1);
            
            return FCS_STATE_WAS_SOLVED;
        }

        instance->num_times++;
#if 0
        {
            if ((instance->num_times % 1000) == 0)
            {
                printf("%i: %i\n", instance->num_times, ptr_state_with_locations->depth);
                fflush(stdout);
            }
        }
#endif

        for(a=0 ;
            a < instance->tests_order_num;
            a++)
        {
            check = freecell_solver_sfs_tests[instance->tests_order[a]] (
                    instance,
                    ptr_state_with_locations,
                    ptr_state_with_locations->depth,
                    num_freestacks,
                    num_freecells,
                    1
                    );
            if ((check == FCS_STATE_BEGIN_SUSPEND_PROCESS) ||
                (check == FCS_STATE_EXCEEDS_MAX_NUM_TIMES) ||
                (check == FCS_STATE_SUSPEND_PROCESS))
            {
                instance->first_state_to_check = ptr_state_with_locations;
                return FCS_STATE_SUSPEND_PROCESS;
            }
        }

        /*
            Extract the next item in the queue/priority queue.
        */
        if (instance->method == FCS_METHOD_BFS)
        {
            if (instance->bfs_queue->next != instance->bfs_queue_last_item)
            {
                save_item = instance->bfs_queue->next;
                ptr_state_with_locations = save_item->s;
                instance->bfs_queue->next = instance->bfs_queue->next->next;
                free(save_item);
            }
            else
            {
                ptr_state_with_locations = NULL;
            }
        }
        else
        {
            /* A* */
            ptr_state_with_locations = PQueuePop(instance->a_star_pqueue);
        }
    }

    return FCS_STATE_IS_NOT_SOLVEABLE;
}


static int freecell_solver_a_star_or_bfs_solve_for_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig
    )
{
    return freecell_solver_a_star_or_bfs_do_solve_or_resume(
            instance,
            ptr_state_with_locations_orig,
            0
            );
}

static int freecell_solver_a_star_or_bfs_resume_solution(
    freecell_solver_instance_t * instance
    )
{
    return freecell_solver_a_star_or_bfs_do_solve_or_resume(
            instance,
            instance->first_state_to_check,
            1
            );
}


#undef state


#if 1
double freecell_solver_a_star_default_weights[5] = {0.5,0,0.5,0,0};
#else
double freecell_solver_a_star_default_weights[5] = {0.5,0,0.3,0,0.2};
#endif

freecell_solver_instance_t * freecell_solver_alloc_instance(void)
{
    freecell_solver_instance_t * instance;

    int a;

    instance = malloc(sizeof(freecell_solver_instance_t));

    instance->unsorted_prev_states_start_at = 0;
#ifdef INDIRECT_STATE_STORAGE
    instance->num_indirect_prev_states = 0;
    instance->max_num_indirect_prev_states = 0;

    instance->max_num_state_packs = 0;
    instance->num_state_packs = 0;
    instance->num_states_in_last_pack = 0;
    instance->state_pack_len = 0;
#endif

    instance->num_times = 0;

    instance->max_num_times = -1;
    instance->max_depth = -1;

    instance->tests_order_num = FCS_TESTS_NUM;
    for(a=0;a<FCS_TESTS_NUM;a++)
    {
        instance->tests_order[a] = a;
    }

    instance->freecells_num = 4;
    instance->stacks_num = 8;
    instance->decks_num = 1;
 
    instance->sequences_are_built_by = FCS_SEQ_BUILT_BY_ALTERNATE_COLOR;
    instance->unlimited_sequence_move = 0;
    instance->empty_stacks_fill = FCS_ES_FILLED_BY_ANY_CARD;

    instance->debug_iter_output = 0;

    instance->dfs_max_depth = 0;

#if 0
    instance->solution_states = NULL;
    instance->proto_solution_moves = NULL;
#endif
#define SET_TO_NULL(what) instance->what = NULL;
    SET_TO_NULL(solution_states);
    SET_TO_NULL(proto_solution_moves);
    SET_TO_NULL(soft_dfs_states_to_check);
    SET_TO_NULL(soft_dfs_states_to_check_move_stacks);
    SET_TO_NULL(soft_dfs_num_states_to_check);
    SET_TO_NULL(soft_dfs_current_state_indexes);
    SET_TO_NULL(soft_dfs_test_indexes);
    SET_TO_NULL(soft_dfs_current_state_indexes);
    SET_TO_NULL(soft_dfs_max_num_states_to_check);
#undef SET_TO_NULL

    instance->method = FCS_METHOD_HARD_DFS;

    instance->bfs_queue = (fcs_states_linked_list_item_t*)malloc(sizeof(fcs_states_linked_list_item_t));
    instance->bfs_queue->next = (fcs_states_linked_list_item_t*)malloc(sizeof(fcs_states_linked_list_item_t));
    instance->bfs_queue_last_item = instance->bfs_queue->next;
    instance->bfs_queue_last_item->next = NULL;

    instance->a_star_pqueue = malloc(sizeof(PQUEUE));
    PQueueInitialise(
        instance->a_star_pqueue,
        1024,
        INT_MAX,
        1
        );

    for(a=0;a<(sizeof(instance->a_star_weights)/sizeof(instance->a_star_weights[0]));a++)
    {
        instance->a_star_weights[a] = freecell_solver_a_star_default_weights[a];
    }

    return instance;
}

void freecell_solver_free_instance(freecell_solver_instance_t * instance)
{
    free(instance);
}

void freecell_solver_init_instance(freecell_solver_instance_t * instance)
{
    instance->num_prev_states_margin = 0;

#ifdef INDIRECT_STATE_STORAGE
    instance->max_num_indirect_prev_states = PREV_STATES_GROW_BY;

    instance->indirect_prev_states = (fcs_state_with_locations_t * *)malloc(sizeof(fcs_state_with_locations_t *) * instance->max_num_indirect_prev_states);
#endif

    fcs_state_ia_init(instance);

    {
        double sum;
        int a;
        sum = 0;
        for(a=0;a<(sizeof(instance->a_star_weights)/sizeof(instance->a_star_weights[0]));a++)
        {
            if (instance->a_star_weights[a] < 0)
            {
                instance->a_star_weights[a] = freecell_solver_a_star_default_weights[a];
            }
            sum += instance->a_star_weights[a];
        }
        if (sum == 0)
        {
            sum = 1;
        }
        for(a=0;a<(sizeof(instance->a_star_weights)/sizeof(instance->a_star_weights[0]));a++)
        {
            instance->a_star_weights[a] /= sum;
        }
    }
}

#if defined(INDIRECT_STACK_STATES)
extern int fcs_stack_compare_for_comparison(const void * v_s1, const void * v_s2);

#if ((FCS_STACK_STORAGE != FCS_STACK_STORAGE_GLIB_TREE) && (FCS_STACK_STORAGE != FCS_STACK_STORAGE_GLIB_HASH))
static int fcs_stack_compare_for_comparison_with_context(
    const void * v_s1, 
    const void * v_s2, 
#if (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBREDBLACK_TREE)
    const
#endif
    void * context

    )
{
    return fcs_stack_compare_for_comparison(v_s1, v_s2);
}
#endif

#if (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH)
static guint freecell_solver_glib_hash_stack_hash_function (
    gconstpointer key
    )
{
    MD5_CTX md5_context;
    fcs_card_t * stack;
    char hash_value[16];
    
    stack = (fcs_card_t * )key;

    MD5Init(&md5_context);
    MD5Update(
        &md5_context, 
        key, 
        fcs_standalone_stack_len(stack)+1
        );
    MD5Final(hash_value, &md5_context);
    
    return *(guint*)hash_value;
}

static gint freecell_solver_glib_hash_stack_compare (
    gconstpointer a,
    gconstpointer b
)
{
    return !(fcs_stack_compare_for_comparison(a,b));
}
#endif

#endif

#ifdef GLIB_HASH_IMPLEMENTATION
/*
 * This hash function is defined in dfs.c
 * 
 * */
extern guint freecell_solver_hash_function(gconstpointer key);
#endif

int freecell_solver_solve_instance(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * init_state
    )
{
    int ret;
    
    fcs_state_with_locations_t * state_copy_ptr;
    state_copy_ptr = fcs_state_ia_alloc(instance);

    fcs_duplicate_state(*state_copy_ptr, *init_state);

#ifdef TREE_STATE_STORAGE
#ifdef LIBREDBLACK_TREE_IMPLEMENTATION
    instance->tree = rbinit(fcs_state_compare_with_context, NULL);
#elif defined(AVL_AVL_TREE_IMPLEMENTATION) 
    instance->tree = avl_create(fcs_state_compare_with_context, NULL);
#elif defined(AVL_REDBLACK_TREE_IMPLEMENTATION)
    instance->tree = rb_create(fcs_state_compare_with_context, NULL);
#elif defined(GLIB_TREE_IMPLEMENTATION)
    instance->tree = g_tree_new(fcs_state_compare);
#endif
#endif

#ifdef HASH_STATE_STORAGE
#ifdef GLIB_HASH_IMPLEMENTATION
    instance->hash = g_hash_table_new(
        freecell_solver_hash_function,
        fcs_state_compare_equal
        );
#elif defined(INTERNAL_HASH_IMPLEMENTATION)
    instance->hash = SFO_hash_init(
            2048,
            fcs_state_compare_with_context,
            NULL
       );
#endif
#endif

#ifdef INDIRECT_STACK_STATES
#if FCS_STACK_STORAGE == FCS_STACK_STORAGE_INTERNAL_HASH
    instance->stacks_hash = SFO_hash_init(
            2048,
            fcs_stack_compare_for_comparison_with_context,
            NULL
        );
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_AVL_TREE) 
    instance->stacks_tree = avl_create(
            fcs_stack_compare_for_comparison_with_context,
            NULL
            );
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_REDBLACK_TREE)
    instance->stacks_tree = rb_create(
            fcs_stack_compare_for_comparison_with_context,
            NULL
            );
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBREDBLACK_TREE)
    instance->stacks_tree = rbinit(
        fcs_stack_compare_for_comparison_with_context,
        NULL
        );
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_TREE)
    instance->stacks_tree = g_tree_new(fcs_stack_compare_for_comparison);
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH)
    instance->stacks_hash = g_hash_table_new(
        freecell_solver_glib_hash_stack_hash_function,
        freecell_solver_glib_hash_stack_compare
        );
#endif
#endif

#ifdef DB_FILE_STATE_STORAGE
    db_open(
        NULL,
        DB_BTREE,
        O_CREAT|O_RDWR,
        0777,
        NULL,
        NULL,
        &(instance->db)
        );
#endif
    


    if (instance->method == FCS_METHOD_HARD_DFS)
    {
        ret = freecell_solver_solve_for_state(
            instance,
            state_copy_ptr,
            0,
            0);
    }
    else if (instance->method == FCS_METHOD_SOFT_DFS)
    {
        ret = freecell_solver_soft_dfs_solve_for_state(
            instance,
            state_copy_ptr
            );
    }
    else if ((instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR))
    {
        if (instance->method == FCS_METHOD_A_STAR)
        {
            freecell_solver_a_star_initialize_rater(
                instance,
                state_copy_ptr
                );
        }

        ret = freecell_solver_a_star_or_bfs_solve_for_state(
            instance,
            state_copy_ptr
            );
        
    }
    else
    {
        ret = FCS_STATE_IS_NOT_SOLVEABLE;
    }

    if (ret == FCS_STATE_WAS_SOLVED)
    {
        int depth;
        instance->solution_moves = fcs_move_stack_create();
        for(depth=instance->num_solution_states-2;depth>=0;depth--)
        {
            fcs_move_stack_swallow_stack(
                instance->solution_moves,
                instance->proto_solution_moves[depth]
                );
        }
        free(instance->proto_solution_moves);
    }

    return ret;
}

int freecell_solver_resume_instance(
    freecell_solver_instance_t * instance
    )
{
    int ret;
    if (instance->method == FCS_METHOD_HARD_DFS)
    {
        ret = freecell_solver_solve_for_state_resume_solution(instance, 0);
    }
    else if (instance->method == FCS_METHOD_SOFT_DFS)
    {
        ret = freecell_solver_soft_dfs_solve_for_state_resume_solution(
            instance
            );
    }
    else if ((instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR))
    {
        ret = freecell_solver_a_star_or_bfs_resume_solution(
            instance
            );
    }
    else
    {
        ret = FCS_STATE_IS_NOT_SOLVEABLE;
    }

    if (ret == FCS_STATE_WAS_SOLVED)
    {
        int depth;
        instance->solution_moves = fcs_move_stack_create();
        for(depth=instance->num_solution_states-2;depth>=0;depth--)
        {
            fcs_move_stack_swallow_stack(
                instance->solution_moves,
                instance->proto_solution_moves[depth]
                );
        }
        free(instance->proto_solution_moves);
    }

    return ret;
}

void freecell_solver_unresume_instance(
    freecell_solver_instance_t * instance
    )
{
    if ((instance->method == FCS_METHOD_HARD_DFS) || (instance->method == FCS_METHOD_SOFT_DFS))
    {
        int depth;
        for(depth=0;depth<instance->num_solution_states-1;depth++)
        {
            fcs_move_stack_destroy(instance->proto_solution_moves[depth]);
            free(instance->solution_states[depth]);
        }
        /* There's one more state than move stacks */        
        free(instance->solution_states[depth]);

        free(instance->proto_solution_moves);
        free(instance->solution_states);
    }
}


#ifdef TREE_STATE_STORAGE

#if defined(AVL_AVL_TREE_IMPLEMENTATION) || defined(AVL_REDBLACK_TREE_IMPLEMENTATION)

static void freecell_solver_tree_do_nothing(void * data, void * context)
{
}

#endif

#endif

#ifdef INDIRECT_STACK_STATES
#if (FCS_STACK_STORAGE == FCS_STACK_STORAGE_INTERNAL_HASH) || (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_AVL_TREE) || (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_REDBLACK_TREE)
static void freecell_solver_stack_free(void * key, void * context)
{
    free(key);
}

#elif FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBREDBLACK_TREE
static void freecell_solver_libredblack_walk_destroy_stack_action
(
    const void * nodep,
    const VISIT which,
    const int depth,
    void * arg
 )
{
    if ((which == leaf) || (which == preorder))
    {
        free((void*)nodep);
    }
}
#elif FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_TREE
static gint freecell_solver_glib_tree_walk_destroy_stack_action
(
    gpointer key,
    gpointer value,
    gpointer data
)
{
    free(key);
    
    return 0;
}

#elif FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH
static void freecell_solver_glib_hash_foreach_destroy_stack_action
(
    gpointer key,
    gpointer value,
    gpointer data
)
{
    free(key);
}
#endif

#endif

void freecell_solver_destroy_move_stack_of_state(
        fcs_state_with_locations_t * ptr_state_with_locations, 
        void * context
        )
{
    if (ptr_state_with_locations->moves_to_parent != NULL)
    {
        fcs_move_stack_destroy(ptr_state_with_locations->moves_to_parent);
    }
}

void freecell_solver_finish_instance(
    freecell_solver_instance_t * instance
    )
{

#ifdef INDIRECT_STATE_STORAGE        
    free(instance->indirect_prev_states);
#endif
    if ((instance->method == FCS_METHOD_A_STAR) || (instance->method == FCS_METHOD_BFS))
    {
        fcs_state_ia_foreach(instance, freecell_solver_destroy_move_stack_of_state, NULL);
    }               
    fcs_state_ia_finish(instance);

#ifdef TREE_STATE_STORAGE
#ifdef LIBREDBLACK_TREE_IMPLEMENTATION
    rbdestroy(instance->tree);
#elif defined(AVL_AVL_TREE_IMPLEMENTATION) 
    avl_destroy(instance->tree, freecell_solver_tree_do_nothing);
#elif defined(AVL_REDBLACK_TREE_IMPLEMENTATION)
    rb_destroy(instance->tree, freecell_solver_tree_do_nothing);
#elif defined(GLIB_TREE_IMPLEMENTATION)
    g_tree_destroy(instance->tree);
#endif
#endif

#ifdef HASH_STATE_STORAGE
#ifdef GLIB_HASH_IMPLEMENTATION
    g_hash_table_destroy(instance->hash);
#elif defined(INTERNAL_HASH_IMPLEMENTATION)
    SFO_hash_free(instance->hash);
#endif
#endif

#ifdef INDIRECT_STACK_STATES
#if FCS_STACK_STORAGE == FCS_STACK_STORAGE_INTERNAL_HASH
    SFO_hash_free_with_callback(instance->stacks_hash, freecell_solver_stack_free);
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_AVL_TREE) 
    avl_destroy(instance->stacks_tree, freecell_solver_stack_free);
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_REDBLACK_TREE)
    rb_destroy(instance->stacks_tree, freecell_solver_stack_free);
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBREDBLACK_TREE)
    rbwalk(instance->stacks_tree, 
        freecell_solver_libredblack_walk_destroy_stack_action,
        NULL
        );
    rbdestroy(instance->stacks_tree);
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_TREE)
    g_tree_traverse(
        instance->stacks_tree,
        freecell_solver_glib_tree_walk_destroy_stack_action,
        G_IN_ORDER,
        NULL
        );

    g_tree_destroy(instance->stacks_tree);
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH)
    g_hash_table_foreach(
        instance->stacks_hash,
        freecell_solver_glib_hash_foreach_destroy_stack_action,
        NULL
        );
    g_hash_table_destroy(instance->stacks_hash);
#endif
#endif
    
#ifdef DB_FILE_STATE_STORAGE
    instance->db->close(instance->db,0);
#endif

    /* Free the BFS linked list */
    {
        fcs_states_linked_list_item_t * item, * next_item;
        item = instance->bfs_queue;
        while (item != NULL)
        {            
            next_item = item->next;
            free(item);
            item = next_item;
        }
    }

    PQueueFree(instance->a_star_pqueue);
    free(instance->a_star_pqueue);
}

void freecell_solver_soft_dfs_add_state(
    freecell_solver_instance_t * instance,
    int depth,
    fcs_state_with_locations_t * state
    )
{
    instance->solution_states[depth] = state;
}

