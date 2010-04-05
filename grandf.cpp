/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#include "grandf.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "speeds.h"
#include "patsolve/grandfsolver.h"

#include "libkcardgame/shuffle.h"

#include <KLocale>


void Grandf::initialize()
{
    static_cast<KStandardCardDeck*>( deck() )->setDeckContents();

    const qreal distx = 1.4;
    const qreal targetOffset = 1.5 * distx;

    for (int i=0; i<4; i++) {
        target[i] = new PatPile(i+1, QString("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setFoundation(true);
        target[i]->setPilePos(targetOffset+i*distx, 0);
        target[i]->setSpread(0, 0);
        addPile(target[i]);
    }

    for (int i=0; i<7; i++) {
        store[i] = new PatPile(5+i, QString("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setPilePos(distx*i, 1.2);
        store[i]->setAutoTurnTop(true);
        store[i]->setReservedSpace( QSizeF( 1.0, 5.0 ) );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Redeal);
    setSolver( new GrandfSolver( this ) );
}

void Grandf::restart()
{
    deal( shuffled( deck()->cards(), gameNumber() ) );
    numberOfDeals = 1;
    emit newCardsPossible( true );
}

KCard *Grandf::newCards()
{
    if (numberOfDeals >= 3)
        return 0;

    if (deck()->hasAnimatedCards())
        for (int i = 0; i < 7; ++i)
            if (store[i]->top())
                return store[i]->top();

    QList<KCard*> collectedCards;
    for ( int pos = 6; pos >= 0; --pos )
    {
        foreach ( KCard * c, store[pos]->cards() )
        {
            store[pos]->remove(c);
            collectedCards << c;
        }
    }
    deal( collectedCards );

    numberOfDeals++;

    if (numberOfDeals == 3)
        emit newCardsPossible(false);

    return store[0]->top();
}

void Grandf::deal( const QList<KCard*> & cardsToDeal )
{
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
        KCard *c = store[round]->top();
        if (c)
            c->setFaceUp(true);
    }

    startDealAnimation();
}

/*****************************

  Does the collecting step of the game

  NOTE: this is not quite correct -- the piles should be turned
  facedown (ie partially reversed) during collection.

******************************/
void Grandf::collect() {
    clearHighlightedItems();


}

bool Grandf::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        if (oldCards.isEmpty())
            return getRank( newCards.first() ) == KStandardCardDeck::King;
        else
            return getRank( newCards.first() ) == getRank( oldCards.last() ) - 1
                   && getSuit( newCards.first() ) == getSuit( oldCards.last() );
    case PatPile::Foundation:
    default:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    }
}

bool Grandf::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau && cards.first()->isFaceUp();
}

QString Grandf::getGameState()
{
    return QString::number(numberOfDeals);
}

void Grandf::setGameState( const QString &s)
{
    numberOfDeals = s.toInt();
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
        return new Grandf();
    }
} grandfDealerInfo;


#include "grandf.moc"
