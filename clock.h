#ifndef CLOCK_H
#define CLOCK_H

#include "dealer.h"

class Clock : public Dealer {
    Q_OBJECT

public:
    Clock( KMainWindow* parent=0, const char* name=0);
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
