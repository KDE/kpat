/*---------------------------------------------------------------------------

  spider.cpp  implements a patience card game

     Copyright (C) 2003  Josh Metzler

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

#ifndef _SPIDER_H_
#define _SPIDER_H_

#include "dealer.h"

class SpiderPile : public Pile
{
public:
    SpiderPile(int _index, Dealer* parent = 0) : Pile(_index, parent) {}
    virtual void moveCards(CardList &c, Pile *to);
    CardList getRun();
};

class Spider : public Dealer
{
    Q_OBJECT

public:
    Spider(int suits, KMainWindow *parent=0, const char *name=0);
    void deal();
    void dealRow();
    void checkPileDeck(Pile *to);
    virtual void restart();
    virtual bool isGameLost() const;

public slots:
    void deckClicked(Card *c);

protected:
    virtual bool checkRemove(int /*checkIndex*/, const Pile *p, const Card *c) const;
    virtual bool checkAdd(int /*checkIndex*/, const Pile *c1, const CardList &c2) const;
    virtual QString getGameState() const;
    virtual void setGameState(const QString &stream);
    virtual void getHints();
    virtual MoveHint *chooseHint();
    virtual Card *demoNewCards();

private:
    CardList getRun(Card *c) const;

    SpiderPile *stack[10];
    Pile *legs[8];
    int m_leg;
    Pile *redeals[5];
    int m_redeal;
    Deck *deck;
};

#endif

//-------------------------------------------------------------------------//
