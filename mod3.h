/*---------------------------------------------------------------------------

  mod3.cpp  implements a patience card game

     Copyright 1997 Rodolfo Borges <barrett@labma.ufrj.br>

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

 (I don't know a name for this one, if you do, please tell me.)

---------------------------------------------------------------------------*/

#ifndef MOD3_H
#define MOD3_H

#include "dealer.h"


class Mod3 : public DealerScene
{
    friend class Mod3Solver;

    Q_OBJECT

public:
    Mod3( );

    void deal();

    virtual void restart();

public slots:
    virtual Card *newCards();

protected:
    virtual void setGameState(const QString & );

private: // functions
    virtual bool checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    void         dealRow(int row);

private:
    Pile  *stack[4][8];
    Pile  *aces;
};

#endif

//-------------------------------------------------------------------------//
