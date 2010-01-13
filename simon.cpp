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

#include "simon.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "patsolve/simonsolver.h"

#include <KDebug>
#include <KLocale>


Simon::Simon( )
    : DealerScene( )
{
    setDeck(new CardDeck());

    const qreal dist_x = 1.11;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, QString( "target%1" ).arg( i ));
        target[i]->setPilePos((i+3)*dist_x, 0);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFlags(Pile::several);
        target[i]->setCheckIndex(0);
        target[i]->setTarget(true);
        target[i]->setSpread(0, 0);
        addPile(target[i]);
    }

    for (int i=0; i<10; i++) {
        store[i] = new Pile(5+i, QString( "store%1" ).arg( i ));
        store[i]->setPilePos(dist_x*i, 1.2);
        store[i]->setAddFlags(Pile::several);
        store[i]->setRemoveFlags(Pile::several);
        store[i]->setReservedSpace( QSizeF( 1.0, 3.5 ) );
        store[i]->setCheckIndex(1);
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

bool Simon::checkPrefering(const Pile *c1, const CardList& c2) const
{
    if (c1->checkIndex() == 1) {
        if (c1->isEmpty())
            return false;

        return (c1->top()->suit() == c2.first()->suit());
    } else return false; // it's just important to keep this unique
}

bool Simon::checkAdd(const Pile * pile, const CardList & cards) const
{
    if (pile->checkIndex() == 1) {
        if (pile->isEmpty())
            return true;

        return (pile->top()->rank() == cards.first()->rank() + 1);
    } else {
        if (!pile->isEmpty())
            return false;
        return (cards.first()->rank() == Card::King && cards.last()->rank() == Card::Ace);
    }
}

bool Simon::checkRemove(const Pile * pile, const CardList & cards) const
{
    if (pile->checkIndex() != 1)
        return false;

    Card * c = cards.first();

    // ok if just one card
    if (c == pile->top())
        return true;

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    int index = pile->indexOf(c) + 1;
    const Card *before = c;
    while (true)
    {
        c = pile->at(index++);

        if (!((c->rank() == (before->rank()-1))
              && (c->suit() == before->suit())))
        {
            return false;
        }
        if (c == pile->top())
            return true;
        before = c;
    }

    return true;
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
