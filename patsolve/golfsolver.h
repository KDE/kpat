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

#ifndef GOLFSOLVER_H
#define GOLFSOLVER_H

class Golf;
#include "patsolve.h"
#ifdef WITH_BH_SOLVER
#include <black-hole-solver/bool.h>
#include <black-hole-solver/black_hole_solver.h>
#endif


class GolfSolver : public Solver<9>
{
public:
    explicit GolfSolver(const Golf *dealer);

#ifdef WITH_BH_SOLVER
    black_hole_solver_instance_t *solver_instance;
    int solver_ret;
    SolverInterface::ExitStatus patsolve( int _max_positions );
    // More than enough space for two decks.
    char board_as_string[4 * 13 * 2 * 4 * 3];
    void free_solver_instance();
#endif
    int get_possible_moves(int *a, int *numout) Q_DECL_OVERRIDE;
    bool isWon() Q_DECL_OVERRIDE;
    void make_move(MOVE *m) Q_DECL_OVERRIDE;
    void undo_move(MOVE *m) Q_DECL_OVERRIDE;
    int getOuts() Q_DECL_OVERRIDE;
    void translate_layout() Q_DECL_OVERRIDE;
    MoveHint translateMove(const MOVE &m) Q_DECL_OVERRIDE;

    void print_layout() Q_DECL_OVERRIDE;

    const Golf *deal;
};

#endif // GOLFSOLVER_H
