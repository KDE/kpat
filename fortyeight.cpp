/*
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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
#include "pileutils.h"
#include "speeds.h"
#include "patsolve/fortyeightsolver.h"

#include <KDebug>
#include <KLocale>


Fortyeight::Fortyeight( )
    : DealerScene()
{
    const qreal dist_x = 1.11;
    const qreal smallNeg = -1e-6;

    setDeck(new CardDeck(2));

    talon = new PatPile(0, "talon");
    talon->setPileRole(PatPile::Stock);
    talon->setPilePos(smallNeg, smallNeg);
    talon->setZValue(20);
    talon->setSpread(0, 0);
    connect(talon, SIGNAL(pressed(Card*)), SLOT(newCards()));
    connect(talon, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    addPile(talon);

    pile = new PatPile(20, "pile");
    pile->setPileRole(PatPile::Waste);
    pile->setPilePos(-dist_x, smallNeg);
    pile->setSpread(-0.21, 0);
    pile->setReservedSpace( QSizeF( -(1 + 6 * dist_x), 1 ) );
    addPile(pile);

    for (int i = 0; i < 8; i++) {
        target[i] = new PatPile(9 + i, QString( "target%1" ).arg( i ));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setTarget(true);
        target[i]->setPilePos(dist_x*i, 0);
        target[i]->setSpread(0, 0);
        addPile(target[i]);

        stack[i] = new PatPile(1 + i, QString( "stack%1" ).arg( i ));
        stack[i]->setPileRole(PatPile::Tableau);
        stack[i]->setPilePos(dist_x*i, 1.1 );
        stack[i]->setAutoTurnTop(true);
        stack[i]->setSpread(0, 0.25);
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
    deck()->returnAllCards();
    deck()->shuffle( gameNumber() );
    deal();
    emit newCardsPossible( true );
}

void Fortyeight::deckClicked( Card * )
{
    if ( pile->top() && pile->top()->animated() )
        return;
    if ( talon->isEmpty())
        newCards();
}

Card *Fortyeight::newCards()
{
    if (talon->isEmpty() && lastdeal)
        return 0;

    if (pile->top() && deck()->hasAnimatedCards())
        return pile->top();

    if (talon->isEmpty())
    {
        lastdeal = true;
        while (pile->cardsLeft() > 1)
        {
            Card *c = pile->top();
            talon->add(c);
            c->flipTo( talon->pos(), DURATION_FLIP );
        }
    }
    else
    {
        talon->top()->flipToPile( pile, DURATION_FLIP );
    }

    onGameStateAlteredByUser();
    if ( talon->isEmpty() && lastdeal )
        emit newCardsPossible( false );

    return pile->top();
}

bool Fortyeight::checkAdd(const PatPile * pile, const CardList & oldCards, const CardList & newCards) const
{
    switch ( pile->pileRole() )
    {
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    case PatPile::Tableau:
        return newCards.size() == 1
               && ( oldCards.isEmpty()
                    || ( oldCards.last()->suit() == newCards.first()->suit()
                         && oldCards.last()->rank() == newCards.first()->rank() + 1 ) );
    case PatPile::Stock:
    case PatPile::Waste:
    default:
        return false;
    }
}

bool Fortyeight::checkRemove( const PatPile * pile, const CardList & cards) const
{
    switch ( pile->pileRole() )
    {
    case PatPile::Waste:
    case PatPile::Tableau:
        return cards.first() == pile->top();
    case PatPile::Foundation:
    case PatPile::Stock:
    default:
        return false;
    }
}

void Fortyeight::deal()
{
    for (int r = 0; r < 4; r++)
    {
        for (int column = 0; column < 8; column++)
        {
            QPointF initPos = stack[column]->pos() - QPointF( 0, 2 * deck()->cardHeight() );
            addCardForDeal( stack[column], deck()->takeCard(), true, initPos );
        }
    }

    deck()->takeAllCards(talon);

    startDealAnimation();

    talon->top()->flipToPile( pile, DURATION_FLIP );
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
