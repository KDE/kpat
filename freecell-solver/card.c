/*
 * card.c - card functions module for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#include <string.h>

#include "card.h"


#define uc(c) ( (((c)>='a') && ((c)<='z')) ?  ((c)+'A'-'a') : (c))

int fcs_u2p_card_number(char * string)
{
    char rest = uc(*string);

    if (rest == 'A')
    {
        return 1;
    }
    else if (rest =='J')
    {
        return 11;
    }
    else if (rest == 'Q')
    {
        return 12;
    }
    else if (rest == 'K')
    {
        return 13;
    }
    else if (rest == '1')
    {
        return (*(string+1) == '0')?10:1;
    }
    else if ((rest == '0') || (rest == 'T'))
    {
        return 10;
    }
    else if ((rest >= '2') && (rest <= '9'))
    {
        return (rest-'0');
    }
    else
    {
        return 0;
    }
}



int fcs_u2p_deck(char * deck)
{
    char c;

    c = uc(*deck);
    while (
            (c != 'H') &&
            (c != 'S') &&
            (c != 'D') &&
            (c != 'C') &&
            (c != ' ') &&
            (c != '\0'))
    {
        deck++;
        c = uc(*deck);
    }

    if (c == 'H')
        return 0;
    else if (c == 'S')
        return 1;
    else if (c == 'D')
        return 2;
    else if (c == 'C')
        return 3;
    else
        return 0;
}

fcs_card_t fcs_card_user2perl(char * str)
{
    fcs_card_t card;
#if defined(COMPACT_STATES)||defined(INDIRECT_STACK_STATES)
    card = 0;
#endif
    fcs_card_set_num(card, fcs_u2p_card_number(str));
    fcs_card_set_deck(card, fcs_u2p_deck(str));

    return card;
}

#ifdef CARD_DEBUG_PRES
char card_map_3_10[14][4] = { "*", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };

char card_map_3_T[14][4] = { "*", "A", "2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K" };

#else
char card_map_3_10[14][4] = { " ", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };

char card_map_3_T[14][4] = { " ", "A", "2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K" };

#endif

char * fcs_p2u_card_number(int num, char * str, int * card_num_is_null, int t)
{
    char (*card_map_3) [4] = card_map_3_10;
    if (t)
    {
        card_map_3 = card_map_3_T;
    }
    if ((num >= 0) && (num <= 13))
    {
        strncpy(str, card_map_3[num], strlen(card_map_3[num])+1);
        *card_num_is_null = (num == 0);
    }
    else
    {
        strncpy(str, card_map_3[0], strlen(card_map_3[0])+1); 
        *card_num_is_null = 1;
    }            
    return str;
}

char * fcs_p2u_deck(int deck, char * str, int card_num_is_null)
{
    if (deck == 0)
    {
        if (card_num_is_null)
#ifdef CARD_DEBUG_PRES
            strncpy(str, "*", 2);
#else
            strncpy(str, " ", 2);
#endif
        else
            strncpy(str, "H", 2);
    }
    else if (deck == 1)
        strncpy(str, "S", 2);
    else if (deck == 2)
        strncpy(str, "D", 2);
    else if (deck == 3)
        strncpy(str, "C", 2);
    else
        strncpy(str, " ", 2);
    return str;
}

char * fcs_card_perl2user(fcs_card_t card, char * str, int t)
{
    int card_num_is_null;

    fcs_p2u_card_number(
            fcs_card_card_num(card), 
            str, 
            &card_num_is_null, 
            t);
    fcs_p2u_deck(fcs_card_deck(card), str+strlen(str), card_num_is_null);

    return str;
}
