
#ifndef GYPSY_H
#define GYPSY_H

#include "dealer.h"

class KAction;
class Pile;
class Deck;
class KMainWindow;

class Gypsy : public Dealer {
    Q_OBJECT

public:
    Gypsy( KMainWindow* parent=0, const char* name=0);

public slots:
    void slotClicked(Card *) { dealRow(true); }
    void deal();
    virtual void restart();

private: // functions
    void dealRow(bool faceup);
    bool CanPutTarget( const Pile* c1, const CardList& c2) const;
    bool CanPutStore( const Pile* c1, const CardList& c2) const;
    bool checkRemove( int index, const Pile* c1, const Card *c) const;
    bool checkAdd( int index, const Pile* c1, const CardList& cl) const;

private:
    Pile* store[8];
    Pile* target[8];
    Deck *deck;
};

#endif
