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

#include "mod3solver.h"

#include "../mod3.h"

#include <QDebug>


#define PRINT 0
#define PRINT2 0

/* These two routines make and unmake moves. */

void Mod3Solver::make_move(MOVE *m)
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

    if ( from == deck )
    {
        int len = m->card_index;
        if ( len > 8 )
            len = 8;
        for ( int i = 0; i < len; i++ )
        {
            card_t card = *Wp[deck];
            Wlen[deck]--;
            Wp[deck]--;

            card = ( card & PS_SUIT ) + RANK( card );
            Wp[24 + i]++;
            Wlen[24 + i]++;
            *Wp[24 + i] = card;
            hashpile( 24 + i );
        }
        hashpile( deck );
#if PRINT
        print_layout();
#endif
        return;
    }

    card_t card = *Wp[from];
    Wlen[from]--;
    Wp[from]--;
    hashpile( from );

    /* Add to pile. */

    Wp[to]++;
    *Wp[to] = card;
    Wlen[to]++;
    hashpile( to );

    if ( m->turn_index == 1 )
    {
        card_t card2 = *Wp[deck];
        Wlen[deck]--;
        Wp[deck]--;

        hashpile(deck);
        /* Add to pile. */

        Wp[from]++;
        *Wp[from] = RANK( card2 ) + ( SUIT( card2 ) << 4 );
        Wlen[from]++;
    }

    hashpile(from);
#if PRINT
    print_layout();
#endif
}

void Mod3Solver::undo_move(MOVE *m)
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

    if ( from == deck )
    {
        int len = m->card_index;
        if ( len > 8 )
            len = 8;
        for ( int i = len; i >= 0; i-- )
        {
            card_t card = *Wp[24+i];
            Wlen[deck]++;
            Wp[deck]++;
            *Wp[deck] = ( 1 << 7 ) + card;

            Wp[24 + i]--;
            Wlen[24 + i]--;
            hashpile( 24 + i );
        }
        hashpile( deck );
#if PRINT2
        print_layout();
#endif
        return;
    }

    if ( m->turn_index == 1)
    {
        card_t card = *Wp[from];
        Wp[deck]++;
        Wlen[deck]++;
        *Wp[deck] = ( 1 << 7 ) + card;
        Wlen[from]--;
        Wp[from]--;

        hashpile(deck);
    }

    card = *Wp[to];
    Wp[from]++;
    *Wp[from] = card;
    Wlen[from]++;
    Wp[to]--;
    Wlen[to]--;
    hashpile(to);
    hashpile( from );

#if PRINT2
    print_layout();
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */

int Mod3Solver::get_possible_moves(int *a, int *numout)
{
    //print_layout();

    card_t card;
    MOVE *mp;

    *a = false;
    *numout = 0;

    int n = 0;
    mp = Possible;

    int first_empty_pile = -1;
    bool foundone = false;

    int firstfree[4] = { -1, -1, -1, -1};

    for (int i = 0; i < 32; ++i )
    {
        if ( !Wlen[i] )
            continue;
        card = *Wp[i];

        if ( RANK( card ) == PS_ACE )
        {
            mp->card_index = 0;
            mp->from = i;
            mp->to = aces;
            mp->totype = O_Type;
            mp->pri = 127;    /* unused */
            mp->turn_index = -1;
            if ( i >= 24 && Wlen[i] == 1 && Wlen[deck] )
                mp->turn_index = 1;
            n++;
            mp++;
            Possible[0] = mp[-1];
            *numout = 1;
            return 1;
        }

        for ( int j = 0; j < 32; ++j )
        {
            if ( i == j )
                continue;

            int current_row = i / 8;
            int row = j / 8;

	    if (!Wlen[j]) {
	      if (firstfree[row] < 0)
		firstfree[row] = j;
	      
	      //fprintf(stderr, "firstfree %d %d %d\n", row, firstfree[row], j);
	      if (j != firstfree[row] && row < 4)
		continue;
	    }
	      
            if ( Wlen[j] && RANK( *Wp[j] ) != row + 2 + ( Wlen[j] - 1 ) * 3 )
            {
                //fprintf( stderr, "rank %d %d\n", i, j );
                continue;
            }

            if ( row < 3 )
            {
                int min = ( card & PS_SUIT ) + row + 2;
                if ( Wlen[j] ) {
                    if ( RANK( *Wp[j] ) > 10 )
                        continue;
                    min = ( *Wp[j] & PS_SUIT ) + row + 2 + Wlen[j] * 3;
                }
                if ( min != card )
                    continue;

                // and now we figure if this makes sense at all
                if ( current_row == row )
                {
                    // if the only difference between i and j is the card,
                    // then it's not worth it
                    if ( Wlen[i] == Wlen[j] + 1 )
                        continue;
                }
                mp->pri = qMin(119, 12 + 20 * Wlen[j] + current_row * 2 + RANK(*Wp[j]) * 5);

                mp->turn_index = -1;
                if ( i >= 24 && Wlen[i] == 1 && Wlen[deck] )
                    mp->turn_index = 1;

                if ( Wlen[j] > 0 && !foundone )
                {
                    foundone = true;
                    // we want to make sure we do not get useless moves
                    // if there are target moves
                    int index = 0;
                    while ( index < n )
                    {
                        if ( Possible[index].pri == 0 )
                        {
                            Possible[index] = Possible[n-1];
                            Possible[n-1] = *mp;
                            mp--;
                            n--;
                        } else
                            index++;
                    }
                }

            } else {
                if ( foundone )
                    continue;

                if ( Wlen[j] || ( Wlen[i] > 1 && current_row != 3 ) )
                    continue;

                if ( current_row == 3 && Wlen[i] == 1 )
                    continue;

                if ( RANK( card ) == current_row + 2 && current_row != 3 )
                    continue;

                if ( first_empty_pile != -1 && first_empty_pile != j )
                    continue;

                if ( first_empty_pile == -1 )
                    first_empty_pile = j;

                mp->pri = 0;
                mp->turn_index = -1;
            }

            mp->card_index = 0;
            mp->from = i;
            mp->to = j;
            mp->totype = W_Type;
            n++;
            mp++;
        }
    }

    if ( Wlen[deck] )
    {
        // move
        mp->card_index = 0;
        mp->from = deck;
        mp->to = 0;
        mp->totype = W_Type;
        mp->pri = -77; // last resort
        mp->turn_index = -1;
        mp->card_index = Wlen[deck];
        n++;
        mp++;
    }

    return n;
}

bool Mod3Solver::isWon()
{
    return getOuts() == 52 * 2;
}

int Mod3Solver::getOuts()
{
    int ret = Wlen[aces];
    for ( int i = 0; i < 8 * 3; i++ )
        ret += Wlen[i];
    return ret;
}

Mod3Solver::Mod3Solver(const Mod3 *dealer)
    : Solver()
{
    // 24 targets, 8 playing fields, deck, aces
    setNumberPiles( 34 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void Mod3Solver::translate_layout()
{
    /* Read the workspace. */

    int w = 0;
    int total = 0;
    for ( int row = 0; row < 4; ++row )
    {
        for ( int col = 0; col < 8; ++col )
        {
            int i = translate_pile(deal->stack[row][col], W[w], 52);
            Wp[w] = &W[w][i - 1];
            Wlen[w] = i;
            total += i;
            w++;
        }
    }

    aces = w;
    int i = translate_pile(deal->aces, W[w], 52);
    Wp[w] = &W[w][i-1];
    Wlen[w] = i;

    deck = ++w;

    i = translate_pile(deal->talon, W[w], 104);
    Wp[w] = &W[w][i-1];
    Wlen[w] = i;
}

MoveHint Mod3Solver::translateMove( const MOVE & m )
{
    if ( m.from == deck )
        return MoveHint();

    PatPile *frompile = deal->stack[m.from / 8][m.from % 8];
    KCard *card = frompile->topCard();

    if ( m.to == aces )
    {
        return MoveHint( card, deal->aces, m.pri );
    } else {
        return MoveHint( card, deal->stack[m.to / 8][m.to % 8], m.pri );
    }
}

void Mod3Solver::print_layout()
{
    int i, w = 0, o;

    fprintf(stderr, "print-layout-begin\n");
    for ( int row = 0; row < 3; ++row )
    {
        fprintf( stderr, "Row%d: ", row );
        for (int col = 0; col < 8; col++)
        {
            if ( Wlen[w] )
                printcard(*Wp[w], stderr);
            else
                fprintf( stderr, "   " );
            fprintf( stderr, "(%02d) ", w );
            w++;
        }
        fputc('\n', stderr);
    }

    for (int col = 0; col < 8; col++)
    {
        fprintf( stderr, "Play%02d: ", w );
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
        w++;
    }

    fprintf( stderr, "Aces: " );
    for (o = 0; o < Wlen[aces]; ++o) {
        printcard(W[aces][o], stderr);
    }
    fputc( '\n', stderr );

    fprintf( stderr, "Deck: " );
    for (o = 0; o < Wlen[deck]; ++o) {
        printcard(W[deck][o], stderr);
    }

    fprintf(stderr, "\nprint-layout-end\n");
}
