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

#ifndef GOLF_H
#define GOLF_H

#include "dealer.h"

class HorRightPile : public Pile
{
    Q_OBJECT

public:
    explicit HorRightPile( int _index, DealerScene* parent = 0);
    virtual QSizeF cardOffset( const Card *) const;
};

class Golf : public DealerScene
{
    friend class GolfSolver;

    Q_OBJECT

public:
    Golf( );
    void deal();
    virtual void restart();

public slots:
    virtual Card *newCards();

protected:
    virtual bool startAutoDrop() { return false; }
    virtual bool cardClicked(Card *c);
    virtual void setGameState( const QString & );

private: // functions
    virtual bool checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c2) const;

private:
    Pile* stack[7];
    HorRightPile* waste;
};

#endif

//-------------------------------------------------------------------------//
