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
#include "patsolve-config.h"
#include "patsolve.h"
#ifdef WITH_BH_SOLVER
#include <black-hole-solver/black_hole_solver.h>
#endif


class GolfSolver : public Solver<9>
{
public:
    explicit GolfSolver(const Golf *dealer);
    int default_max_positions;

#ifdef WITH_BH_SOLVER
    black_hole_solver_instance_t *solver_instance;
    int solver_ret;
    SolverInterface::ExitStatus patsolve( int _max_positions ) override;
    // More than enough space for two decks.
    char board_as_string[4 * 13 * 2 * 4 * 3];
    void free_solver_instance();
#endif
    int get_possible_moves(int *a, int *numout) override;
    bool isWon() override;
    void make_move(MOVE *m) override;
    void undo_move(MOVE *m) override;
    int getOuts() override;
    void translate_layout() override;
    MoveHint translateMove(const MOVE &m) override;

    void print_layout() override;

    const Golf *deal;
};

#endif // GOLFSOLVER_H
