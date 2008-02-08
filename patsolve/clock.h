
#ifndef _CLOCKSOLVER_H
#define _CLOCKSOLVER_H

#include "patsolve.h"

class Clock;

class ClockSolver : public Solver
{
public:
    ClockSolver(const Clock *dealer);
    virtual int get_possible_moves(int *a, int *numout);
    virtual bool isWon();
    virtual void make_move(MOVE *m);
    virtual void undo_move(MOVE *m);
    virtual int getOuts();
    virtual int getClusterNumber();
    virtual void translate_layout();
    virtual void unpack_cluster( int k );
    virtual MoveHint *translateMove( const MOVE &m);

    virtual void print_layout();

/* Names of the cards.  The ordering is defined in pat.h. */

    //card_t O[12]; /* output piles store only the rank or NONE */

    const Clock *deal;
    int m_draw;
};
#endif

