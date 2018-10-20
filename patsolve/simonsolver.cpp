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

#include <stdlib.h>
#include <string.h>

#include "freecell-solver/fcs_user.h"
#include "freecell-solver/fcs_cl.h"

#include "simonsolver.h"

#include "../kpat_debug.h"
#include "../simon.h"

const int CHUNKSIZE = 100;
const long int MAX_ITERS_LIMIT = 200000;

#define PRINT 0

/* These two routines make and unmake moves. */

#if 0
void SimonSolver::make_move(MOVE *m)
{
#if PRINT
    //qCDebug(KPAT_LOG) << "\n\nmake_move\n";
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

        hashpile(from);
        hashpile(to);
#if PRINT
        print_layout();
#endif
}

void SimonSolver::undo_move(MOVE *m)
{
#if PRINT
    //qCDebug(KPAT_LOG) << "\n\nundo_move\n";
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
            for ( int j = PS_KING; j >= PS_ACE; --j )
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
        hashpile(from);
#if PRINT
        print_layout();
#endif
}
#endif

#define CMD_LINE_ARGS_NUM 4
static const char * freecell_solver_cmd_line_args[CMD_LINE_ARGS_NUM] =
{
    "-g", "simple_simon", "--load-config", "the-last-mohican"
};

int SimonSolver::get_cmd_line_arg_count()
{
    return CMD_LINE_ARGS_NUM;
}

const char * * SimonSolver::get_cmd_line_args()
{
    return freecell_solver_cmd_line_args;
}

void SimonSolver::setFcSolverGameParams()
{
    freecell_solver_user_apply_preset(solver_instance, "simple_simon");
}

int SimonSolver::get_possible_moves(int *, int *)
{
    return 0;
}

#if 0
/* Get the possible moves from a position, and store them in Possible[]. */

int SimonSolver::get_possible_moves(int *a, int *numout)
{
    MOVE *mp;
    int n;

    mp = Possible;
    n = 0;
    *a = 1;

    while (freecell_solver_user_get_moves_left(solver_instance))
    {
        fcs_move_t move;
        fcs_move_t * move_ptr;
        if (!freecell_solver_user_get_next_move(solver_instance, &move)) {
            move_ptr = new fcs_move_t;
            *move_ptr = move;
            mp->ptr = (void *)move_ptr;
            mp++;
            n++;
        }
        else
        {
            Q_ASSERT(0);
        }
    }

    *numout = n;
    return n;

#if 0
    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (int w = 0; w < 10; ++w) {
        if (Wlen[w] >= 13 && RANK( *Wp[w] ) == PS_ACE )
        {
            int ace_suit = SUIT( *Wp[w] );
            bool stroke = true;
            for ( int l = 0; l < 13; ++l )
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
    for ( int j = 0; j < 10; ++j )
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

    for(int i=0; i<10; ++i)
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
            for (int j = 0; j < 10; ++j)
            {
                if (i == j)
                    continue;

                bool allowed = false;

                if ( Wlen[j] > 0 &&
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
                if ( allowed && Wlen[i] >= l+2 && Wlen[i] > 1 )
                {
                    Q_ASSERT( Wlen[i]-l-2 >= 0 );
                    card_t card_below = W[i][Wlen[i]-l-2];
                    if ( SUIT( card ) == SUIT( card_below ) &&
                         !DOWN( card_below ) &&
                         RANK( card_below ) == RANK( card ) + 1 )
                    {
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
                    if ( Wlen[j] ) {
                        if ( SUIT( card ) != SUIT( *Wp[j] ) ) {
                            mp->pri /= 2;
                        }
                    } else {
                        mp->pri = 2; // TODO: it should depend on the actual stack's order
                    }
                    if ( Wlen[i] == l+1 )
                        mp->pri = qMin( 127, mp->pri + 20 );
                    else
                        mp->pri = qMin( 127, mp->pri + 2 );

                    /* and now check what sequence we open */
                    int conti_pos = l+1;
                    for ( ; conti_pos < Wlen[i]-1; ++conti_pos )
                    {
                        card_t top = W[i][Wlen[i]-l-2];
                        card_t theone = W[i][Wlen[i]-conti_pos-1];
                        card_t below = W[i][Wlen[i]-conti_pos-2];
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
#endif
}
#endif

#if 0
void SimonSolver::unpack_cluster( unsigned int k )
{
    // TODO: this only works for easy
    for ( unsigned int i = 0; i < 4; ++i )
    {
        if ( i < k )
            O[i] = PS_SPADE;
        else
            O[i] = -1;
    }
}
#endif

#if 0
bool SimonSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; ++o)
        if (O[o] == -1)
            return false;

    return true;
}
#endif

#if 0
int SimonSolver::getOuts()
{
    int k = 0;
    for (int o = 0; o < 4; ++o)
        if (O[o])
            k += 13;

    return k;
}
#endif

SimonSolver::SimonSolver(const Simon *dealer)
    : FcSolveSolver()
{
    deal = dealer;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void SimonSolver::translate_layout()
{
    strcpy(board_as_string, deal->solverFormat().toLatin1());

    if (solver_instance)
    {
        freecell_solver_user_recycle(solver_instance);
        solver_ret = FCS_STATE_NOT_BEGAN_YET;
    }
#if 0
    /* Read the workspace. */
    int total = 0;

    for ( int w = 0; w < 10; ++w ) {
        int i = translate_pile(deal->store[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    for (int i = 0; i < 4; ++i) {
        O[i] = -1;
        KCard *c = deal->target[i]->topCard();
        if (c) {
            total += 13;
            O[i] = translateSuit( c->suit() );
        }
    }
#endif
}

#if 0
unsigned int SimonSolver::getClusterNumber()
{
    unsigned int k = 0;
    for ( int i = 0; i < 4; ++i )
    {
        if ( O[i] != -1 )
            k++;
    }
    return k;
}
#endif

#if 0
void SimonSolver::print_layout()
{
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 10; ++w) {
        Q_ASSERT( Wp[w] == &W[w][Wlen[w]-1] );
        fprintf( stderr, "Play%d: ", w );
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (o = 0; o < 4; ++o) {
        if ( O[o] != -1 )
            printcard( O[o] + PS_KING, stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
}
#endif

MoveHint SimonSolver::translateMove( const MOVE &m )
{
    fcs_move_t move = m.fcs;
    int cards = fcs_move_get_num_cards_in_seq(move);
    PatPile *from = 0;
    PatPile *to = 0;

    switch(fcs_move_get_type(move))
    {
        case FCS_MOVE_TYPE_STACK_TO_STACK:
            from = deal->store[fcs_move_get_src_stack(move)];
            to = deal->store[fcs_move_get_dest_stack(move)];
            break;

        case FCS_MOVE_TYPE_SEQ_TO_FOUNDATION:
            from = deal->store[fcs_move_get_src_stack(move)];
            cards = 13;
            to = deal->target[fcs_move_get_foundation(move)];
            break;

    }
    Q_ASSERT(from);
    Q_ASSERT(cards <= from->cards().count());
    Q_ASSERT(to || cards == 1);
    KCard *card = from->cards()[from->cards().count() - cards];

    if (!to)
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
        to = target ? target : empty;
    }

    Q_ASSERT(to);

    return MoveHint(card, to, 0);

#if 0
    Q_ASSERT( m.from < 10 && m.to < 10 );

    PatPile *frompile = deal->store[m.from];
    KCard *card = frompile->at( frompile->count() - m.card_index - 1);

    if ( m.totype == O_Type )
    {
        for ( int i = 0; i < 4; ++i )
            if ( deal->target[i]->isEmpty() )
                return MoveHint( card, deal->target[i], 127 );
    }

    Q_ASSERT( m.to < 10 );
    return MoveHint( card, deal->store[m.to], m.pri );
#endif
}
