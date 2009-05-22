#ifndef GYPSYSOLVER_H
#define GYPSYSOLVER_H

class Gypsy;
#include "patsolve.h"


class GypsySolver : public Solver
{
public:
    GypsySolver(const Gypsy *dealer);
    int good_automove(int o, int r);
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
    int params[6];
};

#endif // GYPSYSOLVER_H
