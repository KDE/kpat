/***********************-*-C++-*-********

  napoleon.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete

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

****************************************/


#ifndef P_NAPOLEON
#define P_NAPOLEON

#include "dealer.h"

class Napoleon : public Dealer {
    Q_OBJECT
public:
    Napoleon (KMainWindow* parent=0, const char* name=0);

    virtual void restart();
    virtual void getHints();
    virtual Card *demoNewCards();
    virtual bool startAutoDrop() { return false; }
    virtual bool isGameLost() const;

public slots:
    void deal1(Card *c);

private:
    void deal();

    bool CanPutTarget( const Pile *c1, const CardList& c2) const;
    bool CanPutCentre( const Pile* c1, const CardList& c2) const;

    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;

    Pile* pile;
    Pile* target[4];
    Pile* centre;
    Pile* store[4];
    Deck* deck;
};

#endif
