/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>
#include "freecell.h"
#include "../freecell.h"
#include "../pile.h"
#include "memory.h"

/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) ((((a) ^ (b)) & Suit_mask) == Suit_val)
static card_t Suit_mask;
static card_t Suit_val;

#define king_only(card) (RANK(card) == PS_KING)

/* Statistics. */

int FreecellSolver::Xparam[] = { 4, 1, 8, -1, 7, 11, 4, 2, 2, 1, 2 };

/* Hash the whole layout.  This is called once, at the start. */

void FreecellSolver::hash_layout(void)
{
	int w;

	for (w = 0; w < Nwpiles+Ntpiles; w++) {
		hashpile(w);
	}
}

/* These two routines make and unmake moves. */

void FreecellSolver::make_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from pile. */

        card = *Wp[from]--;
        Wlen[from]--;
        hashpile(from);

	/* Add to pile. */

	if (m->totype == O_Type) {
            O[to]++;
        } else {
            *++Wp[to] = card;
            Wlen[to]++;
            hashpile(to);
	}
}

void FreecellSolver::undo_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from 'to' pile. */

	if (m->totype == O_Type) {
            card = O[to] + Osuit[to];
            O[to]--;
        } else {
            card = *Wp[to]--;
            Wlen[to]--;
            hashpile(to);
	}

	/* Add to 'from' pile. */
        *++Wp[from] = card;
        Wlen[from]++;
        hashpile(from);
}

/* Move prioritization.  Given legal, pruned moves, there are still some
that are a waste of time, especially in the endgame where there are lots of
possible moves, but few productive ones.  Note that we also prioritize
positions when they are added to the queue. */

#define NNEED 8

void FreecellSolver::prioritize(MOVE *mp0, int n)
{
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
}

/* Automove logic.  Freecell games must avoid certain types of automoves. */

int FreecellSolver::good_automove(int o, int r)
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

int FreecellSolver::get_possible_moves(int *a, int *numout)
{
	int i, n, t, w, o, empty, emptyw;
	card_t card;
	MOVE *mp;

	/* Check for moves from W to O. */

	n = 0;
	mp = Possible;
	for (w = 0; w < Nwpiles + Ntpiles; w++) {
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

	/* Check for moves from non-singleton W cells to one of any
	empty W cells. */

	emptyw = -1;
	for (w = 0; w < Nwpiles; w++) {
		if (Wlen[w] == 0) {
			emptyw = w;
			break;
		}
	}
	if (emptyw >= 0) {
		for (i = 0; i < Nwpiles + Ntpiles; i++) {
			if (i == emptyw || Wlen[i] == 0) {
				continue;
			}
                        bool allowed = false;
                        if ( i < Nwpiles && king_only(*Wp[i]) )
                            allowed = true;
                        if ( i >= Nwpiles )
                            allowed = true;
                        if ( allowed ) {
				card = *Wp[i];
				mp->card = card;
				mp->from = i;
				mp->to = emptyw;
				mp->totype = W_Type;
                                if ( i >= Nwpiles )
                                    mp->pri = Xparam[6];
                                else
                                    mp->pri = Xparam[3];
				n++;
				mp++;
			}
		}
	}

	/* Check for moves from W to non-empty W cells. */

	for (i = 0; i < Nwpiles + Ntpiles; i++) {
		if (Wlen[i] > 0) {
			card = *Wp[i];
			for (w = 0; w < Nwpiles; w++) {
				if (i == w) {
					continue;
				}
				if (Wlen[w] > 0 &&
				    (RANK(card) == RANK(*Wp[w]) - 1 &&
				     suitable(card, *Wp[w]))) {
					mp->card = card;
					mp->from = i;
					mp->to = w;
					mp->totype = W_Type;
                                        if ( i >= Nwpiles )
                                            mp->pri = Xparam[5];
                                        else
                                            mp->pri = Xparam[4];
					n++;
					mp++;
				}
			}
		}
	}

        /* Check for moves from W to one of any empty T cells. */

        for (t = 0; t < Ntpiles; t++) {
               if (!Wlen[t+Nwpiles]) {
                       break;
               }
        }

        if (t < Ntpiles) {
               for (w = 0; w < Nwpiles; w++) {
                       if (Wlen[w] > 0) {
                               card = *Wp[w];
                               mp->card = card;
                               mp->from = w;
                               mp->to = t+Nwpiles;
                               mp->totype = W_Type;
                               mp->pri = Xparam[7];
                               n++;
                               mp++;
                       }
               }
       }


	return n;
}

void FreecellSolver::unpack_cluster( int k )
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

bool FreecellSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; o++) {
        if (O[o] != PS_KING) {
            return false;
        }
    }

    return true;
}

int FreecellSolver::getOuts()
{
    return O[0] + O[1] + O[2] + O[3];
}

FreecellSolver::FreecellSolver(const Freecell *dealer)
    : Solver()
{
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    Nwpiles = 8;
    Ntpiles = 4;

    setNumberPiles( Nwpiles + Ntpiles );

    Suit_mask = PS_COLOR;
    Suit_val = PS_COLOR;
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void FreecellSolver::translate_layout()
{
	/* Read the workspace. */

	int total = 0;
	for ( int w = 0; w < 8; ++w ) {
		int i = translate_pile(deal->store[w], W[w], 52);
		Wp[w] = &W[w][i - 1];
		Wlen[w] = i;
		total += i;
		if (w == Nwpiles) {
			break;
		}
	}

	/* Temp cells may have some cards too. */

	for (int w = 0; w < Ntpiles; w++)
        {
            int i = translate_pile( deal->freecell[w], W[w+Nwpiles], 52 );
            Wp[w+Nwpiles] = &W[w+Nwpiles][i-1];
            Wlen[w+Nwpiles] = i;
            total += i;
	}

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

int FreecellSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}

void FreecellSolver::print_layout()
{
       int i, t, w, o;

       fprintf(stderr, "print-layout-begin\n");
       for (w = 0; w < Nwpiles; w++) {
               for (i = 0; i < Wlen[w]; i++) {
                       printcard(W[w][i], stderr);
               }
               fputc('\n', stderr);
       }
       for (t = 0; t < Ntpiles; t++) {
           printcard(W[t+Nwpiles][Wlen[t+Nwpiles]], stderr);
       }
       fprintf( stderr, "\n" );
       for (o = 0; o < 4; o++) {
               printcard(O[o] + Osuit[o], stderr);
       }
       fprintf(stderr, "\nprint-layout-end\n");
}
