/*
 * Copyright (C) 1998-2002 Tom Holroyd <tomh@kurage.nimh.nih.gov>
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

#ifndef FREECELLSOLVER_H
#define FREECELLSOLVER_H

class Freecell;
#include "patsolve.h"


class FreecellSolver : public Solver
{
public:
    explicit FreecellSolver(const Freecell *dealer);
    int good_automove(int o, int r);
    int get_possible_moves(int *a, int *numout) Q_DECL_OVERRIDE;
    bool isWon() Q_DECL_OVERRIDE;
    void make_move(MOVE *m) Q_DECL_OVERRIDE;
    void undo_move(MOVE *m) Q_DECL_OVERRIDE;
    void prioritize(MOVE *mp0, int n) Q_DECL_OVERRIDE;
    int getOuts() Q_DECL_OVERRIDE;
    unsigned int getClusterNumber() Q_DECL_OVERRIDE;
    void translate_layout() Q_DECL_OVERRIDE;
    void unpack_cluster( unsigned int k ) Q_DECL_OVERRIDE;
    MoveHint translateMove(const MOVE &m) Q_DECL_OVERRIDE;

    void print_layout() Q_DECL_OVERRIDE;

    int Nwpiles; /* the numbers we're actually using */
    int Ntpiles;

/* Names of the cards.  The ordering is defined in pat.h. */

    card_t O[4]; /* output piles store only the rank or NONE */
    card_t Osuit[4];

    const Freecell *deal;

    static int Xparam[];

};

#endif // FREECELLSOLVER_H
