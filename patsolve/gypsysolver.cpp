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

#include "gypsysolver.h"

#include "../gypsy.h"

#include <QDebug>
#include <assert.h>


#define PRINT 0

#define suitable(a, b) (COLOR( a ) != COLOR( b ) )

/* These two routines make and unmake moves. */

void GypsySolver::make_move(MOVE *m)
{
#if PRINT
    //qDebug() << "\n\nmake_move\n";
    if ( m->totype == O_Type )
        fprintf( stderr, "move %d from %d out (at %d) Prio: %d\n\n", m->card_index, m->from, m->turn_index, m->pri );
    else
        fprintf( stderr, "move %d from %d to %d (%d) Prio: %d\n\n", m->card_index, m->from, m->to, m->turn_index, m->pri );
    print_layout();
#else
    //print_layout();
#endif

    int from, to;
    //card_t card = NONE;

    from = m->from;
    to = m->to;

    if ( m->from == deck )
    {
        for ( int i = 0; i < 8; ++i )
        {
            card_t card = *Wp[from];
            card = ( SUIT( card ) << 4 ) + RANK( card );
            ++Wp[i];
            *Wp[i] = card;
            --Wp[from];
            Wlen[i]++;
            hashpile( i );
            Wlen[from]--;
        }
        hashpile( from );
#if PRINT
        print_layout();
#endif
        return;
    }

    card_t card = NONE;
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
        if ( Wlen[to] )
            *Wp[to] = card;
        else {
            Wp[to]++;
            Wlen[to]++;
            *Wp[to] = card;
        }
        Q_ASSERT( m->card_index == 0 );
    }
    hashpile(to);

#if PRINT
    print_layout();
#endif
}

void GypsySolver::undo_move(MOVE *m)
{
#if PRINT
    //qDebug() << "\n\nundo_move\n";
    if ( m->totype == O_Type )
        fprintf( stderr, "move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else
        fprintf( stderr, "move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();

#endif
    int from, to;
    //card_t card;

    from = m->from;
    to = m->to;

    if ( m->from == deck )
    {
        for ( int i = 7; i >= 0; --i )
        {
            card_t card = *Wp[i];
            Q_ASSERT( !DOWN( card ) );
            card = ( SUIT( card ) << 4 ) + RANK( card ) + ( 1 << 7 );
            ++Wp[from];
            --Wp[i];
            *Wp[from] = card;
            Wlen[from]++;
            Wlen[i]--;
            hashpile( i );
        }
        hashpile( from );
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

    card_t card = NONE;

    if (m->totype == O_Type) {
        card = *Wp[to];
        if ( RANK( card ) == PS_ACE )
        {
            Wlen[to] = 0;
        } else {
            *Wp[to] = card - 1; // SUIT( card ) << 4 + RANK( card ) - 1;
        }
        Wp[from]++;
        *Wp[from] = card;
        Wlen[from]++;
        hashpile( to );
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

/* Automove logic.  Gypsy games must avoid certain types of automoves. */

int GypsySolver::good_automove(int o, int r)
{
    int i;

    if (r <= 2) {
        return true;
    }

    /* Check the Out piles of opposite color. */
    int red_piles[]   = { 0, 1, 4, 5 };
    int black_piles[] = { 2, 3, 6, 7 };

    for (i = 0; i < 4; ++i)
    {
        int pile = 0;
        if ( COLOR( o ) == PS_BLACK )
            pile = red_piles[i] + outs;
        else
            pile = black_piles[i] + outs;
        if ( !Wlen[pile] || RANK( *Wp[pile] ) < r - 1)
        {
            /* Not all the N-1's of opposite color are out
               yet.  We can still make an automove if either
               both N-2's are out or the other same color N-3
               is out (Raymond's rule).  Note the re-use of
               the loop variable i.  We return here and never
               make it back to the outer loop. */

            for (i = 0; i < 4; ++i )
            {
                if ( COLOR( o ) == PS_BLACK )
                    pile = red_piles[i] + outs;
                else
                    pile = black_piles[i] + outs;
                if ( !Wlen[pile] || RANK( *Wp[pile] ) < r - 2)
                    return false;
            }

            for (i = 0; i < 4; ++i )
            {
                if ( COLOR( o ) == PS_BLACK )
                    pile = black_piles[i] + outs;
                else
                    pile = red_piles[i] + outs;
                if ( !Wlen[pile] || RANK( *Wp[pile] ) < r - 3)
                    return false;
            }

            return true;
        }
    }

    return true;
}

/* Get the possible moves from a position, and store them in Possible[]. */

int GypsySolver::get_possible_moves(int *a, int *numout)
{
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (int w = 0; w < 8; ++w) {
        if (Wlen[w] > 0) {
            card_t card = *Wp[w];
            int o = SUIT(card);
            for ( int off = 0; off < 2; ++off )
            {
                bool empty = !Wlen[o*2+off+outs];
                if ((empty && (RANK(card) == PS_ACE)) ||
                    (!empty && (RANK(card) == RANK( *Wp[outs+o*2+off] ) + 1 ) ) )
                {
                    mp->card_index = 0;
                    mp->from = w;
                    mp->to = outs+o*2+off;
                    mp->totype = O_Type;
                    mp->pri = params[4];
                    mp->turn_index = -1;
                    if ( Wlen[w] > 1 && DOWN( W[w][Wlen[w]-2] ) )
                        mp->turn_index = 1;
                    n++;
                    mp++;

                    /* If it's an automove, just do it. */

                    if (params[4] == 127 || good_automove(o, RANK(card))) {
                        *a = true;
                        mp[-1].pri = 127;
                        if (n != 1)
                            Possible[0] = mp[-1];
                        return 1;
                    }
                }
            }
        }
    }

    /* No more automoves, but remember if there were any moves out. */

    *a = false;
    *numout = n;

    for(int i=0; i<8; ++i)
    {
        int len = Wlen[i];
        for (int l=0; l < len; ++l )
        {
            card_t card = W[i][Wlen[i]-1-l];
            if ( DOWN( card ) )
                break;

            if ( l > 0 ) {
                card_t card_on_top = W[i][Wlen[i]-l];
                if ( RANK( card ) != RANK( card_on_top ) + 1 )
                    break;
                if ( !suitable( card, card_on_top ) )
                    break;
            }

            bool wasempty = false;
            for (int j = 0; j < 8; ++j)
            {
                if (i == j)
                    continue;

                bool allowed = false;

                if ( Wlen[j] > 0 && suitable( card, *Wp[j] ) &&
                     RANK(card) == RANK(*Wp[j]) - 1 )
                {
                    allowed = true;
                }
                if ( Wlen[j] == 0 && !wasempty )
                {
                    if ( l != Wlen[i]-1  ) {
                        allowed = true;
                        wasempty = true;
                    }
                }

                if ( !allowed )
                    continue;

                mp->card_index = l;
                mp->from = i;
                mp->to = j;
                mp->totype = W_Type;
                mp->turn_index = -1;
                mp->pri = params[3];
		if (Wlen[i] >= 2+l) {
		  assert(Wlen[i]-2-l >= 0);
		  card_t card2 = W[i][Wlen[i]-2-l];
		  if (DOWN(card2))  {
		     mp->turn_index = 1;
		     mp->pri = params[2];
		  }
		  if ( Wlen[i] >= l+2 && RANK( card ) == RANK( card2 ) - 1 &&
		       COLOR( card ) != COLOR( card2 ) && !DOWN( card2 ) )
		    {
		      int o = SUIT(card2);
		      for ( int off = 0; off < 2; ++off )
			{
			  bool empty = !Wlen[o*2+off+outs];
			  if ((empty && (RANK(card2) == PS_ACE)) ||
			      (!empty && (RANK(card2) == RANK( *Wp[outs+o*2+off] ) + 1 ) ) )
			    {
			      o = -1;
			      break;
			    }
			}
		      if ( o > -1 )
			mp->pri = -117;
		      else
			mp->pri = ( int )qMin( qreal( 127. ), params[1] + qreal( l ) * params[5] / 10 );
		    }
		}

                n++;
                mp++;
		// leave one for redeal
		if (n >= MAXMOVES - 2)  goto redeal;
            }
        }
    }

redeal:
    if ( Wlen[deck] )
    {
        /* check for redeal */
        mp->card_index = 0;
        mp->from = deck;
        mp->to = 0; // unused
        mp->totype = W_Type;
        mp->pri = params[0];
        mp->turn_index = -1;
        n++;
        mp++;
    }

    assert(n < MAXMOVES);
    return n;
}

bool GypsySolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 8; ++o)
    {
        if ( ( Wlen[outs + o] == 0 ) || ( RANK( *Wp[outs + o] ) != PS_KING ) )
            return false;
    }

    return true;
}

int GypsySolver::getOuts()
{
    int k = 0;
    for (int o = 0; o < 8; ++o)
        if ( Wlen[outs + o] )
            k += RANK( *Wp[outs + o] );

    return k;
}

GypsySolver::GypsySolver(const Gypsy *dealer)
    : Solver()
{
    setNumberPiles( 8 + 1 + 8 );
    deal = dealer;

    char buffer[10];
    params[0] = 3;
    params[1] = 3;
    params[2] = 44;
    params[3] = 19;
    params[4] = 5;
    params[5] = 10;
    for ( int i = 1; i <= 6; ++i )
    {
        sprintf( buffer, "DECK%d", i );
        const QByteArray env = qgetenv( buffer );
        if ( !env.isEmpty() )
            params[i-1] = env.toInt();
    }
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
   card).  Temp cells and Out on the last two lines, if any. */

void GypsySolver::translate_layout()
{
    /* Read the workspace. */
    for ( int w = 0; w < 8; ++w ) {
        int i = translate_pile(deal->store[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
    }

    deck = 8;
    int i = translate_pile( deal->talon, W[deck], 80 );
    Wp[deck] = &W[deck][i-1];
    Wlen[deck] = i;

    outs = 9;

    /* Output piles, if any. */
    for (int i = 0; i < 8; ++i)
    {
        Wlen[outs + i] = 0;
        Wp[outs + i] = &W[outs + i][-1];
    }

    for (int i = 0; i < 8; ++i) {
        KCard *c = deal->target[i]->topCard();
        if (c) {
            int suit = translateSuit( c->suit() ) >> 4;
            int target = outs + suit*2;
            if ( Wlen[target] )
                target++;
            Wp[target]++;
            Wlen[target]++;
            *Wp[target] = ( suit << 4 ) + c->rank();
        }
    }
}

void GypsySolver::print_layout()
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
    for (o = 0; o < 8; ++o) {
        if ( Wlen[outs + o] )
            printcard( *Wp[outs + o], stderr);
    }
    fprintf( stderr, "\nDeck: " );
    for (i = 0; i < Wlen[deck]; ++i)
        printcard(W[deck][i], stderr);

    fprintf(stderr, "\nprint-layout-end\n");
    return;
}

MoveHint GypsySolver::translateMove( const MOVE &m )
{
    //print_layout();
    if ( m.from == deck )
        return MoveHint();

    PatPile *frompile = deal->store[m.from];
    KCard *card = frompile->at( frompile->count() - m.card_index - 1);

    if ( m.totype == O_Type )
    {
        PatPile *target = 0;
        PatPile *empty = 0;
        for (int i = 0; i < 8; ++i) {
            KCard *c = deal->target[i]->topCard();
            if (c) {
                if ( c->suit() == card->suit() && c->rank() == card->rank() - 1)
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
    }

    return MoveHint( card, deal->store[m.to], m.pri );
}
