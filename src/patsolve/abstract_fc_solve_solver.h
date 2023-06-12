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

// own
#include "patsolve.h"

struct FcSolveSolver : public Solver<20> {
public:
    FcSolveSolver();
    virtual ~FcSolveSolver();
    int get_possible_moves(int *a, int *numout) override = 0;
    bool isWon() override;
    void make_move(MOVE *m) override;
    void undo_move(MOVE *m) override;
    int getOuts() override;
    unsigned int getClusterNumber() override;
    void translate_layout() override = 0;
    void unpack_cluster(unsigned int k) override;
    MoveHint translateMove(const MOVE &m) override = 0;
    SolverInterface::ExitStatus patsolve(int _max_positions = -1) override;
    virtual void setFcSolverGameParams() = 0;

    void print_layout() override;

    virtual int get_cmd_line_arg_count() = 0;
    virtual const char **get_cmd_line_args() = 0;
    /* Names of the cards.  The ordering is defined in pat.h. */

    void *solver_instance{nullptr};
    long default_max_positions;
    int solver_ret;
    // More than enough space for two decks.
    char board_as_string[4 * 13 * 2 * 4 * 3];

protected:
    void make_solver_instance_ready();
};

#endif // ABSTRACT_FC_SOLVE_SOLVER_H
