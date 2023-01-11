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

#ifndef GRANDFSOLVER_H
#define GRANDFSOLVER_H

// own
#include "patsolve.h"

class Grandf;

class GrandfSolver : public Solver<7 * 3 + 1>
{
public:
    explicit GrandfSolver(const Grandf *dealer);

    int get_possible_moves(int *a, int *numout) override;
    bool isWon() override;
    void make_move(MOVE *m) override;
    void undo_move(MOVE *m) override;
    int getOuts() override;
    unsigned int getClusterNumber() override;
    void translate_layout() override;
    void unpack_cluster(unsigned int k) override;
    MoveHint translateMove(const MOVE &m) override;

    void print_layout() override;

    card_t Osuit[4];

    const Grandf *deal;
    int m_redeal;
    int offs;
};

#endif // GRANDFSOLVER_H
