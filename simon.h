#ifndef SIMON_H
#define SIMON_H

#include "dealer.h"

class Simon : public Dealer {
    Q_OBJECT

public:
    Simon( KMainWindow* parent=0, const char* name=0);

public slots:
    void deal();
    virtual void restart();
	virtual bool isGameLost() const;


protected:
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;

private:
    Pile* store[10];
    Pile* target[4];
    Deck *deck;
};

#endif
