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

#include "idiotsolver.h"

#include "../idiot.h"

#include <QDebug>


#define PRINT 0

/* These two routines make and unmake moves. */

void IdiotSolver::make_move(MOVE *m)
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

    if ( from == 4 )
    {
        Q_ASSERT( Wlen[from] >= 4 );

        for ( int i = 0; i < 4; ++i )
        {
            Wp[i]++;
            card = *Wp[from];
            *Wp[i] = ( SUIT( card ) << 4 ) + RANK( card );
            Wp[from]--;
            Wlen[from]--;
            Wlen[i]++;
            hashpile( i );
        }
        hashpile( from );
    } else {

        card = *Wp[from];
        Wp[from]--;
        Wlen[from]--;
        Wp[to]++;
        *Wp[to] = card;
        Wlen[to]++;

        hashpile( to );
        hashpile(from);
    }

#if PRINT
    print_layout();
#endif
}

void IdiotSolver::undo_move(MOVE *m)
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

    if ( from == 4 )
    {
        for ( int i = 3; i >= 0; --i )
        {
            card = *Wp[i];
            Wp[i]--;
            Wlen[i]--;
            Wp[from]++;
            *Wp[from] = ( SUIT( card ) << 4 ) + RANK( card ) + ( 1 << 7 );
            Wlen[from]++;
            hashpile( i );
        }
        hashpile( from );

    } else {

        card = *Wp[to];
        Wp[to]--;
        Wlen[to]--;
        Wp[from]++;
        *Wp[from] = card;
        Wlen[from]++;

        hashpile( to );
        hashpile(from);
    }

#if PRINT
    print_layout();
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */

int IdiotSolver::get_possible_moves(int *a, int *numout)
{
    MOVE *mp = Possible;
    int n = 0;

    *a = false;
    *numout = n;

    for ( int i = 0; i < 4; ++i )
        if ( Wlen[i] && canMoveAway( i ) )
        {
            mp->card_index = 0;
            mp->from = i;
            mp->to = 5;
            mp->totype = W_Type;
            mp->turn_index = -1;
            mp->pri = 30;
            n++;
            mp++;
            return 1;
        }

    if ( Wlen[4] )
    {
        mp->card_index = 0;
        mp->from = 4;
        mp->to = 0;
        mp->totype = W_Type;
        mp->turn_index = 0;
        mp->pri = 2;
        n++;
        mp++;
    }

    // now let's try to be a bit clever with the empty piles
    for( int i = 0; i < 4; ++i )
    {
        if (Wlen[i] == 0)
        {
            // Find a card to move there
            for( int j = 0; j < 4; ++j )
            {
                if ( i != j && Wlen[j]>0 )
                {
                    mp->card_index = 0;
                    mp->from = j;
                    mp->to = i;
                    mp->totype = W_Type;
                    mp->turn_index = 0;
                    mp->pri = 2;
                    n++;
                    mp++;
                }
            }
        }
    }

    return n;
}

bool IdiotSolver::isWon()
{
    // maybe won?
    return Wlen[5] == 48;
}

int IdiotSolver::getOuts()
{
    return Wlen[5];
}

IdiotSolver::IdiotSolver(const Idiot *dealer)
    : Solver()
{
    setNumberPiles( 6 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void IdiotSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for ( int w = 0; w < 4; ++w ) {
        int i = translate_pile(deal->m_play[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    int i = translate_pile( deal->talon, W[4], 52 );
    Wp[4] = &W[4][i-1];
    Wlen[4] = i;
    total += i;

    i = translate_pile( deal->m_away, W[5], 52 );
    Wp[5] = &W[5][i-1];
    Wlen[5] = i;
    total += i;

    Q_ASSERT( total == 52 );
}

MoveHint IdiotSolver::translateMove( const MOVE &m )
{
    if ( m.from >=4 )
        return MoveHint();
    PatPile *frompile = deal->m_play[m.from];

    KCard *card = frompile->at( frompile->count() - m.card_index - 1);
    Q_ASSERT( card );

    PatPile *target = 0;
    if ( m.to == 5 )
        target = deal->m_away;
    else
        target = deal->m_play[m.to];

    return MoveHint( card, target, m.pri );
}

void IdiotSolver::print_layout()
{
    int i, w;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 6; ++w) {
        if ( w == 4 )
            fprintf( stderr, "Deck: " );
        else if ( w == 5 )
            fprintf( stderr, "Away: " );
        else
            fprintf( stderr, "Play%d: ", w );
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
}

inline bool higher( const card_t &c1, const card_t &c2)
{
    // Sanity check.
    if (c1 == c2)
        return false;

    // Must be same suit.
    if ( SUIT( c1 ) != SUIT( c2 ) )
        return false;

    // Aces form a special case.
    if (RANK( c2 ) == PS_ACE)
        return true;
    if (RANK( c1 ) == PS_ACE)
        return false;

    return (RANK( c1 ) < RANK( c2 ));
}

bool IdiotSolver::canMoveAway(int pile ) const
{
    Q_ASSERT( pile < 4 );
    if ( Wlen[pile] == 0 )
        return false;
    for ( int i = 0; i < 4; ++i )
        Q_ASSERT( Wp[i] = &W[i][Wlen[i] - 1] );
    return ( ( Wlen[0] && higher( *Wp[pile], *Wp[0] ) ) ||
             ( Wlen[1] && higher( *Wp[pile], *Wp[1] ) ) ||
             ( Wlen[2] && higher( *Wp[pile], *Wp[2] ) ) ||
             ( Wlen[3] && higher( *Wp[pile], *Wp[3] ) ) );
}
