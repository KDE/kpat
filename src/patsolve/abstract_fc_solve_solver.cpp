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

#include "abstract_fc_solve_solver.h"

// own
#include "patsolve-config.h"
// freecell-solver
#include "freecell-solver/fcs_cl.h"
#include "freecell-solver/fcs_user.h"
// St
#include <cstdlib>
#include <cstring>

const int CHUNKSIZE = 100;
const long int INITIAL_MAX_ITERS_LIMIT = 200000;

#define PRINT 0

/* These two routines make and unmake moves. */

void FcSolveSolver::make_move(MOVE *)
{
    return;
}

void FcSolveSolver::undo_move(MOVE *)
{
    return;
}
#if WITH_FCS_SOFT_SUSPEND
const auto SOFT_SUSPEND = FCS_STATE_SOFT_SUSPEND_PROCESS;
#define set_soft_limit(u, limit) freecell_solver_user_soft_limit_iterations_long(u, limit)
#else
const auto SOFT_SUSPEND = FCS_STATE_SUSPEND_PROCESS;
#define set_soft_limit(u, limit) freecell_solver_user_limit_iterations(u, limit)
#endif

/* Get the possible moves from a position, and store them in Possible[]. */
SolverInterface::ExitStatus FcSolveSolver::patsolve(int _max_positions)
{
    int current_iters_count = 0;
    max_positions = (_max_positions < 0) ? default_max_positions : _max_positions;

    init();

    // call free once the function ends. ### Replace this mess with QScopeGuard once we can use Qt 5.12
    auto cleanup = [this]() {
        this->free();
    };
    using CleanupFunction = decltype(cleanup);
    struct CleanupHandler {
        CleanupHandler(CleanupFunction cleanup)
            : m_cleanup(std::move(cleanup))
        {
        }
        ~CleanupHandler()
        {
            m_cleanup();
        }

        CleanupFunction m_cleanup;
    } cleaner(cleanup);

    int no_use = 0;
    int num_moves = 0;
    const auto get_possible_moves__ret = get_possible_moves(&no_use, &num_moves);
    Q_ASSERT(num_moves == get_possible_moves__ret);
    Q_ASSERT(m_firstMoves.isEmpty());
    for (int j = 0; j < num_moves; ++j)
        m_firstMoves.append(Possible[j]);

    // Sometimes the solver is invoked with a small maximal iterations
    // quota/limit (e.g: when loading a saved game or checking for autodrop
    // moves) and it is done frequently, so we return prematurely without
    // invoking freecell_solver_user_alloc() and friends which incur extra
    // overhead.
    //
    // The m_firstMoves should be good enough in that case. But if there is no
    // valid return from get_possible_moves, we need to ignore the wish of the
    // dealer and calculate the full solution. This is a trade off, for fast solved
    // games, implementing a special first move calculation is just not feasible,
    // but as mentioned we need this for autodrop
    if (!num_moves) {
        max_positions = default_max_positions;
    }

    if (max_positions < 20) {
        return Solver::UnableToDetermineSolvability;
    }
    if (!solver_instance) {
        {
            solver_instance = freecell_solver_user_alloc();

            solver_ret = FCS_STATE_NOT_BEGAN_YET;

            char *error_string;
            int error_arg;
            const char *known_parameters[1] = {nullptr};
            /*  A "char *" copy instead of "const char *". */

            int parse_args_ret_code = freecell_solver_user_cmd_line_parse_args(solver_instance,
                                                                               get_cmd_line_arg_count(),
                                                                               get_cmd_line_args(),
                                                                               0,
                                                                               known_parameters,
                                                                               nullptr,
                                                                               nullptr,
                                                                               &error_string,
                                                                               &error_arg);

            Q_ASSERT(!parse_args_ret_code);
        }

        /*  Not needed for Simple Simon because it's already specified in
         *  freecell_solver_cmd_line_args. TODO : abstract .
         *
         *      Shlomi Fish
         *  */
        setFcSolverGameParams();

        current_iters_count = CHUNKSIZE;
        set_soft_limit(solver_instance, current_iters_count);
#if WITH_FCS_SOFT_SUSPEND
        freecell_solver_user_limit_iterations(solver_instance, max_positions);
#endif
    }

    if (solver_instance) {
        bool continue_loop = true;
        while (continue_loop && ((solver_ret == FCS_STATE_NOT_BEGAN_YET) || (solver_ret == SOFT_SUSPEND)) && (current_iters_count < max_positions)) {
            current_iters_count += CHUNKSIZE;
            set_soft_limit(solver_instance, current_iters_count);

            if (solver_ret == FCS_STATE_NOT_BEGAN_YET) {
                solver_ret = freecell_solver_user_solve_board(solver_instance, board_as_string);
            } else {
                solver_ret = freecell_solver_user_resume_solution(solver_instance);
            }
            {
                // QMutexLocker lock( &endMutex );
                if (m_shouldEnd) {
                    continue_loop = false;
                }
            }
        }
    }
    const long reached_iters = freecell_solver_user_get_num_times_long(solver_instance);
    Q_ASSERT(reached_iters <= default_max_positions);
#if 0
    fprintf(stderr, "iters = %ld\n", reached_iters);
#endif

    switch (solver_ret) {
    case FCS_STATE_IS_NOT_SOLVEABLE:
        make_solver_instance_ready();
        return Solver::NoSolutionExists;

    case FCS_STATE_WAS_SOLVED: {
        if (solver_instance) {
            m_winMoves.clear();
            fcs_move_t move;
            while (!freecell_solver_user_get_next_move(solver_instance, &move)) {
                MOVE new_move;

                new_move.is_fcs = true;
                new_move.fcs = move;

                m_winMoves.append(new_move);
                if (m_firstMoves.empty())
                    m_firstMoves.append(new_move);
            }

            make_solver_instance_ready();
        }
        return Solver::SolutionExists;
    }

    case FCS_STATE_SUSPEND_PROCESS:
        return Solver::UnableToDetermineSolvability;

    default:
        make_solver_instance_ready();
        return Solver::NoSolutionExists;
    }
}

/* Get the possible moves from a position, and store them in Possible[]. */

bool FcSolveSolver::isWon()
{
    return true;
}

int FcSolveSolver::getOuts()
{
    return 0;
}

FcSolveSolver::FcSolveSolver()
    : Solver()
    , default_max_positions(INITIAL_MAX_ITERS_LIMIT)
    , solver_ret(FCS_STATE_NOT_BEGAN_YET)
    , board_as_string("")
{
}

unsigned int FcSolveSolver::getClusterNumber()
{
    return 0;
}

void FcSolveSolver::print_layout()
{
#if 0
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
#endif
}

void FcSolveSolver::unpack_cluster(unsigned int)
{
    return;
}

void FcSolveSolver::make_solver_instance_ready()
{
    if (solver_instance && (solver_ret != FCS_STATE_NOT_BEGAN_YET)) {
        freecell_solver_user_recycle(solver_instance);
        solver_ret = FCS_STATE_NOT_BEGAN_YET;
    }
}

FcSolveSolver::~FcSolveSolver()
{
    if (solver_instance) {
        freecell_solver_user_free(solver_instance);
        solver_instance = nullptr;
    }
}
