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

#ifndef KLONDIKESOLVER_H
#define KLONDIKESOLVER_H

class Klondike;
#include "patsolve.h"


class KlondikeSolver : public Solver
{
public:
    KlondikeSolver(const Klondike *dealer, int draw);
    int good_automove(int o, int r);
    virtual int get_possible_moves(int *a, int *numout);
    virtual bool isWon();
    virtual void make_move(MOVE *m);
    virtual void undo_move(MOVE *m);
    virtual int getOuts();
    virtual unsigned int getClusterNumber();
    virtual void translate_layout();
    virtual void unpack_cluster( unsigned int k );
    virtual MoveHint translateMove(const MOVE &m);

    virtual void print_layout();

/* Names of the cards.  The ordering is defined in pat.h. */

    card_t O[4]; /* output piles store only the rank or NONE */
    card_t Osuit[4];

    const Klondike *deal;
    int m_draw;
};

#endif // KLONDIKESOLVER_H
