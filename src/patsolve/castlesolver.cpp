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

#include "castlesolver.h"

// own
#include "../castle.h"
#include "../settings.h"
#include "patsolve-config.h"
// freecell-solver
#include "freecell-solver/fcs_cl.h"
#include "freecell-solver/fcs_user.h"
// St
#include <cstdlib>
#include <cstring>

namespace
{
int m_reserves = Settings::castleReserves();
int m_stacks = Settings::castleStacks();
int m_decks = 1;
int m_emptyStackFill = Settings::castleEmptyStackFill();
int m_sequenceBuiltBy = Settings::castleSequenceBuiltBy();
}

/* Automove logic.  Freecell games must avoid certain types of automoves. */
int CastleSolver::good_automove(int o, int r)
{
    if (r <= 2) {
        return true;
    }

    if (m_sequenceBuiltBy == 1) {
        return true;
    }

    for (int foundation_idx = 0; foundation_idx < 4 * m_decks; ++foundation_idx) {
        KCard *c = deal->target[foundation_idx]->topCard();
        if (c) {
            O[translateSuit(c->suit()) >> 4] = c->rank();
        }
    }
    /* Check the Out piles of opposite color. */

    for (int i = 1 - (o & 1); i < 4 * m_decks; i += 2) {
        if (O[i] < r - 1) {
#if 1 /* Raymond's Rule */
            /* Not all the N-1's of opposite color are out
            yet.  We can still make an automove if either
            both N-2's are out or the other same color N-3
            is out (Raymond's rule).  Note the re-use of
            the loop variable i.  We return here and never
            make it back to the outer loop. */

            for (i = 1 - (o & 1); i < 4 * m_decks; i += 2) {
                if (O[i] < r - 2) {
                    return false;
                }
            }
            if (O[(o + 2) & 3] < r - 3) {
                return false;
            }

            return true;
#else /* Horne's Rule */
            return false;
#endif
        }
    }

    return true;
}

int CastleSolver::get_possible_moves(int *a, int *numout)
{
    int w;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (w = 0; w < m_stacks + m_reserves; ++w) {
        if (Wlen[w] > 0) {
            card = *Wp[w];
            int out_suit = SUIT(card);
            const bool empty = (O[out_suit] == NONE);
            if ((empty && (RANK(card) == PS_ACE)) || (!empty && (RANK(card) == O[out_suit] + 1))) {
                mp->is_fcs = false;
                mp->card_index = 0;
                mp->from = w;
                mp->to = out_suit;
                mp->totype = O_Type;
                mp->turn_index = -1;
                mp->pri = 0; /* unused */
                n++;
                mp++;

                /* If it's an automove, just do it. */

                if (good_automove(out_suit, RANK(card))) {
                    *a = true;
                    mp[-1].pri = 127;
                    if (n != 1) {
                        Possible[0] = mp[-1];
                        return (*numout = 1);
                    }
                    return (*numout = n);
                }
            }
        }
    }
    return (*numout = 0);
}

#define CMD_LINE_ARGS_NUM 2

static const char *freecell_solver_cmd_line_args[CMD_LINE_ARGS_NUM] = {
#if WITH_FCS_SOFT_SUSPEND
    "--load-config",
    "video-editing"
#else
    "--load-config",
    "slick-rock"
#endif
};

int CastleSolver::get_cmd_line_arg_count()
{
    return CMD_LINE_ARGS_NUM;
}

const char **CastleSolver::get_cmd_line_args()
{
    return freecell_solver_cmd_line_args;
}

void CastleSolver::setFcSolverGameParams()
{
    /*
     * I'm using the more standard interface instead of the depracated
     * user_set_game one. I'd like that each function will have its
     * own dedicated purpose.
     *
     *     Shlomi Fish
     * */
    freecell_solver_user_set_sequence_move(solver_instance, 0);

    m_reserves = Settings::castleReserves();
    freecell_solver_user_set_num_freecells(solver_instance, m_reserves);

    m_stacks = Settings::castleStacks();
    freecell_solver_user_set_num_stacks(solver_instance, m_stacks);

    freecell_solver_user_set_num_decks(solver_instance, m_decks);

    // FCS_ES_FILLED_BY_ANY_CARD = 0, FCS_ES_FILLED_BY_KINGS_ONLY = 1,FCS_ES_FILLED_BY_NONE = 2
    m_emptyStackFill = Settings::castleEmptyStackFill();
    freecell_solver_user_set_empty_stacks_filled_by(solver_instance, m_emptyStackFill);

    // FCS_SEQ_BUILT_BY_ALTERNATE_COLOR = 0, FCS_SEQ_BUILT_BY_SUIT = 1, FCS_SEQ_BUILT_BY_RANK = 2
    m_sequenceBuiltBy = Settings::castleSequenceBuiltBy();
    freecell_solver_user_set_sequences_are_built_by_type(solver_instance, m_sequenceBuiltBy);
}

CastleSolver::CastleSolver(const Castle *dealer)
    : FcSolveSolver()
{
    deal = dealer;
}

MoveHint CastleSolver::translateMove(const MOVE &m)
{
    if (m.is_fcs) {
        fcs_move_t move = m.fcs;
        int cards = fcs_move_get_num_cards_in_seq(move);
        PatPile *from = nullptr;
        PatPile *to = nullptr;

        switch (fcs_move_get_type(move)) {
        case FCS_MOVE_TYPE_STACK_TO_STACK:
            from = deal->store[fcs_move_get_src_stack(move)];
            to = deal->store[fcs_move_get_dest_stack(move)];
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_STACK:
            from = deal->freecell[fcs_move_get_src_freecell(move)];
            to = deal->store[fcs_move_get_dest_stack(move)];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_FREECELL:
            from = deal->freecell[fcs_move_get_src_freecell(move)];
            to = deal->freecell[fcs_move_get_dest_freecell(move)];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_STACK_TO_FREECELL:
            from = deal->store[fcs_move_get_src_stack(move)];
            to = deal->freecell[fcs_move_get_dest_freecell(move)];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_STACK_TO_FOUNDATION:
            from = deal->store[fcs_move_get_src_stack(move)];
            cards = 1;
            to = nullptr;
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION:
            from = deal->freecell[fcs_move_get_src_freecell(move)];
            cards = 1;
            to = nullptr;
        }
        Q_ASSERT(from);
        Q_ASSERT(cards <= from->cards().count());
        Q_ASSERT(to || cards == 1);
        KCard *card = from->cards()[from->cards().count() - cards];

        if (!to) {
            PatPile *target = nullptr;
            PatPile *empty = nullptr;
            for (int i = 0; i < 4 * m_decks; ++i) {
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

        return MoveHint(card, to, 0);
    } else {
        // this is tricky as we need to want to build the "meta moves"

        PatPile *frompile = nullptr;
        if (m.from < m_stacks)
            frompile = deal->store[m.from];
        else
            frompile = deal->freecell[m.from - m_stacks];
        KCard *card = frompile->at(frompile->count() - m.card_index - 1);

        if (m.totype == O_Type) {
            PatPile *target = nullptr;
            PatPile *empty = nullptr;
            for (int i = 0; i < 4 * m_decks; ++i) {
                KCard *c = deal->target[i]->topCard();
                if (c) {
                    if (c->suit() == card->suit()) {
                        target = deal->target[i];
                        break;
                    }
                } else if (!empty)
                    empty = deal->target[i];
            }
            if (!target)
                target = empty;
            return MoveHint(card, target, m.pri);
        } else {
            PatPile *target = nullptr;
            if (m.to < m_stacks)
                target = deal->store[m.to];
            else
                target = deal->freecell[m.to - m_stacks];

            return MoveHint(card, target, m.pri);
        }
    }
}

void CastleSolver::translate_layout()
{
    strcpy(board_as_string, deal->solverFormat().toLatin1().constData());

    make_solver_instance_ready();
    /* Read the workspace. */

    int total = 0;
    for (int w = 0; w < m_stacks; ++w) {
        int i = translate_pile(deal->store[w], W[w], 52 * m_decks);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
        if (w == m_stacks) {
            break;
        }
    }

    /* Temp cells may have some cards too. */

    for (int w = 0; w < m_reserves; ++w) {
        int i = translate_pile(deal->freecell[w], W[w + m_stacks], 52 * m_decks);
        Wp[w + m_stacks] = &W[w + m_stacks][i - 1];
        Wlen[w + m_stacks] = i;
        total += i;
    }

    /* Output piles, if any. */
    for (int i = 0; i < 4 * m_decks; ++i) {
        O[i] = NONE;
    }
    if (total != 52 * m_decks) {
        for (int i = 0; i < 4 * m_decks; ++i) {
            KCard *c = deal->target[i]->topCard();
            if (c) {
                O[translateSuit(c->suit()) >> 4] = c->rank();
                total += c->rank();
            }
        }
    }
}
