/*
 * Copyright (C) 1998-2002 Tom Holroyd <tomh@kurage.nimh.nih.gov>
 * Copyright (C) 2006-2009 Stephan Kulow <coolo@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "freecellsolver.h"

#include "../freecell.h"


/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) ((((a) ^ (b)) & PS_COLOR) == PS_COLOR)

/* Statistics. */

int FreecellSolver::Xparam[] = { 4, 1, 8, -1, 7, 11, 4, 2, 2, 1, 2 };

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

	for (i = 0; i < NNEED; ++i) {
		pile[i] = -1;
	}
	npile = 0;

	for (s = 0; s < 4; ++s) {
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

	for (w = 0; w < Nwpiles; ++w) {
		j = Wlen[w];
		for (i = 0; i < j; ++i) {
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

	for (i = 0, mp = mp0; i < n; ++i, ++mp) {
		if (mp->card_index != -1) {
			w = mp->from;
			for (j = 0; j < npile; ++j) {
				if (w == pile[j]) {
					mp->pri += Xparam[0];
				}
			}
			if (Wlen[w] > 1) {
				card = W[w][Wlen[w] - 2];
				for (s = 0; s < 4; ++s) {
					if (card == need[s]) {
						mp->pri += Xparam[1];
						break;
					}
				}
			}
			if (mp->totype == W_Type) {
				for (j = 0; j < npile; ++j) {
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
	for (w = 0; w < Nwpiles + Ntpiles; ++w) {
		if (Wlen[w] > 0) {
			card = *Wp[w];
			o = SUIT(card);
			empty = (O[o] == NONE);
			if ((empty && (RANK(card) == PS_ACE)) ||
			    (!empty && (RANK(card) == O[o] + 1))) {
				mp->card_index = 0;
				mp->from = w;
				mp->to = o;
				mp->totype = O_Type;
                                mp->turn_index = -1;
				mp->pri = 0;    /* unused */
				n++;
				mp++;

				/* If it's an automove, just do it. */

				if (good_automove(o, RANK(card))) {
					*a = true;
                                        mp[-1].pri = 127;
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
	for (w = 0; w < Nwpiles; ++w) {
		if (Wlen[w] == 0) {
			emptyw = w;
			break;
		}
	}
	if (emptyw >= 0) {
		for (i = 0; i < Nwpiles + Ntpiles; ++i) {
			if (i == emptyw || Wlen[i] == 0) {
				continue;
			}
                        bool allowed = false;
                        if ( i < Nwpiles)
                            allowed = true;
                        if ( i >= Nwpiles )
                            allowed = true;
                        if ( allowed ) {
				card = *Wp[i];
				mp->card_index = 0;
				mp->from = i;
				mp->to = emptyw;
				mp->totype = W_Type;
                                mp->turn_index = -1;
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

	for (i = 0; i < Nwpiles + Ntpiles; ++i) {
		if (Wlen[i] > 0) {
			card = *Wp[i];
			for (w = 0; w < Nwpiles; ++w) {
				if (i == w) {
					continue;
				}
				if (Wlen[w] > 0 &&
				    (RANK(card) == RANK(*Wp[w]) - 1 &&
				     suitable(card, *Wp[w]))) {
					mp->card_index = 0;
					mp->from = i;
					mp->to = w;
					mp->totype = W_Type;
                                        mp->turn_index = -1;
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

        for (t = 0; t < Ntpiles; ++t) {
               if (!Wlen[t+Nwpiles]) {
                       break;
               }
        }

        if (t < Ntpiles) {
               for (w = 0; w < Nwpiles; ++w) {
                       if (Wlen[w] > 0) {
                               card = *Wp[w];
                               mp->card_index = 0;
                               mp->from = w;
                               mp->turn_index = -1;
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

void FreecellSolver::unpack_cluster( unsigned int k )
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
    for (int o = 0; o < 4; ++o) {
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

	for (int w = 0; w < Ntpiles; ++w)
        {
            int i = translate_pile( deal->freecell[w], W[w+Nwpiles], 52 );
            Wp[w+Nwpiles] = &W[w+Nwpiles][i-1];
            Wlen[w+Nwpiles] = i;
            total += i;
	}

	/* Output piles, if any. */
	for (int i = 0; i < 4; ++i) {
		O[i] = NONE;
	}
	if (total != 52) {
            for (int i = 0; i < 4; ++i) {
                KCard *c = deal->target[i]->topCard();
                if (c) {
                    O[translateSuit( c->suit() ) >> 4] = c->rank();
                    total += c->rank();
                }
            }
	}
}

MoveHint FreecellSolver::translateMove( const MOVE &m )
{
    // this is tricky as we need to want to build the "meta moves"

    PatPile *frompile = 0;
    if ( m.from < 8 )
        frompile = deal->store[m.from];
    else
        frompile = deal->freecell[m.from-8];
    KCard *card = frompile->at( frompile->count() - m.card_index - 1);

    if ( m.totype == O_Type )
    {
        PatPile *target = 0;
        PatPile *empty = 0;
        for (int i = 0; i < 4; ++i) {
            KCard *c = deal->target[i]->topCard();
            if (c) {
                if ( c->suit() == card->suit() )
                {
                    target = deal->target[i];
                    break;
                }
            } else if ( !empty )
                empty = deal->target[i];
        }
        if ( !target )
            target = empty;
        return MoveHint( card, target, m.pri );
    } else {
        PatPile *target = 0;
        if ( m.to < 8 )
            target = deal->store[m.to];
        else
            target = deal->freecell[m.to-8];

        return MoveHint( card, target, m.pri );
    }
}

unsigned int FreecellSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    unsigned int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}

void FreecellSolver::print_layout()
{
       int i, t, w, o;

       fprintf(stderr, "print-layout-begin\n");
       for (w = 0; w < Nwpiles; ++w) {
	 fprintf(stderr, "W-Pile%d: ", w);
               for (i = 0; i < Wlen[w]; ++i) {
                       printcard(W[w][i], stderr);
               }
               fputc('\n', stderr);
       }
       for (t = 0; t < Ntpiles; ++t) {
	 fprintf(stderr, "T-Pile%d: ", t+Nwpiles);
           printcard(W[t+Nwpiles][Wlen[t+Nwpiles]], stderr);
       }
       fprintf( stderr, "\n" );
       for (o = 0; o < 4; ++o) {
               printcard(O[o] + Osuit[o], stderr);
       }
       fprintf(stderr, "\nprint-layout-end\n");
}
