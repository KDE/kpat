/*
 * scans.c - The code that relates to the various scans.
 * Currently Hard DFS, Soft-DFS, A* and BFS are implemented.
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

    int iter_num = instance->num_times;
    instance->num_times++;


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

        /* Mark this state as part of the non-optimized solution path */
        ptr_state_with_locations->visited |= FCS_VISITED_IN_SOLUTION_PATH;
        instance->solution_states[depth] = malloc(sizeof(fcs_state_with_locations_t)); 
        fcs_duplicate_state(*(instance->solution_states[depth]), *ptr_state_with_locations); 

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
            /* Mark this state as part of the non-optimized solution path */
            ptr_state_with_locations->visited |= FCS_VISITED_IN_SOLUTION_PATH;

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


int freecell_solver_solve_for_state_resume_solution(
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
        instance->solution_states = NULL;
        free(instance->proto_solution_moves);
        instance->proto_solution_moves = NULL;
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
    MYREALLOC(soft_dfs_max_num_states_to_check);
    MYREALLOC(soft_dfs_num_freestacks);
    MYREALLOC(soft_dfs_num_freecells);
#undef MYREALLOC
    
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

/*
    freecell_solver_soft_dfs_do_solve_or_resume is the event loop of the Soft-DFS
    scan. DFS which is recursive in nature is handled here without procedural recursion
    by using some dedicated stacks for the traversal.

  */
#define state (ptr_state_with_locations->s)

static int freecell_solver_soft_dfs_do_solve_or_resume(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig,
    int resume
    )
{
    int depth;
    fcs_state_with_locations_t * ptr_state_with_locations, 
        * ptr_recurse_into_state_with_locations;
    int a;
    int check;

    if (!resume)
    {
        /*
            Allocate some space for the states at depth 0. 
        */
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
        /*
            Set the initial depth to that of the last state encountered.
        */
        depth = instance->num_solution_states - 1;
    }

    /*
        The main loop.
    */
    while (depth >= 0)
    {
        /*
            Increase the "maximal" depth if it about to be exceeded.
        */
        if (depth+1 >= instance->dfs_max_depth)
        {
            freecell_solver_increase_dfs_max_depth(instance);
        }
        
        /* All the resultant states in the last test conducted were covered */
        if (instance->soft_dfs_current_state_indexes[depth] == instance->soft_dfs_num_states_to_check[depth])
        {
        
            if (instance->soft_dfs_test_indexes[depth] >= instance->tests_order_num)
            {
                /*
                    Destroy the move stacks that were left by the last test conducted in this
                    depth 
                */
                for(a=0;a<instance->soft_dfs_num_states_to_check[depth];a++)
                {
                    fcs_move_stack_destroy(instance->soft_dfs_states_to_check_move_stacks[depth][a]);
                }
                /* Backtrack to the previous depth. */
                depth--;
                continue; /* Just to make sure depth is not -1 now */    
            }        
        
        
            /* Destroy the move stacks that were left by the previous test */
            if (instance->soft_dfs_test_indexes[depth] != 0)
            {
                for(a=0;a<instance->soft_dfs_num_states_to_check[depth];a++)
                {
                    fcs_move_stack_destroy(instance->soft_dfs_states_to_check_move_stacks[depth][a]);
                }            
            }
            
            instance->soft_dfs_num_states_to_check[depth] = 0;
        
            ptr_state_with_locations = instance->solution_states[depth];
        

            /* If this is the first test, then count the number of unoccupied
               freeceelsc and stacks and check if we are done. */
            if (instance->soft_dfs_test_indexes[depth] == 0)
            {
                int num_freestacks, num_freecells;

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

                /* Check if we have reached the empty state */
                if ((num_freestacks == instance->stacks_num) && (num_freecells == instance->freecells_num))
                {
                    int d;
                    fcs_state_with_locations_t * ptr_state;
        
                    /*
                        Copy all the states to a separetely malloced memory, so they won't 
                        be lost when the state packs are de-allocated.
                    */
                    instance->num_solution_states = depth+1;
                    for(d=0;d<=depth;d++)
                    {
                        /* Mark the states in the solution path as part of the non-optimized
                           solution
                        */
                        instance->solution_states[d]->visited |= FCS_VISITED_IN_SOLUTION_PATH;
                        ptr_state = (fcs_state_with_locations_t *)malloc(sizeof(fcs_state_with_locations_t));
                        fcs_duplicate_state(*ptr_state, *(instance->solution_states[d]));
                        instance->solution_states[d] = ptr_state;
                        if (d < depth)
                        {
                            /* So it won't be destroyed twice because of the path optimization */
                            instance->proto_solution_moves[d] = fcs_move_stack_duplicate(instance->proto_solution_moves[d]);
                        }
                    }
                    return FCS_STATE_WAS_SOLVED;
                }
                /*
                    Cache num_freecells and num_freestacks in their 
                    appropriate stacks, so they won't be calculated over and over
                    again.
                  */
                instance->soft_dfs_num_freecells[depth] = num_freecells;
                instance->soft_dfs_num_freestacks[depth] = num_freestacks;
            }
            

        
        
            check = freecell_solver_sfs_tests[instance->tests_order[
                    instance->soft_dfs_test_indexes[depth]
                ]] (
                    instance,
                    ptr_state_with_locations,
                    depth,
                    instance->soft_dfs_num_freestacks[depth],
                    instance->soft_dfs_num_freecells[depth],
                    1   /* Pass TRUE as ignore_osins */
                );
        
            if ((check == FCS_STATE_BEGIN_SUSPEND_PROCESS) ||
                (check == FCS_STATE_EXCEEDS_MAX_NUM_TIMES) ||
                (check == FCS_STATE_SUSPEND_PROCESS))
            {
                instance->num_solution_states = depth+1;
                return FCS_STATE_SUSPEND_PROCESS;
            }
        
            /* Move the counter to the next test */
            instance->soft_dfs_test_indexes[depth]++;
        
            /* We just performed a test, so the index of the first state that
               ought to be checked in this depth is 0.
               */
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
                ptr_recurse_into_state_with_locations->visited = FCS_VISITED_VISITED;
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

int freecell_solver_soft_dfs_solve_for_state(
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

int freecell_solver_soft_dfs_solve_for_state_resume_solution(
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
        num_cards_in_founds += fcs_foundation_value(state, a);
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






/*
    freecell_solver_a_star_or_bfs_do_solve_or_resume() is the main event
    loop of the A* And BFS scans. It is quite simple as all it does is
    extract elements out of the queue or priority queue and run all the test
    of them. 
    
    It goes on in this fashion until the final state was reached or 
    there are no more states in the queue.
*/
int freecell_solver_a_star_or_bfs_do_solve_or_resume(
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
        /* Initialize the first element to indicate it is the first */
        ptr_state_with_locations_orig->parent = NULL;
        ptr_state_with_locations_orig->moves_to_parent = NULL;
        ptr_state_with_locations_orig->depth = 0;
    }

    ptr_state_with_locations = ptr_state_with_locations_orig;
    
    /* Continue as long as there are states in the queue or 
       priority queue. */
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


        if ((num_freestacks == instance->stacks_num) && (num_freecells == instance->freecells_num))
        {
            /*
                Trace the solution.
            */
            int num_solution_states;
            fcs_state_with_locations_t * s1;

            s1 = ptr_state_with_locations;

            /* The depth of the last state reached is the number of them */
            num_solution_states = instance->num_solution_states = s1->depth+1;
            /* Allocate space for the solution stacks */
            instance->solution_states = malloc(sizeof(fcs_state_with_locations_t *)*num_solution_states);
            instance->proto_solution_moves = malloc(sizeof(fcs_move_stack_t *) * (num_solution_states-1));
            /* Retrace the step from the current state to its parents */
            while (s1->parent != NULL)
            {
                /* Mark the state as part of the non-optimized solution */
                s1->visited |= FCS_VISITED_IN_SOLUTION_PATH;
                /* Duplicate the move stack */
                instance->proto_solution_moves[s1->depth-1] = fcs_move_stack_duplicate(s1->moves_to_parent);
                /* Duplicate the state to a freshly malloced memory */
                instance->solution_states[s1->depth] = (fcs_state_with_locations_t*)malloc(sizeof(fcs_state_with_locations_t));
                fcs_duplicate_state(*(instance->solution_states[s1->depth]), *s1);
        
                /* Move to the parent state */
                s1 = s1->parent;
            }
            /* There's one more state that there are move stacks */
            instance->solution_states[0] = (fcs_state_with_locations_t*)malloc(sizeof(fcs_state_with_locations_t));
            fcs_duplicate_state(*(instance->solution_states[0]), *s1);
            
            return FCS_STATE_WAS_SOLVED;
        }

        /* Increase the number of iterations by one */
        instance->num_times++;

        /* Do all the tests at one go, because that the way it should be
           done for BFS and A* 
        */
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
                /* Save the current position in the scan */
                instance->first_state_to_check = ptr_state_with_locations;
                return FCS_STATE_SUSPEND_PROCESS;
            }
        }

        /*
            Extract the next item in the queue/priority queue.
        */
        if ((instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_OPTIMIZE))
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
            /* It is an A* scan */
            ptr_state_with_locations = PQueuePop(instance->a_star_pqueue);
        }
    }

    return FCS_STATE_IS_NOT_SOLVEABLE;
}


int freecell_solver_a_star_or_bfs_solve_for_state(
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

int freecell_solver_a_star_or_bfs_resume_solution(
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
