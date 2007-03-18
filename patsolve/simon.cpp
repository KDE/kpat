/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <kdebug.h>
#include <sys/types.h>
#include <stdarg.h>
#include "simon.h"
#include "../simon.h"
#include "../pile.h"
#include "../deck.h"

/* These two routines make and unmake moves. */

#define PRINT 0
#define PRINT2 0

void SimonSolver::make_move(MOVE *m)
{
#if PRINT
    kDebug() << "\n\nmake_move\n";
    if ( m->totype == O_Type )
        fprintf( stderr, "move %d from %d out (at %d) Prio: %d\n\n", m->card_index, m->from, m->turn_index, m->pri );
    else
        fprintf( stderr, "move %d from %d to %d (%d) Prio: %d\n\n", m->card_index, m->from, m->to, m->turn_index, m->pri );
    print_layout();
#else
    //print_layout();
#endif

	int from, to;
	card_t card = NONE;

	from = m->from;
	to = m->to;

	if (m->totype == O_Type) {
            O[to] = SUIT( *Wp[from] );
            Wlen[from] -= 13;
            Wp[from] -= 13;
            hashpile( from );
            if ( Wlen[from] && DOWN( *Wp[from] ) )
            {
                *Wp[from] = ( SUIT( *Wp[from] ) << 4 ) + RANK( *Wp[from] );
            }
#if PRINT
            print_layout();
#endif
            return;
        }

        for ( int l = m->card_index; l >= 0; l-- )
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

        hashpile(from);
        hashpile(to);
#if PRINT
        print_layout();
#endif
}

void SimonSolver::undo_move(MOVE *m)
{
#if PRINT
    kDebug() << "\n\nundo_move\n";
    if ( m->totype == O_Type )
        fprintf( stderr, "move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else
        fprintf( stderr, "move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();

#endif
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

        if (m->totype == O_Type) {
            for ( int j = PS_KING; j >= PS_ACE; j-- )
            {
                Wp[from]++;
                *Wp[from] = O[to] + j;
                Wlen[from]++;
            }
            O[to] = -1;
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

        for ( int l = m->card_index; l >= 0; l-- )
        {
            card = W[to][Wlen[to]-l-1];
            Wp[from]++;
            *Wp[from] = card;
            Wlen[from]++;
            *Wp[to]--;
        }
        Wlen[to] -= m->card_index + 1;
        hashpile(to);
        hashpile(from);
#if PRINT
        print_layout();
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */

int SimonSolver::get_possible_moves(int *a, int *numout)
{
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (int w = 0; w < 10; w++) {
        if (Wlen[w] >= 13 && RANK( *Wp[w] ) == PS_ACE )
        {
            int ace_suit = SUIT( *Wp[w] );
            bool stroke = true;
            for ( int l = 0; l < 13; l++ )
            {
                if ( RANK( W[w][Wlen[w]-l-1] ) != l+1 ||
                     SUIT( W[w][Wlen[w]-l-1] ) != ace_suit )
                {
                    stroke = false;
                    break;
                }

            }
            if ( !stroke )
                continue;

            mp->card_index = 12;
            mp->from = w;
            int o = 0;
            while ( O[o] != -1)
                o++; // TODO I need a way to tell spades off from heart off
            mp->to = o;
            mp->totype = O_Type;
            mp->pri = 0;    /* unused */
            mp->turn_index = -1;
            n++;
            mp++;

            return 1;
        }
    }

    /* No more automoves, but remember if there were any moves out. */

    *a = false;
    *numout = n;

    int conti[10];
    for ( int j = 0; j < 10; j++ )
    {
        conti[j] = 0;
        for ( ; conti[j] < Wlen[j]-1; ++conti[j] )
        {
            if ( SUIT( *Wp[j] ) != SUIT( W[j][Wlen[j]-conti[j]-2] ) ||
                 DOWN( W[j][Wlen[j]-conti[j]-2] ))
                break;
            if ( RANK( W[j][Wlen[j]-conti[j]-1] ) !=
                 RANK( W[j][Wlen[j]-conti[j]-2] ) - 1)
                break;
        }
        conti[j]++;
    }

    bool foundgood = false;

    for(int i=0; i<10; i++)
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
                if ( SUIT( card ) != SUIT( card_on_top ) )
                    break;
            }

            bool wasempty = false;
            for (int j = 0; j < 10; j++)
            {
                if (i == j)
                    continue;

                bool allowed = false;

                if ( Wlen[j] > 0 &&
                     RANK(card) == RANK(*Wp[j]) - 1 )
                {
                    allowed = true;
                    if ( ( SUIT( card ) != SUIT( *Wp[j] ) ) && foundgood )
                        allowed = false; // make the tree simpler
                }
                if ( Wlen[j] == 0 && !wasempty )
                {
                    if ( l != Wlen[i]-1  ) {
                        allowed = true;
                        wasempty = true;
                    }
                }
                if ( allowed && Wlen[i] >= l+2 && Wlen[i] > 1 )
                {
                    Q_ASSERT( Wlen[i]-l-2 >= 0 );
                    card_t card_below = W[i][Wlen[i]-l-2];
                    if ( SUIT( card ) == SUIT( card_below ) &&
                         !DOWN( card_below ) &&
                         RANK( card_below ) == RANK( card ) + 1 )
                    {
#if 0
                        printcard( card_below, stderr );
                        printcard( card, stderr );
                        fprintf( stderr, "%d %d %d %d %d\n", i, j, conti[i], conti[j],l );
#endif
                        if ( conti[j]+l != 13 || conti[i]>conti[j]+l || SUIT( card ) != SUIT( *Wp[j] ) ) {
                            // fprintf( stderr, "continue\n" );
                            continue;
                        }
                    }
                }

                if ( allowed ) {
                    mp->card_index = l;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->turn_index = -1;
                    int cont = conti[j];
                    if ( Wlen[j] )
                        cont++;
                    if ( cont )
                        cont += l;
                    mp->pri = 8 * cont + qMax( 0, 10 - Wlen[i] );
                    if ( Wlen[j] )
                        if ( SUIT( card ) != SUIT( *Wp[j] ) )
                            mp->pri /= 2;
                        else
                            foundgood = true;
                    else
                        mp->pri = 2; // TODO: it should depend on the actual stack's order
                    if ( Wlen[i] == l+1 )
                        mp->pri = qMin( 127, mp->pri + 20 );
                    else
                        mp->pri = qMin( 127, mp->pri + 2 );

                    /* and now check what sequence we open */
                    int conti_pos = l+1;
                    for ( ; conti_pos < Wlen[i]-1; conti_pos++ )
                    {
                        card_t top = W[i][Wlen[i]-l-2];
                        card_t theone = W[i][Wlen[i]-conti_pos-1];
                        card_t below = W[i][Wlen[i]-conti_pos-2];
#if 0
                        printcard( top, stderr );
                        printcard( theone, stderr );
                        printcard( below, stderr );
                        fputc( '\n', stderr );
#endif
                        if ( SUIT( top ) != SUIT( below ) || DOWN( below ) )
                            break;
                        if ( RANK( theone ) !=
                             RANK( below ) - 1)
                            break;
                    }
                    mp->pri = qMin( 127, mp->pri + 5 * ( conti_pos - l - 1 ) );

                    n++;
                    mp++;
                }
            }
        }
    }

    return n;
}

void SimonSolver::unpack_cluster( int k )
{
    // TODO: this only works for easy
    for ( int i = 0; i < 4; ++i )
    {
        if ( i < k )
            O[i] = PS_SPADE;
        else
            O[i] = -1;
    }
}

bool SimonSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; o++)
        if (O[o] == -1)
            return false;

    return true;
}

int SimonSolver::getOuts()
{
    int k = 0;
    for (int o = 0; o < 4; o++)
        if (O[o])
            k += 13;

    return k;
}

SimonSolver::SimonSolver(const Simon *dealer)
    : Solver()
{
    setNumberPiles( 10 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void SimonSolver::translate_layout()
{
    /* Read the workspace. */
    int total = 0;

    for ( int w = 0; w < 10; ++w ) {
        int i = translate_pile(deal->store[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    for (int i = 0; i < 4; i++) {
        O[i] = -1;
        Card *c = deal->target[i]->top();
        if (c) {
            total += 13;
            O[i] = translateSuit( c->suit() );
        }
    }
}

int SimonSolver::getClusterNumber()
{
    int k = 0;
    for ( int i = 0; i < 4; ++i )
    {
        if ( O[i] != -1 )
            k++;
    }
    return k;
}

void SimonSolver::print_layout()
{
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 10; w++) {
        Q_ASSERT( Wp[w] == &W[w][Wlen[w]-1] );
        fprintf( stderr, "Play%d: ", w );
        for (i = 0; i < Wlen[w]; i++) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (o = 0; o < 4; o++) {
        if ( O[o] != -1 )
            printcard( O[o] + PS_KING, stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
}

MoveHint *SimonSolver::translateMove( const MOVE &m )
{


    Q_ASSERT( m.from < 10 && m.to < 10 );

    Pile *frompile = deal->store[m.from];
    Card *card = frompile->at( frompile->cardsLeft() - m.card_index - 1);

    kDebug() << "card " << card->name() << endl;
    if ( m.totype == O_Type )
    {
        for ( int i = 0; i < 4; i++ )
            if ( deal->target[i]->isEmpty() )
                return new MoveHint( card, deal->target[i], 127 );
    }

    Q_ASSERT( m.to < 10 );
    return new MoveHint( card, deal->store[m.to], m.pri );
}
