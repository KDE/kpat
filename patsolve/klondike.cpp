/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <kdebug.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>
#include "klondike.h"
#include "../klondike.h"
#include "../pile.h"
#include "../deck.h"
#include "memory.h"

/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) (COLOR( a ) != COLOR( b ) )
static card_t Suit_mask;
static card_t Suit_val;

#define king_only(card) (RANK(card) == PS_KING)

/* Statistics. */

int KlondikeSolver::Xparam[] = { 4, 1, 8, -1, 7, 11, 4, 2, 2, 1, 2 };

/* These two routines make and unmake moves. */

void KlondikeSolver::make_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from pile. */
        if ( from == 7 && to == 8 )
        {
            while ( Wlen[7] )
            {
                card = W[7][Wlen[7]-1] + ( 1 << 7 );
                Wlen[8]++;
                W[8][Wlen[8]-1] = card;
                Wlen[7]--;
            }
            Wp[7] = &W[7][0];
            Wp[8] = &W[8][Wlen[8]-1];
            hashpile( 7 );
            hashpile( 8 );
            return;
        }

        card = *Wp[from]--;
        Wlen[from]--;
        hashpile(from);

	/* Add to pile. */

	if (m->totype == O_Type) {
            O[to]++;
        } else {
            Wp[to]++;
            if ( m->turn )
                if ( DOWN( card ) )
                    card = ( SUIT( card ) << 4 ) + RANK( card );
                else
                    card += ( 1 << 7 );
            *Wp[to] = card;
            Wlen[to]++;
            hashpile(to);
	}
}

void KlondikeSolver::undo_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from 'to' pile. */

        if ( from == 7 && to == 8 )
        {
            while ( Wlen[8] )
            {
                card = W[8][Wlen[8]-1];
                card = ( SUIT( card ) << 4 ) + RANK( card );
                Wlen[7]++;
                W[7][Wlen[7]-1] = card;
                Wlen[8]--;
            }
            Wp[8] = &W[8][0];
            Wp[7] = &W[7][Wlen[7]-1];
            hashpile( 7 );
            hashpile( 8 );
            return;
        }

	if (m->totype == O_Type) {
            card = O[to] + Osuit[to];
            O[to]--;
        } else {
            card = *Wp[to]--;
            Wlen[to]--;
            hashpile(to);
	}

	/* Add to 'from' pile. */
        Wp[from]++;
        if ( m->turn )
            if ( DOWN( card ) )
                card = ( SUIT( card ) << 4 ) + RANK( card );
            else
                card += ( 1 << 7 );
        *Wp[from] = card;
        Wlen[from]++;
        hashpile(from);
}

/* Move prioritization.  Given legal, pruned moves, there are still some
that are a waste of time, especially in the endgame where there are lots of
possible moves, but few productive ones.  Note that we also prioritize
positions when they are added to the queue. */

#define NNEED 8

void KlondikeSolver::prioritize(MOVE *mp0, int n)
{
#if 0
	int i, j, s, w, pile[NNEED], npile;
	card_t card, need[4];
	MOVE *mp;

	/* There are 4 cards that we "need": the next cards to go out.  We
	give higher priority to the moves that remove cards from the piles
	containing these cards. */

	for (i = 0; i < NNEED; i++) {
		pile[i] = -1;
	}
	npile = 0;

	for (s = 0; s < 4; s++) {
		need[s] = NONE;
		if (O[s] == NONE) {
			need[s] = Osuit[s] + PS_ACE;
		} else if (O[s] != PS_KING) {
			need[s] = Osuit[s] + O[s] + 1;
		}
	}

	/* Locate the needed cards.  There's room for optimization here,
	like maybe an array that keeps track of every card; if maintaining
	such an array is not too expensive. */

	for (w = 0; w < Nwpiles; w++) {
		j = Wlen[w];
		for (i = 0; i < j; i++) {
			card = W[w][i];
			s = SUIT(card);

			/* Save the locations of the piles containing
			not only the card we need next, but the card
			after that as well. */

			if (need[s] != NONE &&
			    (card == need[s] || card == need[s] + 1)) {
				pile[npile++] = w;
				if (npile == NNEED) {
					break;
				}
			}
		}
		if (npile == NNEED) {
			break;
		}
	}

	/* Now if any of the moves remove a card from any of the piles
	listed in pile[], bump their priority.  Likewise, if a move
	covers a card we need, decrease its priority.  These priority
	increments and decrements were determined empirically. */

	for (i = 0, mp = mp0; i < n; i++, mp++) {
		if (mp->card != NONE) {
			w = mp->from;
			for (j = 0; j < npile; j++) {
				if (w == pile[j]) {
					mp->pri += Xparam[0];
				}
			}
			if (Wlen[w] > 1) {
				card = W[w][Wlen[w] - 2];
				for (s = 0; s < 4; s++) {
					if (card == need[s]) {
						mp->pri += Xparam[1];
						break;
					}
				}
			}
			if (mp->totype == W_Type) {
				for (j = 0; j < npile; j++) {
					if (mp->to == pile[j]) {
						mp->pri -= Xparam[2];
					}
				}
			}
		}
	}
#endif
}

/* Automove logic.  Klondike games must avoid certain types of automoves. */

int KlondikeSolver::good_automove(int o, int r)
{
	int i;

	if (r <= 2) {
		return true;
	}

	/* Check the Out piles of opposite color. */

	for (i = 1 - (o & 1); i < 4; i += 2) {
		if (O[i] < r - 1) {

#if 1   /* Raymond's Rule */
			/* Not all the N-1's of opposite color are out
			yet.  We can still make an automove if either
			both N-2's are out or the other same color N-3
			is out (Raymond's rule).  Note the re-use of
			the loop variable i.  We return here and never
			make it back to the outer loop. */

			for (i = 1 - (o & 1); i < 4; i += 2) {
				if (O[i] < r - 2) {
					return false;
				}
			}
			if (O[(o + 2) & 3] < r - 3) {
				return false;
			}

			return true;
#else   /* Horne's Rule */
			return false;
#endif
		}
	}

	return true;
}

/* Get the possible moves from a position, and store them in Possible[]. */

int KlondikeSolver::get_possible_moves(int *a, int *numout)
{
    int w, o, empty;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (w = 0; w < 8; w++) {
        if (Wlen[w] > 0) {
            card = *Wp[w];
            o = SUIT(card);
            empty = (O[o] == NONE);
            if ((empty && (RANK(card) == PS_ACE)) ||
                (!empty && (RANK(card) == O[o] + 1))) {
                mp->card = card;
                mp->from = w;
                mp->to = o;
                mp->totype = O_Type;
                mp->pri = 0;    /* unused */
                mp->turn = false;
                n++;
                mp++;

                /* If it's an automove, just do it. */

                if (good_automove(o, RANK(card))) {
                    *a = true;
                    if (n != 1) {
                        Possible[0] = mp[-1];
                        return 1;
                    }
                    return n;
                }
            }
        }
    }

    /* No more automoves, but remember if there were any moves out. */

    *a = false;
    *numout = n;

    /* check for deck->pile */
    if ( Wlen[8] ) {
        card = *Wp[8];
        mp->card = *Wp[8];
        mp->from = 8;
        mp->to = 7;
        mp->totype = W_Type;
        mp->pri = 0;
        mp->turn = true;
        n++;
        mp++;
    }


    for(int i=0; i<8; i++)
    {
        int len = Wlen[i];
        if ( i == 7 && Wlen[i] > 0)
            len = 1;
        for (int l=Wlen[i]-len; l < Wlen[i]; ++l )
        {
            card_t card = W[i][l];
            if ( DOWN( card ) )
                continue;

            for (int j = 0; j < 7; j++)
            {
                if (i == j)
                    continue;

                bool allowed = false;

                if ( Wlen[j] > 0 &&
                     RANK(card) == RANK(*Wp[j]) - 1 &&
                     suitable( card, *Wp[j] ) )
                    allowed = true;
                if ( Wlen[j] == 0 &&
                     RANK(card) ==  PS_KING )
                    allowed = true;
#if 0
                printcard( card, stderr );
                printcard( *Wp[j], stderr );
                kDebug() << " tried " << i << " " << j << " " << allowed << " " << suitable( card, *Wp[j] ) << endl;
#endif
                if ( allowed ) {
                    mp->card = card;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->turn = false;
                    if ( i == 8 )
                        mp->pri = 1;
                    else
                        mp->pri = 7;
                    n++;
                    mp++;
                }
            }
            break; // the first face up
        }
    }

    if ( !Wlen[8] && Wlen[7] )
    {
        card_t card = W[7][0];
        mp->card = card;
        mp->from = 7;
        mp->to = 8;
        mp->totype = W_Type;
        mp->turn = true;
        mp->pri = -1;
        n++;
        mp++;
    }
    return n;
}

void KlondikeSolver::unpack_cluster( int k )
{
    /* Get the Out cells from the cluster number. */
    O[0] = k & 0xF;
    k >>= 4;
    O[1] = k & 0xF;
    k >>= 4;
    O[2] = k & 0xF;
    k >>= 4;
    O[3] = k & 0xF;
}

bool KlondikeSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; o++) {
        if (O[o] != PS_KING) {
            return false;
        }
    }

    return true;
}

int KlondikeSolver::getOuts()
{
    return O[0] + O[1] + O[2] + O[3];
}

KlondikeSolver::KlondikeSolver(const Klondike *dealer)
    : Solver()
{
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    setNumberPiles( 9 );

    Suit_mask = PS_COLOR;
    Suit_val = PS_COLOR;
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void KlondikeSolver::translate_layout()
{
	/* Read the workspace. */

    int total = 0;
	for ( int w = 0; w < 7; ++w ) {
		int i = translate_pile(deal->play[w], W[w], 52);
		Wp[w] = &W[w][i - 1];
		Wlen[w] = i;
		total += i;
	}

        int i = translate_pile( deal->pile, W[7], 52 );
        Wp[7] = &W[7][i-1];
        Wlen[7] = i;
        total += i;

        i = translate_pile( Deck::deck(), W[8], 52 );
        Wp[8] = &W[8][i-1];
        Wlen[8] = i;
        total += i;

        /* Output piles, if any. */
	for (int i = 0; i < 4; i++) {
		O[i] = NONE;
	}
	if (total != 52) {
            for (int i = 0; i < 4; i++) {
                Card *c = deal->target[i]->top();
                if (c) {
                    O[c->suit()] = c->rank();
                    total += c->rank();
                }
            }
	}
}

int KlondikeSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}

bool KlondikeSolver::print_layout()
{
    int count = 0;
    for (int w = 0; w < 9; w++)
        for (int i = 0; i < Wlen[w]; i++)
            if ( SUIT( W[w][i] ) == 2 && RANK( W[w][i] ) == 4  )
                count++;
    if ( O[2] >= 4 )
        count++;

    kDebug() << "count " << count << endl;
    if ( count == 1 ) {
        return true;

    }

       int i, w, o;

       fprintf(stderr, "print-layout-begin\n");
       for (w = 0; w < 9; w++) {
           if ( w == 8 )
               fprintf( stderr, "Deck: " );
           else if ( w == 7 )
               fprintf( stderr, "Pile: " );
           else
               fprintf( stderr, "Play%d: ", w );
           for (i = 0; i < Wlen[w]; i++) {
               printcard(W[w][i], stderr);
           }
           fputc('\n', stderr);
       }
       fprintf( stderr, "Off: " );
       for (o = 0; o < 4; o++) {
               printcard(O[o] + Osuit[o], stderr);
       }
       fprintf(stderr, "\nprint-layout-end\n");
       return false;
}
