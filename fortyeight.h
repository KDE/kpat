#ifndef _FORTY_EIGHT_H
#define _FORTY_EIGHT_H

#include "dealer.h"

class HorLeftPile : public Pile
{
    Q_OBJECT

public:
    HorLeftPile( int _index, Dealer* parent = 0);
    virtual QSize cardOffset( bool _spread, bool _facedown, const Card *before) const;
    virtual void initSizes();
};

class Fortyeight : public Dealer
{
    Q_OBJECT

public:
    Fortyeight( KMainWindow* parent=0, const char* name=0);
    virtual bool isGameLost() const;

public slots:
    void deal();
    virtual void restart();
    void deckClicked(Card *c);

protected:
    virtual bool checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual Card *demoNewCards();
    virtual QString getGameState() const;
    virtual void setGameState( const QString & stream );

private:
    Pile *stack[8];
    Pile *target[8];
    HorLeftPile *pile;
    Deck *deck;
    bool lastdeal;
};

#endif


//-------------------------------------------------------------------------//
