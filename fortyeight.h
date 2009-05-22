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

#ifndef FORTYEIGHT_H
#define FORTYEIGHT_H

#include "dealer.h"


class HorLeftPile : public Pile
{
    Q_OBJECT

public:
    explicit HorLeftPile( int _index, DealerScene* parent = 0);
    virtual QSizeF cardOffset(const Card *before) const;
    virtual void relayoutCards();
};

class Fortyeight : public DealerScene
{
    friend class FortyeightSolver;

    Q_OBJECT

public:
    Fortyeight( );
    virtual void restart();
    virtual bool checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual QString getGameState();
    virtual void setGameState( const QString & stream );

public slots:
    virtual Card *newCards();
    void deckClicked(Card *c);

private:
    void deal();

    Pile *stack[8];
    Pile *target[8];
    HorLeftPile *pile;
    bool lastdeal;
};

#endif


//-------------------------------------------------------------------------//
