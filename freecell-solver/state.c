/*
 * state.c - state functions module for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "state.h"
#include "card.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif


#ifdef DEBUG_STATES

fcs_card_t fcs_empty_card = {0,0};

#elif defined(COMPACT_STATES) || defined (INDIRECT_STACK_STATES)

fcs_card_t fcs_empty_card = 0;

#endif

#ifdef INDIRECT_STACK_STATES
void fcs_duplicate_state_proto(
    fcs_state_with_locations_t * dest,
    fcs_state_with_locations_t * src
    )
{
    int a;
    int stack_len;
    
    *dest = *src;
    for(a=0;a<MAX_NUM_STACKS;a++)
    {
        if (src->s.stacks[a] != NULL)
        {
            stack_len = src->s.stacks[a][0];
            dest->s.stacks[a] = malloc(stack_len+13);
            memset(dest->s.stacks[a], '\0', stack_len+13);
            memcpy(dest->s.stacks[a], src->s.stacks[a], stack_len+1);
        }
    }
}

#endif


int fcs_card_compare(const void * card1, const void * card2)
{
    const fcs_card_t * c1 = (const fcs_card_t *)card1;
    const fcs_card_t * c2 = (const fcs_card_t *)card2;

    if (fcs_card_card_num(*c1) > fcs_card_card_num(*c2))
    {
        return 1;
    }
    else if (fcs_card_card_num(*c1) < fcs_card_card_num(*c2))
    {
        return -1;
    }
    else
    {
        if (fcs_card_deck(*c1) > fcs_card_deck(*c2))
        {
            return 1;
        }
        else if (fcs_card_deck(*c1) < fcs_card_deck(*c2))
        {
            return -1;
        }
        else
        {
            return 0;
        }            
    }
}

#ifdef DEBUG_STATES
int fcs_stack_compare(const void * s1, const void * s2)
{
    fcs_card_t card1 = ((const fc_stack_t *)s1)->cards[0];
    fcs_card_t card2 = ((const fc_stack_t *)s2)->cards[0]; 

    return fcs_card_compare(&card1, &card2);
}
#elif defined(COMPACT_STATES)
int fcs_stack_compare(const void * s1, const void * s2)
{
    fcs_card_t card1 = ((fcs_card_t*)s1)[1];
    fcs_card_t card2 = ((fcs_card_t*)s2)[1];

    return fcs_card_compare(&card1, &card2);
}
#elif defined(INDIRECT_STACK_STATES)


int fcs_stack_compare_for_stack_sort(const void * s1, const void * s2)
{
    fcs_card_t card1 = ((fcs_card_t*)s1)[1];
    fcs_card_t card2 = ((fcs_card_t*)s2)[1];

    return fcs_card_compare(&card1, &card2);
}

int fcs_stack_compare_for_comparison(const void * v_s1, const void * v_s2)
{
    const fcs_card_t * s1 = (const fcs_card_t *)v_s1;
    const fcs_card_t * s2 = (const fcs_card_t *)v_s2;

    if (s1[0] < s2[0])
    {
        return -1;
    }
    else if (s1[0] > s2[0])
    {
        return 1;
    }
    else
    {
        int a, ret;
        for(a=0;a<s1[0];a++)
        {
            ret = fcs_card_compare(s1+a+1,s2+a+1);
            if (ret != 0)
            {
                return ret;
            }
        }
    }
    return 0;    
}

#endif

#ifdef DEBUG_STATES
void fcs_canonize_state(fcs_state_with_locations_t * state, int freecells_num, int stacks_num)
{
    int b,c;

    fc_stack_t temp_stack;
    fcs_card_t temp_freecell;    
    int temp_loc;

    /* Insertion-sort the stacks */
    for(b=1;b<stacks_num;b++)
    {
        c = b;
        while(
            (c>0) && 
            (fcs_stack_compare(
                &(state->s.stacks[c]), 
                &(state->s.stacks[c-1])
                ) < 0)
            )
        {
            temp_stack = state->s.stacks[c];
            state->s.stacks[c] = state->s.stacks[c-1];
            state->s.stacks[c-1] = temp_stack;

            temp_loc = state->stack_locs[c];
            state->stack_locs[c] = state->stack_locs[c-1];
            state->stack_locs[c-1] = temp_loc;

            c--;
        }
    }
            
    /* Insertion sort the freecells */

    for(b=1;b<freecells_num;b++)
    {
        c = b;
        while(
            (c>0)     &&
            (fcs_card_compare(
                &(state->s.freecells[c]),
                &(state->s.freecells[c-1])
                ) < 0)
            )
        {
            temp_freecell = state->s.freecells[c];
            state->s.freecells[c] = state->s.freecells[c-1];
            state->s.freecells[c-1] = temp_freecell;

            temp_loc = state->fc_locs[c];
            state->fc_locs[c] = state->fc_locs[c-1];
            state->fc_locs[c-1] = temp_loc;

            c--;
        }
    }   
}

#elif defined(COMPACT_STATES)

void fcs_canonize_state(
    fcs_state_with_locations_t * state, 
    int freecells_num, 
    int stacks_num)
{
    int b,c;

    char temp_stack[(MAX_NUM_CARDS_IN_A_STACK+1)];
    fcs_card_t temp_freecell;
    char temp_loc;

    /* Insertion-sort the stacks */

    for(b=1;b<stacks_num;b++)
    {
        c = b;
        while(
            (c>0)    &&
            (fcs_stack_compare(
                state->s.data+c*(MAX_NUM_CARDS_IN_A_STACK+1),
                state->s.data+(c-1)*(MAX_NUM_CARDS_IN_A_STACK+1)
                ) < 0)
            )
        {
            memcpy(temp_stack, state->s.data+c*(MAX_NUM_CARDS_IN_A_STACK+1), (MAX_NUM_CARDS_IN_A_STACK+1));
            memcpy(state->s.data+c*(MAX_NUM_CARDS_IN_A_STACK+1), state->s.data+(c-1)*(MAX_NUM_CARDS_IN_A_STACK+1), (MAX_NUM_CARDS_IN_A_STACK+1));
            memcpy(state->s.data+(c-1)*(MAX_NUM_CARDS_IN_A_STACK+1), temp_stack, (MAX_NUM_CARDS_IN_A_STACK+1));

            temp_loc = state->stack_locs[c];
            state->stack_locs[c] = state->stack_locs[c-1];
            state->stack_locs[c-1] = temp_loc;            

            c--;
        }
    }

    /* Insertion-sort the freecells */

    for(b=1;b<freecells_num;b++)
    {
        c = b;

        while(
            (c>0)    &&
            (fcs_card_compare(
                state->s.data+FCS_FREECELLS_OFFSET+c,
                state->s.data+FCS_FREECELLS_OFFSET+c-1
                ) < 0)
            )
        {
            temp_freecell = (state->s.data[FCS_FREECELLS_OFFSET+c]);
            state->s.data[FCS_FREECELLS_OFFSET+c] = state->s.data[FCS_FREECELLS_OFFSET+c-1];
            state->s.data[FCS_FREECELLS_OFFSET+c-1] = temp_freecell;

            temp_loc = state->fc_locs[c];
            state->fc_locs[c] = state->fc_locs[c-1];
            state->fc_locs[c-1] = temp_loc;

            c--;
        }
    }
}
#elif defined(INDIRECT_STACK_STATES)
void fcs_canonize_state(
    fcs_state_with_locations_t * state,
    int freecells_num,
    int stacks_num)
{
    int b,c;
    fcs_card_t * temp_stack;
    fcs_card_t temp_freecell;
    char temp_loc;

    /* Insertion-sort the stacks */
    for(b=1;b<stacks_num;b++)
    {
        c = b;
        while(
            (c>0) && 
            (
#if MAX_NUM_DECKS > 1
                fcs_stack_compare_for_comparison
#else
                fcs_stack_compare_for_stack_sort
#endif
                (
                    (state->s.stacks[c]), 
                    (state->s.stacks[c-1])
                ) 
                < 0
            )
        )
        {
            temp_stack = state->s.stacks[c];
            state->s.stacks[c] = state->s.stacks[c-1];
            state->s.stacks[c-1] = temp_stack;

            temp_loc = state->stack_locs[c];
            state->stack_locs[c] = state->stack_locs[c-1];
            state->stack_locs[c-1] = temp_loc;

            c--;
        }
    }
            
    /* Insertion sort the freecells */

    for(b=1;b<freecells_num;b++)
    {
        c = b;
        while(
            (c>0)     &&
            (fcs_card_compare(
                &(state->s.freecells[c]),
                &(state->s.freecells[c-1])
                ) < 0)
            )
        {
            temp_freecell = state->s.freecells[c];
            state->s.freecells[c] = state->s.freecells[c-1];
            state->s.freecells[c-1] = temp_freecell;

            temp_loc = state->fc_locs[c];
            state->fc_locs[c] = state->fc_locs[c-1];
            state->fc_locs[c-1] = temp_loc;

            c--;
        }
    }   
}
    
#endif

void fcs_state_init(fcs_state_with_locations_t * state, int stacks_num)
{
    int a;
    memset((void*)&(state->s), 0, sizeof(fcs_state_t));
    for(a=0;a<MAX_NUM_STACKS;a++)
    {
        state->stack_locs[a] = a;
    }
#ifdef INDIRECT_STACK_STATES
    for(a=0;a<stacks_num;a++)
    {
        state->s.stacks[a] = malloc(MAX_NUM_DECKS*52+1);
        memset(state->s.stacks[a], '\0', MAX_NUM_DECKS*52+1);
    }
    for(;a<MAX_NUM_STACKS;a++)
    {
        state->s.stacks[a] = NULL;
    }
#endif    
    for(a=0;a<MAX_NUM_FREECELLS;a++)
    {
        state->fc_locs[a] = a;
    }
}


#if defined(TREE_STATE_STORAGE) || defined(HASH_STATE_STORAGE)
int fcs_state_compare(const void * s1, const void * s2)
{
    return memcmp(s1,s2,sizeof(fcs_state_t));
}

int fcs_state_compare_equal(const void * s1, const void * s2)
{
    return (!memcmp(s1,s2,sizeof(fcs_state_t)));
}


int fcs_state_compare_with_context(
    const void * s1, 
    const void * s2, 
    fcs_compare_context_t context
    )
{
    return memcmp(s1,s2,sizeof(fcs_state_t));
}
#elif defined(INDIRECT_STATE_STORAGE)
int fcs_state_compare_indirect(const void * s1, const void * s2)
{
    return memcmp(*(fcs_state_with_locations_t * *)s1, *(fcs_state_with_locations_t * *)s2, sizeof(fcs_state_t));
}

int fcs_state_compare_indirect_with_context(const void * s1, const void * s2, void * context)
{
    return memcmp(*(fcs_state_with_locations_t * *)s1, *(fcs_state_with_locations_t * *)s2, sizeof(fcs_state_t));
}
#endif

static const char * freecells_prefixes[] = { "FC:", "Freecells:", "Freecell:", ""};
static const char * foundations_prefixes[] = { "Decks:", "Deck:", "Founds:", "Foundations:", "Foundation:", "Found:", ""};

#ifdef WIN32
#define strncasecmp(a,b,c) (strnicmp((a),(b),(c)))
#endif

fcs_state_with_locations_t fcs_initial_user_state_to_c(
    char * string, 
    int freecells_num,
    int stacks_num,
    int decks_num
    )
{
    fcs_state_with_locations_t ret_with_locations;

    int s,c;
    char * str;
    fcs_card_t card;
    int first_line;
    
    int prefix_found;
    char * * prefixes;
    int i;
    int decks_index[4];

    fcs_state_init(&ret_with_locations, stacks_num);
    str = string;

    first_line = 1;

#define ret (ret_with_locations.s)
    
    for(s=0;s<stacks_num;s++)
    {
        /* Move to the next stack */
        if (!first_line)
        {
            while((*str) != '\n')
            {
                str++;
            }
            str++;
        }
        first_line = 0;

        prefixes = freecells_prefixes;
        prefix_found = 0;
        for(i=0;prefixes[i][0] != '\0'; i++)
        {
            if (!strncasecmp(str, prefixes[i], strlen(prefixes[i])))
            {
                prefix_found = 1;
                str += strlen(prefixes[i]);
                break;
            }
        }
        
        if (prefix_found)
        {
            for(c=0;c<freecells_num;c++)
            {
                fcs_empty_freecell(ret, c);
            }
            for(c=0;c<freecells_num;c++)
            {
                if (c!=0)
                {
                    while(
                            ((*str) != ' ') && 
                            ((*str) != '\t') && 
                            ((*str) != '\n') && 
                            ((*str) != '\r')
                         )
                    {
                        str++;
                    }
                    if ((*str == '\n') || (*str == '\r'))
                    {
                        break;
                    }
                    str++;
                }

                while ((*str == ' ') || (*str == '\t'))
                {
                    str++;
                }
                if ((*str == '\r') || (*str == '\n'))
                    break;

                if ((*str == '*') || (*str == '-'))
                {
                    card = fcs_empty_card;
                }
                else
                {
                    card = fcs_card_user2perl(str);
                }

                fcs_put_card_in_freecell(ret, c, card);
            }

            while ((*str != '\n') && (*str != '\0'))
            {
                str++;
            }
            s--;
            continue;
        }
        
        prefixes = foundations_prefixes;
        prefix_found = 0;
        for(i=0;prefixes[i][0] != '\0'; i++)
        {
            if (!strncasecmp(str, prefixes[i], strlen(prefixes[i])))
            {
                prefix_found = 1;
                str += strlen(prefixes[i]);
                break;
            }
        }
    
        if (prefix_found)
        {
            int d;

            for(d=0;d<decks_num*4;d++)
            {
                fcs_set_deck(ret, d, 0);
            }
            
            for(d=0;d<4;d++)
            {
                decks_index[d] = 0;
            }
            while (1)
            {
                while((*str == ' ') || (*str == '\t'))
                    str++;
                if ((*str == '\n') || (*str == '\r'))
                    break;
                d = fcs_u2p_deck(str);
                str++;
                while (*str == '-')
                    str++;
                c = fcs_u2p_card_number(str);
                while (
                        (*str != ' ') && 
                        (*str != '\t') && 
                        (*str != '\n') && 
                        (*str != '\r')
                      )
                {
                    str++;
                }

                fcs_set_deck(ret, (decks_index[d]*4+d), c);
                decks_index[d]++;
                if (decks_index[d] >= decks_num)
                {
                    decks_index[d] = 0;
                }
            }
            s--;
            continue;
        }
        
        for(c=0 ; c < MAX_NUM_CARDS_IN_A_STACK ; c++)
        {
            /* Move to the next card */
            if (c!=0)
            {
                while(
                    ((*str) != ' ') && 
                    ((*str) != '\t') && 
                    ((*str) != '\n') && 
                    ((*str) != '\r')
                )
                {
                    str++;
                }
                if ((*str == '\n') || (*str == '\r'))
                {
                    break;
                }
            }

            while ((*str == ' ') || (*str == '\t'))
            {
                str++;
            }
            if ((*str == '\n') || (*str == '\r'))
            {
                break;
            }
            card = fcs_card_user2perl(str);

            fcs_push_card_into_stack(ret, s, card);
        } 
    }

    return ret_with_locations;
}

#undef ret

int fcs_check_state_validity(
    fcs_state_with_locations_t * state_with_locations, 
    int freecells_num,
    int stacks_num,
    int decks_num,
    fcs_card_t * misplaced_card)
{
    int cards[4][14];
    int c, s, d, f;

    fcs_state_t * state;

    state = (&(state_with_locations->s));

    /* Initialize all cards to 0 */
    for(d=0;d<4;d++)
    {
        for(c=1;c<=13;c++)
        {
            cards[d][c] = 0;
        }
    }

    /* Mark the cards in the decks */
    for(d=0;d<decks_num*4;d++)
    {
        for(c=1;c<=fcs_deck_value(*state, d);c++)
        {
            cards[d%4][c]++;
        }
    }
    
    /* Mark the cards in the freecells */
    for(f=0;f<freecells_num;f++)
    {
        if (fcs_freecell_card_num(*state, f) != 0)
        {
            cards
                [fcs_freecell_card_deck(*state, f)]
                [fcs_freecell_card_num(*state, f)] ++;
        }
    }
    
    /* Mark the cards in the stacks */
    for(s=0;s<stacks_num;s++)
    {
        for(c=0;c<fcs_stack_len(*state,s);c++)
        {
            if (fcs_stack_card_num(*state, s, c) == 0)
            {
                *misplaced_card = fcs_empty_card;
                return 3;
            }
            cards
                [fcs_stack_card_deck(*state, s, c)]
                [fcs_stack_card_num(*state, s, c)] ++;
        }
    }
    
    /* Now check if there are extra or missing cards */
    
    for(d=0;d<4;d++)
    {
        for(c=1;c<=13;c++)
        {
            if (cards[d][c] != decks_num)
            {
                fcs_card_set_deck(*misplaced_card, d);
                fcs_card_set_num(*misplaced_card, c);
                return (cards[d][c] < decks_num) ? 1 : 2;
            }
        }
    }
    
    return 0;
}

#undef state


char * fcs_state_as_string(
    fcs_state_with_locations_t * state_with_locations, 
    int freecells_num, 
    int stacks_num, 
    int decks_num,
    int parseable_output, 
    int canonized_order_output,
    int display_10_as_t
    )
{
    fcs_state_t * state;
    char freecell[10], decks[MAX_NUM_DECKS*4][10], stack_card_[10];
    int a, card_num_is_null, b;
    int max_num_cards, s, card_num, len;

    char string[8000];
    char str2[128], str3[128], * str2_ptr, * str3_ptr;

    char * str;

    int stack_locs[MAX_NUM_STACKS];
    int freecell_locs[MAX_NUM_FREECELLS];

    state = (&(state_with_locations->s));

    if (canonized_order_output)
    {
        for(a=0;a<stacks_num;a++)
        {
            stack_locs[a] = a;
        }
        for(a=0;a<freecells_num;a++)
        {
            freecell_locs[a] = a;
        }
    }
    else
    {
        for(a=0;a<stacks_num;a++)
        {
            stack_locs[(int)(state_with_locations->stack_locs[a])] = a;
        }
        for(a=0;a<freecells_num;a++)
        {
            freecell_locs[(int)(state_with_locations->fc_locs[a])] = a;
        }
    }
    
    for(a=0;a<decks_num*4;a++)
    {
        fcs_p2u_card_number( 
            fcs_deck_value(*state, a), 
            decks[a], 
            &card_num_is_null,
            display_10_as_t
            );
        if (decks[a][0] == ' ')
            decks[a][0] = '0';
    }

    str = string;

    if(!parseable_output)
    {
        for(a=0;a<((freecells_num/4)+((freecells_num%4==0)?0:1));a++)
        {
            str2_ptr = str2;
            str3_ptr = str3;
            for(b=0;b<min(freecells_num-a*4, 4);b++)
            {
                str2_ptr += sprintf(str2_ptr, "%3s ", 
                    fcs_card_perl2user(
                        fcs_freecell_card(
                            *state, 
                            freecell_locs[a*4+b]
                        ),
                        freecell,
                        display_10_as_t
                    )
                );
                str3_ptr += sprintf(str3_ptr, "--- ");
            }
            if (a < decks_num)
            {
                str += sprintf(str, "%-16s        H-%1s S-%1s D-%1s C-%1s\n",
                    str2,
                    decks[a*4],
                    decks[a*4+1],
                    decks[a*4+2],
                    decks[a*4+3]);                    
            }
            else
            {
                str += sprintf(str, "%s\n", str2);
            }
            str += sprintf(str, "%s\n", str3);
        }
        for(;a<decks_num;a++)
        {
            str += sprintf(str, "%-16s        H-%1s S-%1s D-%1s C-%1s\n",
                    "",
                    decks[a*4],
                    decks[a*4+1],
                    decks[a*4+2],
                    decks[a*4+3]);    
        }
        *(str++) = '\n';
        *(str++) = '\n';
        *str = '\0';

        for(s=0;s<stacks_num;s++)
        {
            str += sprintf(str, " -- ");
        }
        *(str++) = '\n';
        *str = '\0';

        max_num_cards = 0;
        for(s=0;s<stacks_num;s++)
        {
            if (fcs_stack_len(*state, stack_locs[s]) > max_num_cards)
            {
                max_num_cards = fcs_stack_len(*state, stack_locs[s]);
            }
        }

        for(card_num=0;card_num<max_num_cards;card_num++)
        {
            for(s = 0; s<stacks_num; s++)
            {
                if (card_num >= fcs_stack_len(*state, stack_locs[s]))
                {
                    strcpy(str, "    ");
                    str += strlen(str);
                }
                else
                {
                    str += sprintf(
                        str, 
                        "%3s ", 
                        fcs_card_perl2user( 
                            fcs_stack_card(
                                *state, 
                                stack_locs[s], 
                                card_num), 
                            stack_card_,
                            display_10_as_t
                            )
                        );                   

                }
            }
            strcpy(str, "\n");
            str += strlen(str);
        }
    }
    else
    {
#if 0        
        str += sprintf(str,
            "Foundations: H-%1s S-%1s D-%1s C-%1s\n"
            "Freecells: ",
            decks[0],
            decks[1],
            decks[2],
            decks[3]);
#endif
        str += sprintf(str, "Foundations: ");
        for(a=0;a<decks_num;a++)
        {
            str += sprintf(str,
                "H-%s S-%s D-%s C-%s ",
                decks[a*4],
                decks[a*4+1],
                decks[a*4+2],
                decks[a*4+3]
                );
        }
        str += sprintf(str, "\nFreecells: ");
        
        for(a=0;a<freecells_num;a++)
        {
            str += sprintf(
                str, 
                "%3s", 
                fcs_card_perl2user(
                    fcs_freecell_card(
                        *state, 
                        freecell_locs[a]
                    ), 
                    freecell,
                    display_10_as_t
                )
            );
            if (a < freecells_num-1)
            {
                *(str++) = ' ';
                *str = '\0';
            }
        }
        *(str++) = '\n';
        *str = '\0';

        for(s=0;s<stacks_num;s++)
        {
            strcpy(str, ": ");
            str += strlen(str);

            len = fcs_stack_len(*state, stack_locs[s]);
            for(card_num=0;card_num<len;card_num++)
            {
                fcs_card_perl2user(
                    fcs_stack_card(
                        *state, 
                        stack_locs[s], 
                        card_num
                    ), 
                    stack_card_,
                    display_10_as_t
                );
                strcpy(str, stack_card_);
                str += strlen(str);
                if (card_num < len-1)
                {
                    strcpy(str, " ");
                    str += strlen(str);
                }
            }
            strcpy(str,"\n");
            str += strlen(str);
        }
    }

    return strdup(string);
}
