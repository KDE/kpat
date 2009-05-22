#ifndef KLONDIKESOLVER_H
#define KLONDIKESOLVER_H

class Klondike;
#include "patsolve.h"


class KlondikeSolver : public Solver
{
public:
    KlondikeSolver(const Klondike *dealer, int draw);
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

    card_t O[4]; /* output piles store only the rank or NONE */
    card_t Osuit[4];

    const Klondike *deal;
    int m_draw;
};

#endif // KLONDIKESOLVER_H
