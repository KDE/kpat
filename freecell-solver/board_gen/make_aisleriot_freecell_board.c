/*
 * make_aisleriot_freecell_board.c - Program to generate a GNOME Aisleriot 
 * Freecell board for input to Freecell Solver.
 *
 * Usage: make-aisleriot-frecell-board [board number] | fc-solve
 *
 * Note: this program uses srandom() and random() so it generates different
 * boards on different systems. If you want it to generate the board
 * you are playing, make sure it uses the same libc as the computer on
 * which you run GNOME Aisleriot.
 *
 * Written by Shlomi Fish, 2000
 *
 * This code is under the public domain.
 * 
 * 
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define make_card(value, suit ) (((value)<<8)+(suit))
#define get_suit(card) ((card)&0xFF)
#define get_value(card) ((card)>>8)

#ifdef WIN32
#define random() (rand())
#define srandom(a) (srand(a))
#endif

int deck[52];

void make_standard_deck(void)
{
    int suit, value, card_num;
    card_num = 0;
    for(suit=0;suit<4;suit++)
    {
        for(value=1;value<=13;value++)
        {
            deck[card_num] = make_card(value,suit);
            card_num++;
        }
    }
}

void shuffle_deck(void)
{
    int ref1, ref2;
    int temp_card;
    int len = sizeof(deck) / sizeof(deck[0]);
    
    for(ref1=0 ; ref1<len ; ref1++)
    {
        ref2 = ref1 + (random()%(len-ref1));
        temp_card = deck[ref2];
        deck[ref2] = deck[ref1];
        deck[ref1] = temp_card;
    }
}

char * card_to_string(char * s, int card, int not_append_ws)
{
    int suit = get_suit(card);
    int v = get_value(card);

    if (v == 1)
    {
        strcpy(s, "A");
    }
    else if (v <= 10)
    {
        sprintf(s, "%i", v);
    }
    else
    {
        strcpy(s, (v == 11)?"J":((v == 12)?"Q":"K"));
    }

    switch (suit)
    {
        case 0:
            strcat(s, "C");
            break;
        case 1:
            strcat(s, "D");
            break;
        case 2:
            strcat(s, "H");
            break;
        case 3:
            strcat(s, "S");
            break;
    }

    if (!not_append_ws)
    {
        strcat(s, " ");
    }
    
    return s;
}

int main(int argc, char * argv[])
{
    char output[8][30];
    char card_string[10];
    int i;
    
    if (argc == 1)
    {
        srandom(time(NULL));
    }
    else
    {
        srandom(atoi(argv[1]));
    }

    make_standard_deck();
    shuffle_deck();


    for (i = 0; i < 8; i++) {
        output[i][0] = '\0';
    }

    for (i = 0; i < 52; i++) {
        strcat(output[i % 8], card_to_string(card_string, deck[i], (i>=52-8)));
    }

    for (i = 0; i < 8; i++) {
        printf("%s\n", output[i]);
    }

    return 0;
}

    
 
