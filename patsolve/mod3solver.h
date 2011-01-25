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

#ifndef MOD3SOLVER_H
#define MOD3SOLVER_H

class Mod3;
#include "patsolve.h"


class Mod3Solver : public Solver
{
public:
    Mod3Solver(const Mod3 *dealer);
    virtual int get_possible_moves(int *a, int *numout);
    virtual bool isWon();
    virtual void make_move(MOVE *m);
    virtual void undo_move(MOVE *m);
    virtual int getOuts();
    virtual void translate_layout();
    virtual MoveHint translateMove(const MOVE &m);

    virtual void print_layout();

    const Mod3 *deal;
    int aces;
    int deck;
};

#endif // MOD3SOLVER_H
