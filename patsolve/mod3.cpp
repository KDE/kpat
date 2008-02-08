/* Common routines & arrays. */

#include "mod3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <kdebug.h>
#include <sys/types.h>
#include <stdarg.h>
#include "../mod3.h"
#include "../pile.h"
#include "../deck.h"

/* These two routines make and unmake moves. */

#define PRINT 0
#define PRINT2 0

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

    card_t card = *Wp[from];
    Wlen[from]--;
    Wp[from]--;

    hashpile(from);
    /* Add to pile. */

    Wp[to]++;
    *Wp[to] = card;
    Wlen[to]++;
    hashpile( to );

    if ( from >= 24 && Wlen[deck] )
    {
        Q_ASSERT( m->turn_index == 1 );
        card_t card = *Wp[deck];
        Wlen[deck]--;
        Wp[deck]--;

        hashpile(deck);
        /* Add to pile. */

        Wp[from]++;
        *Wp[from] = RANK( card ) + SUIT( card );
        Wlen[from]++;
        hashpile( from );
    }

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

    if ( from >= 24 && m->turn_index == 1)
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
    *Wp[to]--;
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
    card_t card;
    MOVE *mp;

    *a = false;
    *numout = 0;

    int n = 0;
    mp = Possible;

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
            if ( i >= 24 && Wlen[deck] )
                mp->turn_index = 1;
            n++;
            mp++;
            Possible[0] = mp[-1];
            *numout = 1;
            return 1;
        }

        if ( Wlen[i] > 1 && i < 24 )
            continue;

        for ( int j = 0; j < 32; ++j )
        {
            if ( i == j )
                continue;

#if 0
            if ( !Wlen[j] && i >= 24) // later
                continue;
#endif
            int current_row = i / 8;
            int row = j / 8;

            if ( Wlen[j] && RANK( *Wp[j] ) != row + 2 + ( Wlen[j] - 1 ) * 3 )
            {
                //fprintf( stderr, "rank %d %d\n", i, j );
                continue;
            }

            if ( row < 3 )
            {
                int min = ( card & PS_SUIT ) + row + 2;
                if ( Wlen[j] )
                    min = ( *Wp[j] & PS_SUIT ) + row + 2 + Wlen[j] * 3;
#if 0
                fprintf( stderr, "from %d to %d %d ", i, j, row );
                printcard( min, stderr );
                fprintf( stderr, "\n" );
#endif
                if ( min == card && ( Wlen[i] > 1 || current_row != row ) )
                {
                    mp->card_index = 0;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->pri = 12;    /* unused */
                    mp->turn_index = -1;
                    if ( i >= 24 && Wlen[deck] )
                        mp->turn_index = 1;
                    n++;
                    mp++;
                }
            }
        }
    }

#if 0
    if ( foundone )
        return;

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *store = *it;
        Card *top = store->top();

        if ( !top || store == aces || store == Deck::deck() )
            continue;

        if ( store->cardsLeft() > 1 && store->target() )
            continue;

        if ( store->cardsLeft() == 1 && !store->target() )
            continue;

        if ( store->cardsLeft() == 1 )
        {
            // HACK: cards that are already on the right target should not be moved away
            if (top->rank() == Card::Four && store->y() == stack[2][0]->y())
                continue;
            if (top->rank() == Card::Three && store->y() == stack[1][0]->y())
                continue;
            if (top->rank() == Card::Two && store->y() == stack[0][0]->y())
                continue;
        }

        if (store->legalRemove(top)) {
//                kDebug(11111) << "could remove" << top->name();
            for (PileList::Iterator pit = piles.begin(); pit != piles.end(); ++pit)
            {
                Pile *dest = *pit;
                if (dest == store || dest == Deck::deck() )
                    continue;
                if (dest->isEmpty() && !dest->target())
                {
                    newHint(new MoveHint(top, dest, 0));
                    continue;
                }
            }
        }
    }
#endif

    return n;
}

void Mod3Solver::unpack_cluster( int  )
{
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

    i = translate_pile(Deck::deck(), W[w], 104);
    Wp[w] = &W[w][i-1];
    Wlen[w] = i;

}

int Mod3Solver::getClusterNumber()
{
    return 0;
}

MoveHint *Mod3Solver::translateMove( const MOVE & )
{
#if 0
    Pile *frompile = deal->store[m.from];
    Card *card = frompile->top();

    if ( m.totype == O_Type )
    {
        return new MoveHint( card, deal->target[m.to], m.pri );
    } else {
        return new MoveHint( card, deal->store[m.to], m.pri );
    }
#endif
    return 0;
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
        for (i = 0; i < Wlen[w]; i++) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
        w++;
    }

    fprintf( stderr, "Aces: " );
    for (o = 0; o < Wlen[aces]; o++) {
        printcard(W[aces][o], stderr);
    }
    fputc( '\n', stderr );

    fprintf( stderr, "Deck: " );
    for (o = 0; o < Wlen[deck]; o++) {
        printcard(W[deck][o], stderr);
    }

    fprintf(stderr, "\nprint-layout-end\n");
}
