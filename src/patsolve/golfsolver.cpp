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

// own
#include "../golf.h"
#include "../kpat_debug.h"

const int CHUNKSIZE = 10000;

#define BHS__GOLF__NUM_COLUMNS 7
#define BHS__GOLF__MAX_NUM_CARDS_IN_COL 5
#define BHS__GOLF__BITS_PER_COL 3

static volatile std::atomic_bool stop_exec = false;

GolfSolver::GolfSolver(const Golf *dealer)
{
    deal = dealer;
    // Set default_max_positions to an initial
    // value so it will not accidentally be used uninitialized.
    // Note: it will be overrided by the Settings anyhow.
    default_max_positions = 100000;
    solver_instance = nullptr;
    solver_ret = BLACK_HOLE_SOLVER__OUT_OF_ITERS;
}

void GolfSolver::stopExecution()
{
    stop_exec = true;
}

void GolfSolver::free_solver_instance()
{
    if (solver_instance) {
        black_hole_solver_free(solver_instance);
        solver_instance = nullptr;
    }
}

SolverInterface::ExitStatus GolfSolver::patsolve(int max_positions)
{
    int current_iters_count = 0;
    if (max_positions < 0) {
        max_positions = default_max_positions;
    }
    stop_exec = false;
    if (solver_instance) {
        return SolverInterface::UnableToDetermineSolvability;
    }
    if (black_hole_solver_create(&solver_instance)) {
        fputs("Could not initialise solver_instance (out-of-memory)\n", stderr);
        exit(-1);
    }
    black_hole_solver_enable_rank_reachability_prune(solver_instance, true);
    black_hole_solver_enable_wrap_ranks(solver_instance, false);
    black_hole_solver_enable_place_queens_on_kings(solver_instance, true);
#ifdef BLACK_HOLE_SOLVER__API__REQUIRES_SETUP_CALL
    black_hole_solver_config_setup(solver_instance);
#endif

    int error_line_num;
    int num_columns = BHS__GOLF__NUM_COLUMNS;
    if (black_hole_solver_read_board(solver_instance,
                                     board_as_string,
                                     &error_line_num,
                                     num_columns,
                                     BHS__GOLF__MAX_NUM_CARDS_IN_COL,
                                     BHS__GOLF__BITS_PER_COL)) {
        fprintf(stderr, "Error reading the board at line No. %d!\n", error_line_num);
        exit(-1);
    }
#ifdef BLACK_HOLE_SOLVER__API__REQUIRES_SETUP_CALL
    black_hole_solver_setup(solver_instance);
#endif
    solver_ret = BLACK_HOLE_SOLVER__OUT_OF_ITERS;

    if (solver_instance) {
        while (!stop_exec && ((solver_ret == BLACK_HOLE_SOLVER__OUT_OF_ITERS)) && (current_iters_count < max_positions)) {
            current_iters_count += CHUNKSIZE;
            black_hole_solver_set_max_iters_limit(solver_instance, current_iters_count);

            solver_ret = black_hole_solver_run(solver_instance);
        }
    }
    switch (solver_ret) {
    case BLACK_HOLE_SOLVER__OUT_OF_ITERS:
        free_solver_instance();
        return SolverInterface::UnableToDetermineSolvability;

    case 0: {
        if (solver_instance) {
            m_winMoves.clear();
            int col_idx, card_rank, card_suit;
            int next_move_ret_code;
#ifdef BLACK_HOLE_SOLVER__API__REQUIRES_SETUP_CALL
            black_hole_solver_init_solution_moves(solver_instance);
#endif
            while ((next_move_ret_code = black_hole_solver_get_next_move(solver_instance, &col_idx, &card_rank, &card_suit)) == BLACK_HOLE_SOLVER__SUCCESS) {
                MOVE new_move;
                new_move.card_index = 0;
                new_move.from = col_idx;
                m_winMoves.append(new_move);
            }
        }
        free_solver_instance();
        return SolverInterface::SolutionExists;
    }

    default:
        free_solver_instance();
        return SolverInterface::UnableToDetermineSolvability;

    case BLACK_HOLE_SOLVER__NOT_SOLVABLE:
        free_solver_instance();
        return SolverInterface::NoSolutionExists;
    }
}

void GolfSolver::translate_layout()
{
    strcpy(board_as_string, deal->solverFormat().toLatin1().constData());
    free_solver_instance();
}

MoveHint GolfSolver::translateMove(const MOVE &m)
{
    if (m.from >= 7)
        return MoveHint();
    PatPile *frompile = deal->stack[m.from];

    KCard *card = frompile->at(frompile->count() - m.card_index - 1);

    return MoveHint(card, deal->waste, m.pri);
}
