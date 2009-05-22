#ifndef CLOCKSOLVER_H
#define CLOCKSOLVER_H

class Clock;
#include "patsolve.h"


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

    const Clock *deal;
};

#endif // CLOCKSOLVER_H
