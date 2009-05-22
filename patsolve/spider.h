#ifndef SPIDERSOLVER_H
#define SPIDERSOLVER_H

#include "patsolve.h"
class Spider;


class SpiderSolver : public Solver
{
public:
    SpiderSolver(const Spider *dealer);
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

    int O[8];
    const Spider *deal;
};

#endif // SPIDERSOLVER_H
