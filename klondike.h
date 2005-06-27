/***********************-*-C++-*-********

  klondike.cpp  implements a patience card game

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


//
// 7 positions, alternating red and black
//


****************************************/

#ifndef P_KLONDIKE
#define P_KLONDIKE

#include "dealer.h"

class KlondikePile;

class Klondike : public Dealer {
    Q_OBJECT

public:
    Klondike( bool easy, KMainWindow* parent=0, const char* name=0);

public:
    virtual void restart();
    virtual bool startAutoDrop();

    void redeal(); // put pile back into deck
    void deal();
    void deal3(); // move up to 3 cards from deck to pile

    virtual bool cardClicked(Card *);
    virtual void pileClicked(Pile *c);
    virtual void getHints();

    virtual Card *demoNewCards();

    bool tryToDrop(Card *t);

    virtual bool isGameLost() const;

private:
    bool EasyRules;
    bool redealt;

    Pile* play[7];
    Pile* target[4];

    KlondikePile *pile;
    Deck* deck;
    Card::Rank target_tops[4];
    bool noLongerNeeded(Card::Rank r, Card::Suit s );
};

#endif
