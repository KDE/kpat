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

#include "freecellsolver.h"

// own
#include "../freecell.h"
#include "../settings.h"
#include "patsolve-config.h"
// freecell-solver
#include "freecell-solver/fcs_cl.h"
#include "freecell-solver/fcs_user.h"
// St
#include <cstdlib>
#include <cstring>

int m_reserves = Settings::freecellReserves();
int m_stacks = Settings::freecellStacks() + 6;
int m_decks = Settings::freecellDecks() + 1;
int m_emptyStackFill = Settings::freecellEmptyStackFill();
int m_sequenceBuiltBy = Settings::freecellSequenceBuiltBy();

const int CHUNKSIZE = 100;
const long int MAX_ITERS_LIMIT = 200000;

/* The following function implements
    (Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
namespace
{
constexpr bool suitable(const card_t a, const card_t b)
{
    return ((a ^ b) & PS_COLOR) == PS_COLOR;
}
}

/* Statistics. */

#if 0
int FreecellSolver::Xparam[] = { 4, 1, 8, -1, 7, 11, 4, 2, 2, 1, 2 };
#endif

/* These two routines make and unmake moves. */

#if 0
void FreecellSolver::make_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from pile. */

        card = *Wp[from]--;
        Wlen[from]--;
        hashpile(from);

	/* Add to pile. */

	if (m->totype == O_Type) {
            O[to]++;
        } else {
            *++Wp[to] = card;
            Wlen[to]++;
            hashpile(to);
	}
}

void FreecellSolver::undo_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from 'to' pile. */

	if (m->totype == O_Type) {
            card = O[to] + Osuit[to];
            O[to]--;
        } else {
            card = *Wp[to]--;
            Wlen[to]--;
            hashpile(to);
	}

	/* Add to 'from' pile. */
        *++Wp[from] = card;
        Wlen[from]++;
        hashpile(from);
}
#endif

#if 0
/* Move prioritization.  Given legal, pruned moves, there are still some
that are a waste of time, especially in the endgame where there are lots of
possible moves, but few productive ones.  Note that we also prioritize
positions when they are added to the queue. */

namespace {
    constexpr auto NNEED = 8;
}

void FreecellSolver::prioritize(MOVE *mp0, int n)
{
	int i, j, s, w, pile[NNEED], npile;
	card_t card, need[4];
	MOVE *mp;

	/* There are 4 cards that we "need": the next cards to go out.  We
	give higher priority to the moves that remove cards from the piles
	containing these cards. */

	for (i = 0; i < NNEED; ++i) {
		pile[i] = -1;
	}
	npile = 0;

	for (s = 0; s < 4; ++s) {
		need[s] = NONE;
		if (O[s] == NONE) {
			need[s] = Osuit[s] + PS_ACE;
		} else if (O[s] != PS_KING) {
			need[s] = Osuit[s] + O[s] + 1;
		}
	}

	/* Locate the needed cards.  There's room for optimization here,
	like maybe an array that keeps track of every card; if maintaining
	such an array is not too expensive. */

	for (w = 0; w < Nwpiles; ++w) {
		j = Wlen[w];
		for (i = 0; i < j; ++i) {
			card = W[w][i];
			s = SUIT(card);

			/* Save the locations of the piles containing
			not only the card we need next, but the card
			after that as well. */

			if (need[s] != NONE &&
			    (card == need[s] || card == need[s] + 1)) {
				pile[npile++] = w;
				if (npile == NNEED) {
					break;
				}
			}
		}
		if (npile == NNEED) {
			break;
		}
	}

	/* Now if any of the moves remove a card from any of the piles
	listed in pile[], bump their priority.  Likewise, if a move
	covers a card we need, decrease its priority.  These priority
	increments and decrements were determined empirically. */

	for (i = 0, mp = mp0; i < n; ++i, ++mp) {
		if (mp->card_index != -1) {
			w = mp->from;
			for (j = 0; j < npile; ++j) {
				if (w == pile[j]) {
					mp->pri += Xparam[0];
				}
			}
			if (Wlen[w] > 1) {
				card = W[w][Wlen[w] - 2];
				for (s = 0; s < 4; ++s) {
					if (card == need[s]) {
						mp->pri += Xparam[1];
						break;
					}
				}
			}
			if (mp->totype == W_Type) {
				for (j = 0; j < npile; ++j) {
					if (mp->to == pile[j]) {
						mp->pri -= Xparam[2];
					}
				}
			}
		}
	}
}
#endif

/* Automove logic.  Freecell games must avoid certain types of automoves. */

#if 1
int FreecellSolver::good_automove(int o, int r)
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

int FreecellSolver::get_possible_moves(int *a, int *numout)
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
#endif

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

int FreecellSolver::get_cmd_line_arg_count()
{
    return CMD_LINE_ARGS_NUM;
}

const char **FreecellSolver::get_cmd_line_args()
{
    return freecell_solver_cmd_line_args;
}

void FreecellSolver::setFcSolverGameParams()
{
    /*
     * I'm using the more standard interface instead of the depracated
     * user_set_game one. I'd like that each function will have its
     * own dedicated purpose.
     *
     *     Shlomi Fish
     * */
    freecell_solver_user_set_sequence_move(solver_instance, 0);

    m_reserves = Settings::freecellReserves();
    freecell_solver_user_set_num_freecells(solver_instance, m_reserves);

    m_stacks = Settings::freecellStacks() + 6;
    freecell_solver_user_set_num_stacks(solver_instance, m_stacks);

    m_decks = Settings::freecellDecks() + 1;
    freecell_solver_user_set_num_decks(solver_instance, m_decks);

    // FCS_ES_FILLED_BY_ANY_CARD = 0, FCS_ES_FILLED_BY_KINGS_ONLY = 1,FCS_ES_FILLED_BY_NONE = 2
    m_emptyStackFill = Settings::freecellEmptyStackFill();
    freecell_solver_user_set_empty_stacks_filled_by(solver_instance, m_emptyStackFill);

    // FCS_SEQ_BUILT_BY_ALTERNATE_COLOR = 0, FCS_SEQ_BUILT_BY_SUIT = 1, FCS_SEQ_BUILT_BY_RANK = 2
    m_sequenceBuiltBy = Settings::freecellSequenceBuiltBy();
    freecell_solver_user_set_sequences_are_built_by_type(solver_instance, m_sequenceBuiltBy);
}
#if 0
void FreecellSolver::unpack_cluster( unsigned int k )
{
    /* Get the Out cells from the cluster number. */
    O[0] = k & 0xF;
    k >>= 4;
    O[1] = k & 0xF;
    k >>= 4;
    O[2] = k & 0xF;
    k >>= 4;
    O[3] = k & 0xF;
}
#endif

FreecellSolver::FreecellSolver(const Freecell *dealer)
    : FcSolveSolver()
{
#if 0
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    Nwpiles = 8;
    Ntpiles = 4;

#endif

    deal = dealer;
}

MoveHint FreecellSolver::translateMove(const MOVE &m)
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

void FreecellSolver::translate_layout()
{
    strcpy(board_as_string, deal->solverFormat().toLatin1().constData());

    make_solver_instance_ready();
#if 0
    /* Read the workspace. */
    int total = 0;

    for ( int w = 0; w < 10; ++w ) {
        int i = translate_pile(deal->store[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    for (int i = 0; i < 4; ++i) {
        O[i] = -1;
        KCard *c = deal->target[i]->top();
        if (c) {
            total += 13;
            O[i] = translateSuit( c->suit() );
        }
    }
#endif
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

#if 0
unsigned int FreecellSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    unsigned int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}
#endif

#if 0
void FreecellSolver::print_layout()
{
       int i, t, w, o;

       fprintf(stderr, "print-layout-begin\n");
       for (w = 0; w < Nwpiles; ++w) {
	 fprintf(stderr, "W-Pile%d: ", w);
               for (i = 0; i < Wlen[w]; ++i) {
                       printcard(W[w][i], stderr);
               }
               fputc('\n', stderr);
       }
       for (t = 0; t < Ntpiles; ++t) {
	 fprintf(stderr, "T-Pile%d: ", t+Nwpiles);
           printcard(W[t+Nwpiles][Wlen[t+Nwpiles]], stderr);
       }
       fprintf( stderr, "\n" );
       for (o = 0; o < 4; ++o) {
               printcard(O[o] + Osuit[o], stderr);
       }
       fprintf(stderr, "\nprint-layout-end\n");
}
#endif
