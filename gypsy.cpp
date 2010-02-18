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

#include "gypsy.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "patsolve/gypsysolver.h"

#include "libkcardgame/shuffle.h"

#include <KLocale>

Gypsy::Gypsy( )
    : DealerScene(  )
{
    const qreal dist_x = 1.11;
    const qreal dist_y = 1.11;

    setupDeck(new KStandardCardDeck(2));

    talon = new PatPile(0, "talon");
    talon->setPileRole(PatPile::Stock);
    talon->setPilePos(8.5 * dist_x + 0.4, 4 * dist_y);
    connect(talon, SIGNAL(clicked(KCard*)), SLOT(newCards()));
    addPile(talon);

    for (int i=0; i<8; i++) {
        target[i] = new PatPile(i+1, QString("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setFoundation(true);
        target[i]->setPilePos(dist_x*(8+(i/4)) + 0.4, (i%4)*dist_y);
        addPile(target[i]);
    }

    for (int i=0; i<8; i++) {
        store[i] = new PatPile(9+i, QString("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setPilePos(dist_x*i,0);
        store[i]->setAutoTurnTop(true);
        store[i]->setReservedSpace( QSizeF( 1.0, 4 * dist_y + 1.0 ) );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new GypsySolver( this ) );
}

void Gypsy::restart() {
    foreach( KCardPile * p, piles() )
        p->clear();
    deal();
    emit newCardsPossible(true);
}

bool Gypsy::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return checkAddAlternateColorDescending(oldCards, newCards);
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    case PatPile::Stock:
    default:
        return false;
    }
}

bool Gypsy::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return isAlternateColorDescending(cards);
    case PatPile::Foundation:
        return cards.first() == pile->top();
    case PatPile::Stock:
    default:
        return false;
    }
}


void Gypsy::dealRow(bool faceup)
{
    for (int round=0; round < 8; round++)
        store[round]->animatedAdd(talon->top(), faceup);
}

void Gypsy::deal()
{
    QList<KCard*> cards = shuffled( deck()->cards(), gameNumber() );

    for (int round=0; round < 8; round++)
        addCardForDeal(store[round], cards.takeLast(), false, store[round]->pos() + QPointF(-2*deck()->cardWidth(),-1.1*deck()->cardHeight()));

    for (int round=0; round < 8; round++)
        addCardForDeal(store[round], cards.takeLast(), true, store[round]->pos() + QPointF(-3*deck()->cardWidth(),-1.6*deck()->cardHeight()));

    for (int round=0; round < 8; round++)
        addCardForDeal(store[round], cards.takeLast(), true, store[round]->pos() + QPointF(-4*deck()->cardWidth(),-2.1*deck()->cardHeight()));

    while ( !cards.isEmpty() )
    {
        KCard * c = cards.takeFirst();
        c->setPos( talon->pos() );
        c->setFaceUp( false );
        talon->add( c );
    }

    startDealAnimation();
}

KCard *Gypsy::newCards()
{
    if (talon->isEmpty())
        return 0;

    if (deck()->hasAnimatedCards())
        for (int i = 0; i < 8; ++i)
            if (store[i]->top())
                return store[i]->top();

    clearHighlightedItems();

    dealRow(true);

    onGameStateAlteredByUser();
    if (talon->isEmpty())
        emit newCardsPossible(false);

    return store[0]->top();
}

void Gypsy::setGameState(const QString &)
{
    emit newCardsPossible(!talon->isEmpty());
}



static class GypsyDealerInfo : public DealerInfo
{
public:
    GypsyDealerInfo()
      : DealerInfo(I18N_NOOP("Gypsy"), GypsyId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Gypsy();
    }
} gypsyDealerInfo;


#include "gypsy.moc"
