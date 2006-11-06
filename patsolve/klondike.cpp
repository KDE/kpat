/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <kdebug.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>
#include "klondike.h"
#include "../klondike.h"
#include "../pile.h"
#include "../deck.h"
#include "memory.h"

/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) (COLOR( a ) != COLOR( b ) )

/* These two routines make and unmake moves. */

#define PRINT 0
#define PRINT2 0

void KlondikeSolver::make_move(MOVE *m)
{
#if PRINT2
    kDebug() << "\n\nmake_move\n";
    if ( m->totype == O_Type )
        fprintf( stderr, "move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index );
    else
        fprintf( stderr, "move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();
#else
    //print_layout();
#endif

	int from, to;
	card_t card = NONE;

	from = m->from;
	to = m->to;

	/* Remove from pile. */
        if ( from == 7 && to == 8 )
        {
            deck_flip_counter++;
            while ( Wlen[7] )
            {
                card = W[7][Wlen[7]-1] + ( 1 << 7 );
                Wlen[8]++;
                W[8][Wlen[8]-1] = card;
                Wlen[7]--;
            }
            Wp[7] = &W[7][0];
            Wp[8] = &W[8][Wlen[8]-1];
            hashpile( 7 );
            hashpile( 8 );
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

void KlondikeSolver::undo_move(MOVE *m)
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

	/* Remove from 'to' pile. */

        if ( from == 7 && to == 8 )
        {
            while ( Wlen[8] )
            {
                card = W[8][Wlen[8]-1];
                card = ( SUIT( card ) << 4 ) + RANK( card );
                Wlen[7]++;
                W[7][Wlen[7]-1] = card;
                Wlen[8]--;
            }
            Wp[8] = &W[8][0];
            Wp[7] = &W[7][Wlen[7]-1];
            hashpile( 7 );
            hashpile( 8 );
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
            card = O[to] + Osuit[to];
            O[to]--;
            Wp[from]++;
            *Wp[from] = card;
            Wlen[from]++;
        } else {
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

/* Automove logic.  Klondike games must avoid certain types of automoves. */

int KlondikeSolver::good_automove(int o, int r)
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

int KlondikeSolver::get_possible_moves(int *a, int *numout)
{
    int w, o, empty;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (w = 0; w < 8; w++) {
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

    if ( deck_flip_counter > 3000 ) // really huge number!
        return 0;


    /* check for deck->pile */
    if ( Wlen[8] ) {
         mp->card_index = 0;
        mp->from = 8;
        mp->to = 7;
        mp->totype = W_Type;
        mp->pri = 5;
        mp->turn_index = 0;
        n++;
        mp++;
    }

    for(int i=0; i<8; i++)
    {
        int len = Wlen[i];
        if ( i == 7 && Wlen[i] > 0)
            len = 1;
        for (int l=0; l < len; ++l )
        {
            card_t card = W[i][Wlen[i]-1-l];
            if ( DOWN( card ) )
                break;

            for (int j = 0; j < 7; j++)
            {
                if (i == j)
                    continue;

                bool allowed = false;

                if ( Wlen[j] > 0 &&
                     RANK(card) == RANK(*Wp[j]) - 1 &&
                     suitable( card, *Wp[j] ) )
                    allowed = true;
                if ( RANK( card ) == PS_KING && Wlen[j] == 0 )
                {
                    if ( l != Wlen[i]-1 || i == 7 )
                        allowed = true;
                }
                if ( allowed ) {
                    mp->card_index = l;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->turn_index = -1;
                    if ( Wlen[i] > l+1 && DOWN( W[i][Wlen[i]-l-2] ) )
                        mp->turn_index = 1;
                    if ( i == 7 )
                        mp->pri = 40;
                    else {
                        if ( mp->turn_index > 0 || Wlen[i] == l+1)
                            mp->pri = 30;
                        else
                            mp->pri = 1;
                    }
                    n++;
                    mp++;
                }
            }
        }
    }

    if ( !Wlen[8] && Wlen[7] )
    {
        mp->card_index = 0;
        mp->from = 7;
        mp->to = 8;
        mp->totype = W_Type;
        mp->turn_index = 0;
        mp->pri = 2;
        n++;
        mp++;
    }
    return n;
}

void KlondikeSolver::unpack_cluster( int k )
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

bool KlondikeSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; o++) {
        if (O[o] != PS_KING) {
            return false;
        }
    }

    return true;
}

int KlondikeSolver::getOuts()
{
    return O[0] + O[1] + O[2] + O[3];
}

KlondikeSolver::KlondikeSolver(const Klondike *dealer)
    : Solver()
{
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    setNumberPiles( 9 );
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void KlondikeSolver::translate_layout()
{
    /* Read the workspace. */
    deck_flip_counter = 0;

    int total = 0;
    for ( int w = 0; w < 7; ++w ) {
        int i = translate_pile(deal->play[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    int i = translate_pile( deal->pile, W[7], 52 );
    Wp[7] = &W[7][i-1];
    Wlen[7] = i;
    total += i;

    i = translate_pile( Deck::deck(), W[8], 52 );
    Wp[8] = &W[8][i-1];
    Wlen[8] = i;
    total += i;

    /* Output piles, if any. */
    for (int i = 0; i < 4; i++) {
        O[i] = NONE;
    }
    if (total != 52) {
        for (int i = 0; i < 4; i++) {
            Card *c = deal->target[i]->top();
            if (c) {
                O[translateSuit( c->suit() )] = c->rank();
                total += c->rank();
            }
        }
    }
}

int KlondikeSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}

bool KlondikeSolver::print_layout()
{
    int count = 0;
    int total = O[0] + O[1] + O[2] + O[3];
    for (int w = 0; w < 9; w++)
    {
        for (int i = 0; i < Wlen[w]; i++)
            if ( SUIT( W[w][i] ) == 2 && RANK( W[w][i] ) == 4  )
                count++;
        total += Wlen[w];
    }
    if ( O[2] >= 4 )
        count++;

    bool broke = false;
    for ( int w = 0; w < 7; w++ )
    {
        if ( Wp[w] != &W[w][Wlen[w] - 1] )
        {
            kDebug() << w << " " << Wp[w] << " " << &W[w][Wlen[w] - 1] << " " << Wlen[w] << " " << Wp[w] - W[w] << endl;
            broke = true;
        }
        int top = Wlen[w] - 1;
        bool showsup = true;
        while ( top > 0 )
        {
            if ( showsup && DOWN( W[w][top-1] ) )
                showsup = false;
            if ( ( !DOWN( W[w][top] ) && !DOWN( W[w][top-1] ) && ( RANK( W[w][top] ) != RANK( W[w][top-1] ) - 1 )
                   || ( DOWN( W[w][top] && !DOWN( W[w][top-1] ) ) ) ) )
            {
                printcard( W[w][top-1] ,  stderr);
                printcard( W[w][top] ,  stderr);
                fprintf( stderr, "\n" );

                fprintf( stderr, "Play%d: ", w );
                for (int i = 0; i < Wlen[w]; i++) {
                    printcard(W[w][i], stderr);
                }
                fprintf( stderr, "\n" );
                broke = true;
                // kDebug() << "w " << RANK( W[w][top] ) << " " << RANK( W[w][top-1] ) << endl;
            }
            if ( DOWN( W[w][top] ) && !DOWN( W[w][top-1] ) )
            {
                broke = true;
                fprintf( stderr, "down incon %d\n", w );
            }
            top--;
        }
    }

    for ( int l = 0; l < Wlen[7]; l++ )
    {
        if ( DOWN( W[7][l] ) ) {
            broke = true;
            fprintf( stderr, "pile broken\n" );
        }
    }
    for ( int l = 0; l < Wlen[8]; l++ )
    {
        if ( !DOWN( W[8][l] ) ) {
            broke = true;
            fprintf( stderr, "deck broken\n" );
        }
    }
    //kDebug() << "count " << count << " " << total << endl;
    if ( count == 1 && total == 52 && !broke ) {
        broke = false;
        //return true;
    } else
        broke = true;
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 9; w++) {
        if ( w == 8 )
            fprintf( stderr, "Deck: " );
        else if ( w == 7 )
            fprintf( stderr, "Pile: " );
        else
            fprintf( stderr, "Play%d: ", w );
        for (i = 0; i < Wlen[w]; i++) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (o = 0; o < 4; o++) {
        printcard(O[o] + Osuit[o], stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
    if ( broke )
        exit( 1 );
    return broke;
}
