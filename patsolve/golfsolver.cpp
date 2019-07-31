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

#include "golfsolver.h"

#include "../golf.h"
#include "../kpat_debug.h"

const int CHUNKSIZE = 10000;

#define BHS__GOLF__NUM_COLUMNS 7
#define BHS__GOLF__MAX_NUM_CARDS_IN_COL 5
#define BHS__GOLF__BITS_PER_COL 3

#define PRINT 0

void GolfSolver::make_move(MOVE *m)
{
#ifndef WITH_BH_SOLVER
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
#else
    Q_UNUSED(m);
#endif
}

void GolfSolver::undo_move(MOVE *m)
{
#ifndef WITH_BH_SOLVER
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
#else
    Q_UNUSED(m);
#endif
}

/* Get the possible moves from a position, and store them in Possible[]. */

int GolfSolver::get_possible_moves(int *a, int *numout)
{
#ifndef WITH_BH_SOLVER
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
#else
    Q_UNUSED(a);
    Q_UNUSED(numout);
    return 0;
#endif
}

bool GolfSolver::isWon()
{
#ifndef WITH_BH_SOLVER
    return Wlen[7] == 52 ;
#else
    return false;
#endif
}

int GolfSolver::getOuts()
{
#ifndef WITH_BH_SOLVER
    return Wlen[7];
#else
    return 0;
#endif
}

GolfSolver::GolfSolver(const Golf *dealer)
    : Solver()
{
    deal = dealer;
    // Set default_max_positions to an initial
    // value so it will not accidentally be used uninitialized.
    // Note: it will be overrided by the Settings anyhow.
    default_max_positions = 100000;
#ifdef WITH_BH_SOLVER
    solver_instance = NULL;
    solver_ret = BLACK_HOLE_SOLVER__OUT_OF_ITERS;
#endif
}

#ifdef WITH_BH_SOLVER
void GolfSolver::free_solver_instance()
{
    if (solver_instance)
    {
        black_hole_solver_free(solver_instance);
        solver_instance = NULL;
    }
}

SolverInterface::ExitStatus GolfSolver::patsolve( int _max_positions )
{
    int current_iters_count = 0;
    max_positions = (_max_positions < 0) ? default_max_positions : _max_positions;
    init();

    if (solver_instance)
    {
        return Solver::UnableToDetermineSolvability;
    }
    if (black_hole_solver_create(&solver_instance))
    {
        fputs("Could not initialise solver_instance (out-of-memory)\n", stderr);
        exit(-1);
    }
    black_hole_solver_enable_rank_reachability_prune(
        solver_instance, true);
    black_hole_solver_enable_wrap_ranks(solver_instance, false);
    black_hole_solver_enable_place_queens_on_kings(
        solver_instance, true);
#ifdef BLACK_HOLE_SOLVER__API__REQUIRES_SETUP_CALL
black_hole_solver_config_setup(solver_instance);
#endif

    int error_line_num;
    int num_columns = BHS__GOLF__NUM_COLUMNS;
    if (black_hole_solver_read_board(solver_instance, board_as_string, &error_line_num,
            num_columns,
            BHS__GOLF__MAX_NUM_CARDS_IN_COL,
            BHS__GOLF__BITS_PER_COL
    ))
    {
        fprintf(stderr, "Error reading the board at line No. %d!\n",
            error_line_num);
        exit(-1);
    }
#ifdef BLACK_HOLE_SOLVER__API__REQUIRES_SETUP_CALL
black_hole_solver_setup(solver_instance);
#endif
    solver_ret = BLACK_HOLE_SOLVER__OUT_OF_ITERS;

    if (solver_instance)
    {
        bool continue_loop = true;
        while (continue_loop &&
                (   (solver_ret == BLACK_HOLE_SOLVER__OUT_OF_ITERS)
                  )
                    &&
                 (current_iters_count < max_positions)
              )
        {
            current_iters_count += CHUNKSIZE;
            black_hole_solver_set_max_iters_limit(solver_instance, current_iters_count);

            solver_ret = black_hole_solver_run(solver_instance);
            {
                // QMutexLocker lock( &endMutex );
                if ( m_shouldEnd )
                {
                    continue_loop = false;
                }
            }
        }
    }
    switch (solver_ret)
    {
        case BLACK_HOLE_SOLVER__OUT_OF_ITERS:
            free_solver_instance();
            return Solver::UnableToDetermineSolvability;

        case 0:
            {
                if (solver_instance)
                {
                    m_winMoves.clear();
                    int col_idx, card_rank, card_suit;
                    int next_move_ret_code;
#ifdef BLACK_HOLE_SOLVER__API__REQUIRES_SETUP_CALL
                    black_hole_solver_init_solution_moves(solver_instance);
#endif
                    while ((next_move_ret_code = black_hole_solver_get_next_move(
                                solver_instance, &col_idx, &card_rank, &card_suit)) ==
                        BLACK_HOLE_SOLVER__SUCCESS)
                    {
                        MOVE new_move;
                        new_move.card_index = 0;
                        new_move.from = col_idx;
                        m_winMoves.append( new_move );
                    }

                }
                free_solver_instance();
                return Solver::SolutionExists;
            }

        default:
            free_solver_instance();
            return Solver::UnableToDetermineSolvability;

        case BLACK_HOLE_SOLVER__NOT_SOLVABLE:
            free_solver_instance();
            return Solver::NoSolutionExists;
    }
}
#endif

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void GolfSolver::translate_layout()
{
#ifdef WITH_BH_SOLVER
    strcpy(board_as_string, deal->solverFormat().toLatin1());
    free_solver_instance();
#else
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

    i = translate_pile( deal->talon, W[8], 52 );
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
#endif
}

MoveHint GolfSolver::translateMove( const MOVE &m )
{
    if ( m.from >= 7 )
        return MoveHint();
    PatPile *frompile = deal->stack[m.from];

    KCard *card = frompile->at( frompile->count() - m.card_index - 1);

    return MoveHint( card, deal->waste, m.pri );
}

void GolfSolver::print_layout()
{
#ifndef WITH_BH_SOLVER
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
#endif
}
