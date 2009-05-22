#ifndef MOD3SOLVER_H
#define MOD3SOLVER_H

class Mod3;
#include "patsolve.h"


class Mod3Solver : public Solver
{
public:
    Mod3Solver(const Mod3 *dealer);
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

    const Mod3 *deal;
    int aces;
    int deck;
};

#endif // MOD3SOLVER_H
