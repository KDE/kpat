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

#include "simon.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "patsolve/simonsolver.h"

#include <KDebug>
#include <KLocale>


Simon::Simon( )
    : DealerScene( )
{
    setupDeck(new CardDeck());

    const qreal dist_x = 1.11;

    for (int i=0; i<4; i++) {
        target[i] = new PatPile(i+1, QString( "target%1" ).arg( i ));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setFoundation(true);
        target[i]->setPilePos((i+3)*dist_x, 0);
        target[i]->setSpread(0, 0);
        addPile(target[i]);
    }

    for (int i=0; i<10; i++) {
        store[i] = new PatPile(5+i, QString( "store%1" ).arg( i ));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setPilePos(dist_x*i, 1.2);
        store[i]->setReservedSpace( QSizeF( 1.0, 3.5 ) );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new SimonSolver( this ) );
    //setNeededFutureMoves( 1 ); // could be some nonsense moves
}

void Simon::restart() {
    deck()->returnAllCards();
    deck()->shuffle( gameNumber() );
    deal();
}

void Simon::deal() {
    for ( int piles = 9; piles >= 3; piles-- )
    {
        for (int j = 0; j < piles; j++)
            addCardForDeal(store[j], deck()->takeCard(), true, QPointF(0,-deck()->cardHeight()));
    }
    for ( int j = 0; j < 10; j++ )
        addCardForDeal(store[j], deck()->takeCard(), true, QPointF(0,-deck()->cardHeight()));

    Q_ASSERT(!deck()->hasUndealtCards());

    startDealAnimation();
}

bool Simon::checkPrefering(const PatPile * pile, const QList<Card*> & oldCards, const QList<Card*> & newCards) const
{
    return pile->pileRole() == PatPile::Tableau
           && !oldCards.isEmpty()
           && oldCards.last()->suit() == newCards.first()->suit();
}

bool Simon::checkAdd(const PatPile * pile, const QList<Card*> & oldCards, const QList<Card*> & newCards) const
{
    if (pile->pileRole() == PatPile::Tableau)
    {
        return oldCards.isEmpty()
               || oldCards.last()->rank() == newCards.first()->rank() + 1;
    }
    else
    {
        return oldCards.isEmpty()
               && newCards.first()->rank() == Card::King
               && newCards.last()->rank() == Card::Ace;
    }
}

bool Simon::checkRemove(const PatPile * pile, const QList<Card*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau
           && isSameSuitDescending(cards);
}



static class SimonDealerInfo : public DealerInfo
{
public:
    SimonDealerInfo()
      : DealerInfo(I18N_NOOP("Simple Simon"), SimpleSimonId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Simon();
    }
} simonDealerInfo;

#include "simon.moc"
