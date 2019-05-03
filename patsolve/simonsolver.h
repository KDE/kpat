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

#include "abstract_fc_solve_solver.h"
#include "simon.h"
class Simon;


class SimonSolver : public FcSolveSolver
{
public:
    explicit SimonSolver(const Simon *dealer);
    int get_possible_moves(int *a, int *numout) Q_DECL_OVERRIDE;
#if 0
    bool isWon() Q_DECL_OVERRIDE;
    void make_move(MOVE *m) Q_DECL_OVERRIDE;
    void undo_move(MOVE *m) Q_DECL_OVERRIDE;
    int getOuts() Q_DECL_OVERRIDE;
    unsigned int getClusterNumber() Q_DECL_OVERRIDE;
#endif
    void translate_layout() override;
    MoveHint translateMove(const MOVE &m) override;
#if 0
    void unpack_cluster( unsigned int k ) Q_DECL_OVERRIDE;
    void print_layout() Q_DECL_OVERRIDE;
#endif
    void setFcSolverGameParams() override;

    int get_cmd_line_arg_count() override;
    const char * * get_cmd_line_args() override;
#if 0
/* Names of the cards.  The ordering is defined in pat.h. */
    int O[4];
#endif
    const Simon *deal;
};

#endif // SIMONSOLVER_H
