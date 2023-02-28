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

#include "solverinterface.h"
// black-hole-solver
#ifdef WITH_BH_SOLVER
#include <black-hole-solver/black_hole_solver.h>
#endif

class Golf;

class GolfSolver : public SolverInterface
{
public:
    GolfSolver(const Golf *dealer);
    ~GolfSolver()
    {
    }
    int default_max_positions;

    SolverInterface::ExitStatus patsolve(int _max_positions = -1) override;
    void translate_layout() override;
    MoveHint translateMove(const MOVE &m) override;
    void stopExecution() override;
    QList<MOVE> firstMoves() const override
    {
        return m_firstMoves;
    }
    QList<MOVE> winMoves() const override
    {
        return m_winMoves;
    }

private:
    const Golf *deal;

#ifdef WITH_BH_SOLVER
    black_hole_solver_instance_t *solver_instance;
    int solver_ret;
    // More than enough space for two decks.
    char board_as_string[4 * 13 * 2 * 4 * 3];
    void free_solver_instance();
#endif
    QList<MOVE> m_firstMoves;
    QList<MOVE> m_winMoves;
};

#endif // GOLFSOLVER_H
