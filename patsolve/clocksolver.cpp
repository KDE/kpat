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

#include "clocksolver.h"

#include "../clock.h"

#include <QDebug>

#define PRINT 0
#define PRINT2 0

/* These two routines make and unmake moves. */

void ClockSolver::make_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
        fprintf( stderr, "\nmake move %d from %d out %d (at %d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    else
        fprintf( stderr, "\nmake move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();
#else
    //print_layout();
#endif

    int from, to;
    from = m->from;
    to = m->to;

    card_t card = *Wp[from];
    Wlen[from]--;
    Wp[from]--;

    hashpile(from);
    /* Add to pile. */

    if (m->totype == O_Type) {
        if ( RANK( W[8][to] ) == PS_KING )
            W[8][to] = W[8][to] - PS_KING + PS_ACE;
        else
            W[8][to]++;
        Q_ASSERT( m->card_index == 0 );
        hashpile( 8 );
    } else {
        Wp[to]++;
        *Wp[to] = card;
        Wlen[to]++;
        hashpile( to );
    }

#if PRINT
    print_layout();
#endif
}

void ClockSolver::undo_move(MOVE *m)
{
#if PRINT2
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

    if (m->totype == O_Type) {
        card = W[8][to];
        if ( RANK( card ) == PS_ACE )
            W[8][to] = W[8][to] - PS_ACE + PS_KING;
        else
            W[8][to]--;
        Wp[from]++;
        *Wp[from] = card;
        Wlen[from]++;
        hashpile( 8 );
        hashpile( from );
    } else {
        card = *Wp[to];
        Wp[from]++;
        *Wp[from] = card;
        Wlen[from]++;
        Wp[to]--;
        Wlen[to]--;
        hashpile(to);
        hashpile( from );
    }

#if PRINT2
    print_layout();
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */

int ClockSolver::get_possible_moves(int *a, int *numout)
{
    int w, o;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int first_empty = -1;

    int left_in_play = 0;
    int n = 0;
    mp = Possible;
    for (w = 0; w < 8; ++w)
    {
        left_in_play += Wlen[w];

        if (Wlen[w] > 0)
        {
            card = *Wp[w];
            o = SUIT(card);
            for ( int i = 0; i < 12; ++i )
            {
                if ( o != SUIT( W[8][i] ) )
                    continue;

                if ( RANK( card ) == PS_ACE )
                {
                    if ( RANK( W[8][i] ) != PS_KING )
                        continue;
                } else {
                    if ( RANK( W[8][i] ) != RANK( card ) - 1 )
                        continue;
                }
                mp->card_index = 0;
                mp->from = w;
                mp->to = i;
                mp->totype = O_Type;
                mp->pri = 50;
                mp->turn_index = -1;
                n++;
                mp++;
                //*a = true;
                //return n;
            }
        } else if ( first_empty < 0 )
            first_empty = w;
    }

    /* No more automoves, but remember if there were any moves out. */

    *a = false;
    *numout = n;

    for(int i=0; i<8; ++i)
    {
        if ( !Wlen[i] )
            continue;

        for (int j=0; j < 8; ++j )
        {
            if ( i == j )
                continue;

            card_t card = *Wp[i];

            bool allowed = false;

            if ( Wlen[j] == 0 )
            {
                if ( Wlen[i] > 1 && first_empty == j )
                    allowed = 1;
            } else
            {
                card_t below = *Wp[j];

                if ( RANK(card) == RANK( below ) - 1 )
                {
                    allowed = 1;
                }
            }

            if ( allowed )
            {
                    mp->card_index = 0;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->turn_index = -1;
                    mp->pri = 1;
                    n++;
                    mp++;
            }
        }
    }

    return n;
}

bool ClockSolver::isWon()
{
    // maybe won?
    for ( int i = 0; i < 8; ++i )
        if ( Wlen[i] )
            return false;
    return true;
}

int ClockSolver::getOuts()
{
    int ret = 52;
    for ( int i = 0; i < 8; ++i )
        ret -= Wlen[i];
    return ret;
}

ClockSolver::ClockSolver(const Clock *dealer)
    : Solver()
{
    setNumberPiles( 9 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void ClockSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for ( int w = 0; w < 8; ++w )
    {
        int i = translate_pile(deal->store[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    /* Output piles, if any. */
    for (int i = 0; i < 12; ++i)
    {
        KCard *c = deal->target[i]->topCard();

        // It is not safe to assume that each target pile will always have a
        // card on it. If it takes particularly long to render the card graphics
        // at the start of a new game, it is possible that this method can be
        // called before the initial deal has been completed.
        if (c)
            W[8][i] = translateSuit( c->suit() ) + c->rank();
    }
    Wp[8] = &W[8][11];
    Wlen[8] = 12;
}

MoveHint ClockSolver::translateMove( const MOVE &m )
{
    PatPile *frompile = deal->store[m.from];
    KCard *card = frompile->topCard();

    if ( m.totype == O_Type )
    {
        return MoveHint( card, deal->target[m.to], m.pri );
    } else {
        return MoveHint( card, deal->store[m.to], m.pri );
    }
}

void ClockSolver::print_layout()
{
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 8; ++w) {
        fprintf( stderr, "Play%d: ", w );
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (o = 0; o < 12; ++o) {
        printcard(W[8][o], stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
}
