
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
    static bool CanTarget( const Pile* c1, const CardList& c2);
    static bool CanStore( const Pile* c1, const CardList& c2);
    static bool CanStoreRemove( const Pile* c1, const Card *c);

private:
    Pile* store[8];
    Pile* target[8];
    Deck *deck;
};

#endif
