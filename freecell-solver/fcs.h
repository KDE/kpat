/*
 * fcs.h - header file of freecell_solver_instance and of user-level
 * functions for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __FCS_H
#define __FCS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "state.h"
#include "move.h"
#include "fcs_enums.h"

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBREDBLACK_TREE) || (defined(INDIRECT_STACK_STATES) && (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBREDBLACK_TREE))

#include <redblack.h>

#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_AVL_TREE) || (defined(INDIRECT_STACK_STATES) && (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_AVL_TREE))

#include <avl.h>

#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE) || (defined(INDIRECT_STACK_STATES) && (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_REDBLACK_TREE))

#include <rb.h>

/* #define TREE_IMP_PREFIX(func_name) rb_##func_name */

#endif


#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_TREE) || (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_HASH) || (defined(INDIRECT_STACK_STATES) && ((FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_TREE) || (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH)))

#include <glib.h>

#endif


#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
#include "fcs_hash.h"
#include "md5.h"
#endif

#ifdef INDIRECT_STACK_STATES
#include "fcs_hash.h"
#include "md5.h"
#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_DB_FILE)
#include <sys/types.h>
#include <limits.h>
#include <db.h>
#endif

#include "pqueue.h"

/*
 * This is a linked list item that is used to implement a queue for the BFS
 * scan.
 * */
struct fcs_states_linked_list_item_struct
{
    fcs_state_with_locations_t * s;
    struct fcs_states_linked_list_item_struct * next;
};

typedef struct fcs_states_linked_list_item_struct fcs_states_linked_list_item_t;



#ifdef FCS_WITH_TALONS
#define FCS_TESTS_NUM 12
#else
#define FCS_TESTS_NUM 10
#endif

typedef struct freecell_solver_instance
{
    /* The sort-margin */
    fcs_state_with_locations_t * indirect_prev_states_margin[PREV_STATES_SORT_MARGIN];
    
    /* The number of states in the sort margin */
    int num_prev_states_margin;    

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INDIRECT)
    /* The sorted cached states, their number and their maximal size.
     * max_num_indirect_prev_states may increase as the 
     * indirect_prev_states is realloced.
     * */
    fcs_state_with_locations_t * * indirect_prev_states;
    int num_indirect_prev_states;
    int max_num_indirect_prev_states;
#endif    

    /*
     * The State Packs variables are used by all the state cache 
     * management routines. A pack stores as many states as can fit
     * in a 64KB segment, and those variables manage an array of
     * such packs.
     * 
     * Such allocation is possible, because at the worst situation 
     * the last state is released.
     * */
    fcs_state_with_locations_t * * state_packs;
    int max_num_state_packs;
    int num_state_packs;
    int num_states_in_last_pack;
    int state_pack_len;
  
    /* The number of states that were checked by the solving algorithm.
     * Badly named, should be renamed to num_iters or num_checked_states */
    int num_times;
    /*
     * A vector of the states leading to the solution. It is actively
     * being modified by a Soft-DFS scan. Its usage for output has been
     * depreciated because solution_moves was introduced.
     * */
    fcs_state_with_locations_t * * solution_states;
    /*
     * The number of states in solution_states.
     * */
    int num_solution_states;
    
    /* 
     * A move stack that contains the moves leading to the solution.
     *
     * It is created only after the solution was found by swallowing 
     * all the stacks of each depth.
     * */
    fcs_move_stack_t * solution_moves;
    /*
     * proto_solution_moves[i] are the moves that lead from 
     * solution_states[i] to solution_states[i+1].     
     * */
    fcs_move_stack_t * * proto_solution_moves;

    /*
     * Limits for the maximal depth and for the maximal number of checked
     * states. max_num_times is useful because it enables the process to
     * stop before it consumes too much memory.
     *
     * max_depth is quite dangerous because it blocks some intermediate moves
     * and doesn't allow a program to fully reach its solution.
     *
     * */
    int max_depth;
    int max_num_times;

    /*
     * The debug_iter_output variables provide a programmer programmable way
     * to debug the algorithm while it is running. This works well for DFS
     * and Soft-DFS scans but at present support for A* and BFS is not
     * too good, as its hard to tell which state came from which parent state.
     *
     * debug_iter_output is a flag that indicates whether to use this feature
     * at all.
     *
     * debug_iter_output_func is a pointer to the function that performs the
     * debugging.
     *
     * debug_iter_output_context is a user-specified context for it, that
     * may include data that is not included in the instance structure.
     *
     * This feature is used by the "-s" and "-i" flags of fc-solve-debug.
     * */
    int debug_iter_output;
    void (*debug_iter_output_func)(
        void * context, 
        int iter_num, 
        int depth, 
        void * instance,
        fcs_state_with_locations_t * state
        );
    void * debug_iter_output_context;
   
    /*
     * The tests' order indicates which tests (i.e: kinds of multi-moves) to 
     * do at what order. This is at most relevant to DFS and Soft-DFS.
     *
     * tests_order_num is the number of tests in the test's order. Notice
     * that it can be lower than FCS_TESTS_NUM, thus enabling several tests
     * to be removed completely.
     * */
    int tests_order_num;
    int tests_order[FCS_TESTS_NUM];

    /*
     * tree is the balanced binary tree that is used to store and index
     * the checked states.
     *
     * */
#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBREDBLACK_TREE)
    struct rbtree * tree;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_AVL_TREE)
    avl_tree * tree;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE)
    rb_tree * tree;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_TREE)
    GTree * tree;
#endif

    /*
     * hash is the hash table that is used to store the previous
     * states of the scan.
     * */
#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_HASH)
    GHashTable * hash;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
    SFO_hash_t * hash;
#endif

    /*
     * These variables are used to compute the MD5 checksum of a state
     * that is about to be checked. I decided to make them globals so 
     * they won't have to be re-allocated and freed all the time.
     *
     * Notice that it is only used with my internal hash implmentation
     * as GLib requires a dedicated hash function, which cannot 
     * access the instance.
     *
     * */
#if (defined(INDIRECT_STACK_STATES) && (FCS_STACK_STORAGE == FCS_STACK_STORAGE_INTERNAL_HASH)) || (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
    MD5_CTX md5_context;
    unsigned char hash_value[MD5_HASHBYTES];    
#endif

    /*
     * The storage mechanism for the stacks assuming INDIRECT_STACK_STATES is
     * used. 
     * */
#if defined(INDIRECT_STACK_STATES)
#if (FCS_STACK_STORAGE == FCS_STACK_STORAGE_INTERNAL_HASH)
    SFO_hash_t * stacks_hash;
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_AVL_TREE)
    avl_tree * stacks_tree;
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBAVL_REDBLACK_TREE)
    rb_tree * stacks_tree;
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_LIBREDBLACK_TREE)
    struct rbtree * stacks_tree;
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_TREE)
    GTree * stacks_tree;
#elif (FCS_STACK_STORAGE == FCS_STACK_STORAGE_GLIB_HASH)
    GHashTable * stacks_hash;
#endif
#endif

    /*
     * Storing using Berkeley DB is not operational for some reason so
     * pay no attention to it for the while
     * */
#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_DB_FILE)
    DB * db;
#endif  

    /*
     * The number of Freecells, Stacks and Foundations present in the game.
     *
     * freecells_num and stacks_num are variable and may be specified at
     * the beginning of the execution of the algorithm. However, there
     * is a maximal limit to them which is set in config.h.
     * 
     * decks_num can be 4 or 8
     * */
    int freecells_num;
    int stacks_num;
    int decks_num;

    /* What two adjacent cards in the same sequence can be: */
    int sequences_are_built_by;
    /* Whether an entire sequence can be moved from one place to the
     * other regardless of the number of unoccupied Freecells there are. */
    int unlimited_sequence_move;
    /*
     * With what cards can empty stacks be filled with.
     * */
    int empty_stacks_fill;


    /*
     * The (temporary) max depth of the Soft-DFS scans)
     * */
    int dfs_max_depth;
    /* 
     * The method (i.e: DFS, Soft-DFS, BFS or A*) that is used by this
     * instance.
     *
     * */
    int method;

    /* 
     * A place-holder for the original method of the scan in case 
     * it is replaced by FCS_METHOD_OPTIMIZE
     *
     * */
    int orig_method;

    /*
     * A linked list that serves as the queue for the BFS scan.
     * */
    fcs_states_linked_list_item_t * bfs_queue;
    /*
     * The last item in the linked list, so new items can be added at it,
     * thus making it a queue.
     * */
    fcs_states_linked_list_item_t * bfs_queue_last_item;

    /* 
     * The priority queue of the A* scan */
    PQUEUE * a_star_pqueue;
    double a_star_initial_cards_under_sequences;

    /*
     * The A* weights of the different A* tests. Those weights determine the
     * commulative weight of the state.
     * 
     * */
    double a_star_weights[5];

    /*
     * The first state to be checked by the scan. It is a kind of bootstrap
     * for the algorithm.
     * */
    fcs_state_with_locations_t * first_state_to_check;

    /*
     * These are stacks used by the Soft-DFS for various uses.
     *
     * states_to_check[i] - an array of states to be checked next. Not all
     * of them will be checked because it is possible that future states
     * already visited them.
     *
     * states_to_check_move_stacks[i] - an array of move stacks that lead
     * to those states.
     *
     * num_states_to_check[i] - the size of states_to_check[i]
     *
     * max_num_states_to_check[i] - the limit of pointers that can be
     * placed in states_to_check[i] without resizing.
     *
     * current_state_indexes[i] - the index of the last checked state
     * in depth i.
     *
     * test_indexes[i] - the index of the test that was last performed. 
     * FCS performs each test separately, so states_to_check[i] and
     * friends will not be overpopulated.
     *
     * num_freestacks[i] - the number of unoccpied stacks that correspond
     * to solution_states[i].
     *
     * num_freecells[i] - ditto for the freecells.
     *
     * */
    fcs_state_with_locations_t * * * soft_dfs_states_to_check;
    fcs_move_stack_t * * * soft_dfs_states_to_check_move_stacks;
    int * soft_dfs_num_states_to_check;
    int * soft_dfs_max_num_states_to_check;
    int * soft_dfs_current_state_indexes;
    int * soft_dfs_test_indexes;
    int * soft_dfs_num_freestacks;
    int * soft_dfs_num_freecells;

#ifdef FCS_WITH_TALONS
    /*
     * The talon for Gypsy-like games. Since only the position changes from
     * state to state.
     * We can keep it here.
     *
     * */
    fcs_card_t * gypsy_talon;

    /*
     * The length of the Gypsy talon 
     * */
    int gypsy_talon_len;

    int talon_type;
    
    /* The Klondike Talons' Cache */
    SFO_hash_t * talons_hash; 

#endif

    /* A flag that indicates whether to optimize the solution path 
       at the end of the scan */
    int optimize_solution_path;

    /* This is a place-holder for the initial state */
    fcs_state_with_locations_t * state_copy_ptr;
} freecell_solver_instance_t;

#define FCS_SOFT_DFS_STATES_TO_CHECK_GROW_BY 32

/*
 * An enum that specifies the meaning of each A* weight.
 * */
#define FCS_A_STAR_WEIGHT_CARDS_OUT 0
#define FCS_A_STAR_WEIGHT_MAX_SEQUENCE_MOVE 1
#define FCS_A_STAR_WEIGHT_CARDS_UNDER_SEQUENCES 2
#define FCS_A_STAR_WEIGHT_SEQS_OVER_RENEGADE_CARDS 3
#define FCS_A_STAR_WEIGHT_DEPTH 4 

freecell_solver_instance_t * freecell_solver_alloc_instance(void);

extern void freecell_solver_init_instance(
    freecell_solver_instance_t * instance
    );

extern void freecell_solver_free_instance(
    freecell_solver_instance_t * instance
    );

extern void freecell_solver_finish_instance(
    freecell_solver_instance_t * instance
    );

extern int freecell_solver_solve_instance(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * init_state
    );

extern int freecell_solver_resume_instance(
    freecell_solver_instance_t * instance
    );

extern void freecell_solver_unresume_instance(
    freecell_solver_instance_t * instance
    );

extern int freecell_solver_solve_for_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations,
    int depth,
    int ignore_osins
    );
    
extern int freecell_solver_soft_dfs_solve_for_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig
    );
    
extern void freecell_solver_a_star_initialize_rater(
    freecell_solver_instance_t * instance, 
    fcs_state_with_locations_t * ptr_state_with_locations
    );
    
extern int freecell_solver_a_star_or_bfs_do_solve_or_resume(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig,
    int resume
    );
    
extern int freecell_solver_solve_for_state_resume_solution(
    freecell_solver_instance_t * instance,
    int depth
    );

extern int freecell_solver_soft_dfs_solve_for_state_resume_solution(
    freecell_solver_instance_t * instance
    );

extern int freecell_solver_a_star_or_bfs_solve_for_state(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * ptr_state_with_locations_orig
    );
    
extern int freecell_solver_a_star_or_bfs_resume_solution(
    freecell_solver_instance_t * instance
    );
#ifdef __cplusplus
}
#endif

#endif /* __FCS_H */
