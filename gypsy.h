
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
	virtual bool isGameLost() const;

public slots:
    void slotClicked(Card *) { dealRow(true); }
    void deal();
    virtual void restart();

private: // functions
    void dealRow(bool faceup);
    virtual Card *demoNewCards();

private:
    Pile* store[8];
    Pile* target[8];
    Deck *deck;
};

#endif
