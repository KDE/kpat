/*
 * Copyright (C) 2006-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2026 Sam Streeper <ottovox@gmail.com>
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

/* Much of this code is from Kulow's klondike solver.
 * Changes by Streeper for bizilizi rule & pile differences.
 * Responsibility for bugs with Streeper
 */
#include "bizilizisolver.h"

// own
#include "../bizilizi.h"
#include "../kpat_debug.h"

/* Some macros used in get_possible_moves(). */

/* The following macro implements
    (Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) (COLOR(a) != COLOR(b))

#define PRINT 0

/* These two routines make and unmake moves. */

void BiziliziSolver::make_move(MOVE *m)
{
#if PRINT
    if (m->totype == O_Type)
        fprintf(stderr, "\nmake move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index);
    else
        fprintf(stderr, "\nmake move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index);
    print_layout();
#else
    // print_layout();
#endif

    int from, to;
    card_t card = NONE;

    from = m->from;
    to = m->to;

    int final_index = 0;
    if (from == 7)
        // bsource pile, remove mid card but not cards above it
        final_index = m->card_index;

    // add cards to end of TO
    for (int l = m->card_index; l >= final_index; --l) {
        card = W[from][Wlen[from] - l - 1];
        Wp[from]--;
        if (m->totype != O_Type) {
            Wp[to]++;
            *Wp[to] = card;
            Wlen[to]++;
        }
    }
    if (from == 7) {
        // if BSOURCE mid card, move tail in & close gap
        int start = Wlen[7] - m->card_index - 1;
        for (int l = m->card_index; l >= 1; --l) {
            W[7][start] = W[7][start + 1];
            start++;
        }
        Wlen[7]--;
    } else
        // delete cards at tail of FROM
        Wlen[from] -= m->card_index + 1;

    if (m->turn_index != -1) {
        card_t card2 = *Wp[from];
        if (DOWN(card2))
            card2 = (SUIT(card2) << 4) + RANK(card2);
        *Wp[from] = card2;
    }

    hashpile(from);
    /* Add to pile. */

    if (m->totype == O_Type) {
        O[to]++;
        Q_ASSERT(m->card_index == 0);
    } else {
        hashpile(to);
    }
#if PRINT
    print_layout();
#endif
}

void BiziliziSolver::undo_move(MOVE *m)
{
#if PRINT
    if (m->totype == O_Type)
        fprintf(stderr, "\nundo move %d from %d out (at %d)\n\n", m->card_index, m->from, m->turn_index);
    else
        fprintf(stderr, "\nundo move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index);
    print_layout();

#endif
    int from, to;
    card_t card;

    from = m->from;
    to = m->to;

    /* Add to 'from' pile. */
    if (m->turn_index > 0) {
        card_t card2 = *Wp[from];
        if (!DOWN(card2)) {
            card2 = (SUIT(card2) << 4) + RANK(card2) + (1 << 7);
            *Wp[from] = card2;
        }
    }

    if (from == 7) {
        // open gap in BSOURCE by moving tail above index out
        int start = Wlen[7] - 1;
        for (int l = m->card_index; l >= 1; --l) {
            W[7][start + 1] = W[7][start];
            start--;
        }
    }
    if (m->totype == O_Type) {
        card = O[to] + Osuit[to];
        // remove card from target/foundation
        O[to]--;
        Wp[from]++;
        // add card to (from)card_index, not end (works also for BSOURCE)
        //*Wp[from] = card;  // broken on bsource
#if 0
        auto oldp = Wp[from];
        auto newp = &W[from][Wlen[from] - m->card_index];
        fprintf(stderr, "--- old %p, new %p   ------\n", (void *)oldp, (void *)newp);
#endif
        W[from][Wlen[from] - m->card_index] = card;
        Wlen[from]++;
    } else {
        int final_index = 0;
        if (from == 7) {
            final_index = m->card_index;
        }
        for (int l = m->card_index; l >= final_index; --l) {
            Wp[from]++;
            if (from == 7) {
                // just pop the last one
                card = *Wp[to];
                *(Wp[7] - m->card_index) = card;
            } else {
                // iterating with first card
                card = W[to][Wlen[to] - l - 1];
                *Wp[from] = card;
            }
            Wlen[from]++;
            // remove card(s) from TO
            Wp[to]--;
        }
        if (from == 7)
            Wlen[to]--;
        else
            Wlen[to] -= m->card_index + 1;
        hashpile(to);
    }

    hashpile(from);
#if PRINT
    print_layout();
#endif
}

/* Automove logic.  Bizilizi games must avoid certain types of automoves. */

int BiziliziSolver::good_automove(int o, int r)
{
    int i;

    if (r <= 2) {
        return true;
    }

    /* Check the Out piles of opposite color. */

    for (i = 1 - (o & 1); i < 4; i += 2) {
        if (O[i] < r - 1) {
#if 1 /* Raymond's Rule */
            /* Not all the N-1's of opposite color are out
            yet.  We can still make an automove if either
            both N-2's are out or the other same color N-3
            is out (Raymond's rule).  Note the re-use of
            the loop variable i.  We return here and never
            make it back to the outer loop. */

            for (i = 1 - (o & 1); i < 4; i += 2) {
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

/* Get the possible moves from a position, and store them in Possible[]. */

int BiziliziSolver::get_possible_moves(int *a, int *numout)
{
    int w, o, empty;
    card_t card;
    MOVE *mp;

    /* Check for moves from W to O. */

    int n = 0;
    mp = Possible;
    for (w = 0; w < 8; ++w) {
        if (Wlen[w] > 0) {
            card = *Wp[w];
            o = SUIT(card);
            empty = (O[o] == NONE);
            if ((empty && (RANK(card) == PS_ACE)) || (!empty && (RANK(card) == O[o] + 1))) {
                // automove to target/foundation
                mp->card_index = 0; // top card
                mp->from = w;
                mp->to = o;
                mp->totype = O_Type;
                mp->pri = 3; /* unused */
                mp->turn_index = -1;
                if (Wlen[w] > 1 && DOWN(W[w][Wlen[w] - 2]))
                    mp->turn_index = 1;
                n++;
                mp++;

                if (good_automove(o, RANK(card))) {
                    *a = true;
                    mp[-1].pri = 127;
                    if (n != 1) {
                        // good automove and other possible foundation move(s)
                        // just return the automove
                        Possible[0] = mp[-1];
                        return 1;
                    }
                    return n;
                }
            }
        }
    }

    /* No more automoves, but remember if there were any moves out. */

    *a = false;
    *numout = n;

    // we first check where to put a king, so we don't
    // try each king on each empty pile
    int first_empty_pile = -1;
    for (int i = 0; i < 8; ++i)
        if (!Wlen[i]) {
            first_empty_pile = i;
            break;
        }

    // from all play & bsource
    for (int i = 0; i < 8; ++i) {
        int len = Wlen[i];

        // top card, working down
        for (int l = 0; l < len; ++l) {
            card_t card = W[i][Wlen[i] - 1 - l];
            if (DOWN(card))
                break;

            // to all play piles
            for (int j = 0; j < 7; ++j) {
                if (i == j)
                    continue;

                int allowed = 0;

                if (Wlen[j] > 0 && RANK(card) == RANK(*Wp[j]) - 1 && suitable(card, *Wp[j])) {
                    // can move suitable(rank,color) card to occupied play pile
                    allowed = 1;
                    if (Wlen[i] == l + 1) {
                        // this allowed move is all the cards from source
                        allowed = 2;
                    } else {
                        // this allowed move is all faceup cards, but down cards would remain
                        if (DOWN(W[i][Wlen[i] - l - 2]))
                            allowed = 3;
                    }
                }
                if (RANK(card) == PS_KING && j == first_empty_pile) {
                    if (l != Wlen[i] - 1 || i == 7)
                        // allowed to move king from play pile with cards,
                        // or from bsource
                        allowed = 4;
                }
                if (allowed && i == 7)
                    // note bsource
                    allowed = 5;

                if (allowed == 1) {
                    // print_layout();
                    card_t below = W[i][Wlen[i] - 2 - l];
                    int o = SUIT(below);
                    bool empty = (O[o] == NONE);
                    // fprintf( stderr, "%d %d\n", i, l );
                    // printcard( below, stderr );

                    if ((empty && RANK(below) != PS_ACE) || (!empty && RANK(below) != O[o] + 1))
                        // all..1: ^^^ always true? ^^^^  bc Ace not remaining exposed on source move
                        //   remaining source pile card can't go to target pile, not allowed here (?)
                        allowed = 0;
                    // fprintf( stderr, " allowed %d %d %d %d\n",allowed, empty, RANK( below ), PS_ACE);
                }
                if (allowed) {
                    mp->card_index = l;
                    mp->from = i;
                    mp->to = j;
                    mp->totype = W_Type;
                    mp->turn_index = -1;
                    if (Wlen[i] > l + 1 && DOWN(W[i][Wlen[i] - l - 2]))
                        // move will flip down reveal
                        mp->turn_index = 1;
                    if (i == 7)
                        // bsource / waste
                        mp->pri = 40;
                    else {
                        if (mp->turn_index > 0 || Wlen[i] == l + 1)
                            // flip or empty source
                            mp->pri = 30;
                        else
                            // possible but no advantage known
                            mp->pri = 1;
                    }
                    n++;
                    mp++;
                }
            }
        }
    }

    return n;
}

void BiziliziSolver::unpack_cluster(unsigned int k)
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

bool BiziliziSolver::isWon()
{
    // maybe won?
    for (int o = 0; o < 4; ++o) {
        if (O[o] != PS_KING) {
            return false;
        }
    }

    return true;
}

int BiziliziSolver::getOuts()
{
    return O[0] + O[1] + O[2] + O[3];
}

BiziliziSolver::BiziliziSolver(const Bizilizi *dealer)
    : Solver()
{
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;

    deal = dealer;
}

void BiziliziSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for (int w = 0; w < 7; ++w) {
        int i = translate_pile(deal->play[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    int i = translate_pile(deal->bsource, W[7], 52);
    Wp[7] = &W[7][i - 1];
    Wlen[7] = i;
    total += i;

    /* Output piles, if any. */
    for (int i = 0; i < 4; ++i) {
        O[i] = NONE;
    }
    if (total != 52) {
        for (int i = 0; i < 4; ++i) {
            KCard *c = deal->target[i]->topCard();
            if (c) {
                O[translateSuit(c->suit()) >> 4] = c->rank();
                total += c->rank();
            }
        }
    }
}

unsigned int BiziliziSolver::getClusterNumber()
{
    int i = O[0] + (O[1] << 4);
    unsigned int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    return k;
}

MoveHint BiziliziSolver::translateMove(const MOVE &m)
{
    PatPile *frompile = nullptr;
    if (m.from == 7)
        frompile = deal->bsource;
    else
        frompile = deal->play[m.from];

    KCard *card = frompile->at(frompile->count() - m.card_index - 1);

    if (m.totype == O_Type) {
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
        if (!target)
            target = empty;
        return MoveHint(card, target, m.pri);
    } else {
        if (m.to == 7) {
            // no moving to bsource ??
            return MoveHint();
        } else
            return MoveHint(card, deal->play[m.to], m.pri);
    }
}

void BiziliziSolver::print_layout()
{
    int i, w, o;

    fprintf(stderr, "print-layout-begin\n");
    for (w = 0; w < 8; ++w) {
        if (w == 7)
            fprintf(stderr, "Bsource: ");
        else
            fprintf(stderr, "Play%d: ", w);
        for (i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf(stderr, "Off: ");
    for (o = 0; o < 4; ++o) {
        printcard(O[o] + Osuit[o], stderr);
    }
    fprintf(stderr, "\nprint-layout-end\n");
}
