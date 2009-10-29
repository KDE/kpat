/*
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
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

#include "fortyeight.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "speeds.h"
#include "patsolve/fortyeightsolver.h"

#include <KDebug>
#include <KLocale>


HorLeftPile::HorLeftPile( int _index, const QString & objectName )
    : Pile(_index, objectName)
{
}

QSizeF HorLeftPile::cardOffset( const Card *) const
{
    return QSizeF(-0.21, 0);
}

void HorLeftPile::relayoutCards()
{
    // this is just too heavy to animate, so we stop it
    Pile::relayoutCards();
    foreach ( Card *c, m_cards )
    {
        if ( c != top() )
            c->stopAnimation();
    }
}


Fortyeight::Fortyeight( )
    : DealerScene()
{
    const qreal dist_x = 1.11;
    const qreal smallNeg = -1e-6;

    deck = new CardDeck(2);

    talon = new Pile(0, "talon");
    talon->setPilePos(smallNeg, smallNeg);
    talon->setZValue(20);
    connect(talon, SIGNAL(pressed(Card*)), SLOT(newCards()));
    connect(talon, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    addPile(talon);

    pile = new HorLeftPile(20, "pile");
    pile->setAddFlags(Pile::addSpread | Pile::disallow);
    pile->setPilePos(-dist_x, smallNeg);
    pile->setReservedSpace( QSizeF( -(1 + 6 * dist_x), 1 ) );
    addPile(pile);

    for (int i = 0; i < 8; i++) {
        target[i] = new Pile(9 + i, QString( "target%1" ).arg( i ));
        target[i]->setPilePos(dist_x*i, 0);
        target[i]->setType(Pile::KlondikeTarget);
        addPile(target[i]);

        stack[i] = new Pile(1 + i, QString( "stack%1" ).arg( i ));
        stack[i]->setPilePos(dist_x*i, 1.1 );
        stack[i]->setAddFlags(Pile::addSpread);
        stack[i]->setRemoveFlags(Pile::autoTurnTop);
        stack[i]->setCheckIndex(1);
        stack[i]->setSpread(stack[i]->spread() * 3 / 4);
        stack[i]->setReservedSpace( QSizeF( 1.0, 4.0 ) );
        addPile(stack[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new FortyeightSolver( this ) );
}

//-------------------------------------------------------------------------//

void Fortyeight::restart()
{
    lastdeal = false;
    deck->returnAllCards();
    deck->shuffle( gameNumber() );
    deal();
    emit newCardsPossible( true );
}

void Fortyeight::deckClicked( Card * )
{
    //kDebug(11111) << "deckClicked" << c->name() << " " << pile->top()->name() << " " << pile->top()->animated();
    if ( pile->top() && pile->top()->animated() )
        return;
    if ( talon->isEmpty())
        newCards();
}

Card *Fortyeight::newCards()
{
    if (talon->isEmpty() && lastdeal)
        return 0;

    if (pile->top() && pile->top()->animated())
        return pile->top();

    if (talon->isEmpty())
    {
        lastdeal = true;
        while (!pile->isEmpty())
        {
            Card *c = pile->at(pile->cardsLeft()-1);
            c->stopAnimation();
            talon->animatedAdd(c, false);
        }
    }

    Card *c = talon->top();
    pile->animatedAdd(c, false);
    c->stopAnimation();
    QPointF destPos = c->realPos();
    c->setPos( talon->pos() );
    c->flipTo( destPos, DURATION_FLIP );

    takeState();
    considerGameStarted();
    if ( talon->isEmpty() && lastdeal )
        emit newCardsPossible( false );

    return c;
}

bool Fortyeight::checkAdd(int, const Pile *c1, const CardList &c2) const
{
    if (c1->isEmpty())
        return true;

    // ok if in sequence, same suit
    return (c1->top()->suit() == c2.first()->suit())
       && (c1->top()->rank() == (c2.first()->rank()+1));
}

void Fortyeight::deal()
{
    for (int r = 0; r < 4; r++)
    {
        for (int column = 0; column < 8; column++)
        {
            QPointF initPos = stack[column]->pos() - QPointF( 0, 2 * deck->cardHeight() );
            addCardForDeal( stack[column], deck->takeCard(), true, initPos );
        }
    }

    deck->takeAllCards(talon);

    startDealAnimation();

    Card *c = talon->top();
    pile->animatedAdd(c, false);
    c->stopAnimation();
    QPointF destPos = c->realPos();
    c->setPos( talon->pos() );
    c->flipTo( destPos, DURATION_FLIP );
}

QString Fortyeight::getGameState()
{
    return QString::number(lastdeal);
}

void Fortyeight::setGameState( const QString &s )
{
    lastdeal = s.toInt();
    emit newCardsPossible( !lastdeal || !talon->isEmpty() );
}



static class FortyEightDealerInfo : public DealerInfo
{
public:
    FortyEightDealerInfo()
      : DealerInfo(I18N_NOOP("Forty & Eight"), FortyAndEightId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Fortyeight();
    }
} fortyEightDealerInfo;


#include "fortyeight.moc"
