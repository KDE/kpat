#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fcs_user.h"


#ifndef _MT
static long microsoft_holdrand = 1L;
#endif  /* _MT */

/***
*void srand(seed) - seed the random number generator
*
*Purpose:
*       Seeds the random number generator with the int given.  Adapted from the
*       BASIC random number generator.
*
*Entry:
*       unsigned seed - seed to seed rand # generator with
*
*Exit:
*       None.
*
*Exceptions:
*
*******************************************************************************/

void microsoft_srand (
        unsigned int seed
        )
{
#ifdef _MT

        _getptd()->_holdrand = (unsigned long)seed;

#else  /* _MT */
        microsoft_holdrand = (long)seed;
#endif  /* _MT */
}


/***
*int rand() - returns a random number
*
*Purpose:
*       returns a pseudo-random number 0 through 32767.
*
*Entry:
*       None.
*
*Exit:
*       Returns a pseudo-random number 0 through 32767.
*
*Exceptions:
*
*******************************************************************************/

int microsoft_rand (
        void
        )
{
#ifdef _MT

        _ptiddata ptd = _getptd();

        return( ((ptd->_holdrand = ptd->_holdrand * 214013L
            + 2531011L) >> 16) & 0x7fff );

#else  /* _MT */
        return(((microsoft_holdrand = microsoft_holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
#endif  /* _MT */
}


typedef int CARD;

#define     BLACK           0               /* COLOUR(card) */
#define     RED             1

#define     ACE             0               /*  VALUE(card) */
#define     DEUCE           1

#define     CLUB            3               /*  SUIT(card)  */
#define     DIAMOND         1
#define     HEART           2
#define     SPADE           0

#define     SUIT(card)      ((card) % 4)
#define     VALUE(card)     ((card) / 4)
#define     COLOUR(card)    (SUIT(card) == DIAMOND || SUIT(card) == HEART)

#define     MAXPOS         21
#define     MAXCOL          9    /* includes top row as column 0 */

char * card_to_string(char * s, CARD card, int not_append_ws)
{
    int suit = SUIT(card);
    int v = VALUE(card)+1;

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
        case CLUB:
            strcat(s, "C");
            break;
        case DIAMOND:
            strcat(s, "D");
            break;
        case HEART:
            strcat(s, "H");
            break;
        case SPADE:
            strcat(s, "S");
            break;
    }
    if (!not_append_ws)
    {
        strcat(s, " ");
    }

    
    return s;
}

char * get_board(int gamenumber)
{

    CARD    card[MAXCOL][MAXPOS];    /* current layout of cards, CARDs are ints */

    int  i, j;                /*  generic counters */
    int  wLeft = 52;          /*  cards left to be chosen in shuffle */
    CARD deck[52];            /* deck of 52 unique cards */
    char * ret;
    
    
    ret = malloc(1024);
    ret[0] = '\0';

    /* shuffle cards */

    for (i = 0; i < 52; i++)      /* put unique card in each deck loc. */
        deck[i] = i;

    microsoft_srand(gamenumber);            /* gamenumber is seed for rand() */
    for (i = 0; i < 52; i++)
    {
        j = microsoft_rand() % wLeft;
        card[(i%8)+1][i/8] = deck[j];
        deck[j] = deck[--wLeft];
    }
    
    
    {
        int stack;
        int c;

        char card_string[10];

        for(stack=1 ; stack<9 ; stack++ )
        {
            for(c=0 ; c < (6+(stack<5)) ; c++)
            {
                sprintf(ret+strlen(ret), "%s", 
                    card_to_string(
                        card_string, 
                        card[stack][c], 
                        (c == (6+(stack<5)))
                    )
                );        
            }
            sprintf(ret+strlen(ret), "%s", "\n");
        }
    }

    return ret;
}

#define LIMIT_STEP 10000
#define LIMIT_MAX 50000

int main(int argc, char * argv[])
{
    void * user;
    /* char buffer[2048]; */
    int ret;
    fcs_move_t move;
    int board_num;
    char * buffer;
    int limit;
    int start_board, end_board, stop_at;
    char line[80];
    
    
    start_board = atoi(argv[1]);
    end_board = atoi(argv[2]);
    stop_at = atoi(argv[3]);

    /* for(board_num=1;board_num<100000;board_num++) */
    for(board_num=start_board;board_num<=end_board;board_num++)
    {
        user = freecell_solver_user_alloc();
        freecell_solver_user_set_solving_method(
            user,
            FCS_METHOD_SOFT_DFS
            );

        buffer = get_board(board_num);
        
        limit = LIMIT_STEP;
        
        freecell_solver_user_limit_iterations(user, limit);

        ret = freecell_solver_user_solve_board(user, buffer);
        
        printf("%i, %i\n", board_num, limit);
        fflush(stdout);
        
        free(buffer);

        while ((ret == FCS_STATE_SUSPEND_PROCESS) && (limit < LIMIT_MAX))
        {
            limit += LIMIT_STEP;
            freecell_solver_user_limit_iterations(user, limit);
            ret = freecell_solver_user_resume_solution(user);
            printf("%i, %i\n", board_num, limit);
            fflush(stdout);

        }

        if (ret == FCS_STATE_WAS_SOLVED)
        {
            while (freecell_solver_user_get_next_move(user, &move) == 0)
            {
            }
        }

        freecell_solver_user_free(user);
        
        if (board_num % stop_at == 0)
        {
            printf ("Press Return to continue:\n");
            fgets(line, sizeof(line), stdin);
        }
    }    
    
    return 0;
}
