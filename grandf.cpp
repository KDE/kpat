/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#include "grandf.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "speeds.h"
#include "patsolve/grandfsolver.h"

#include <KDebug>
#include <KLocale>


Grandf::Grandf( )
    : DealerScene(  )
{
    deck = new CardDeck();

    const qreal distx = 1.4;
    const qreal targetOffset = 1.5 * distx;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, QString("target%1").arg(i));
        target[i]->setPilePos(targetOffset+i*distx, 0);
        target[i]->setType(Pile::KlondikeTarget);
        addPile(target[i]);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, QString("store%1").arg(i));
        store[i]->setPilePos(distx*i, 1.2);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setCheckIndex(1);
        store[i]->setReservedSpace( QSizeF( 1.0, 5.0 ) );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Redeal);
    setSolver( new GrandfSolver( this ) );
}

void Grandf::restart() {
    deck->returnAllCards();
    deck->shuffle( gameNumber() );
    deal();
    numberOfDeals = 1;
    emit newCardsPossible( true );
}

Card *Grandf::newCards()
{
//     if ( waiting() || demoActive() || isGameWon()  )
//         return;

    if (numberOfDeals >= 3)
        return 0;

    unmarkAll();
    collect();
    deal();
    numberOfDeals++;

    Card *t = store[3]->top();
    Q_ASSERT(t);
    if (t)
    {
       t->turn(false);
       t->flipTo( t->pos().x(), t->pos().y(), DURATION_FLIP );
    }

    takeState();
    considerGameStarted();
    if (numberOfDeals == 3)
        emit newCardsPossible(false);

    return t;
}

void Grandf::deal() {
    QPointF initPos( 1.4 * 3 * deck->cardWidth(), 1.2 * deck->cardHeight() );

    int start = 0;
    int stop = 7-1;
    int dir = 1;

    for (int round=0; round < 7; round++)
    {
        int i = start;
        do
        {
            Card *next = deck->takeCard();
            if (next)
                store[i]->add(next, i != start, initPos);
            i += dir;
        } while ( i != stop + dir);
        int t = start;
        start = stop;
        stop = t+dir;
        dir = -dir;
    }

    int i = 0;
    Card *next = deck->takeCard();
    while (next)
    {
        store[i+1]->add(next, false, initPos);
        next = deck->takeCard();
        i = (i+1)%6;
    }

    for (int round=0; round < 7; round++)
    {
        Card *c = store[round]->top();
        if (c)
            c->turn(true);
    }
}

/*****************************

  Does the collecting step of the game

  NOTE: this is not quite correct -- the piles should be turned
  facedown (ie partially reversed) during collection.

******************************/
void Grandf::collect() {
    unmarkAll();

    for (int pos = 6; pos >= 0; pos--) {
        foreach (Card *c, store[pos]->cards())
            deck->returnCard(c);
    }
}

bool Grandf::checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const {
    kDebug(11111) << checkIndex << c1->isEmpty();
    Q_ASSERT (checkIndex == 1);
    if (c1->isEmpty())
        return c2.first()->rank() == Card::King;
    else
        return (c2.first()->rank() == c1->top()->rank() - 1)
               && c2.first()->suit() == c1->top()->suit();
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
      : DealerInfo(I18N_NOOP("Grandfather"), 1)
    {}

    virtual DealerScene *createGame() const
    {
        return new Grandf();
    }
} grandfDealerInfo;


#include "grandf.moc"
