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

#ifndef FORTYEIGHTSOLVER_H
#define FORTYEIGHTSOLVER_H

class Fortyeight;
#include "patsolve.h"

class FortyeightSolverState;

class FortyeightSolver : public Solver
{
public:
    explicit FortyeightSolver(const Fortyeight *dealer);
    int good_automove(int o, int r);
    int get_possible_moves(int *a, int *numout) Q_DECL_OVERRIDE;
    bool isWon() Q_DECL_OVERRIDE;
    void make_move(MOVE *m) Q_DECL_OVERRIDE;
    void undo_move(MOVE *m) Q_DECL_OVERRIDE;
    int getOuts() Q_DECL_OVERRIDE;
    unsigned int getClusterNumber() Q_DECL_OVERRIDE;
    void translate_layout() Q_DECL_OVERRIDE;
    void unpack_cluster( unsigned int k ) Q_DECL_OVERRIDE;
    MoveHint translateMove(const MOVE &m) Q_DECL_OVERRIDE;
    bool checkMove( int from, int to, MOVE *mp );
    bool checkMoveOut( int from, MOVE *mp, int *dropped );
    void checkState(FortyeightSolverState &d);

    void print_layout() Q_DECL_OVERRIDE;

    const Fortyeight *deal;
    bool lastdeal;

    card_t O[8]; /* output piles store only the rank or NONE */
    card_t Osuit[8];

};

#endif // FORTYEIGHTSOLVER_H
