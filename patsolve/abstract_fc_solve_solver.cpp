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

#include "abstract_fc_solve_solver.h"

const int CHUNKSIZE = 100;
const long int MAX_ITERS_LIMIT = 200000;

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

/* Get the possible moves from a position, and store them in Possible[]. */
SolverInterface::ExitStatus FcSolveSolver::patsolve( int _max_positions )
{
    int current_iters_count;
    max_positions = (_max_positions < 0) ? MAX_ITERS_LIMIT : _max_positions;

    init();

    int no_use = 0;
    int num_moves = 0;
    get_possible_moves(&no_use, &num_moves);
    Q_ASSERT( m_firstMoves.count() == 0 );
    for (int j = 0; j < num_moves; ++j)
        m_firstMoves.append( Possible[j] );
    if (!solver_instance)
    {
        {
            solver_instance = freecell_solver_user_alloc();

            solver_ret = FCS_STATE_NOT_BEGAN_YET;

            char * error_string;
            int error_arg;
            const char * known_parameters[1] = {NULL};
            /*  A "char *" copy instead of "const char *". */

            int parse_args_ret_code = freecell_solver_user_cmd_line_parse_args(
                solver_instance,
                get_cmd_line_arg_count() ,
                get_cmd_line_args(),
                0,
                known_parameters,
                NULL,
                NULL,
                &error_string,
                &error_arg
            );

            Q_ASSERT(!parse_args_ret_code);
        }

        /*  Not needed for Simple Simon because it's already specified in
         *  freecell_solver_cmd_line_args. TODO : abstract .
         *
         *      Shlomi Fish
         *  */
        setFcSolverGameParams();

        current_iters_count = CHUNKSIZE;
        freecell_solver_user_limit_iterations(solver_instance, current_iters_count);
    }

    if (solver_instance)
    {
        bool continue_loop = true;
        while (continue_loop &&
                (   (solver_ret == FCS_STATE_NOT_BEGAN_YET)
                 || (solver_ret == FCS_STATE_SUSPEND_PROCESS))
                    &&
                 (current_iters_count < MAX_ITERS_LIMIT)
              )
        {
            current_iters_count += CHUNKSIZE;
            freecell_solver_user_limit_iterations(solver_instance, current_iters_count);

            if (solver_ret == FCS_STATE_NOT_BEGAN_YET)
            {
                solver_ret =
                    freecell_solver_user_solve_board(
                        solver_instance,
                        board_as_string
                    );
            }
            else
            {
                solver_ret = freecell_solver_user_resume_solution(solver_instance);
            }
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
        case FCS_STATE_IS_NOT_SOLVEABLE:
            if (solver_instance)
            {
                freecell_solver_user_free(solver_instance);
                solver_instance = NULL;
            }
            return Solver::NoSolutionExists;

        case FCS_STATE_WAS_SOLVED:
            {
                if (solver_instance)
                {
                    m_winMoves.clear();
                    while (freecell_solver_user_get_moves_left(solver_instance))
                    {
                        fcs_move_t move;
                        MOVE new_move;
                        const int verdict = !freecell_solver_user_get_next_move(
                                                                solver_instance, &move)
                            ;

                        Q_ASSERT (verdict);

                        new_move.is_fcs = true;
                        new_move.fcs = move;

                        m_winMoves.append( new_move );
                    }

                    freecell_solver_user_free(solver_instance);
                    solver_instance = NULL;
                }
                return Solver::SolutionExists;
            }

        case FCS_STATE_SUSPEND_PROCESS:
            return Solver::UnableToDetermineSolvability;

        default:
            if (solver_instance)
            {
                freecell_solver_user_free(solver_instance);
                solver_instance = NULL;
            }
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
    , solver_instance(NULL)
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

void FcSolveSolver::unpack_cluster( unsigned int)
{
    return;
}

FcSolveSolver::~FcSolveSolver()
{
    if (solver_instance)
    {
        freecell_solver_user_free(solver_instance);
        solver_instance = NULL;
    }
}

