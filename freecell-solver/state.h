/*
 * state.h - header file for state functions and macros for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */
#include "config.h"

#include "fcs_move.h"

#ifndef __STATE_H
#define __STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#if MAX_NUM_INITIAL_CARDS_IN_A_STACK+12>(MAX_NUM_DECKS*52)
#define MAX_NUM_CARDS_IN_A_STACK (MAX_NUM_DECKS*52)
#else
#define MAX_NUM_CARDS_IN_A_STACK (MAX_NUM_INITIAL_CARDS_IN_A_STACK+12)
#endif
    
#ifdef DEBUG_STATES

struct fcs_struct_card_t
{
    short card_num;
    char suit;
    char flags;
};

typedef struct fcs_struct_card_t fcs_card_t;

extern fcs_card_t fcs_empty_card;

struct fcs_struct_stack_t
{
    int num_cards;
    fcs_card_t cards[MAX_NUM_CARDS_IN_A_STACK];
};

typedef struct fcs_struct_stack_t fc_stack_t;

struct fcs_struct_state_t
{
    fc_stack_t stacks[MAX_NUM_STACKS];
    fcs_card_t freecells[MAX_NUM_FREECELLS];
    int foundations[MAX_NUM_DECKS*4];
#ifdef FCS_WITH_TALONS    
    fcs_card_t * talon;
    char talon_params[4];
#endif
};

typedef struct fcs_struct_state_t fcs_state_t;

struct fcs_struct_state_with_locations_t
{
    fcs_state_t s;
    int stack_locs[MAX_NUM_STACKS];
    int fc_locs[MAX_NUM_FREECELLS];
    struct fcs_struct_state_with_locations_t * parent;
    fcs_move_stack_t * moves_to_parent;
    int depth;
    int visited;
};

typedef struct fcs_struct_state_with_locations_t fcs_state_with_locations_t;

#define fcs_stack_len(state, s) \
    ( (state).stacks[(s)].num_cards )    
  
#define fcs_stack_card(state, s, c) \
    ( (state).stacks[(s)].cards[(c)] )

#define fcs_stack_card_suit(state, s, c) \
    ( fcs_card_suit(fcs_stack_card((state),(s),(c))) )

#define fcs_stack_card_num(state, s, c) \
    ( fcs_card_card_num(fcs_stack_card((state),(s),(c))) )
    
#define fcs_card_card_num(card) \
    ( (card).card_num )

#define fcs_card_suit(card) \
    ((int)( (card).suit ))

#define fcs_card_get_flipped(card) \
    ( (card).flags )

#define fcs_freecell_card(state, f) \
    ( (state).freecells[(f)] )
    
#define fcs_freecell_card_num(state, f) \
    ( fcs_card_card_num(fcs_freecell_card((state),(f))) )

#define fcs_freecell_card_suit(state, f) \
    ( fcs_card_suit(fcs_freecell_card((state),(f))) )

#define fcs_foundation_value(state, found) \
    ( (state).foundations[(found)] )

#define fcs_increment_foundation(state, found) \
    ( (state).foundations[(found)]++ )

#define fcs_set_foundation(state, found, value) \
    ( (state).foundations[(found)] = (value) )
    
#define fcs_pop_stack_card(state, s, into) \
    {        \
        into = (state).stacks[(s)].cards[(state).stacks[(s)].num_cards-1]; \
        (state).stacks[(s)].cards[(state).stacks[(s)].num_cards-1] = fcs_empty_card; \
        (state).stacks[(s)].num_cards--;         \
    }
    
#define fcs_push_stack_card_into_stack(state, ds, ss, sc) \
    {             \
        (state).stacks[(ds)].cards[(state).stacks[(ds)].num_cards] = (state).stacks[(ss)].cards[(sc)]; \
        (state).stacks[(ds)].num_cards++; \
    }
    
#define fcs_push_card_into_stack(state, ds, from) \
    {            \
        (state).stacks[(ds)].cards[(state).stacks[(ds)].num_cards] = (from); \
        (state).stacks[(ds)].num_cards++;  \
    }

#define fcs_duplicate_state(dest, src) \
    (dest) = (src)

#define fcs_put_card_in_freecell(state, f, card) \
    (state).freecells[(f)] = (card)

#define fcs_empty_freecell(state, f) \
    (state).freecells[(f)] = fcs_empty_card

#define fcs_card_set_suit(card, d) \
    (card).suit = (d) 

#define fcs_card_set_num(card, num) \
    (card).card_num = (num)

#define fcs_card_set_flipped(card, flipped) \
    (card).flags = (flipped)

#define fcs_flip_stack_card(state, s, c) \
    fcs_card_set_flipped(fcs_stack_card((state),(s),(c)), 0)

#ifdef FCS_WITH_TALONS
#define fcs_talon_len(state) \
    ((state).talon_params[0])

#define fcs_talon_pos(state) \
    ((state).talon_params[1])

#define fcs_get_talon_card(state, pos) \
    ((state).talon[pos])

#define fcs_put_card_in_talon(state, pos, card) \
    ((state).talon[pos] = (card))
#endif    
    
#define fcs_clean_state(state) 

#elif defined(COMPACT_STATES)    /* #ifdef DEBUG_STATES */






    
typedef char fcs_card_t;
/*
 * Card:
 * Bits 0-3 - Card Number
 * Bits 4-5 - Deck
 * 
 */
extern fcs_card_t fcs_empty_card;

struct fcs_struct_state_t
{
    char data[MAX_NUM_STACKS*(MAX_NUM_CARDS_IN_A_STACK+1)+MAX_NUM_FREECELLS+4*MAX_NUM_DECKS];
#ifdef FCS_WITH_TALON    
    fcs_card_t * talon;
    char talon_params[4];    
#endif
};
/*
 * Stack: 0 - Number of cards 
 *        1-19 - Cards
 * Stacks: stack_num*20 where stack_num >= 0 and 
 *         stack_num <= (MAX_NUM_STACKS-1)
 * Bytes: (MAX_NUM_STACKS*20) to 
 *      (MAX_NUM_STACKS*20+MAX_NUM_FREECELLS-1) 
 *      are Freecells.
 * Bytes: (MAX_NUM_STACKS*20+MAX_NUM_FREECELLS) to
 *      MAX_NUM_STACKS*20+MAX_NUM_FREECELLS+3
 *      are Foundations.
 * */

/*  ===== Depracated Information =====
 * Stack: 0 - Number of cards 1-19 - Cards
 * Stacks: stack_num*20 where stack_num >= 0 and stack_num <= 7
 * Bytes 160-163 - Freecells
 * Bytes 164-167 - Decks
 */

typedef struct fcs_struct_state_t fcs_state_t;

struct fcs_struct_state_with_locations_t
{
    fcs_state_t s;
    char stack_locs[MAX_NUM_STACKS];
    char fc_locs[MAX_NUM_FREECELLS];
    struct fcs_struct_state_with_locations_t * parent;
    fcs_move_stack_t * moves_to_parent;
    int depth;
    int visited;
};

typedef struct fcs_struct_state_with_locations_t fcs_state_with_locations_t;

#define fcs_card_card_num(card) \
    ( (card) & 0x0F )

#define fcs_card_suit(card) \
    ( ((card) >> 4) & 0x03 )

#define fcs_stack_len(state, s) \
    ( (state).data[s*(MAX_NUM_CARDS_IN_A_STACK+1)] )

#define fcs_stack_card(state, s, c) \
    ( (state).data[(s)*(MAX_NUM_CARDS_IN_A_STACK+1)+(c)+1] )

#define fcs_stack_card_num(state, s, c) \
    ( fcs_card_card_num(fcs_stack_card((state),(s),(c))) )

#define fcs_stack_card_suit(state, s, c) \
    ( fcs_card_suit(fcs_stack_card((state),(s),(c))) )

#define FCS_FREECELLS_OFFSET ((MAX_NUM_STACKS)*(MAX_NUM_CARDS_IN_A_STACK+1))
    
#define fcs_freecell_card(state, f) \
    ( (state).data[FCS_FREECELLS_OFFSET+(f)] )

#define fcs_freecell_card_num(state, f) \
    ( fcs_card_card_num(fcs_freecell_card((state),(f))) )

#define fcs_freecell_card_suit(state, f) \
    ( fcs_card_suit(fcs_freecell_card((state),(f))) )

#define FCS_FOUNDATIONS_OFFSET (((MAX_NUM_STACKS)*(MAX_NUM_CARDS_IN_A_STACK+1))+(MAX_NUM_FREECELLS))
    
#define fcs_foundation_value(state, d) \
    ( (state).data[FCS_FOUNDATIONS_OFFSET+(d)])

#define fcs_increment_foundation(state, d) \
    ( (state).data[FCS_FOUNDATIONS_OFFSET+(d)]++ )

#define fcs_set_foundation(state, d, value) \
    ( (state).data[FCS_FOUNDATIONS_OFFSET+(d)] = (value) )

#define fcs_pop_stack_card(state, s, into) \
    {             \
        into = fcs_stack_card((state), (s), (fcs_stack_len((state), (s))-1)); \
        (state).data[((s)*(MAX_NUM_CARDS_IN_A_STACK+1))+1+(fcs_stack_len((state), (s))-1)] = fcs_empty_card; \
        (state).data[(s)*(MAX_NUM_CARDS_IN_A_STACK+1)]--;        \
    }
    
#define fcs_push_card_into_stack(state, ds, from) \
    {              \
        (state).data[(ds)*(MAX_NUM_CARDS_IN_A_STACK+1)+1+fcs_stack_len((state), (ds))] = (from); \
        (state).data[(ds)*(MAX_NUM_CARDS_IN_A_STACK+1)]++;     \
    }

#define fcs_push_stack_card_into_stack(state, ds, ss, sc) \
    fcs_push_card_into_stack((state), (ds), fcs_stack_card((state), (ss), (sc)))

#define fcs_duplicate_state(dest, src) \
    (dest) = (src);

#define fcs_put_card_in_freecell(state, f, card) \
    (state).data[FCS_FREECELLS_OFFSET+(f)] = (card);

#define fcs_empty_freecell(state, f) \
    fcs_put_card_in_freecell((state), (f), fcs_empty_card)

#define fcs_card_set_num(card, num) \
    (card) = (((card)&0xF0)|(num));

#define fcs_card_set_suit(card, suit) \
    (card) = (((card)&0x4F)|((suit)<<4));

#define fcs_card_set_flipped(card, flipped) \
    (card) = (((card)&((fcs_card_t)0x3F))|((fcs_card_t)((flipped)<<6)))

#define fcs_card_get_flipped(card) \
    ( (card) >> 6 )


#define fcs_clean_state(state) 

#ifdef FCS_WITH_TALONS
#define fcs_talon_len(state) \
    ((state).talon_params[0])

#define fcs_talon_pos(state) \
    ((state).talon_params[1])

#define fcs_put_card_in_talon(state, pos, card) \
    ((state).talon[pos] = (card))

#define fcs_get_talon_card(state, pos) \
    ((state).talon[pos])
#endif

#define fcs_flip_stack_card(state, s, c) \
    (fcs_card_set_flipped(fcs_stack_card((state),(s),(c)), ((fcs_card_t)0) ))
    
#elif defined(INDIRECT_STACK_STATES)

typedef char fcs_card_t;

extern fcs_card_t fcs_empty_card;

struct fcs_struct_state_t
{
    fcs_card_t * stacks[MAX_NUM_STACKS];
    fcs_card_t freecells[MAX_NUM_FREECELLS];
    char foundations[MAX_NUM_DECKS*4];
#ifdef FCS_WITH_TALONS    
    fcs_card_t * talon;
    char talon_params[4];
#endif
};
 
typedef struct fcs_struct_state_t fcs_state_t;

struct fcs_struct_state_with_locations_t
{
    fcs_state_t s;
    char stack_locs[MAX_NUM_STACKS];
    char fc_locs[MAX_NUM_FREECELLS];
    struct fcs_struct_state_with_locations_t * parent;
    fcs_move_stack_t * moves_to_parent;
    int depth;
    int visited;
};

typedef struct fcs_struct_state_with_locations_t fcs_state_with_locations_t;

#define fcs_card_card_num(card) \
    ( (card) & 0x0F )

#define fcs_card_suit(card) \
    ( ((card) >> 4) & 0x03 )

#define fcs_card_get_flipped(card) \
    ( (card) >> 6 )

#define fcs_standalone_stack_len(stack) \
    ( (int)(stack[0]) )

#define fcs_stack_len(state, s) \
    ( (int)(state).stacks[(s)][0] )

#define fcs_stack_card(state, s, c) \
    ( (state).stacks[(s)][c+1] )

#define fcs_stack_card_num(state, s, c) \
    ( fcs_card_card_num(fcs_stack_card((state),(s),(c))) )

#define fcs_stack_card_suit(state, s, c) \
    ( fcs_card_suit(fcs_stack_card((state),(s),(c))) )

#define fcs_freecell_card(state, f) \
    ( (state).freecells[(f)] )

#define fcs_freecell_card_num(state, f) \
    ( fcs_card_card_num(fcs_freecell_card((state),(f))) )

#define fcs_freecell_card_suit(state, f) \
    ( fcs_card_suit(fcs_freecell_card((state),(f))) )

#define fcs_foundation_value(state, d) \
    ( (state).foundations[(d)] )

#define fcs_increment_foundation(state, d) \
    ( (state).foundations[(d)]++ )

#define fcs_set_foundation(state, d, value) \
    ( (state).foundations[(d)] = (value) )

#define fcs_pop_stack_card(state, s, into) \
    {          \
        into = fcs_stack_card((state), (s), (fcs_stack_len((state), (s))-1)); \
        (state).stacks[s][fcs_stack_len((state), (s))] = fcs_empty_card; \
        (state).stacks[s][0]--;        \
    }
        

#define fcs_push_card_into_stack(state, ds, from) \
    {        \
        (state).stacks[(ds)][fcs_stack_len((state), (ds))+1] = (from); \
        (state).stacks[(ds)][0]++;      \
    }

#define fcs_push_stack_card_into_stack(state, ds, ss, sc) \
    fcs_push_card_into_stack((state), (ds), fcs_stack_card((state), (ss), (sc)))

#define fcs_put_card_in_freecell(state, f, card) \
    (state).freecells[(f)] = (card)

#define fcs_empty_freecell(state, f) \
    fcs_put_card_in_freecell((state), (f), fcs_empty_card)

#define fcs_card_set_num(card, num) \
    (card) = (((card)&0xF0)|(num))

#define fcs_card_set_suit(card, suit) \
    (card) = (((card)&0x4F)|((suit)<<4))

#define fcs_card_set_flipped(card, flipped) \
    (card) = (fcs_card_t)(((card)&0x3F)|((fcs_card_t)(flipped<<6)))

#ifdef FCS_WITH_TALONS
#define fcs_talon_len(state) \
    ((state).talon_params[0])

#define fcs_talon_pos(state) \
    ((state).talon_params[1])

#define fcs_put_card_in_talon(state, pos, card) \
    ((state).talon[pos] = (card))

#define fcs_get_talon_card(state, pos) \
    ((state).talon[pos])
#endif    

#define fcs_flip_stack_card(state, s, c) \
    (fcs_card_set_flipped(fcs_stack_card(state,s,c), ((fcs_card_t)0) ))

extern void fcs_duplicate_state_proto(
    fcs_state_with_locations_t * dest, 
    fcs_state_with_locations_t * src
    );
    
#define fcs_duplicate_state(dest, src) \
    fcs_duplicate_state_proto(&(dest), &(src));

void fcs_clean_state(
    fcs_state_with_locations_t * state
    );    
    
#endif /* #ifdef DEBUG_STATES - 
          #elif defined COMPACT_STATES - 
          #elif defined INDIRECT_STACK_STATES
        */

#ifdef FCS_WITH_TALONS
#define fcs_klondike_talon_len(state) \
    ((state).talon[0])
    
#define fcs_klondike_talon_stack_pos(state) \
    ((state).talon_params[0])

#define fcs_klondike_talon_queue_pos(state) \
    ((state).talon_params[1])

#define fcs_klondike_talon_num_redeals_left(state) \
    ((state).talon_params[2])

#define fcs_klondike_talon_get_top_card(state) \
    ((state).talon[(int)fcs_klondike_talon_stack_pos(state)])

#define fcs_klondike_talon_queue_to_stack(state) \
    ( ((state).talon[(int)((++fcs_klondike_talon_stack_pos(state))+1)]) =  \
      ((state).talon[(int)((fcs_klondike_talon_queue_pos(state)++)+1)]) )

#define fcs_klondike_talon_redeal_bare(state) \
    {         \
        fcs_klondike_talon_stack_pos(state) = -1; \
        fcs_klondike_talon_queue_pos(state) = 0; \
    }         
    
#define fcs_klondike_talon_decrement_stack(state) \
    ((state).talon[(int)((fcs_klondike_talon_stack_pos(state)--)+1)] = fcs_empty_card)   
#endif

extern void fcs_canonize_state(
    fcs_state_with_locations_t * state, 
    int freecells_num,
    int stacks_num
    );

#if (FCS_STATE_STORAGE != FCS_STATE_STORAGE_INDIRECT)

#if (FCS_STATE_STORAGE != FCS_STATE_STORAGE_LIBREDBLACK_TREE)
typedef void * fcs_compare_context_t;
#else
typedef const void * fcs_compare_context_t;
#endif

extern int fcs_state_compare(const void * s1, const void * s2);
extern int fcs_state_compare_equal(const void * s1, const void * s2);
extern int fcs_state_compare_with_context(const void * s1, const void * s2, fcs_compare_context_t context);
#else
extern int fcs_state_compare_indirect(const void * s1, const void * s2);
extern int fcs_state_compare_indirect_with_context(const void * s1, const void * s2, void * context);
#endif

#ifdef FCS_WITH_TALONS
extern int fcs_talon_compare_with_context(const void * s1, const void * s2, fcs_compare_context_t context);
#endif

extern void fcs_state_init(
    fcs_state_with_locations_t * state,
    int stacks_num
    );
extern fcs_state_with_locations_t fcs_initial_user_state_to_c(
    const char * string,
    int freecells_num,
    int stacks_num,
    int decks_num
#ifdef FCS_WITH_TALONS
    ,int talon_type
#endif
    );


extern char * fcs_state_as_string(
    fcs_state_with_locations_t * state, 
    int freecells_num, 
    int stacks_num, 
    int decks_num,
    int parseable_output, 
    int canonized_order_output,
    int display_10_as_t
    );
extern int fcs_check_state_validity(
    fcs_state_with_locations_t * state, 
    int freecells_num,
    int stacks_num,
    int decks_num,
#ifdef FCS_WITH_TALONS    
    int talon_type,
#endif    
    fcs_card_t * misplaced_card
    );

#ifdef __cplusplus
}
#endif

enum FCS_VISITED_T
{
    FCS_VISITED_VISITED = 0x1,
    FCS_VISITED_IN_SOLUTION_PATH = 0x2,
    FCS_VISITED_IN_OPTIMIZED_PATH = 0x4
};


#endif /* __STATE_H */
