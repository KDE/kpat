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

#ifndef BAKERSDOZENSOLVER_H
#define BAKERSDOZENSOLVER_H

// own
#include "abstract_fc_solve_solver.h"

constexpr auto Nwpiles = 13;
constexpr auto Ntpiles = 0;
class BakersDozen;

class BakersDozenSolver : public FcSolveSolver
{
public:
    explicit BakersDozenSolver(const BakersDozen *dealer);
    int good_automove(int o, int r);
    int get_possible_moves(int *a, int *numout) override;
    void translate_layout() override;

    MoveHint translateMove(const MOVE &m) override;
    void setFcSolverGameParams() override;
    int get_cmd_line_arg_count() override;
    const char **get_cmd_line_args() override;

    card_t O[4]; /* output piles store only the rank or NONE */

    const BakersDozen *deal;
};

#endif
