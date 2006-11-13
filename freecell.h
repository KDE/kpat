/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright 1997 Rodolfo Borges <barrett@labma.ufrj.br>
     Copyright 2000 Stephan Kulow <coolo@kde.org>

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

---------------------------------------------------------------------------*/

#ifndef _FREECELL_H_
#define _FREECELL_H_

#include "dealer.h"
//Added by qt3to4:
#include <QList>

class FreecellPile : public Pile
{
public:
    explicit FreecellPile(int _index, DealerScene* parent = 0) : Pile(_index, parent) {}
    virtual void moveCards(CardList &c, Pile *to);
};

class Freecell : public DealerScene
{
    friend class FreecellSolver;

    Q_OBJECT

public:
    Freecell();
    void moveCards(CardList &c, FreecellPile *from, Pile *to);
    virtual ~Freecell();

public slots:
    virtual void restart();
    void waitForMoving(Card *c);
    void startMoving();

protected:
    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;

    bool CanPutStore(const Pile *c1, const CardList& c2) const;
    bool CanRemove(const Pile *c1, const Card *c) const;

    void countFreeCells(int &free_cells, int &free_stores) const;

    void movePileToPile(CardList &c, Pile *to, PileList fss, PileList &fcs,
                        int start, int count, int debug_level);

    Pile *pileForName(QString line) const;

    MoveHint *translateMove(void *m);

    virtual void newDemoMove(Card *m);
    virtual bool cardDblClicked(Card *c);
    virtual void deal();
    virtual void getHints();

protected:
    FreecellPile* store[8];
    Pile* freecell[4];
    Pile* target[4];

private:
    HintList moves;
    Card *waitfor;
    bool noLongerNeeded(const Card &);
};

#endif

//-------------------------------------------------------------------------//
