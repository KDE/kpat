/* 
 * intrface.c - instance interface functions for Freecell Solver
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if FCS_STATE_STORAGE==FCS_STATE_STORAGE_LIBREDBLACK_TREE
#include <search.h>
#endif


#include "config.h"
#include "state.h"
#include "card.h"
#include "fcs_dm.h"
#include "fcs.h"

#include "fcs_isa.h"

/*
    General use of this interface:
    1. freecell_solver_alloc_instance()
    2. Set the parameters of the game
    3. If you wish to revert, go to step #11.
    4. freecell_solver_init_instance()
    5. Call freecell_solver_solve_instance() with the initial board.
    6. If it returns FCS_STATE_SUSPEND_PROCESS and you wish to proceed,
       then increase the iteration limit and call 
       freecell_solver_resume_instance().
    7. Repeat Step #6 zero or more times.
    8. If the last call to solve_instance() or resume_instance() returned
       FCS_STATE_SUSPEND_PROCESS then call 
       freecell_solver_unresume_instance().
    9. If the solving was successful you can use the move stacks or the
       intermediate stacks. (Just don't destory them in any way).
    10. Call freecell_solver_finish_instance().
    11. Call freecell_solver_free_instance().
       
    The library functions inside lib.c (a.k.a fcs_user()) give an
    easier approach for embedding Freecell Solver into your library. The 
    intent of this comment is to document the code, rather than to be 
    a guideline for the user.
*/

#if 0
static const double freecell_solver_a_star_default_weights[5] = {0.5,0,0.5,0,0};
#else
static const double freecell_solver_a_star_default_weights[5] = {0.5,0,0.3,0,0.2};
#endif







void freecell_solver_initialize_bfs_queue(freecell_solver_instance_t * instance)
{
    /* Initialize the BFS queue. We have one dummy element at the beginning
       in order to make operations simpler. */
    instance->bfs_queue = (fcs_states_linked_list_item_t*)malloc(sizeof(fcs_states_linked_list_item_t));
    instance->bfs_queue->next = (fcs_states_linked_list_item_t*)malloc(sizeof(fcs_states_linked_list_item_t));
    instance->bfs_queue_last_item = instance->bfs_queue->next;
    instance->bfs_queue_last_item->next = NULL;
}


void freecell_solver_clean_soft_dfs(
    freecell_solver_instance_t * instance
    )
{
    /* De-allocate the Soft-DFS specific stacks */
    {    
        int depth, a;
        for(depth=0;depth<instance->num_solution_states-1;depth++)
        {
            for(a=0;a<instance->soft_dfs_num_states_to_check[depth];a++)
            {
                fcs_move_stack_destroy(instance->soft_dfs_states_to_check_move_stacks[depth][a]);
            }
            free(instance->soft_dfs_states_to_check[depth]);
            free(instance->soft_dfs_states_to_check_move_stacks[depth]);            
        }
        for(;depth<instance->dfs_max_depth;depth++)
        {
            if (instance->soft_dfs_max_num_states_to_check[depth] != 0)
            {
                free(instance->soft_dfs_states_to_check[depth]);
                free(instance->soft_dfs_states_to_check_move_stacks[depth]);            
            }
        }

#define MYFREE(what) free(instance->what);
        MYFREE(soft_dfs_states_to_check);
        MYFREE(soft_dfs_states_to_check_move_stacks);
        MYFREE(soft_dfs_num_states_to_check);
        MYFREE(soft_dfs_test_indexes);
        MYFREE(soft_dfs_current_state_indexes);
        MYFREE(soft_dfs_max_num_states_to_check);
        MYFREE(soft_dfs_num_freecells);
        MYFREE(soft_dfs_num_freestacks);
#undef MYFREE
    }    
}





/*
    This function allocates a Freecell Solver instance struct and set the
    default values in it. After the call to this function, the program can
    set parameters in it which are different from the default.
    
    Afterwards freecell_solver_init_instance() should be called in order 
    to really prepare it for solving.
  */
freecell_solver_instance_t * freecell_solver_alloc_instance(void)
{
    freecell_solver_instance_t * instance;

    unsigned int a;

    instance = malloc(sizeof(freecell_solver_instance_t));

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INDIRECT)
    instance->num_indirect_prev_states = 0;
    instance->max_num_indirect_prev_states = 0;
#endif

    instance->num_times = 0;

    instance->max_num_times = -1;
    instance->max_depth = -1;

    /* Initialize the default test order */
    instance->tests_order_num = FCS_TESTS_NUM;
    for(a=0;a<FCS_TESTS_NUM;a++)
    {
        instance->tests_order[a] = a;
    }

    /* All these parameters are for playing Freecell. See fcs.h for 
       explanation on what each is. 
    */

    instance->freecells_num = 4;
    instance->stacks_num = 8;
    instance->decks_num = 1;
 
    instance->sequences_are_built_by = FCS_SEQ_BUILT_BY_ALTERNATE_COLOR;
    instance->unlimited_sequence_move = 0;
    instance->empty_stacks_fill = FCS_ES_FILLED_BY_ANY_CARD;

#ifdef FCS_WITH_TALONS
    instance->talon_type = FCS_TALON_NONE;
#endif

    /****************************************/

    instance->debug_iter_output = 0;

    instance->dfs_max_depth = 0;

    /* Initialize all the Soft-DFS stacks to NULL */
#define SET_TO_NULL(what) instance->what = NULL;
    SET_TO_NULL(solution_moves);
    SET_TO_NULL(solution_states);
    SET_TO_NULL(proto_solution_moves);
    SET_TO_NULL(soft_dfs_states_to_check);
    SET_TO_NULL(soft_dfs_states_to_check_move_stacks);
    SET_TO_NULL(soft_dfs_num_states_to_check);
    SET_TO_NULL(soft_dfs_current_state_indexes);
    SET_TO_NULL(soft_dfs_test_indexes);
    SET_TO_NULL(soft_dfs_current_state_indexes);
    SET_TO_NULL(soft_dfs_max_num_states_to_check);
    SET_TO_NULL(soft_dfs_num_freecells);
    SET_TO_NULL(soft_dfs_num_freestacks);
#undef SET_TO_NULL

    /* The default solving method */
    instance->method = FCS_METHOD_HARD_DFS;

    instance->orig_method = FCS_METHOD_NONE;

    freecell_solver_initialize_bfs_queue(instance);

    /* Initialize the priotity queue of the A* scan */
    instance->a_star_pqueue = malloc(sizeof(PQUEUE));
    PQueueInitialise(
        instance->a_star_pqueue,
        1024,
        INT_MAX,
        1
        );

    /* Set the default A* weigths */
    for(a=0;a<(sizeof(instance->a_star_weights)/sizeof(instance->a_star_weights[0]));a++)
    {
        instance->a_star_weights[a] = freecell_solver_a_star_default_weights[a];
    }

    instance->optimize_solution_path = 0;


    return instance;
}





void freecell_solver_free_bfs_queue(freecell_solver_instance_t * instance)
{
    /* Free the BFS linked list */
    fcs_states_linked_list_item_t * item, * next_item;
    item = instance->bfs_queue;
    while (item != NULL)
    {
        next_item = item->next;
        free(item);
        item = next_item;
    }
}







/*
    This function is the last function that should be called in the
    sequence of operations on instance, and it is meant for de-allocating
    whatever memory was allocated by alloc_instance().
  */
void freecell_solver_free_instance(freecell_solver_instance_t * instance)
{
    freecell_solver_free_bfs_queue(instance);

    PQueueFree(instance->a_star_pqueue);
    free(instance->a_star_pqueue);

    free(instance);
}






void freecell_solver_init_instance(freecell_solver_instance_t * instance)
{
    instance->num_prev_states_margin = 0;

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INDIRECT)
    instance->max_num_indirect_prev_states = PREV_STATES_GROW_BY;

    instance->indirect_prev_states = (fcs_state_with_locations_t * *)malloc(sizeof(fcs_state_with_locations_t *) * instance->max_num_indirect_prev_states);
#endif

    /* Initialize the state packs */
    fcs_state_ia_init(instance);

    /* Normalize the A* Weights, so the sum of all of them would be 1. */
    {
        double sum;
        unsigned int a;
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




/* These are all stack comparison functions to be used for the stacks
   cache when using INDIRECT_STACK_STATES 
*/
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
/* A hash calculation function for use in glib's hash */
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
#endif // (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH)





#endif // defined(INDIRECT_STACK_STATES)





#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_HASH)
/*
 * This hash function is defined in caas.c
 * 
 * */
extern guint freecell_solver_hash_function(gconstpointer key);
#endif





/*
    This functs.ion combines the move stacks of every depth to one big
    move stack.
*/
static void freecell_solver_create_total_moves_stack(
    freecell_solver_instance_t * instance
    )
{
    int depth;
    fcs_move_stack_t * temp_move_stack;
    instance->solution_moves = fcs_move_stack_create();
    /* The moves are inserted from the highest depth to depth 0 in order
       to preserve their order stack-wise 
    */
    for(depth=instance->num_solution_states-2;depth>=0;depth--)
    {
        if (instance->method == FCS_METHOD_SOFT_DFS)
        {
            /* If we are using Soft-DFS, then this move stack is going
               to be de-allocated later, as part of 
               soft_dfs_states_to_check_move_stacks. So let's duplicate
               it */
            temp_move_stack = fcs_move_stack_duplicate(
                instance->proto_solution_moves[depth]
                );
        }
        else
        {
            temp_move_stack = instance->proto_solution_moves[depth];
        }
        fcs_move_stack_swallow_stack(
            instance->solution_moves,
            temp_move_stack
            );
    }
    free(instance->proto_solution_moves);
    instance->proto_solution_moves = NULL;
}

/*
    This function optimizes the solution path using a BFS scan on the 
    states in the solution path.
*/
static int freecell_solver_optimize_solution(
    freecell_solver_instance_t * instance
    )
{
    if (instance->method == FCS_METHOD_SOFT_DFS)
    {
        freecell_solver_clean_soft_dfs(instance);
    }
    instance->orig_method = instance->method;
    instance->method = FCS_METHOD_OPTIMIZE;

    /* Re-init the queue in preparation for the BFS scan */
    freecell_solver_free_bfs_queue(instance);
    freecell_solver_initialize_bfs_queue(instance);

    {
        int d;
        for(d=0;d<instance->num_solution_states-1;d++)
        {
            fcs_clean_state(instance->solution_states[d]);
            free(instance->solution_states[d]);
            fcs_move_stack_destroy(instance->proto_solution_moves[d]);    
        }
        /* There's one more state than move stack */
        fcs_clean_state(instance->solution_states[d]);
        free(instance->solution_states[d]);
        
        free(instance->solution_states);
     
        free(instance->proto_solution_moves);
    }

    return freecell_solver_a_star_or_bfs_solve_for_state(
        instance,
        instance->state_copy_ptr
        );
}


extern void freecell_solver_cache_talon(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * new_state
    );

/*
    This function starts the solution process _for the first time_. If one
    wishes to proceed after the iterations limit was reached, one should
    use freecell_solver_resume_instance.

  */
int freecell_solver_solve_instance(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * init_state
    )
{
    int ret;
    
    fcs_state_with_locations_t * state_copy_ptr;
    
    /* Allocate the first state and initialize it to init_state */
    state_copy_ptr = fcs_state_ia_alloc(instance);

    fcs_duplicate_state(*state_copy_ptr, *init_state);

    instance->state_copy_ptr = state_copy_ptr;

    /* Initialize the data structure that will manage the state collection */
#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBREDBLACK_TREE)    
    instance->tree = rbinit(fcs_state_compare_with_context, NULL);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_AVL_TREE)
    instance->tree = avl_create(fcs_state_compare_with_context, NULL);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE)
    instance->tree = rb_create(fcs_state_compare_with_context, NULL);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_TREE)
    instance->tree = g_tree_new(fcs_state_compare);
#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_HASH)
    instance->hash = g_hash_table_new(
        freecell_solver_hash_function,
        fcs_state_compare_equal
        );
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
    instance->hash = SFO_hash_init(
            2048,
            fcs_state_compare_with_context,
            NULL
       );
#endif

    /****************************************************/

#ifdef INDIRECT_STACK_STATES
    /* Initialize the data structure that will manage the stack
       collection */
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

    /***********************************************/

#ifdef FCS_WITH_TALONS
    /* Initialize the Talon's Cache */
    if (instance->talon_type == FCS_TALON_KLONDIKE)
    {
        instance->talons_hash = SFO_hash_init(
            512,
            fcs_talon_compare_with_context,
            NULL
            );

        freecell_solver_cache_talon(instance, instance->state_copy_ptr);
    }
#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_DB_FILE)
    /* Not working - ignore */
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
    


    /* Call the solving function that is specific to each scan */
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
        if (instance->optimize_solution_path)
        {
            ret = freecell_solver_optimize_solution(instance);
        }
    }

    if (ret == FCS_STATE_WAS_SOLVED)
    {
        freecell_solver_create_total_moves_stack(instance);
    }
    return ret;
}





/* Resume a solution process that was stopped in the middle */
int freecell_solver_resume_instance(
    freecell_solver_instance_t * instance
    )
{
    int ret;
    /* Call the resume function that is specific to each scan */
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
    else if ((instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_A_STAR) || (instance->method == FCS_METHOD_OPTIMIZE))
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
        if (instance->optimize_solution_path)
        {
            freecell_solver_optimize_solution(instance);
        }
        freecell_solver_create_total_moves_stack(instance);
    }

    return ret;
}



/*
    Clean up a solving process that was terminated in the middle. 
    This function does not substitute for later calling 
    finish_instance() and free_instance().
  */
void freecell_solver_unresume_instance(
    freecell_solver_instance_t * instance
    )
{
    if ((instance->method == FCS_METHOD_HARD_DFS) || (instance->method == FCS_METHOD_SOFT_DFS))
    {
        if (instance->method == FCS_METHOD_HARD_DFS)
        {
            /* Free the intermediate states and move stacks of the DFS scan */
            int depth;
            for(depth=0;depth<instance->num_solution_states-1;depth++)
            {
                free(instance->solution_states[depth]);
                fcs_move_stack_destroy(instance->proto_solution_moves[depth]);
            }
            /* There's one more state than move stacks */
            free(instance->solution_states[depth]);
        }
        
        free(instance->proto_solution_moves);
        instance->proto_solution_moves = NULL;
        free(instance->solution_states);
        instance->solution_states = NULL;
    }
}


#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_AVL_TREE) || (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE)

static void freecell_solver_tree_do_nothing(void * data, void * context)
{
}

#endif


/* A function for freeing a stack for the cleanup of the 
   stacks collection 
*/
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

/***********************************************************/




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

/*
    This function should be called after the user has retrieved the
    results generated by the scan as it will destroy them.
  */
void freecell_solver_finish_instance(
    freecell_solver_instance_t * instance
    )
{
    fcs_clean_state(instance->state_copy_ptr);

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INDIRECT)
    free(instance->indirect_prev_states);
#endif

    /* Destroy all the intermediate move stacks in the solution graph */
    if ((instance->method == FCS_METHOD_A_STAR) || (instance->method == FCS_METHOD_BFS) || (instance->method == FCS_METHOD_OPTIMIZE))
    {
        fcs_state_ia_foreach(instance, freecell_solver_destroy_move_stack_of_state, NULL);
    }
    /* De-allocate the state packs */
    fcs_state_ia_finish(instance);
    
    /* De-allocate the state collection */
#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBREDBLACK_TREE)
    rbdestroy(instance->tree);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_AVL_TREE)
    avl_destroy(instance->tree, freecell_solver_tree_do_nothing);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE)
    rb_destroy(instance->tree, freecell_solver_tree_do_nothing);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_TREE)
    g_tree_destroy(instance->tree);
#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_HASH)
    g_hash_table_destroy(instance->hash);
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
    SFO_hash_free(instance->hash);
#endif



    /* De-allocate the stack collection while free()'ing the stacks
    in the process */
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

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_DB_FILE)
    instance->db->close(instance->db,0);
#endif


    if (instance->method == FCS_METHOD_SOFT_DFS)
    {
        freecell_solver_clean_soft_dfs(instance);
    }


    
    
    
    if (instance->proto_solution_moves != NULL)
    {
        free(instance->proto_solution_moves);
        instance->proto_solution_moves = NULL;
    }
    
    if (instance->solution_states != NULL)
    {
        free(instance->solution_states);
        instance->solution_states = NULL;
    }
    
}
