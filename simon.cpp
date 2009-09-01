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

#include "deck.h"
#include "patsolve/simon.h"

#include <KDebug>
#include <KLocale>


Simon::Simon( )
    : DealerScene( )
{
    Deck::createDeck(this, 1, 4);
    Deck::deck()->hide();

    const qreal dist_x = 1.11;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos((i+3)*dist_x, 0);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFlags(Pile::several);
        target[i]->setCheckIndex(0);
        target[i]->setTarget(true);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    for (int i=0; i<10; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->setPilePos(dist_x*i, 1.2);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several);
        store[i]->setReservedSpace( QSizeF( 1.0, 3.5 ) );
        store[i]->setCheckIndex(1);
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new SimonSolver( this ) );
    //setNeededFutureMoves( 1 ); // could be some nonsense moves
}

void Simon::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
}

void Simon::deal() {
    for ( int piles = 9; piles >= 3; piles-- )
    {
        for (int j = 0; j < piles; j++)
        {
            Card *c = Deck::deck()->nextCard();
            store[j]->add(c, false);
        }
    }
    for ( int j = 0; j < 10; j++ )
    {
        Card *c = Deck::deck()->nextCard();
        store[j]->add(c, false);
    }

    Q_ASSERT(Deck::deck()->isEmpty());
}

bool Simon::checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const
{
    if (checkIndex == 1) {
        if (c1->isEmpty())
            return false;

        return (c1->top()->suit() == c2.first()->suit());
    } else return false; // it's just important to keep this unique
}

bool Simon::checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const
{
    if (checkIndex == 1) {
        if (c1->isEmpty())
            return true;

        return (c1->top()->rank() == c2.first()->rank() + 1);
    } else {
        if (!c1->isEmpty())
            return false;
        return (c2.first()->rank() == Card::King && c2.last()->rank() == Card::Ace);
    }
}

bool Simon::checkRemove(int checkIndex, const Pile *p, const Card *c) const
{
    if (checkIndex != 1)
        return false;

    // ok if just one card
    if (c == p->top())
        return true;

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    int index = p->indexOf(c) + 1;
    const Card *before = c;
    while (true)
    {
        c = p->at(index++);

        if (!((c->rank() == (before->rank()-1))
              && (c->suit() == before->suit())))
        {
            return false;
        }
        if (c == p->top())
            return true;
        before = c;
    }

    return true;
}

static class LocalDealerInfo9 : public DealerInfo
{
public:
    LocalDealerInfo9() : DealerInfo(I18N_NOOP("Simple Simon"), 9) {}
    virtual DealerScene *createGame() const { return new Simon(); }
} gfi9;

#include "simon.moc"
