/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright (C) 1997 Rodolfo Borges
               (C) 2000 Stephan Kulow

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

class FreecellPile : public Pile
{
public:
    FreecellPile(int _index, Dealer* parent = 0) : Pile(_index, parent) {}
    virtual void moveCards(CardList &c, Pile *to);
};

class FreecellBase : public Dealer
{
    Q_OBJECT

public:
    FreecellBase( int decks, int stores, int freecells, int es_filling, bool unlimited_move,
                  KMainWindow* parent=0, const char* name=0);
    void moveCards(CardList &c, FreecellPile *from, Pile *to);
    QString solverFormat() const;
    virtual ~FreecellBase();

public slots:
    virtual void deal() = 0;
    virtual void restart();
    void waitForMoving(Card *c);
    void startMoving();
    void resumeSolution();
    virtual void demo();

protected:
    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;

    bool CanPutStore(const Pile *c1, const CardList& c2) const;
    bool CanRemove(const Pile *c1, const Card *c) const;

    void countFreeCells(int &free_cells, int &free_stores) const;

    virtual void getHints();
    void movePileToPile(CardList &c, Pile *to, PileList fss, PileList &fcs,
                        uint start, uint count, int debug_level);

    Pile *pileForName(QString line) const;
    void findSolution();

    virtual MoveHint *chooseHint();
    MoveHint *translateMove(void *m);
    void freeSolution();

    virtual void stopDemo();
    virtual void newDemoMove(Card *m);
    virtual bool cardDblClicked(Card *c);

protected:
    QValueList<FreecellPile*> store;
    PileList freecell;
    PileList target;
    Deck *deck;
private:
    HintList moves;
    HintList oldmoves;
    Card *waitfor;
    void *solver_instance;
    int es_filling;
    int solver_ret;
    bool unlimited_move;
    bool noLongerNeeded(const Card &);
};

#endif

//-------------------------------------------------------------------------//
