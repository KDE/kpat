/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright (C) 1997  Rodolfo Borges

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

#ifndef _FREECELL_H_
#define _FREECELL_H_

#include "dealer.h"

class Freecell : public Dealer
{
    Q_OBJECT

public:
    Freecell( KMainWindow* parent=0, const char* name=0);

    virtual bool isGameWon() const;

public slots:
    void deal();
    virtual void restart();

private: // functions
    static bool CanPutStack(const Pile *c1, const CardList& c2);
    static bool CanPutStore(const Pile *c1, const CardList& c2);
    static bool CanPutFreeCell(const Pile *c1, const CardList& c2);
    static bool CanRemove(const Pile *c1, const Card *c);

public:
    int CountFreeCells();

private:
    Pile *stack[8];
    Pile *freecell[4];
    Pile *store[4];
    Deck *deck;
};

#endif


//-------------------------------------------------------------------------//
