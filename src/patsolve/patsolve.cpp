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

#include "patsolve.h"

// own
#include "../kpat_debug.h"
#include "../patpile.h"
// Std
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>

#ifdef ERR
#undef ERR
#endif

long all_moves = 0;

/* This is a 32 bit FNV hash.  For more information, see
http://www.isthe.com/chongo/tech/comp/fnv/index.html */

namespace
{
constexpr qint32 FNV_32_PRIME = 0x01000193; // FIXME: move into fnv_hash once we depend on C++14
constexpr qint32 fnv_hash(qint32 x, qint32 hash)
{
    return (hash * FNV_32_PRIME) ^ x;
}
/* Hash a 0 terminated string. */

quint32 fnv_hash_str(const quint8 *s)
{
    constexpr qint32 FNV1_32_INIT = 0x811C9DC5;
    quint32 h;

    h = FNV1_32_INIT;
    while (*s) {
        h = fnv_hash(*s++, h);
    }

    return h;
}
}

/* Hash a pile. */

template<size_t NumberPiles>
void Solver<NumberPiles>::hashpile(int w)
{
    W[w][Wlen[w]] = 0;
    Whash[w] = fnv_hash_str(W[w]);

    /* Invalidate this pile's id.  We'll calculate it later. */

    Wpilenum[w] = -1;
}

/* Generate an array of the moves we can make from this position. */

template<size_t NumberPiles>
MOVE *Solver<NumberPiles>::get_moves(int *nmoves)
{
    int i, n, alln, a = 0, numout = 0;
    MOVE *mp, *mp0;

    /* Fill in the Possible array. */

    alln = n = get_possible_moves(&a, &numout);
#ifdef GET_MOVES_DEBUG
    {
        print_layout();
        fprintf(stderr, "moves %d\n", n);
        for (int j = 0; j < n; j++) {
            fprintf(stderr, "  ");
            if (Possible[j].totype == O_Type)
                fprintf(stderr, "move from %d out (at %d) Prio: %d\n", Possible[j].from, Possible[j].turn_index, Possible[j].pri);
            else
                fprintf(stderr,
                        "move %d from %d to %d (%d) Prio: %d\n",
                        Possible[j].card_index,
                        Possible[j].from,
                        Possible[j].to,
                        Possible[j].turn_index,
                        Possible[j].pri);
        }
    }
#endif

    /* No moves?  Maybe we won. */

    if (n == 0) {
        /* No more moves - won or lost */
        // print_layout();
        return nullptr;
    }

    /* Prioritize these moves.  Automoves don't get queued, so they
    don't need a priority. */

    if (!a) {
        prioritize(Possible, alln);
    }

    /* Now copy to safe storage and return.  Non-auto moves out get put
    at the end.  Queueing them isn't a good idea because they are still
    good moves, even if they didn't pass the automove test.  So we still
    do the recursive solve() on them, but only after queueing the other
    moves. */

    mp = mp0 = mm_new_array<MOVE>(n);
    if (mp == nullptr) {
        return nullptr;
    }
    *nmoves = n;
    i = 0;
    if (a || numout == 0) {
        for (i = 0; i < alln; ++i) {
            if (Possible[i].card_index != -1) {
                *mp = Possible[i]; /* struct copy */
                mp++;
            }
        }
    } else {
        for (i = numout; i < alln; ++i) {
            if (Possible[i].card_index != -1) {
                *mp = Possible[i]; /* struct copy */
                mp++;
            }
        }
        for (i = 0; i < numout; ++i) {
            if (Possible[i].card_index != -1) {
                *mp = Possible[i]; /* struct copy */
                mp++;
            }
        }
    }

    return mp0;
}

/* Test the current position to see if it's new (or better).  If it is, save
it, along with the pointer to its parent and the move we used to get here. */

int Posbytes;

template<size_t NumberPiles>
void Solver<NumberPiles>::pilesort(void)
{
    /* Make sure all the piles have id numbers. */

    for (size_t w = 0; w < NumberPiles; w++) {
        if (Wpilenum[w] < 0) {
            Wpilenum[w] = get_pilenum(w);
            if (Wpilenum[w] < 0) {
                return;
            }
        }
        // fprintf( stderr, "%d ", Wpilenum[w] );
    }
    // fprintf( stderr, "\n" );
}

#define NBUCKETS 65521 /* the largest 16 bit prime */
#define NPILES 65536 /* a 16 bit code */

typedef struct bucketlist {
    quint8 *pile; /* 0 terminated copy of the pile */
    quint32 hash; /* the pile's hash code */
    int pilenum; /* the unique id for this pile */
    struct bucketlist *next;
} BUCKETLIST;

BUCKETLIST *Bucketlist[NBUCKETS];
int Pilenum; /* the next pile number to be assigned */

BUCKETLIST *Pilebucket[NPILES]; /* reverse lookup for unpack to get the bucket
                   from the pile */

/* Compact position representation.  The position is stored as an
array with the following format:
    pile0# pile1# ... pileN# (N = Nwpiles)
where each pile number is packed into 16 bits (so a pile take 2 bytes).
Positions in this format are unique can be compared with memcmp().  The O
cells are encoded as a cluster number: no two positions with different
cluster numbers can ever be the same, so we store different clusters in
different trees.  */

int Treebytes;

template<size_t NumberPiles>
TREE *Solver<NumberPiles>::pack_position(void)
{
    int j;
    quint8 *p;
    TREE *node;

    /* Allocate space and store the pile numbers.  The tree node
    will get filled in later, by insert_node(). */

    p = mm->new_from_block(Treebytes);
    if (p == nullptr) {
        Status = UnableToDetermineSolvability;
        return nullptr;
    }
    node = (TREE *)p;
    p += sizeof(TREE);

    /* Pack the pile numers j into bytes p.
               j             j
        +--------+----:----+--------+
        |76543210|7654:3210|76543210|
        +--------+----:----+--------+
            p         p         p
    */

    quint16 *p2 = (quint16 *)p;
    for (size_t w = 0; w < NumberPiles; ++w) {
        j = Wpilenum[w];
        if (j < 0) {
            mm->give_back_block(p);
            return nullptr;
        }
        *p2++ = j;
    }

    return node;
}

/* Like strcpy() but return the length of the string. */

static inline int strecpy(unsigned char *d, unsigned char *s)
{
    int i;

    i = 0;
    while ((*d++ = *s++) != '\0') {
        i++;
    }

    return i;
}

/* Unpack a compact position rep.  T cells must be restored from the
array following the POSITION struct. */

template<size_t NumberPiles>
void Solver<NumberPiles>::unpack_position(POSITION *pos)
{
    int i = 0;
    BUCKETLIST *l;

    unpack_cluster(pos->cluster);

    /* Unpack bytes p into pile numbers j.
            p         p         p
        +--------+----:----+--------+
        |76543210|7654:3210|76543210|
        +--------+----:----+--------+
               j             j
    */

    size_t w = 0;
    quint16 *p2 = (quint16 *)((quint8 *)(pos->node) + sizeof(TREE));
    while (w < NumberPiles) {
        i = *p2++;
        Wpilenum[w] = i;
        l = Pilebucket[i];
        i = strecpy(W[w], l->pile);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        Whash[w] = l->hash;
        w++;
    }
}

template<size_t NumberPiles>
void Solver<NumberPiles>::printcard(card_t card, FILE *outfile)
{
    static char Rank[] = " A23456789TJQK";
    static char Suit[] = "DCHS";

    if (RANK(card) == NONE) {
        fprintf(outfile, "   ");
    } else {
        if (DOWN(card))
            fprintf(outfile, "|%c%c ", Rank[RANK(card)], Suit[SUIT(card)]);
        else
            fprintf(outfile, "%c%c ", Rank[RANK(card)], Suit[SUIT(card)]);
    }
}

/* Win.  Print out the move stack. */

template<size_t NumberPiles>
void Solver<NumberPiles>::win(POSITION *pos)
{
    int i, nmoves;
    POSITION *p;
    MOVE **mpp, **mpp0;

    /* Go back up the chain of parents and store the moves
       in reverse order. */

    i = 0;
    for (p = pos; p->parent; p = p->parent) {
        i++;
    }
    nmoves = i;

    // printf("Winning in %d moves.\n", nmoves);

    mpp0 = mm_new_array<MOVE *>(nmoves);
    if (mpp0 == nullptr) {
        Status = UnableToDetermineSolvability;
        return; /* how sad, so close... */
    }
    mpp = mpp0 + nmoves - 1;
    for (p = pos; p->parent; p = p->parent) {
        *mpp-- = &p->move;
    }

    for (i = 0, mpp = mpp0; i < nmoves; ++i, ++mpp)
        m_winMoves.append(**mpp);

    MemoryManager::free_array(mpp0, nmoves);
}

/* Initialize the hash buckets. */

template<size_t NumberPiles>
void Solver<NumberPiles>::init_buckets(void)
{
    int i;

    /* Packed positions need 3 bytes for every 2 piles. */

    i = (NumberPiles) * sizeof(quint16);
    i += (NumberPiles)&0x1;

    mm->Pilebytes = i;

    memset(Bucketlist, 0, sizeof(Bucketlist));
    Pilenum = 0;
    Treebytes = sizeof(TREE) + mm->Pilebytes;

    /* In order to keep the TREE structure aligned, we need to add
    up to 7 bytes on Alpha or 3 bytes on Intel -- but this is still
    better than storing the TREE nodes and keys separately, as that
    requires a pointer.  On Intel for -f Treebytes winds up being
    a multiple of 8 currently anyway so it doesn't matter. */

#define ALIGN_BITS 0x7
    if (Treebytes & ALIGN_BITS) {
        Treebytes |= ALIGN_BITS;
        Treebytes++;
    }
    Posbytes = sizeof(POSITION);
    if (Posbytes & ALIGN_BITS) {
        Posbytes |= ALIGN_BITS;
        Posbytes++;
    }
}

/* For each pile, return a unique identifier.  Although there are a
large number of possible piles, generally fewer than 1000 different
piles appear in any given game.  We'll use the pile's hash to find
a hash bucket that contains a short list of piles, along with their
identifiers. */

template<size_t NumberPiles>
int Solver<NumberPiles>::get_pilenum(int w)
{
    int bucket, pilenum;
    BUCKETLIST *l, *last;

    /* For a given pile, get its unique pile id.  If it doesn't have
    one, add it to the appropriate list and give it one.  First, get
    the hash bucket. */

    bucket = Whash[w] % NBUCKETS;

    /* Look for the pile in this bucket. */

    last = nullptr;
    for (l = Bucketlist[bucket]; l; l = l->next) {
        if (l->hash == Whash[w] && strncmp((char *)l->pile, (char *)W[w], Wlen[w]) == 0) {
            break;
        }
        last = l;
    }

    /* If we didn't find it, make a new one and add it to the list. */

    if (l == nullptr) {
        if (Pilenum >= NPILES) {
            Status = UnableToDetermineSolvability;
            // qCDebug(KPAT_LOG) << "out of piles";
            return -1;
        }
        l = mm_allocate<BUCKETLIST>();
        if (l == nullptr) {
            Status = UnableToDetermineSolvability;
            // qCDebug(KPAT_LOG) << "out of buckets";
            return -1;
        }
        l->pile = mm_new_array<quint8>(Wlen[w] + 1);
        if (l->pile == nullptr) {
            Status = UnableToDetermineSolvability;
            MemoryManager::free_ptr(l);
            // qCDebug(KPAT_LOG) << "out of memory";
            return -1;
        }

        /* Store the new pile along with its hash.  Maintain
        a reverse mapping so we can unpack the piles swiftly. */

        strncpy((char *)l->pile, (char *)W[w], Wlen[w] + 1);
        l->hash = Whash[w];
        l->pilenum = pilenum = Pilenum++;
        l->next = nullptr;
        if (last == nullptr) {
            Bucketlist[bucket] = l;
        } else {
            last->next = l;
        }
        Pilebucket[pilenum] = l;
    }

#if 0
if (w < 4) {
        fprintf( stderr, "get_pile_num %d ", l->pilenum );
        for (int i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fprintf( stderr, "\n" );
}
#endif
    return l->pilenum;
}

template<size_t NumberPiles>
void Solver<NumberPiles>::free_buckets(void)
{
    int i, j;
    BUCKETLIST *l, *n;

    for (i = 0; i < NBUCKETS; i++) {
        l = Bucketlist[i];
        while (l) {
            n = l->next;
            j = strlen((char *)l->pile); /* @@@ use block? */
            MemoryManager::free_array(l->pile, j + 1);
            MemoryManager::free_ptr(l);
            l = n;
        }
    }
}

/* Solve patience games. Prioritized breadth-first search. Simple breadth-
first uses exponential memory. Here the work queue is kept sorted to give
priority to positions with more cards out, so the solution found is not
guaranteed to be the shortest, but it'll be better than with a depth-first
search. */

template<size_t NumberPiles>
void Solver<NumberPiles>::doit()
{
    int i, q;
    POSITION *pos;
    MOVE m;

    /* Init the queues. */

    for (i = 0; i < NQUEUES; ++i) {
        Qhead[i] = nullptr;
    }
    Maxq = 0;

    /* Queue the initial position to get started. */

    hash_layout();
    pilesort();
    m.card_index = -1;
    m.turn_index = -1;
    pos = new_position(nullptr, &m);
    if (pos == nullptr) {
        Status = UnableToDetermineSolvability;
        return;
    }
    queue_position(pos, 0);

    /* Solve it. */

    while ((pos = dequeue_position()) != nullptr) {
        q = solve(pos);
        if (!q) {
            free_position(pos, true);
        }
    }
}

/* Generate all the successors to a position and either queue them or
recursively solve them.  Return whether any of the child nodes, or their
descendents, were queued or not (if not, the position can be freed). */

template<size_t NumberPiles>
bool Solver<NumberPiles>::solve(POSITION *parent)
{
    int i, nmoves, qq;
    MOVE *mp, *mp0;
    POSITION *pos;

    bool q;
    all_moves++;

    /* If we've won already (or failed), we just go through the motions
    but always return false from any position.  This enables the cleanup
    of the move stack and eventual destruction of the position store. */

    if (Status != NoSolutionExists) {
        return false;
    }

    if (m_shouldEnd.load()) {
        Status = SearchAborted;
        return false;
    }

    if (max_positions != -1 && Total_positions > (unsigned long)max_positions) {
        Status = MemoryLimitReached;
        return false;
    }

    /* Generate an array of all the moves we can make. */

    if ((mp0 = get_moves(&nmoves)) == nullptr) {
        if (isWon()) {
            Status = SolutionExists;
            win(parent);
        }
        return false;
    }

    if (parent->depth == 0) {
        Q_ASSERT(m_firstMoves.isEmpty());
        for (int j = 0; j < nmoves; ++j)
            m_firstMoves.append(Possible[j]);
    }

    parent->nchild = nmoves;

    /* Make each move and either solve or queue the result. */

    q = false;
    for (i = 0, mp = mp0; i < nmoves; ++i, ++mp) {
        make_move(mp);

        /* Calculate indices for the new piles. */

        pilesort();

        /* See if this is a new position. */

        if ((pos = new_position(parent, mp)) == nullptr) {
            undo_move(mp);
            parent->nchild--;
            continue;
        }

        /* If this position is in a new cluster, a card went out.
        Don't queue it, just keep going.  A larger cutoff can also
        force a recursive call, which can help speed things up (but
        reduces the quality of solutions).  Otherwise, save it for
        later. */

        if (pos->cluster != parent->cluster || !nmoves) {
            qq = solve(pos);
            undo_move(mp);
            if (!qq) {
                free_position(pos, false);
            }
            q |= (bool)qq;
        } else {
            queue_position(pos, mp->pri);
            undo_move(mp);
            q = true;
        }
    }
    MemoryManager::free_array(mp0, nmoves);

    /* Return true if this position needs to be kept around. */
    return q;
}

/* We can't free the stored piles in the trees, but we can free some of the
POSITION structs.  We have to be careful, though, because there are many
threads running through the game tree starting from the queued positions.
The nchild element keeps track of descendents, and when there are none left
in the parent we can free it too after solve() returns and we get called
recursively (rec == true). */

template<size_t NumberPiles>
void Solver<NumberPiles>::free_position(POSITION *pos, int rec)
{
    /* We don't really free anything here, we just push it onto a
       freelist (using the queue member), so we can use it again later. */

    if (!rec) {
        pos->queue = Freepos;
        Freepos = pos;
        pos->parent->nchild--;
    } else {
        do {
            pos->queue = Freepos;
            Freepos = pos;
            pos = pos->parent;
            if (pos == nullptr) {
                return;
            }
            pos->nchild--;
        } while (pos->nchild == 0);
    }
}

/* Save positions for consideration later.  pri is the priority of the move
that got us here.  The work queue is kept sorted by priority (simply by
having separate queues). */

template<size_t NumberPiles>
void Solver<NumberPiles>::queue_position(POSITION *pos, int pri)
{
    /* In addition to the priority of a move, a position gets an
    additional priority depending on the number of cards out.  We use a
    "queue squashing function" to map nout to priority.  */

    int nout = getOuts();

    static qreal Yparam[] = {0.0032, 0.32, -3.0};
    qreal x = (Yparam[0] * nout + Yparam[1]) * nout + Yparam[2];
    pri += (int)floor(x + .5);

    if (pri < 0) {
        pri = 0;
    } else if (pri >= NQUEUES) {
        pri = NQUEUES - 1;
    }
    if (pri > Maxq) {
        Maxq = pri;
    }

    /* We always dequeue from the head.  Here we either stick the move
    at the head or tail of the queue, depending on whether we're
    pretending it's a stack or a queue. */

    pos->queue = nullptr;
    if (Qhead[pri] == nullptr) {
        Qhead[pri] = pos;
    } else {
        pos->queue = Qhead[pri];
        Qhead[pri] = pos;
    }
}

/* Return the position on the head of the queue, or NULL if there isn't one. */

template<size_t NumberPiles>
POSITION *Solver<NumberPiles>::dequeue_position()
{
    int last;
    POSITION *pos;
    static int qpos = 0;
    static int minpos = 0;

    /* This is a kind of prioritized round robin.  We make sweeps
    through the queues, starting at the highest priority and
    working downwards; each time through the sweeps get longer.
    That way the highest priority queues get serviced the most,
    but we still get lots of low priority action (instead of
    ignoring it completely). */

    last = false;
    do {
        qpos--;
        if (qpos < minpos) {
            if (last) {
                return nullptr;
            }
            qpos = Maxq;
            minpos--;
            if (minpos < 0) {
                minpos = Maxq;
            }
            if (minpos == 0) {
                last = true;
            }
        }
    } while (Qhead[qpos] == nullptr);

    pos = Qhead[qpos];
    Qhead[qpos] = pos->queue;

    /* Decrease Maxq if that queue emptied. */

    while (Qhead[qpos] == nullptr && qpos == Maxq && Maxq > 0) {
        Maxq--;
        qpos--;
        if (qpos < minpos) {
            minpos = qpos;
        }
    }

    /* Unpack the position into the work arrays. */

    unpack_position(pos);

    return pos;
}

template<size_t NumberPiles>
Solver<NumberPiles>::Solver()
    : mm(new MemoryManager)
{
    /* Initialize work arrays. */
    for (auto &workspace : W) {
        workspace = new card_t[84];
        memset(workspace, 0, sizeof(card_t) * 84);
    }
}

template<size_t NumberPiles>
Solver<NumberPiles>::~Solver()
{
    for (size_t i = 0; i < NumberPiles; ++i) {
        delete[] W[i];
    }
}

template<size_t NumberPiles>
void Solver<NumberPiles>::init()
{
    m_shouldEnd.store(false);
    init_buckets();
    mm->init_clusters();

    m_winMoves.clear();
    m_firstMoves.clear();

    /* Reset stats. */

    Status = NoSolutionExists;
    Total_positions = 0;
    Total_generated = 0;
    depth_sum = 0;
}

template<size_t NumberPiles>
void Solver<NumberPiles>::free()
{
    free_buckets();
    mm->free_clusters();
    mm->free_blocks();
    Freepos = nullptr;
}

template<size_t NumberPiles>
SolverInterface::ExitStatus Solver<NumberPiles>::patsolve(int _max_positions)
{
    max_positions = _max_positions;

    /* Initialize the suitable() macro variables. */
    init();

    /* Go to it. */
    doit();

    if (Status == SearchAborted) // thread quit
    {
        m_firstMoves.clear();
        m_winMoves.clear();
    }
#if 0
    printf("%ld positions generated (%f).\n", Total_generated, depth_sum / Total_positions);
    printf("%ld unique positions.\n", Total_positions);
    printf("Mem_remain = %ld\n", ( long int )mm->Mem_remain);
#endif
    free();
    return Status;
}

template<size_t NumberPiles>
void Solver<NumberPiles>::stopExecution()
{
    m_shouldEnd.store(true);
}

template<size_t NumberPiles>
QList<MOVE> Solver<NumberPiles>::winMoves() const
{
    return m_winMoves;
}

template<size_t NumberPiles>
QList<MOVE> Solver<NumberPiles>::firstMoves() const
{
    return m_firstMoves;
}

template<size_t NumberPiles>
void Solver<NumberPiles>::print_layout()
{
}

template<size_t NumberPiles>
int Solver<NumberPiles>::translateSuit(int s)
{
    int suit = s * 0x10;
    if (suit == PS_DIAMOND)
        return PS_CLUB;
    else if (suit == PS_CLUB)
        return PS_DIAMOND;
    return suit;
}

template<size_t NumberPiles>
int Solver<NumberPiles>::translate_pile(const KCardPile *pile, card_t *w, int size)
{
    Q_UNUSED(size);
    Q_ASSERT(pile->count() <= size);

    for (int i = 0; i < pile->count(); ++i) {
        KCard *c = pile->at(i);
        *w = +translateSuit(c->suit()) + c->rank();
        if (!c->isFaceUp())
            *w += 1 << 7;
        w++;
    }
    return pile->count();
}

/* Insert key into the tree unless it's already there.  Return true if
it was new. */

template<size_t NumberPiles>
MemoryManager::inscode Solver<NumberPiles>::insert(unsigned int *cluster, int d, TREE **node)
{
    /* Get the cluster number from the Out cell contents. */

    unsigned int k = getClusterNumber();
    *cluster = k;

    /* Get the tree for this cluster. */

    TREELIST *tl = mm->cluster_tree(k);
    if (tl == nullptr) {
        return MemoryManager::ERR;
    }

    /* Create a compact position representation. */

    TREE *newtree = pack_position();
    if (newtree == nullptr) {
        return MemoryManager::ERR;
    }
    Total_generated++;

    MemoryManager::inscode i2 = mm->insert_node(newtree, d, &tl->tree, node);

    if (i2 != MemoryManager::NEW) {
        mm->give_back_block((quint8 *)newtree);
    }

    return i2;
}

template<size_t NumberPiles>
POSITION *Solver<NumberPiles>::new_position(POSITION *parent, MOVE *m)
{
    unsigned int depth, cluster;
    quint8 *p;
    POSITION *pos;
    TREE *node;

    /* Search the list of stored positions.  If this position is found,
    then ignore it and return (unless this position is better). */

    if (parent == nullptr) {
        depth = 0;
    } else {
        depth = parent->depth + 1;
    }
    MemoryManager::inscode i = insert(&cluster, depth, &node);
    if (i == MemoryManager::NEW) {
        Total_positions++;
        depth_sum += depth;
    } else
        return nullptr;

    /* A new or better position.  insert() already stashed it in the
    tree, we just have to wrap a POSITION struct around it, and link it
    into the move stack.  Store the temp cells after the POSITION. */

    if (Freepos) {
        p = (quint8 *)Freepos;
        Freepos = Freepos->queue;
    } else {
        p = mm->new_from_block(Posbytes);
        if (p == nullptr) {
            Status = UnableToDetermineSolvability;
            return nullptr;
        }
    }

    pos = (POSITION *)p;
    pos->queue = nullptr;
    pos->parent = parent;
    pos->node = node;
    pos->move = *m; /* struct copy */
    pos->cluster = cluster;
    pos->depth = depth;
    pos->nchild = 0;
#if 0
        QString dummy;
        quint16 *t = ( quint16* )( ( char* )node + sizeof( TREE ) );
        for ( int i = 0; i < NumberPiles; ++i )
        {
            QString s = "      " + QString( "%1" ).arg( ( int )t[i] );
            dummy += s.right( 5 );
        }
        if ( Total_positions % 1000 == 1000 )
            print_layout();
        //qCDebug(KPAT_LOG) << "new" << dummy;
#endif
    p += sizeof(POSITION);
    return pos;
}

/* Hash the whole layout.  This is called once, at the start. */

template<size_t NumberPiles>
void Solver<NumberPiles>::hash_layout(void)
{
    for (size_t w = 0; w < NumberPiles; w++) {
        hashpile(w);
    }
}

template<size_t NumberPiles>
void Solver<NumberPiles>::prioritize(MOVE *, int)
{
}

constexpr auto Nwpiles = 8;
constexpr auto Ntpiles = 4;

template class Solver<9>;
template class Solver<10>;
template class Solver<7 * 3 + 1>;
template class Solver<8 + 1 + 8>;
template class Solver<6>;
template class Solver<34>;
template class Solver<15>;
template class Solver<7>;
template class Solver<13>;
template class Solver<20>;
template class Solver<Nwpiles + Ntpiles>;
