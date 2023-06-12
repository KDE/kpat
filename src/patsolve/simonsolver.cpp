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

#include "simonsolver.h"

// own
#include "../kpat_debug.h"
#include "../simon.h"
// freecell-solver
#include "freecell-solver/fcs_cl.h"
#include "freecell-solver/fcs_user.h"
// Std
#include <cstdlib>
#include <cstring>

#define CMD_LINE_ARGS_NUM 4
static const char *freecell_solver_cmd_line_args[CMD_LINE_ARGS_NUM] = {"-g", "simple_simon", "--load-config", "the-last-mohican"};

int SimonSolver::get_cmd_line_arg_count()
{
    return CMD_LINE_ARGS_NUM;
}

const char **SimonSolver::get_cmd_line_args()
{
    return freecell_solver_cmd_line_args;
}

void SimonSolver::setFcSolverGameParams()
{
    freecell_solver_user_apply_preset(solver_instance, "simple_simon");
}

int SimonSolver::get_possible_moves(int *, int *numout)
{
    return (*numout = 0);
}

SimonSolver::SimonSolver(const Simon *dealer)
    : FcSolveSolver()
{
    deal = dealer;
}

void SimonSolver::translate_layout()
{
    strcpy(board_as_string, deal->solverFormat().toLatin1().constData());
    make_solver_instance_ready();
}

MoveHint SimonSolver::translateMove(const MOVE &m)
{
    fcs_move_t move = m.fcs;
    int cards = fcs_move_get_num_cards_in_seq(move);
    PatPile *from = nullptr;
    PatPile *to = nullptr;
    int priority = 0;

    switch (fcs_move_get_type(move)) {
    case FCS_MOVE_TYPE_STACK_TO_STACK:
        from = deal->store[fcs_move_get_src_stack(move)];
        to = deal->store[fcs_move_get_dest_stack(move)];
        break;

    case FCS_MOVE_TYPE_SEQ_TO_FOUNDATION:
        from = deal->store[fcs_move_get_src_stack(move)];
        cards = 13;
        to = deal->target[fcs_move_get_foundation(move)];
        // no reason to delay this
        priority = 127;
        break;
    }
    Q_ASSERT(from);
    Q_ASSERT(cards <= from->cards().count());
    Q_ASSERT(to || cards == 1);
    KCard *card = from->cards()[from->cards().count() - cards];

    if (!to) {
        PatPile *target = nullptr;
        PatPile *empty = nullptr;
        for (int i = 0; i < 4; ++i) {
            KCard *c = deal->target[i]->topCard();
            if (c) {
                if (c->suit() == card->suit()) {
                    target = deal->target[i];
                    break;
                }
            } else if (!empty)
                empty = deal->target[i];
        }
        to = target ? target : empty;
    }

    Q_ASSERT(to);
    return MoveHint(card, to, priority);
}
