
#ifndef _SPIDERSOLVER_H
#define _SPIDERSOLVER_H

#include "patsolve.h"

class Gypsy;

class GypsySolver : public Solver
{
public:
    GypsySolver(const Gypsy *dealer);
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

    const Gypsy *deal;

    int deck, outs;
};
#endif

