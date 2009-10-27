/*
 * Copyright (C) 2001-2009 Stephan Kulow <coolo@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of 
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "golf.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "speeds.h"
#include "patsolve/golfsolver.h"

#include <KDebug>
#include <KLocale>


HorRightPile::HorRightPile( int _index, const QString & objectName )
    : Pile(_index, objectName)
{
}

QSizeF HorRightPile::cardOffset( const Card *) const
{
    return QSizeF(0.12, 0);
}

//-------------------------------------------------------------------------//

Golf::Golf( )
    : DealerScene( )
{
    const qreal dist_x = 1.11;
    const qreal smallNeg = -1e-6;

    deck = new CardDeck();

    talon = new Pile(0, "talon");
    talon->setPilePos(0, smallNeg);
    connect(talon, SIGNAL(clicked(Card*)), SLOT(newCards()));
    addPile(talon);

    waste=new HorRightPile(8, "waste");
    waste->setPilePos(1.1, smallNeg);
    waste->setTarget(true);
    waste->setCheckIndex( 0 );
    waste->setAddFlags( Pile::addSpread);
    waste->setReservedSpace( QSizeF( 4.0, 1.0 ) );
    addPile(waste);

    for( int r = 0; r < 7; r++ ) {
        stack[r]=new Pile(1+r, QString("stack%1").arg(r));
        stack[r]->setPilePos(r*dist_x,0);
        stack[r]->setAddFlags( Pile::addSpread | Pile::disallow);
        stack[r]->setCheckIndex( 1 );
        stack[r]->setReservedSpace( QSizeF( 1.0, 3.0 ) );
        addPile(stack[r]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new GolfSolver( this ) );
}

//-------------------------------------------------------------------------//

bool Golf::checkAdd( int checkIndex, const Pile *c1, const CardList& cl) const
{
    if (checkIndex == 1)
        return false;

    Card *c2 = cl.first();

    kDebug(11111)<<"check add "<< c1->objectName()<<" " << c2->objectName() <<" ";

    if ((c2->rank() != (c1->top()->rank()+1)) && (c2->rank() != (c1->top()->rank()-1)))
        return false;

    return true;
}

bool Golf::checkRemove( int checkIndex, const Pile *, const Card *c2) const
{
    if (checkIndex == 0)
        return false;
    return (c2 ==  c2->source()->top());
}

//-------------------------------------------------------------------------//

void Golf::restart()
{
    deck->returnAllCards();
    deck->shuffle( gameNumber() );
    deal();
    emit newCardsPossible( true );
}

//-------------------------------------------------------------------------//

void Golf::deal()
{
    for(int i=0;i<5;i++)
        for(int r=0;r<7;r++)
            addCardForDeal( stack[r], deck->takeCard(), true, stack[6]->pos() );

    deck->takeAllCards(talon);

    startDealAnimation();

    Card *c = talon->top();
    waste->add(c, false);
    c->stopAnimation();
    qreal x = c->x();
    qreal y = c->y();
    c->setPos( talon->pos() );
    c->flipTo(x, y, DURATION_FLIP );
}

Card *Golf::newCards()
{
    if (talon->isEmpty())
         return 0;

    if ( waste->top() && waste->top()->animated() )
        return waste->top();

    unmarkAll();

    Card *c = talon->top();
    waste->add( c, false );
    c->stopAnimation();
    qreal x = c->x();
    qreal y = c->y();
    c->setPos( talon->pos() );
    c->flipTo(x, y, DURATION_FLIP );

    takeState();
    considerGameStarted();
    if ( talon->isEmpty() )
        emit newCardsPossible( false );

    return c;
}

bool Golf::cardClicked(Card *c)
{
    if (c->source()->checkIndex() !=1) {
        return DealerScene::cardClicked(c);
    }

    if (c != c->source()->top())
        return false;

    Pile*p=findTarget(c);
    if (p)
    {
        CardList empty;
        empty.append(c);
        c->source()->moveCards(empty, p);
        return true;
    }
    return false;
}

void Golf::setGameState( const QString & )
{
    emit newCardsPossible( !talon->isEmpty() );
}



static class GolfDealerInfo : public DealerInfo
{
public:
    GolfDealerInfo()
      : DealerInfo(I18N_NOOP("Golf"), 12)
    {}

    virtual DealerScene *createGame() const
    {
        return new Golf();
    }
} golfDealerInfo;


#include "golf.moc"
