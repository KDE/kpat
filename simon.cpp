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

#include "Shuffle"

#include <KLocale>


void Simon::initialize()
{
    static_cast<KStandardCardDeck*>( deck() )->setDeckContents();

    const qreal dist_x = 1.11;

    for (int i=0; i<4; i++) {
        target[i] = new PatPile(i+1, QString( "target%1" ).arg( i ));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setPilePos((i+3)*dist_x, 0);
        target[i]->setSpread(0, 0);
        addPile(target[i]);
    }

    for (int i=0; i<10; i++) {
        store[i] = new PatPile(5+i, QString( "store%1" ).arg( i ));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setPilePos(dist_x*i, 1.2);
        store[i]->setReservedSpace( 0, 0, 1, 3.5 );
        store[i]->setZValue( 0.01 * i );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new SimonSolver( this ) );
    //setNeededFutureMoves( 1 ); // could be some nonsense moves
}

void Simon::restart()
{
    QList<KCard*> cards = shuffled( deck()->cards(), gameNumber() );

    QPointF initPos( 0, -deck()->cardHeight() );

    for ( int piles = 9; piles >= 3; --piles )
        for ( int j = 0; j < piles; ++j )
            addCardForDeal( store[j], cards.takeLast(), true, initPos );

    for ( int j = 0; j < 10; ++j )
        addCardForDeal( store[j], cards.takeLast(), true, initPos );

    Q_ASSERT( cards.isEmpty() );

    startDealAnimation();
}

bool Simon::checkPrefering(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    return pile->pileRole() == PatPile::Tableau
           && !oldCards.isEmpty()
           && getSuit( oldCards.last() ) == getSuit( newCards.first() );
}

bool Simon::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    if (pile->pileRole() == PatPile::Tableau)
    {
        return oldCards.isEmpty()
               || getRank( oldCards.last() ) == getRank( newCards.first() ) + 1;
    }
    else
    {
        return oldCards.isEmpty()
               && getRank( newCards.first() ) == KStandardCardDeck::King
               && getRank( newCards.last() ) == KStandardCardDeck::Ace;
    }
}

bool Simon::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
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
