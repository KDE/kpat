/*
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
*/
#ifndef CLOCK_H
#define CLOCK_H

#include "dealer.h"

class Clock : public Dealer {
    Q_OBJECT

public:
    Clock( KMainWindow* parent=0 );
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool startAutoDrop() { return false; }

public slots:
    void deal();
    virtual void restart();

private:
    Pile* store[8];
    Pile* target[12];
    Deck *deck;
};

#endif
