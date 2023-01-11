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

#ifndef PATSOLVE_H
#define PATSOLVE_H

// own
#include "../hint.h"
#include "memory.h"
#include "solverinterface.h"
// KCardGame
#include <KCardPile>
// Qt
#include <QMap>
// Std
#include <array>
#include <atomic>
#include <cstdio>
#include <memory>

/* A card is represented as ( down << 6 ) + (suit << 4) + rank. */

typedef quint8 card_t;

struct POSITION {
    POSITION *queue; /* next position in the queue */
    POSITION *parent; /* point back up the move stack */
    TREE *node; /* compact position rep.'s tree node */
    MOVE move; /* move that got us here from the parent */
    quint32 cluster; /* the cluster this node is in */
    short depth; /* number of moves so far */
    quint8 nchild; /* number of child nodes left */
};

class MemoryManager;

template<size_t NumberPiles>
class Solver : public SolverInterface
{
public:
    Solver();
    virtual ~Solver();
    ExitStatus patsolve(int max_positions = -1) override;
    bool recursive(POSITION *pos = nullptr);
    void translate_layout() override = 0;
    MoveHint translateMove(const MOVE &m) override = 0;
    void stopExecution() final override;
    QList<MOVE> firstMoves() const final override;
    QList<MOVE> winMoves() const final override;

protected:
    MOVE *get_moves(int *nmoves);
    bool solve(POSITION *parent);
    void doit();
    void win(POSITION *pos);
    virtual int get_possible_moves(int *a, int *numout) = 0;
    int translateSuit(int s);

    void queue_position(POSITION *pos, int pri);
    void free_position(POSITION *pos, int);
    POSITION *dequeue_position();
    void hashpile(int w);
    POSITION *new_position(POSITION *parent, MOVE *m);
    TREE *pack_position(void);
    void unpack_position(POSITION *pos);
    void init_buckets(void);
    int get_pilenum(int w);
    MemoryManager::inscode insert(unsigned int *cluster, int d, TREE **node);
    void free_buckets(void);
    void printcard(card_t card, FILE *outfile);
    int translate_pile(const KCardPile *pile, card_t *w, int size);
    virtual void print_layout();

    void pilesort(void);
    void hash_layout(void);
    virtual void make_move(MOVE *m) = 0;
    virtual void undo_move(MOVE *m) = 0;
    virtual void prioritize(MOVE *mp0, int n);
    virtual bool isWon() = 0;
    virtual int getOuts() = 0;
    virtual unsigned int getClusterNumber()
    {
        return 0;
    }
    virtual void unpack_cluster(unsigned int)
    {
    }
    void init();
    void free();

    /* Work arrays. */

    std::array<card_t *, NumberPiles> W; /* the workspace */
    std::array<card_t *, NumberPiles> Wp; /* point to the top card of each work pile */
    std::array<int, NumberPiles> Wlen; /* the number of cards in each pile */

    /* Every different pile has a hash and a unique id. */
    std::array<quint32, NumberPiles> Whash;
    std::array<int, NumberPiles> Wpilenum = {}; // = {} for zero initialization

    /* Position freelist. */

    POSITION *Freepos = nullptr;

    static constexpr auto MAXMOVES = 64; /* > max # moves from any position */
    MOVE Possible[MAXMOVES];

    std::unique_ptr<MemoryManager> mm;
    ExitStatus Status; /* win, lose, or fail */

    static constexpr auto NQUEUES = 127;

    POSITION *Qhead[NQUEUES]; /* separate queue for each priority */
    int Maxq;

    unsigned long Total_generated, Total_positions;
    qreal depth_sum;

    POSITION *Stack = nullptr;
    QMap<qint32, bool> recu_pos;
    int max_positions;

protected:
    QList<MOVE> m_firstMoves;
    QList<MOVE> m_winMoves;
    std::atomic_bool m_shouldEnd;
};

/* Misc. */

constexpr card_t PS_DIAMOND = 0x00; /* red */
constexpr card_t PS_CLUB = 0x10; /* black */
constexpr card_t PS_HEART = 0x20; /* red */
constexpr card_t PS_SPADE = 0x30; /* black */
constexpr card_t PS_BLACK = 0x10;
constexpr card_t PS_COLOR = 0x10; /* black if set */
constexpr card_t PS_SUIT = 0x30; /* mask both suit bits */

constexpr card_t NONE = 0;
constexpr card_t PS_ACE = 1;
constexpr card_t PS_KING = 13;

constexpr card_t RANK(card_t card)
{
    return card & 0xF;
}
constexpr card_t SUIT(card_t card)
{
    return (card >> 4) & 3;
}

constexpr card_t COLOR(card_t card)
{
    return card & PS_COLOR;
}
constexpr card_t DOWN(card_t card)
{
    return (card) & (1 << 7);
}

#endif // PATSOLVE_H
