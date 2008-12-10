#ifndef FREECELLSOLVER_H
#define FREECELLSOLVER_H


#include "patsolve.h"

class Freecell;

class FreecellSolver : public Solver
{
public:
    FreecellSolver(const Freecell *dealer);
    int good_automove(int o, int r);
    virtual int get_possible_moves(int *a, int *numout);
    virtual bool isWon();
    virtual void make_move(MOVE *m);
    virtual void undo_move(MOVE *m);
    virtual void prioritize(MOVE *mp0, int n);
    virtual int getOuts();
    virtual int getClusterNumber();
    virtual void translate_layout();
    virtual void unpack_cluster( int k );
    virtual MoveHint *translateMove( const MOVE &m);

    virtual void print_layout();

    int Nwpiles; /* the numbers we're actually using */
    int Ntpiles;

/* Names of the cards.  The ordering is defined in pat.h. */

    card_t O[4]; /* output piles store only the rank or NONE */
    card_t Osuit[4];

    const Freecell *deal;

    static int Xparam[];

};

#endif // FREECELLSOLVER_H
