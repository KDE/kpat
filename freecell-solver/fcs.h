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

struct fcs_states_linked_list_item_struct
{
    fcs_state_with_locations_t * s;
    struct fcs_states_linked_list_item_struct * next;
};

typedef struct fcs_states_linked_list_item_struct fcs_states_linked_list_item_t;



#define FCS_TESTS_NUM 10

typedef struct freecell_solver_instance
{
    int unsorted_prev_states_start_at;

    fcs_state_with_locations_t * indirect_prev_states_margin[PREV_STATES_SORT_MARGIN];
    
    int num_prev_states_margin;    

#ifdef INDIRECT_STATE_STORAGE    
    fcs_state_with_locations_t * * indirect_prev_states;
    int num_indirect_prev_states;
    int max_num_indirect_prev_states;
#endif    

    fcs_state_with_locations_t * * state_packs;
    int max_num_state_packs;
    int num_state_packs;
    int num_states_in_last_pack;
    int state_pack_len;
    
    int num_times;
    fcs_state_with_locations_t * * solution_states;
    int num_solution_states;
    
    fcs_move_stack_t * solution_moves;
    fcs_move_stack_t * * proto_solution_moves;

    int max_depth;
    int max_num_times;

    int debug_iter_output;
    void (*debug_iter_output_func)(
        void * context, 
        int iter_num, 
        int depth, 
        void * instance,
        fcs_state_with_locations_t * state
        );
    void * debug_iter_output_context;
    
    int tests_order_num;
    int tests_order[FCS_TESTS_NUM];

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBREDBLACK_TREE)
    struct rbtree * tree;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_AVL_TREE)
    avl_tree * tree;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_LIBAVL_REDBLACK_TREE)
    rb_tree * tree;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_TREE)
    GTree * tree;
#endif

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_GLIB_HASH)
    GHashTable * hash;
#elif (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
    SFO_hash_t * hash;
#endif

#if (defined(INDIRECT_STACK_STATES) && (FCS_STACK_STORAGE == FCS_STACK_STORAGE_INTERNAL_HASH)) || (FCS_STATE_STORAGE == FCS_STATE_STORAGE_INTERNAL_HASH)
    MD5_CTX md5_context;
    unsigned char hash_value[MD5_HASHBYTES];    
#endif

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

#if (FCS_STATE_STORAGE == FCS_STATE_STORAGE_DB_FILE)
    DB * db;
#endif  

    int freecells_num;
    int stacks_num;
    int decks_num;

    int sequences_are_built_by;
    int unlimited_sequence_move;
    int empty_stacks_fill;


    int dfs_max_depth;
    int method;

    fcs_states_linked_list_item_t * bfs_queue;
    fcs_states_linked_list_item_t * bfs_queue_last_item;

    PQUEUE * a_star_pqueue;
    double a_star_initial_cards_under_sequences;
    double a_star_weights[5];

    fcs_state_with_locations_t * first_state_to_check;

    fcs_state_with_locations_t * * * soft_dfs_states_to_check;
    fcs_move_stack_t * * * soft_dfs_states_to_check_move_stacks;
    int * soft_dfs_num_states_to_check;
    int * soft_dfs_max_num_states_to_check;
    int * soft_dfs_current_state_indexes;
    int * soft_dfs_test_indexes;
    int * soft_dfs_num_freestacks;
    int * soft_dfs_num_freecells;
} freecell_solver_instance_t;

#define FCS_SOFT_DFS_STATES_TO_CHECK_GROW_BY 32

#define FCS_A_STAR_WEIGHT_CARDS_OUT 0
#define FCS_A_STAR_WEIGHT_MAX_SEQUENCE_MOVE 1
#define FCS_A_STAR_WEIGHT_CARDS_UNDER_SEQUENCES 2
#define FCS_A_STAR_WEIGHT_SEQS_OVER_RENEGADE_CARDS 3
#define FCS_A_STAR_WEIGHT_DEPTH 4 

freecell_solver_instance_t * freecell_solver_alloc_instance(void);
void freecell_solver_init_instance(freecell_solver_instance_t * instance);
void freecell_solver_free_instance(freecell_solver_instance_t * instance);
void freecell_solver_finish_instance(freecell_solver_instance_t * instance);
int freecell_solver_solve_instance(
    freecell_solver_instance_t * instance,
    fcs_state_with_locations_t * init_state
    );

int freecell_solver_resume_instance(
    freecell_solver_instance_t * instance
    );

void freecell_solver_unresume_instance(
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
