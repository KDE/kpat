#ifndef _GOLF_H_
#define _GOLF_H_

#include "dealer.h"

class HorRightPile : public Pile
{
    Q_OBJECT

public:
    HorRightPile( int _index, Dealer* parent = 0);
    virtual QSize cardOffset( bool _spread, bool _facedown, const Card *before) const;
};

class Golf : public Dealer
{
    Q_OBJECT

public:
    Golf( KMainWindow* parent=0, const char* name=0);
    void deal();
    virtual void restart();
    virtual bool isGameLost() const;

protected slots:
    void deckClicked(Card *);

protected:
    virtual bool startAutoDrop() { return false; }
    virtual Card *demoNewCards();
    virtual bool cardClicked(Card *c);

private: // functions
    virtual bool checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c2) const;

private:
    Pile* stack[7];
    HorRightPile* waste;
    Deck* deck;
};

#endif

//-------------------------------------------------------------------------//
