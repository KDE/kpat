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

#ifndef GYPSYSOLVER_H
#define GYPSYSOLVER_H

class Gypsy;
#include "patsolve.h"


class GypsySolver : public Solver
{
public:
    GypsySolver(const Gypsy *dealer);
    int good_automove(int o, int r);
    virtual int get_possible_moves(int *a, int *numout);
    virtual bool isWon();
    virtual void make_move(MOVE *m);
    virtual void undo_move(MOVE *m);
    virtual int getOuts();
    virtual void translate_layout();
    virtual MoveHint translateMove(const MOVE &m);

    virtual void print_layout();

/* Names of the cards.  The ordering is defined in pat.h. */

    const Gypsy *deal;

    int deck, outs;
    int params[6];
};

#endif // GYPSYSOLVER_H
