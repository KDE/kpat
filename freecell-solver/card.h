/*
 * card.h - header file for card functions for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */


#ifndef __CARD_H
#define __CARD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __STATE_H
#include "state.h"
#endif

extern fcs_card_t fcs_card_user2perl(const char * str);
extern char * fcs_card_perl2user(fcs_card_t card, char * str, int t);
extern char * fcs_p2u_card_number(
    int num, 
    char * str, 
    int * card_num_is_null,
    int t);
char * fcs_p2u_deck(
        int deck, 
        char * str, 
        int card_num_is_null);
extern int fcs_u2p_card_number(const char * string);
extern int fcs_u2p_deck(const char * deck);

#ifdef __cplusplus
}
#endif

#endif /* __CARD_H */
