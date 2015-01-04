/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "grandf.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "speeds.h"
#include "patsolve/grandfsolver.h"

#include <KLocalizedString>


Grandf::Grandf( const DealerInfo * di )
  : DealerScene( di )
{
}


void Grandf::initialize()
{
    setDeckContents();

    const qreal distx = 1.4;
    const qreal targetOffset = 1.5 * distx;

    for ( int i = 0; i < 4; ++i )
    {
        target[i] = new PatPile( this, i + 1, QString("target%1").arg(i) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(targetOffset+i*distx, 0);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint( KCardPile::NeverFocus );
        target[i]->setKeyboardDropHint( KCardPile::ForceFocusTop );
    }

    for ( int i = 0; i < 7; ++i )
    {
        store[i] = new PatPile( this, 5 + i, QString("store%1").arg(i) );
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(distx*i, 1.2);
        store[i]->setAutoTurnTop(true);
        store[i]->setBottomPadding( 4 );
        store[i]->setHeightPolicy( KCardPile::GrowDown );
        store[i]->setKeyboardSelectHint( KCardPile::FreeFocus );
        store[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Redeal);
    setSolver( new GrandfSolver( this ) );
}

void Grandf::restart( const QList<KCard*> & cards )
{
    deal( cards );
    numberOfDeals = 1;
    emit newCardsPossible( true );
}

bool Grandf::newCards()
{
    if ( numberOfDeals >= 3 )
        return false;

    // NOTE: This is not quite correct. The piles should be turned face down
    //       (i.e. partially reversed) during collection.
    QList<KCard*> collectedCards;
    for ( int pos = 6; pos >= 0; --pos )
    {
        collectedCards << store[pos]->cards();
        store[pos]->clear();
    }
    deal( collectedCards );
    takeState();

    numberOfDeals++;

    if (numberOfDeals == 3)
        emit newCardsPossible(false);

    return true;
}

void Grandf::deal( const QList<KCard*> & cardsToDeal )
{
    setKeyboardModeActive( false );

    QList<KCard*> cards = cardsToDeal;

    QPointF initPos( 1.4 * 3 * deck()->cardWidth(), 1.2 * deck()->cardHeight() );

    int start = 0;
    int stop = 7-1;
    int dir = 1;

    for (int round=0; round < 7; round++)
    {
        int i = start;
        do
        {
            if (!cards.isEmpty())
                addCardForDeal( store[i], cards.takeLast(), (i == start), initPos );
            i += dir;
        } while ( i != stop + dir);
        int t = start;
        start = stop;
        stop = t+dir;
        dir = -dir;
    }

    int i = 0;
    while (!cards.isEmpty())
    {
        addCardForDeal( store[i+1], cards.takeLast(), true, initPos );
        i = (i+1)%6;
    }

    for (int round=0; round < 7; round++)
    {
        KCard *c = store[round]->topCard();
        if (c)
            c->setFaceUp(true);
    }

    startDealAnimation();
}

bool Grandf::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        if (oldCards.isEmpty())
            return newCards.first()->rank() == KCardDeck::King;
        else
            return newCards.first()->rank() == oldCards.last()->rank() - 1
                   && newCards.first()->suit() == oldCards.last()->suit();
    case PatPile::Foundation:
    default:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    }
}

bool Grandf::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau && cards.first()->isFaceUp();
}

QString Grandf::getGameState() const
{
    return QString::number(numberOfDeals);
}

void Grandf::setGameState( const QString & state )
{
    numberOfDeals = state.toInt();
    emit newCardsPossible(numberOfDeals < 3);
}



static class GrandfDealerInfo : public DealerInfo
{
public:
    GrandfDealerInfo()
      : DealerInfo(I18N_NOOP("Grandfather"), GrandfatherId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Grandf( this );
    }
} grandfDealerInfo;



