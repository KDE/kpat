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

#include "grandfsolver.h"

#include "../grandf.h"

#include <QDebug>


#define PRINT 0

/* These two routines make and unmake moves. */

void GrandfSolver::make_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
        fprintf( stderr, "\nmake move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else if ( m->from == offs )
        fprintf( stderr, "\nmake redeal\n\n" );
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

    if ( from == offs )
    {
        // redeal
        m_redeal++;

        card_t deck[52];
        int len = 0;

        for ( int i = m_redeal * 7; i < m_redeal * 7 + 7; ++i )
        {
            Wlen[i] = 0;
            Wp[i] = &W[i][-1];
        }

        for (int pos = 6; pos >= 0; --pos)
        {
            int oldpile = ( m_redeal - 1 ) * 7 + pos;
            for ( int l = 0; l < Wlen[oldpile]; ++l )
            {
                card_t card = W[oldpile][l];
                deck[len++] = ( SUIT( card ) << 4 ) + RANK( card );
            }
        }

        int start = 0;
        int stop = 7-1;
        int dir = 1;

        for (int round=0; round < 7; ++round)
        {
            int i = start;
            do
            {
                card_t card = NONE;
                if (len > 0)
                {
                    card = deck[--len];
                    int currentpile = m_redeal * 7 + i;
                    Wp[currentpile]++;
                    *Wp[currentpile] = card;
                    if ( i != start )
                        *Wp[currentpile] += ( 1 << 7 );
                    Wlen[currentpile]++;
                }
                i += dir;
            } while ( i != stop + dir);
            int t = start;
            start = stop;
            stop = t+dir;
            dir = -dir;
        }

        int j = 0;
        while (len > 0)
        {
            card_t card = deck[--len];
            int currentpile = m_redeal * 7 + ( j + 1 );
            Wp[currentpile]++;
            *Wp[currentpile] = card;
            Wlen[currentpile]++;
            j = (j+1)%6;
        }

        for (int round=0; round < 7; ++round)
        {
            int currentpile = m_redeal * 7 + round;
            if ( Wlen[currentpile] )
            {
                card_t card = W[currentpile][Wlen[currentpile]-1];
                W[currentpile][Wlen[currentpile]-1] = ( SUIT( card ) << 4 ) + RANK( card );
            }
            hashpile( currentpile );
            hashpile( currentpile - 7 );
        }

#if PRINT
        print_layout();
#endif
        return;
    }

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
        if ( DOWN( W[offs][to] ) )
            W[offs][to] = ( SUIT( card ) << 4 ) + PS_ACE;
        else
            W[offs][to]++;
        Q_ASSERT( m->card_index == 0 );
        hashpile( offs );
    } else {
        hashpile(to);
    }
#if PRINT
    print_layout();
#endif
}

void GrandfSolver::undo_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
        fprintf( stderr, "\nundo move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else if ( m->from == offs )
        fprintf( stderr, "\nundo redeal\n\n" );
    else
        fprintf( stderr, "\nundo move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();

#endif
    int from, to;
    card_t card;

    from = m->from;
    to = m->to;

    if ( from == offs )
    {
        /* just erase */

        for ( int i = 0; i < 7; ++i )
        {
            int pile = m_redeal * 7 + i;
            Wlen[pile] = 0;
            Wp[pile] = &W[pile][0];
            hashpile( pile );
        }
        m_redeal--;
#if PRINT
    print_layout();
#endif
        return;
    }

    /* Add to 'from' pile. */
    if ( m->turn_index > 0 )
    {
        card_t card2 = *Wp[from];
        if ( !DOWN( card2 ) )
            card2 = ( SUIT( card2 ) << 4 ) + RANK( card2 ) + ( 1 << 7 );
        *Wp[from] = card2;
    }

    if (m->totype == O_Type) {
        card = W[offs][Osuit[to]];
        W[offs][to]--;
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

/* Get the possible moves from a position, and store them in Possible[]. */

int GrandfSolver::get_possible_moves(int *a, int *numout)
{
    int w, o, empty;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;

    for (w = m_redeal * 7 + 0; w < m_redeal * 7 + 7; ++w) {
        if (Wlen[w] > 0) {
            card = *Wp[w];
            o = SUIT(card);
            empty = DOWN( W[offs][o] );
            if ((empty && (RANK(card) == PS_ACE)) ||
                (!empty && (RANK(card) == RANK( W[offs][o] ) + 1)))
            {
                mp->card_index = 0;
                mp->from = w;
                mp->to = o;
                mp->totype = O_Type;
                mp->pri = 127;    /* unused */
                mp->turn_index = -1;
                if ( Wlen[w] > 1 && DOWN( W[w][Wlen[w]-2] ) )
                    mp->turn_index = 1;
                n++;
                mp++;

                *a = true;
                return n;
            }
        }
    }

    /* No more automoves, but remember if there were any moves out. */

    *a = false;
    *numout = n;

    for(int i= m_redeal * 7 + 0; i < m_redeal * 7 + 7; ++i)
    {
        int len = Wlen[i];
        for (int l=0; l < len; ++l )
        {
            card_t card = W[i][Wlen[i]-1-l];
            if ( DOWN( card ) )
                break;

            for (int j = m_redeal * 7 + 0; j < m_redeal * 7 + 7; ++j)
            {
                if (i == j)
                    continue;

                int allowed = 0;

                if ( Wlen[j] > 0 &&
                     RANK(card) == RANK(*Wp[j]) - 1 &&
                     SUIT( card ) == SUIT( *Wp[j] ) )
                {
                    allowed = 1;

                }
                if ( RANK( card ) == PS_KING && Wlen[j] == 0 )
                {
                    if ( l != Wlen[i]-1 )
                        allowed = 4;
		    else if (m_redeal < 2)
  		        allowed = 1;
                }
                // TODO: there is no point in moving if we're not opening anything
                // e.g. if both i and j have perfect runs below the cards
#if 0
                fprintf( stderr, "%d %d %d\n", i, l, j );
                printcard( card, stderr );
                printcard( *Wp[j], stderr );
                fprintf( stderr, " allowed %d\n",allowed );
#endif
                if ( allowed )
		{
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
		    /*if (mp->pri == 30 && mp->from == 9 && mp->to == 10)
		      abort();*/
                    n++;
                    mp++;
                }
            }
        }
    }

    if ( !n && m_redeal < 2 )
    {
        mp->card_index = 0;
        mp->from = offs;
        mp->to = 0; // unused
        mp->totype = W_Type;
        mp->turn_index = -1;
        mp->pri = -1;
        n++;
        mp++;
    }

    return n;
}

unsigned int GrandfSolver::getClusterNumber()
{
    return m_redeal;
}

void GrandfSolver::unpack_cluster( unsigned int k )
{
    m_redeal = k;
}

bool GrandfSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; ++o) {
        if (RANK( W[offs][o] ) != PS_KING) {
            return false;
        }
    }

    return true;
}

int GrandfSolver::getOuts()
{
    int outs = 0;
    for ( int i = 0; i< 4; ++i )
        if ( !DOWN( W[offs][i] ) )
            outs += RANK( W[offs][i] );
    return outs;
}

GrandfSolver::GrandfSolver(const Grandf *dealer)
    : Solver()
{
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    setNumberPiles( 7 * 3 + 1 );
    offs = 7 * 3;
    deal = dealer;
    m_redeal = -1;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void GrandfSolver::translate_layout()
{
    /* Read the workspace. */

    m_redeal = deal->numberOfDeals - 1;

    for ( int w = 0; w < 7 * 3; ++w )
        Wlen[w] = 0;

    for ( int w = 0; w < 7; ++w ) {
        int i = translate_pile(deal->store[w], W[w + m_redeal * 7 ], 52);
        Wp[w + m_redeal * 7] = &W[w + m_redeal * 7][i - 1];
        Wlen[w + m_redeal * 7] = i;
    }

    Wlen[offs] = 4;

    for ( int i = 0; i < 4; ++i )
        W[offs][i] = NONE;

    for (int i = 0; i < 4; ++i) {
        KCard *c = deal->target[i]->topCard();
        if (c)
            W[offs][translateSuit( c->suit() ) >> 4] = translateSuit( c->suit() ) + c->rank();
    }
    for ( int i = 0; i < 4; ++i )
        if ( W[offs][i] == NONE )
            W[offs][i] = ( i << 4 ) + PS_ACE + ( 1 << 7 );

    Wp[offs] = &W[offs][3];
}

MoveHint GrandfSolver::translateMove( const MOVE &m )
{
    if ( m.from == offs )
        return MoveHint();

    PatPile *frompile = 0;
    frompile = deal->store[m.from % 7];

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
        return MoveHint( card, deal->store[m.to % 7], m.pri );
    }
    return MoveHint();
}

void GrandfSolver::print_layout()
{
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 21; ++w) {
        fprintf( stderr, "Play%d-%d(%d): ", w / 7, w % 7, w );
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (o = 0; o < 4; ++o) {
        if ( !DOWN( W[offs][o] ) )
            printcard(RANK( W[offs][o] ) + Osuit[o], stderr);
    }
    fprintf( stderr, "\nRedeals: %d", m_redeal );
    fprintf(stderr, "\nprint-layout-end\n");
}
