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
#ifndef _GOLF_H_
#define _GOLF_H_

#include "dealer.h"

class HorRightPile : public Pile
{
    Q_OBJECT

public:
    explicit HorRightPile( int _index, DealerScene* parent = 0);
    virtual QSizeF cardOffset( bool _spread, bool _facedown, const Card *before) const;
};

class Golf : public DealerScene
{
    Q_OBJECT

public:
    Golf( );
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
};

#endif

//-------------------------------------------------------------------------//
