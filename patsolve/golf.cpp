/* Common routines & arrays. */

#include "golf.h"

#include "../deck.h"
#include "../golf.h"
#include "../pile.h"

#include <KDebug>


#define PRINT 0

void GolfSolver::make_move(MOVE *m)
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

    Q_ASSERT( to == 7 );
    Q_ASSERT( from != 7 );

    // move to pile
    if ( from == 8 && to == 7 )
    {
        card_t card = *Wp[8];
        Wp[8]--;
        Wlen[8]--;
        card = ( SUIT( card ) << 4 ) + RANK( card );
        Wp[7]++;
        *Wp[7] = card;
        Wlen[7]++;
        hashpile( 7 );
        hashpile( 8 );
#if PRINT
        print_layout();
#endif
        return;
    }

    card_t card = *Wp[from];
    Wp[from]--;
    Wlen[from]--;

    Wp[to]++;
    *Wp[to] = card;
    Wlen[to]++;

    hashpile(from);
    hashpile(to);
#if PRINT
    print_layout();
#endif
}

void GolfSolver::undo_move(MOVE *m)
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

    Q_ASSERT( to == 7 );
    Q_ASSERT( from != 7 );

    // move to pile
    if ( from == 8 && to == 7 )
    {
        card_t card = *Wp[7];
        Wp[7]--;
        Wlen[7]--;
        card = ( SUIT( card ) << 4 ) + RANK( card ) + ( 1 << 7 );
        Wp[8]++;
        *Wp[8] = card;
        Wlen[8]++;
        hashpile( 7 );
        hashpile( 8 );
#if PRINT
        print_layout();
#endif
        return;
    }

    card_t card = *Wp[to];
    Wp[to]--;
    Wlen[to]--;

    Wp[from]++;
    *Wp[from] = card;
    Wlen[from]++;

    hashpile(from);
    hashpile(to);
#if PRINT
    print_layout();
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */

int GolfSolver::get_possible_moves(int *a, int *numout)
{
    int n = 0;
    MOVE *mp = Possible;

    *a = false;
    *numout = 0;

    card_t top = *Wp[7];
    for (int w = 0; w < 7; w++) {
        if (Wlen[w] > 0) {
            card_t card = *Wp[w];
            if (RANK(card) == RANK(top) - 1 ||
                RANK(card) == RANK(top) + 1 )
            {
                mp->card_index = 0;
                mp->from = w;
                mp->to = 7;
                mp->totype = W_Type;
                mp->pri = 13;
                if ( RANK( card ) == PS_ACE || RANK( card ) == PS_KING )
                    mp->pri = 30;
                mp->turn_index = -1;
                n++;
                mp++;
            }
        }
    }

    /* check for deck->pile */
    if ( !n && Wlen[8] ) {
        mp->card_index = 1;
        mp->from = 8;
        mp->to = 7;
        mp->totype = W_Type;
        mp->pri = 5;
        mp->turn_index = 0;
        n++;
        mp++;
    }

    return n;
}

void GolfSolver::unpack_cluster( int )
{
}

bool GolfSolver::isWon()
{
    //kDebug(11111) << "isWon" << Wlen[7];
    return Wlen[7] == 52 ;
}

int GolfSolver::getOuts()
{
    return Wlen[7];
}

GolfSolver::GolfSolver(const Golf *dealer)
    : Solver()
{
    setNumberPiles( 9 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void GolfSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for ( int w = 0; w < 7; ++w ) {
        int i = translate_pile(deal->stack[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    int i = translate_pile( deal->waste, W[7], 52 );
    Wp[7] = &W[7][i-1];
    Wlen[7] = i;
    total += i;

    i = translate_pile( Deck::deck(), W[8], 52 );
    Wp[8] = &W[8][i-1];
    Wlen[8] = i;
    total += i;

    for ( int i = 0; i < 9; i++ )
    {
        for ( int l = 0; l < Wlen[i]; l++ )
        {
            card_t card = W[i][l];
            if ( DOWN( card ) )
                card = RANK( card ) + PS_SPADE + ( 1 << 7 );
            else
                card = RANK( card ) + PS_SPADE;
            W[i][l] = card;
        }
    }
}

int GolfSolver::getClusterNumber()
{
    return Wlen[8] / 8;
}

MoveHint *GolfSolver::translateMove( const MOVE &m )
{
    if ( m.from >= 7 )
        return 0;
    Pile *frompile = deal->stack[m.from];

    Card *card = frompile->at( frompile->cardsLeft() - m.card_index - 1);

    return new MoveHint( card, deal->waste, m.pri );
}

void GolfSolver::print_layout()
{
    fprintf(stderr, "print-layout-begin\n");
    for (int w = 0; w < 9; w++) {
        if ( w == 8 )
            fprintf( stderr, "Deck: " );
        else if ( w == 7 )
            fprintf( stderr, "Pile: " );
        else
            fprintf( stderr, "Play%d: ", w );
        for (int i = 0; i < Wlen[w]; i++) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf(stderr, "print-layout-end\n");
}
