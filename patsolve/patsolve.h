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

#include "../hint.h"
#include "memory.h"

#include "KCardPile"

#include <QtCore/QMap>
#include <QtCore/QMutex>

#include <cstdio>


/* A card is represented as ( down << 6 ) + (suit << 4) + rank. */

typedef quint8 card_t;

/* Represent a move. */

enum PileType { O_Type = 1, W_Type };

class MOVE {
public:
    int card_index;         /* the card we're moving (0 is the top)*/
    quint8 from;            /* from pile number */
    quint8 to;              /* to pile number */
    PileType totype;
    signed char pri;        /* move priority (low priority == low value) */
    int turn_index;         /* turn the card index */

    bool operator<( const MOVE &m) const
    {
	return pri < m.pri;
    }
};

struct POSITION;

struct POSITION {
        POSITION *queue;      /* next position in the queue */
	POSITION *parent;     /* point back up the move stack */
	TREE *node;             /* compact position rep.'s tree node */
	MOVE move;              /* move that got us here from the parent */
	quint32 cluster; /* the cluster this node is in */
	short depth;            /* number of moves so far */
	quint8 ntemp;           /* number of cards in T */
	quint8 nchild;          /* number of child nodes left */
};

class MemoryManager;

class Solver
{
public:
    enum ExitStatus
    {
        MemoryLimitReached = -3,
        SearchAborted = -2,
        UnableToDetermineSolvability = -1,
        NoSolutionExists = 0,
        SolutionExists = 1
    };

    Solver();
    virtual ~Solver();
    ExitStatus patsolve( int max_positions = -1, bool debug = false);
    bool recursive(POSITION *pos = 0);
    virtual void translate_layout() = 0;
    bool m_shouldEnd;
    QMutex endMutex;
    virtual MoveHint translateMove(const MOVE &m ) = 0;
    QList<MOVE> firstMoves;
    QList<MOVE> winMoves;

protected:
    MOVE *get_moves(int *nmoves);
    bool solve(POSITION *parent);
    void doit();
    void win(POSITION *pos);
    virtual int get_possible_moves(int *a, int *numout) = 0;
    int translateSuit( int s );

    int wcmp(int a, int b);
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
    void hash_layout( void );
    virtual void make_move(MOVE *m) = 0;
    virtual void undo_move(MOVE *m) = 0;
    virtual void prioritize(MOVE *mp0, int n);
    virtual bool isWon() = 0;
    virtual int getOuts() = 0;
    virtual unsigned int getClusterNumber() { return 0; }
    virtual void unpack_cluster( unsigned int  ) {}

    void setNumberPiles( int i );
    int m_number_piles;

    void init();
    void free();

    /* Work arrays. */

    card_t **W; /* the workspace */
    card_t **Wp;   /* point to the top card of each work pile */
    int *Wlen;     /* the number of cards in each pile */

    /* Every different pile has a hash and a unique id. */
    quint32 *Whash;
    int *Wpilenum;

    /* Position freelist. */

    POSITION *Freepos;

#define MAXMOVES 64             /* > max # moves from any position */
    MOVE Possible[MAXMOVES];

    MemoryManager *mm;
    ExitStatus Status;             /* win, lose, or fail */

#define NQUEUES 127

    POSITION *Qhead[NQUEUES]; /* separate queue for each priority */
    int Maxq;

    bool m_newer_piles_first;
    unsigned long Total_generated, Total_positions;
    qreal depth_sum;

    POSITION *Stack;
    QMap<qint32,bool> recu_pos;
    int max_positions;
    bool debug;
};

/* Misc. */

#define PS_DIAMOND 0x00         /* red */
#define PS_CLUB    0x10         /* black */
#define PS_HEART   0x20         /* red */
#define PS_SPADE   0x30         /* black */
#define PS_BLACK   0x10
#define PS_COLOR   0x10         /* black if set */
#define PS_SUIT    0x30         /* mask both suit bits */

#define NONE    0
#define PS_ACE  1
#define PS_KING 13

#define RANK(card) ((card) & 0xF)
#define SUIT(card) (( (card) >> 4 ) & 3)
#define COLOR(card) ((card) & PS_COLOR)
#define DOWN(card) ((card) & ( 1 << 7 ) )

extern long all_moves;

#endif // PATSOLVE_H
