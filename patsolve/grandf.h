
#ifndef _KLONDIKESOLVER_H
#define _KLONDIKESOLVER_H

#include "patsolve.h"

class Grandf;

class GrandfSolver : public Solver
{
public:
    GrandfSolver(const Grandf *dealer);
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

    card_t Osuit[4];

    const Grandf *deal;
    int m_redeal;
    int offs;
};
#endif

