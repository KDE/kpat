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

#include "gypsy.h"

#include "deck.h"
#include "patsolve/gypsysolver.h"

#include <KLocale>

Gypsy::Gypsy( )
    : DealerScene(  )
{
    const qreal dist_x = 1.11;
    const qreal dist_y = 1.11;

    Deck::self()->setScene(this);
    Deck::self()->setDeckProperties(2, 4);
    Deck::self()->setPilePos(8.5 * dist_x + 0.4, 4 * dist_y);
    connect(Deck::self(), SIGNAL(clicked(Card*)), SLOT(newCards()));
    addPile(Deck::self());

    for (int i=0; i<8; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos(dist_x*(8+(i/4)) + 0.4, (i%4)*dist_y);
        target[i]->setAddType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(9+i, this);
        store[i]->setPilePos(dist_x*i,0);
        store[i]->setAddType(Pile::GypsyStore);
        store[i]->setRemoveType(Pile::FreecellStore);
        store[i]->setReservedSpace( QSizeF( 1.0, 4 * dist_y + 1.0 ) );
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new GypsySolver( this ) );
}

void Gypsy::restart() {
    Deck::self()->collectAndShuffle();
    deal();
    emit newCardsPossible(true);
}

void Gypsy::dealRow(bool faceup) {
    for (int round=0; round < 8; round++)
        store[round]->add(Deck::self()->nextCard(), !faceup);
}

void Gypsy::deal() {
    dealRow(false);
    dealRow(false);
    dealRow(true);
    takeState();
}

Card *Gypsy::newCards()
{
    if (Deck::self()->isEmpty())
        return 0;

    unmarkAll();
    dealRow(true);
    takeState();
    considerGameStarted();
    if (Deck::self()->isEmpty())
        emit newCardsPossible(false);

    return store[0]->top();
}

void Gypsy::setGameState(const QString &)
{
    emit newCardsPossible(!Deck::self()->isEmpty());
}

static class LocalDealerInfo7 : public DealerInfo
{
public:
    LocalDealerInfo7() : DealerInfo(I18N_NOOP("Gypsy"), 7) {}
    virtual DealerScene *createGame() const { return new Gypsy(); }
} gyfdi;

#include "gypsy.moc"
