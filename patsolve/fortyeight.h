
#ifndef _FORTYEIGHT_H
#define _FORTYEIGHT_H


#include "patsolve.h"

class Fortyeight;

class FortyeightSolver : public Solver
{
public:
    FortyeightSolver(const Fortyeight *dealer);
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
    bool checkMove( int from, int to, MOVE *mp );

    virtual void print_layout();

    const Fortyeight *deal;
    bool lastdeal;
    int freestores;
};
#endif

