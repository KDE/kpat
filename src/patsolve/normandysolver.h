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

#ifndef NORMANDYSOLVER_H
#define NORMANDYSOLVER_H

// own
#include "patsolve.h"

class Normandy;

class NormandySolver : public Solver<8 + 1 + 8>
{
public:
    explicit NormandySolver(const Normandy *dealer);
    int good_automove(int o, int r);
    int get_possible_moves(int *a, int *numout) override;
    bool isWon() override;
    void make_move(MOVE *m) override;
    void undo_move(MOVE *m) override;
    int getOuts() override;
    void translate_layout() override;
    MoveHint translateMove(const MOVE &m) override;

    void print_layout() override;

    /* Names of the cards.  The ordering is defined in pat.h. */

    const Normandy *deal;

    int deck, outs;
    int params[6];
};

#endif // NORMANDYSOLVER_H
