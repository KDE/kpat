/***********************-*-C++-*-********

  computation.h  implements a patience card game

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
// This one was discussed on the newsgroup rec.games.abstract
//


****************************************/

#ifndef P_COMPUTATION_H
#define P_COMPUTATION_H

#include "dealer.h"

class Computation : public Dealer {
    Q_OBJECT

public:
    Computation( KMainWindow *parent = 0, const char *name=0 );

    virtual void restart();

private:
    Card *getCardByValue( char v );
    void deal();

    bool checkStore(const Pile* c1, const CardList& c2) const;
    virtual bool checkAdd( int index, const Pile* c1, const CardList& c2) const;

    Deck *deck;
    Pile *pile;

    Pile *play[4];
    Pile *target[4];
};

#endif
