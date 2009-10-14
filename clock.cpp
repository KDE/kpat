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

#include "clock.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "patsolve/clocksolver.h"

#include <KLocale>


Clock::Clock( )
    : DealerScene( )
{
    CardDeck::self()->setDeckType();

    const qreal dist_x = 1.11;
    const qreal ys[12] = {   0./96,  15./96,  52./96, 158./96, 264./96, 301./96, 316./96, 301./96, 264./96, 158./96,  52./96,  15./96};
    const qreal xs[12] = { 200./72, 280./72, 360./72, 400./72, 360./72, 280./72, 200./72, 120./72, 40./72, 0./72, 40./72, 120./72};

    for (int i=0; i<12; i++) {
        target[i] = new Pile(i+1, QString("target%1").arg(i));
        target[i]->setPilePos(4 * dist_x + 0.4 + xs[i], 0.2 + ys[i]);
        target[i]->setCheckIndex(1);
        target[i]->setTarget(true);
        target[i]->setRemoveFlags(Pile::disallow);
        addPile(target[i]);
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(14+i, QString("store%1").arg(i));
        store[i]->setPilePos(dist_x*(i%4), 2.5 * (i/4));
        store[i]->setAddFlags(Pile::addSpread);
        store[i]->setCheckIndex(0);
        store[i]->setReservedSpace( QSizeF( 1.0, 1.8 ) );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new ClockSolver( this ) );
}

void Clock::restart()
{
    CardDeck::self()->returnAllCards();
    CardDeck::self()->shuffle( gameNumber() );
    deal();
}

bool Clock::checkAdd( int ci, const Pile *c1, const CardList& c2) const
{
    Card *newone = c2.first();
    if (ci == 0) {
        if (c1->isEmpty())
            return true;

        return (newone->rank() == c1->top()->rank() - 1);
    } else {
        if (c1->top()->suit() != newone->suit())
            return false;
        if (c1->top()->rank() == Card::King)
            return (newone->rank() == Card::Ace);
        return (newone->rank() == c1->top()->rank() + 1);
    }
}

void Clock::deal() {
    static const Card::Suit suits[12] = { Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs,
                                          Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs,
                                          Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs };
    static const Card::Rank ranks[12] = { Card::Nine, Card::Ten, Card::Jack, Card::Queen,
                                          Card::King, Card::Two, Card::Three, Card::Four,
                                          Card::Five, Card::Six, Card::Seven, Card::Eight };

    int j = 0;
    while (CardDeck::self()->hasUndealtCards()) {
        Card *c = CardDeck::self()->takeCard();
        for (int i = 0; i < 12; i++)
            if (c->rank() == ranks[i] && c->suit() == suits[i]) {
                target[i]->add(c, false);
                c = 0;
                break;
            }
        if (c)
            store[j++]->add(c, false);
        if (j == 8)
            j = 0;
    }
}

static class LocalDealerInfo11 : public DealerInfo
{
public:
    LocalDealerInfo11() : DealerInfo(I18N_NOOP("Grandfather's Clock"), 11) {}
    virtual DealerScene *createGame() const { return new Clock(); }
} gfi11;

#include "clock.moc"
