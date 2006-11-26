#include "patsolve.h"

class Golf;

class GolfSolver : public Solver
{
public:
    GolfSolver(const Golf *dealer);
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

    const Golf *deal;
};
