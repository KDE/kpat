/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <kdebug.h>
#include <sys/types.h>
#include <stdarg.h>
#include "fortyeight.h"
#include "../fortyeight.h"
#include "../pile.h"
#include "../deck.h"

#define PRINT 0
#define PRINT2 0

void FortyeightSolver::make_move(MOVE *m)
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

    int from = m->from;
    int to = m->to;

    // move to pile
    if ( from == 17 && to == 16 )
    {
        card_t card = *Wp[17];
        Wp[17]--;
        Wlen[17]--;
        card = ( SUIT( card ) << 4 ) + RANK( card );
        Wp[16]++;
        *Wp[16] = card;
        Wlen[16]++;
        hashpile( 17 );
        hashpile( 16 );
#if PRINT
        print_layout();
#endif
        return;
    }

    // move to deck
    if ( from == 16 && to == 17 )
    {
        while ( Wlen[16] > 1 )
        {
            Wlen[17]++;
            Wp[17]++;
            card_t card = *Wp[16] + ( 1 << 7 );
            *Wp[17] = card;
            Wlen[16]--;
            Wp[16]--;
        }
        hashpile( 17 );
        hashpile( 16 );
        lastdeal = true;
#if PRINT
        print_layout();
#endif
        return;
    }

    for ( int i = m->card_index + 1; i > 0; --i )
    {
        card_t card = W[from][Wlen[from]-i];
        Wp[from]--;

        Wp[to]++;
        *Wp[to] = card;
        Wlen[to]++;
    }
    Wlen[from] -= m->card_index+1;

    hashpile(from);
    hashpile(to);
#if PRINT
    print_layout();
#endif
}

void FortyeightSolver::undo_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
        fprintf( stderr, "\nundo move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else
        fprintf( stderr, "\nundo move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();

#endif

    int from = m->from;
    int to = m->to;

    // move to pile
    if ( from == 17 && to == 16 )
    {
        card_t card = *Wp[16];
        Wp[16]--;
        Wlen[16]--;
        card = ( SUIT( card ) << 4 ) + RANK( card ) + ( 1 << 7 );
        Wp[17]++;
        *Wp[17] = card;
        Wlen[17]++;
        hashpile( 17 );
        hashpile( 16 );
#if PRINT
        print_layout();
#endif
        return;
    }

    // move to deck
    if ( from == 16 && to == 17 )
    {
        while ( Wlen[17] )
        {
            Wlen[16]++;
            Wp[16]++;
            *Wp[16] = ( SUIT( *Wp[17] ) << 4 ) + RANK( *Wp[17] );
            Wlen[17]--;
            Wp[17]--;
        }
        hashpile( 17 );
        hashpile( 16 );
        lastdeal = false;
#if PRINT
        print_layout();
#endif
        return;
    }

    for ( int i = m->card_index + 1; i > 0; --i )
    {
        card_t card = W[to][Wlen[to]-i];
        Wp[to]--;

        Wp[from]++;
        *Wp[from] = card;
        Wlen[from]++;
    }
    Wlen[to] -= m->card_index+1;

    hashpile(from);
    hashpile(to);
#if PRINT
    print_layout();
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */
bool FortyeightSolver::checkMove( int from, int to, MOVE *mp )
{
    if ( !Wlen[from] )
        return false;

    card_t card = *Wp[from];
    bool allowed = false;
    if ( to < 8 )
    {
        if ( Wlen[to])
        {
            card_t top = *Wp[to];
            if ( SUIT( top ) == SUIT( card ) &&
                 RANK( top ) == RANK( card ) + 1 )
                allowed = true;
        } else
            allowed = true;
    } else {
        if ( !Wlen[to] )
        {
            if ( RANK( card ) == PS_ACE )
                allowed = true;
        } else {
            card_t top = *Wp[to];
            //kDebug() << "to " << to << " " << from << endl;
            if ( SUIT( top ) == SUIT( card ) &&
                 RANK( top ) == RANK( card ) - 1 )
                allowed = true;
        }
    }
    if ( !allowed )
        return false;

    // meta moves we check extra
    if ( Wlen[from] > 1 && to < 8 && from < 8 )
    {
        card_t card1 = *Wp[from];
        card_t card2 = W[from][Wlen[from]-2];
        if ( SUIT( card1 ) == SUIT( card2 ) &&
             RANK( card1 ) == RANK( card2 ) - 1 )
            return false;
    }

    mp->card_index = 0;
    mp->from = from;
    mp->to = to;
    mp->totype = W_Type;
    mp->pri = 13;
    mp->turn_index = -1;
    return true;
}

int FortyeightSolver::get_possible_moves(int *a, int *numout)
{
    int n = 0;
    MOVE *mp = Possible;
    freestores = 0;

    *a = false;

    // off
    for ( int w = 0; w < 8; w++ )
    {
        for ( int i = 0; i < 8; i++ )
            if ( checkMove( w, i+8, mp ) )
            {
                n++;
                mp->pri = 127;
                mp++;
                break;
            }
    }
    for ( int i = 0; i < 8; i++ )
    {
        if ( checkMove( 16, i+8, mp ) )
        {
            n++;
            mp->pri = 127;
            mp++;
            break;
        }
        if ( !Wlen[i] )
            freestores++;
    }

    // if a specific target got two possible drops, we don't make it auto drop
    int dropped[8] = { 0, 0, 0, 0, 0, 0, 0, 0};

    for ( int i = 0; i < n; i++ )
    {
        if ( Possible[i].to >= 8 && Possible[i].to < 16 && RANK(*Wp[Possible[i].from]) != PS_ACE )
            dropped[Possible[i].to-8]++;
    }

    for ( int i = 0; i < 8; i++ )
    {
        if ( dropped[i] > 1 )
        {
            for ( int j = 0; j < n; j++ )
                if ( Possible[j].to == i + 8 )
                    Possible[j].pri = qMin( 119, 100 + Wlen[Possible[j].from] );
        }
        if ( dropped[i] == 1 )
        {
            // good automove
            if (n != 1)
            {
                for ( int j = 0; j < n; j++ )
                {
                    if ( Possible[j].to == i + 8 )
                    {
                        Possible[0] = Possible[j];
                        break;
                    }
                }
            }
            *a = true;
            *numout = 1;
            return 1;
        }
    }

    *numout = n;

    bool found_p2e = false;
    for (int w = 0; w < 8; w++)
    {
        bool foundempty = false;
        for ( int j = 0; j < 8; j++ )
        {
            if ( j == w )
                continue;
            if ( checkMove( w, j, mp ) )
            {
                if ( !Wlen[j] )
                {
                    if (Wlen[w] == 1)
			continue; // ignore it
                    if ( foundempty ) // one is enough
                        continue;
                    foundempty = true;
                    mp->pri = 20;
                } else
                    mp->pri = 20 + RANK( *Wp[j] );

                n++;
                mp++;
            }
        }
        if ( checkMove( 16, w, mp ) )
        {
            if ( !Wlen[w] )
            {
                if ( found_p2e )
                    continue;
                found_p2e = true;
            }
            n++;
            mp++;
        }
        if ( Wlen[w] > 1 && freestores )
        {
            if ( SUIT( *Wp[w] ) == SUIT( W[w][Wlen[w]-2] ) )
            {
                //print_layout();

                for ( int to = 0; to < 8; to++ )
                {
                    if ( to == w )
                        continue;
                    if ( Wlen[to] && SUIT( *Wp[to] ) != SUIT( *Wp[w] ) )
                        continue;
                    if ( !Wlen[to] && foundempty )
                        continue;
                    int moves = 1 << freestores;
                    if ( !Wlen[to] )
                        moves = 1 << ( freestores - 1 );

                    if ( moves >= Wlen[w] )
                        moves = Wlen[w];

                    bool switched = false;
                    for ( int i = 2; i < moves; ++i )
                    {
                        /*      printcard( W[w][Wlen[w]-i-1], stderr );
                        printcard( *Wp[w], stderr );
                        fprintf( stderr, " switch? %d\n", i ); */
                        if ( SUIT( W[w][Wlen[w]-i-1] ) != SUIT( *Wp[w] ) ) {
                            switched = true;
                            moves = i;
                            break;
                        }
                    }
                    if ( !Wlen[to] )
                    {
                        if ( moves < 2 || moves == Wlen[w] || !switched)
                            continue;
                        //print_layout();
                        //kDebug() << "EMPTY move to empty " << w << " " << to << " " << moves << " " << switched << endl;
                        mp->card_index = moves-1;
                        mp->from = w;
                        mp->to = to;
                        mp->totype = W_Type;
                        mp->pri = 60;
                        mp->turn_index = -1;
                        n++;
                        mp++;
                        foundempty = true;
                        continue;
                    }
                    card_t top = *Wp[to];
                    for ( int i = 2; i <= moves && i <= Wlen[w]; i++ )
                    {
                        card_t cur = W[w][Wlen[w]-i];
                        /* printcard( top, stderr );
                        printcard( cur, stderr );
                        fprintf( stderr, " %d\n", i ); */
                        Q_ASSERT( SUIT( top ) == SUIT( cur ) );
                        if ( RANK( top ) == RANK( cur ) + 1 )
                        {
                            //kDebug() << "MOVEEEEE move to non-empty " << w << " " << to << " " << moves << " " << switched << endl;
                            mp->card_index = i-1;
                            mp->from = w;
                            mp->to = to;
                            mp->totype = W_Type;
                            mp->pri = 80;
                            mp->turn_index = -1;
                            n++;
                            mp++;
                        }
                    }
                }
            }
        }
    }
    /* check for deck->pile */
    if ( Wlen[17] ) {
        mp->card_index = 1;
        mp->from = 17;
        mp->to = 16;
        mp->totype = W_Type;
        mp->pri = 9;
        mp->turn_index = 0;
        n++;
        mp++;
    } else if ( !lastdeal )
    {
        mp->card_index = 1;
        mp->from = 16;
        mp->to = 17;
        mp->totype = W_Type;
        mp->pri = 50;
        mp->turn_index = 0;
        n++;
        mp++;
    }

    return n;
}

void FortyeightSolver::unpack_cluster( int )
{
}

bool FortyeightSolver::isWon()
{
    for ( int i = 8; i < 16; ++i )
        if ( Wlen[i] != 13 )
            return false;
    kDebug() << "isWon " << getOuts() << endl;
    return true;
}

int FortyeightSolver::getOuts()
{
    int total = 0;
    for ( int i = 8; i < 16; ++i )
        total += Wlen[i];
    return total;
}

FortyeightSolver::FortyeightSolver(const Fortyeight *dealer)
    : Solver()
{
    setNumberPiles( 18 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void FortyeightSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for ( int w = 0; w < 8; ++w ) {
        int i = translate_pile(deal->stack[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }
    for ( int w = 0; w < 8; ++w ) {
        int i = translate_pile(deal->target[w], W[w+8], 52);
        Wp[w+8] = &W[w+8][i - 1];
        Wlen[w+8] = i;
        total += i;
    }

    int i = translate_pile( deal->pile, W[16], 80 );
    Wp[16] = &W[16][i-1];
    Wlen[16] = i;
    total += i;

    i = translate_pile( Deck::deck(), W[17], 80 );
    Wp[17] = &W[17][i-1];
    Wlen[17] = i;
    total += i;

    lastdeal = deal->lastdeal;
}

int FortyeightSolver::getClusterNumber()
{
    return 0;
}

MoveHint *FortyeightSolver::translateMove( const MOVE &m )
{
    if ( m.from == 17 || m.to == 17 )
        return 0;
    Pile *frompile = 0;
    if ( m.from < 8 )
        frompile = deal->stack[m.from];
    else
        frompile = deal->pile;

    Q_ASSERT( frompile );
    Card *card = frompile->at( frompile->cardsLeft() - m.card_index - 1);
    Q_ASSERT( card );
    Q_ASSERT( m.to < 16 );
    if ( m.to >= 8 )
        return new MoveHint( card, deal->target[m.to-8], m.pri );
    else
        return new MoveHint( card, deal->stack[m.to], m.pri );
}

void FortyeightSolver::print_layout()
{
    fprintf(stderr, "print-layout-begin\n");
    for (int w = 0; w < 18; w++) {
        if ( w == 17 )
            fprintf( stderr, "Deck: " );
        else if ( w == 16 )
            fprintf( stderr, "Pile: " );
        else if ( w < 8 )
            fprintf( stderr, "Play%d: ", w );
        else
            fprintf( stderr, "Target%d: ", w - 8 );
        for (int i = 0; i < Wlen[w]; i++) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Last-Deal: %d\n", lastdeal );
    fprintf(stderr, "print-layout-end\n");
}
