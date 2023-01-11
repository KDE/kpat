#ifndef SolverInterface_H
#define SolverInterface_H

// own
#include "../hint.h"
// freecell-solver
#include "freecell-solver/fcs_user.h"
// Qt
#include <QList>

/* A card is represented as ( down << 6 ) + (suit << 4) + rank. */

typedef quint8 card_t;

/* Represent a move. */

enum PileType { O_Type = 1, W_Type };

class MOVE
{
public:
    int card_index; /* the card we're moving (0 is the top)*/
    quint8 from; /* from pile number */
    quint8 to; /* to pile number */
    PileType totype;
    signed char pri; /* move priority (low priority == low value) */
    int turn_index; /* turn the card index */
    bool is_fcs;
    fcs_move_t fcs; /* A Freecell Solver move. */

    bool operator<(const MOVE &m) const
    {
        return pri < m.pri;
    }

    MOVE()
        : card_index(0)
        , from(0)
        , to(0)
        , totype(O_Type)
        , pri(0)
        , turn_index(0)
        , is_fcs(false)
        , fcs()
    {
    }
};

class SolverInterface
{
public:
    enum ExitStatus { MemoryLimitReached = -3, SearchAborted = -2, UnableToDetermineSolvability = -1, NoSolutionExists = 0, SolutionExists = 1 };

    virtual ~SolverInterface(){};
    virtual ExitStatus patsolve(int max_positions = -1) = 0;
    virtual void translate_layout() = 0;
    virtual MoveHint translateMove(const MOVE &m) = 0;

    virtual void stopExecution() = 0;
    virtual QList<MOVE> firstMoves() const = 0;
    virtual QList<MOVE> winMoves() const = 0;
};

extern long all_moves;

#endif
