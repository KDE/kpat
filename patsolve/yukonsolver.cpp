/*
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

#include "yukonsolver.h"

#include "../yukon.h"

#include <QDebug>


/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) (COLOR( a ) != COLOR( b ) )

#define PRINT 0

/* These two routines make and unmake moves. */


void YukonSolver::make_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
        fprintf( stderr, "\nmake move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else
        fprintf( stderr, "\nmake move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();
#else
    //print_layout();
#endif

	int from, to;
	card_t card = NONE;

	from = m->from;
	to = m->to;

        for ( int l = m->card_index; l >= 0; --l )
        {
            card = W[from][Wlen[from]-l-1];
            Wp[from]--;
            if ( m->totype != O_Type )
            {
                Wp[to]++;
                *Wp[to] = card;
                Wlen[to]++;
            }
        }
        Wlen[from] -= m->card_index + 1;

        if ( m->turn_index == 0 )
        {
            if ( DOWN( card ) )
                card = ( SUIT( card ) << 4 ) + RANK( card );
            else
                card += ( 1 << 7 );
            W[to][Wlen[to]-m->card_index-1] = card;
        } else if ( m->turn_index != -1 )
        {
            card_t card2 = *Wp[from];
            if ( DOWN( card2 ) )
                card2 = ( SUIT( card2 ) << 4 ) + RANK( card2 );
            *Wp[from] = card2;
        }

        hashpile(from);
	/* Add to pile. */

	if (m->totype == O_Type) {
            O[to]++;
            Q_ASSERT( m->card_index == 0 );
        } else {
            hashpile(to);
	}
#if PRINT
        print_layout();
#endif
}

void YukonSolver::undo_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
        fprintf( stderr, "\nundo move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else
        fprintf( stderr, "\nundo move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();

#endif
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

        /* Add to 'from' pile. */
        if ( m->turn_index > 0 )
        {
            card_t card2 = *Wp[from];
            if ( !DOWN( card2 ) )
                card2 = ( SUIT( card2 ) << 4 ) + RANK( card2 ) + ( 1 << 7 );
            *Wp[from] = card2;
        }

	if (m->totype == O_Type) {
            card = O[to] + Osuit[to];
            O[to]--;
            Wp[from]++;
            *Wp[from] = card;
            Wlen[from]++;
        } else {
            for ( int l = m->card_index; l >= 0; --l )
            {
                card = W[to][Wlen[to]-l-1];
                Wp[from]++;
                *Wp[from] = card;
                Wlen[from]++;
                Wp[to]--;
            }
            Wlen[to] -= m->card_index + 1;
            hashpile(to);
	}

        if ( m->turn_index == 0 )
        {
            card_t card = *Wp[from];
            if ( DOWN( card ) )
                card = ( SUIT( card ) << 4 ) + RANK( card );
            else
                card += ( 1 << 7 );
            *Wp[from] = card;
        }

        hashpile(from);
#if PRINT
        print_layout();
#endif
}

/* Automove logic.  Yukon games must avoid certain types of automoves. */

int YukonSolver::good_automove(int o, int r)
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

int YukonSolver::get_possible_moves(int *a, int *numout)
{
    int w, o, empty;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (w = 0; w < 7; ++w) {
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
                mp->pri = 3;    /* unused */
                mp->turn_index = -1;
                if ( Wlen[w] > 1 && DOWN( W[w][Wlen[w]-2] ) )
                    mp->turn_index = 1;
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

    for(int i=0; i<7; ++i)
    {
        int len = Wlen[i];
        for (int l=0; l < len; ++l )
        {
            card_t card = W[i][Wlen[i]-1-l];
            if ( DOWN( card ) )
                break;

            for (int j = 0; j < 7; ++j)
            {
                if (i == j)
                    continue;

                int allowed = 0;

                if ( Wlen[j] > 0 &&
                     RANK(card) == RANK(*Wp[j]) - 1 &&
                     suitable( card, *Wp[j] ) )
                {
                    allowed = 1;
                    if ( Wlen[i] == l + 1 ) {
                        allowed = 2;
                    } else {
                        if ( DOWN( W[i][Wlen[i]-l-2] ) )
                            allowed = 3;
                    }
                }
                if ( RANK( card ) == PS_KING && Wlen[j] == 0 )
                {
                    if ( l != Wlen[i]-1 || i == 7 )
                        allowed = 4;
                }
                // TODO: there is no point in moving if we're not opening anything
                // e.g. if both i and j have perfect runs below the cards
#if 0
                fprintf( stderr, "%d %d %d\n", i, l, j );
                printcard( card, stderr );
                printcard( *Wp[j], stderr );
                fprintf( stderr, " allowed %d\n",allowed );
#endif
                if ( allowed ) {
                    mp->card_index = l;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->turn_index = -1;
                    if ( Wlen[i] > l+1 && DOWN( W[i][Wlen[i]-l-2] ) )
                        mp->turn_index = 1;
                    if ( mp->turn_index > 0 || Wlen[i] == l+1)
                         mp->pri = 30;
                    else
                         mp->pri = 1;
                    n++;
                    mp++;
                }
            }
        }
    }

    return n;
}

void YukonSolver::unpack_cluster( unsigned int k )
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

bool YukonSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; ++o) {
        if (O[o] != PS_KING) {
            return false;
        }
    }

    return true;
}

int YukonSolver::getOuts()
{
    return O[0] + O[1] + O[2] + O[3];
}

YukonSolver::YukonSolver(const Yukon *dealer)
    : Solver()
{
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    setNumberPiles( 7 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void YukonSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for ( int w = 0; w < 7; ++w ) {
        int i = translate_pile(deal->store[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
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
    Q_ASSERT( total == 52 );
}

unsigned int YukonSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    unsigned int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}

MoveHint YukonSolver::translateMove( const MOVE &m )
{
    PatPile *frompile = 0;
    frompile = deal->store[m.from];

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
        return MoveHint( card, deal->store[m.to], m.pri );
    }
}

void YukonSolver::print_layout()
{
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 7; ++w) {
        fprintf( stderr, "Play%d: ", w );
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (o = 0; o < 4; ++o) {
        printcard(O[o] + Osuit[o], stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
}
