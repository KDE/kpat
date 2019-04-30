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

#ifndef ABSTRACT_FC_SOLVE_SOLVER_H
#define ABSTRACT_FC_SOLVE_SOLVER_H

#include "patsolve.h"

struct FcSolveSolver : public Solver<12>
{
public:
    FcSolveSolver();
    virtual ~FcSolveSolver();
    int get_possible_moves(int *a, int *numout) Q_DECL_OVERRIDE = 0;
    bool isWon() Q_DECL_OVERRIDE;
    void make_move(MOVE *m) Q_DECL_OVERRIDE;
    void undo_move(MOVE *m) Q_DECL_OVERRIDE;
    int getOuts() Q_DECL_OVERRIDE;
    unsigned int getClusterNumber() Q_DECL_OVERRIDE;
    void translate_layout() Q_DECL_OVERRIDE = 0;
    void unpack_cluster( unsigned int k ) Q_DECL_OVERRIDE;
    MoveHint translateMove(const MOVE &m) Q_DECL_OVERRIDE = 0;
    SolverInterface::ExitStatus patsolve( int _max_positions = -1) Q_DECL_OVERRIDE;
    virtual void setFcSolverGameParams() = 0;

    void print_layout() Q_DECL_OVERRIDE;

    virtual int get_cmd_line_arg_count() = 0;
    virtual const char * * get_cmd_line_args() = 0;
/* Names of the cards.  The ordering is defined in pat.h. */

    void * solver_instance;
    int solver_ret;
    // More than enough space for two decks.
    char board_as_string[4 * 13 * 2 * 4 * 3];
};

#endif // ABSTRACT_FC_SOLVE_SOLVER_H
