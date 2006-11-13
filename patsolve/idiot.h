#include "patsolve.h"

class Idiot;

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

    virtual bool print_layout();

    bool canMoveAway( int pile ) const;

    const Idiot *deal;
};
