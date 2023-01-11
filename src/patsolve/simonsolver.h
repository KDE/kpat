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

#ifndef SIMONSOLVER_H
#define SIMONSOLVER_H

// own
#include "abstract_fc_solve_solver.h"
#include "simon.h"

class Simon;

class SimonSolver : public FcSolveSolver
{
public:
    explicit SimonSolver(const Simon *dealer);
    int get_possible_moves(int *a, int *numout) override;
#if 0
    bool isWon() override;
    void make_move(MOVE *m) override;
    void undo_move(MOVE *m) override;
    int getOuts() override;
    unsigned int getClusterNumber() override;
#endif
    void translate_layout() override;
    MoveHint translateMove(const MOVE &m) override;
#if 0
    void unpack_cluster( unsigned int k ) override;
    void print_layout() override;
#endif
    void setFcSolverGameParams() override;

    int get_cmd_line_arg_count() override;
    const char **get_cmd_line_args() override;
#if 0
/* Names of the cards.  The ordering is defined in pat.h. */
    int O[4];
#endif
    const Simon *deal;
};

#endif // SIMONSOLVER_H
