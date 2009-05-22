#ifndef IDIOTSOLVER_H
#define IDIOTSOLVER_H

class Idiot;
#include "patsolve.h"


class IdiotSolver : public Solver
{
public:
    IdiotSolver(const Idiot *dealer);
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

    bool canMoveAway( int pile ) const;

    const Idiot *deal;
};

#endif // IDIOTSOLVER_H
