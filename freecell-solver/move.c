/*
 * move.c - move and move stacks routines for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "move.h"
#include "state.h"

#define FCS_MOVE_STACK_GROW_BY 16

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

fcs_move_stack_t * fcs_move_stack_create(void)
{
    fcs_move_stack_t * ret;
    
    ret = (fcs_move_stack_t *)malloc(sizeof(fcs_move_stack_t));

    ret->max_num_moves = FCS_MOVE_STACK_GROW_BY;
    ret->num_moves = 0;
    ret->moves = (fcs_move_t *)malloc(sizeof(fcs_move_t)*ret->max_num_moves);

    return ret;
}

int fcs_move_stack_push(fcs_move_stack_t * stack, fcs_move_t move)
{
    if (stack->num_moves == stack->max_num_moves)
    {
        int a, b;
        a = (stack->max_num_moves >> 3);
        b = FCS_MOVE_STACK_GROW_BY;
        stack->max_num_moves += max(a,b);
        stack->moves = realloc(
            stack->moves, 
            stack->max_num_moves * sizeof(fcs_move_t)
            );
    }
    stack->moves[stack->num_moves++] = move;

    return 0;
}

int fcs_move_stack_pop(fcs_move_stack_t * stack, fcs_move_t * move)
{
    if (stack->num_moves > 0)
    {
        *move = stack->moves[--stack->num_moves];
        return 0;
    }
    else
    {
        return 1;
    }
}

void fcs_move_stack_destroy(fcs_move_stack_t * stack)
{
    free(stack->moves);
    free(stack);
}

void fcs_move_stack_swallow_stack(
    fcs_move_stack_t * stack, 
    fcs_move_stack_t * src_stack
    )
{
    fcs_move_t move;
    while (!fcs_move_stack_pop(src_stack, &move))
    {
        fcs_move_stack_push(stack, move);
    }
    fcs_move_stack_destroy(src_stack);
}

void fcs_move_stack_reset(
    fcs_move_stack_t * stack
    )
{
    stack->num_moves = 0;
}

int fcs_move_stack_get_num_moves(
    fcs_move_stack_t * stack
    )
{
    return stack->num_moves;
}

fcs_move_stack_t * fcs_move_stack_duplicate(
    fcs_move_stack_t * stack
    )
{
    fcs_move_stack_t * ret;

    ret = (fcs_move_stack_t *)malloc(sizeof(fcs_move_stack_t));
    
    ret->max_num_moves = stack->max_num_moves;
    ret->num_moves = stack->num_moves;
    ret->moves = (fcs_move_t *)malloc(sizeof(fcs_move_t) * ret->max_num_moves);
    memcpy(ret->moves, stack->moves, sizeof(fcs_move_t) * ret->max_num_moves);

    return ret;
}

void fcs_apply_move(fcs_state_with_locations_t * state_with_locations, fcs_move_t move, int freecells_num, int stacks_num, int decks_num)
{
    fcs_state_t * state;
    fcs_card_t temp_card;
    int a;
    int src_stack, dest_stack;
    int src_freecell, dest_freecell;
    int src_stack_len;

    state = (&(state_with_locations->s));

    dest_stack = fcs_move_get_dest_stack(move);
    src_stack = fcs_move_get_src_stack(move);
    dest_freecell = fcs_move_get_dest_freecell(move);
    src_freecell = fcs_move_get_src_freecell(move);


    switch(fcs_move_get_type(move))
    {
        case FCS_MOVE_TYPE_STACK_TO_STACK:
        {
            src_stack_len = fcs_stack_len(*state, src_stack);
            for(a=0 ; a<fcs_move_get_num_cards_in_seq(move) ; a++)
            {
                fcs_push_stack_card_into_stack(
                    *state, 
                    dest_stack,
                    src_stack,
                    src_stack_len - fcs_move_get_num_cards_in_seq(move)+a
                    );            
            }
            for(a=0 ; a<fcs_move_get_num_cards_in_seq(move) ; a++)
            {
                fcs_pop_stack_card(
                    *state,
                    src_stack,
                    temp_card
                    );
            }        
        }
        break;
        case FCS_MOVE_TYPE_FREECELL_TO_STACK:
        {
            temp_card = fcs_freecell_card(*state, src_freecell);
            fcs_push_card_into_stack(*state, dest_stack, temp_card);
            fcs_empty_freecell(*state, src_freecell);
        }
        break;
        case FCS_MOVE_TYPE_FREECELL_TO_FREECELL:
        {
            temp_card = fcs_freecell_card(*state, src_freecell);
            fcs_put_card_in_freecell(*state, dest_freecell, temp_card);
            fcs_empty_freecell(*state, src_freecell);
        }
        break;
        case FCS_MOVE_TYPE_STACK_TO_FREECELL:
        {
            fcs_pop_stack_card(*state, src_stack, temp_card);
            fcs_put_card_in_freecell(*state, dest_freecell, temp_card);
        }
        break;
        case FCS_MOVE_TYPE_STACK_TO_FOUNDATION:
        {
            fcs_pop_stack_card(*state, src_stack, temp_card);
            fcs_increment_deck(*state, fcs_move_get_foundation(move));
        }
        break;
        case FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION:
        {
            fcs_empty_freecell(*state, src_freecell);
            fcs_increment_deck(*state, fcs_move_get_foundation(move));        
        }
        break;
        case FCS_MOVE_TYPE_CANONIZE:
        {
            fcs_canonize_state(state_with_locations, freecells_num, stacks_num);
        }
        break;
    }
}

void fcs_move_stack_normalize(
    fcs_move_stack_t * moves,
    fcs_state_with_locations_t * init_state,
    int freecells_num,
    int stacks_num,
    int decks_num
    )
{
    fcs_move_stack_t * temp_moves;
    fcs_move_t in_move, out_move;
    fcs_state_with_locations_t dynamic_state;

    temp_moves = fcs_move_stack_create();

    fcs_duplicate_state(dynamic_state, *init_state);

    while (
        fcs_move_stack_pop(
            moves,
            &in_move
            ) == 0)
    {
        fcs_apply_move(
            &dynamic_state,
            in_move,
            freecells_num,
            stacks_num,
            decks_num
            );
        if (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_CANONIZE)
        {
            /* Do Nothing */
        }
        else
        {
            fcs_move_set_type(out_move, fcs_move_get_type(in_move));
            if ((fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_STACK) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_FREECELL) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_FOUNDATION))
            {
                fcs_move_set_src_stack(out_move,dynamic_state.stack_locs[(int)fcs_move_get_src_stack(in_move)]);
            }
            
            if (
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_FREECELL_TO_STACK) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_FREECELL_TO_FREECELL) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION))
            {
                fcs_move_set_src_freecell(out_move,dynamic_state.fc_locs[(int)fcs_move_get_src_freecell(in_move)]);
            }

            if (
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_STACK) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_FREECELL_TO_STACK)
                )
            {
                fcs_move_set_dest_stack(out_move,dynamic_state.stack_locs[(int)fcs_move_get_dest_stack(in_move)]);
            }

            if (
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_FREECELL) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_FREECELL_TO_FREECELL)
                )
            {
                fcs_move_set_dest_freecell(out_move,dynamic_state.fc_locs[(int)fcs_move_get_dest_freecell(in_move)]);
            }

            if ((fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_FOUNDATION) ||
                (fcs_move_get_type(in_move) == FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION))
            {
                fcs_move_set_foundation(out_move,fcs_move_get_foundation(in_move));
            }

            if ((fcs_move_get_type(in_move) == FCS_MOVE_TYPE_STACK_TO_STACK))
            {
                fcs_move_set_num_cards_in_seq(out_move,fcs_move_get_num_cards_in_seq(in_move));
            }

            fcs_move_stack_push(temp_moves, out_move);
        }
    }

    /*
     * temp_moves contain the needed moves in reverse order. So let's use
     * swallow_stack to put them in the original in the correct order.
     * 
     * */
    fcs_move_stack_reset(moves);

    fcs_move_stack_swallow_stack(moves, temp_moves);
}

char * fcs_move_to_string(fcs_move_t move)
{
    char string[256];
    switch(fcs_move_get_type(move))
    {
        case FCS_MOVE_TYPE_STACK_TO_STACK:
            sprintf(string, "Move %i cards from stack %i to stack %i",
                fcs_move_get_num_cards_in_seq(move),
                fcs_move_get_src_stack(move),
                fcs_move_get_dest_stack(move)
                );
            
        break;

        case FCS_MOVE_TYPE_FREECELL_TO_STACK:
            sprintf(string, "Move a card from freecell %i to stack %i",
                fcs_move_get_src_freecell(move),
                fcs_move_get_dest_stack(move)
                );
            
        break;

        case FCS_MOVE_TYPE_FREECELL_TO_FREECELL:
            sprintf(string, "Move a card from freecell %i to freecell %i",
                fcs_move_get_src_freecell(move),
                fcs_move_get_dest_freecell(move)
                );

        break;

        case FCS_MOVE_TYPE_STACK_TO_FREECELL:
            sprintf(string, "Move a card from stack %i to freecell %i",
                fcs_move_get_src_stack(move),
                fcs_move_get_dest_freecell(move)
                );

        break;

        case FCS_MOVE_TYPE_STACK_TO_FOUNDATION:
            sprintf(string, "Move a card from stack %i to the foundations",
                fcs_move_get_src_stack(move)
                );

        break;
        

        case FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION:
            sprintf(string, "Move a card from freecell %i to the foundations",
                fcs_move_get_src_freecell(move)
                );

        break;

        default:
            string[0] = '\0';
        break;
    }

    return strdup(string);
    
}
