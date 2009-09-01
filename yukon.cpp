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

#include "yukon.h"

#include "deck.h"
#include "patsolve/yukon.h"

#include <KDebug>
#include <KLocale>


Yukon::Yukon( )
    : DealerScene( )
{
    const qreal dist_x = 1.11;
    const qreal dist_y = 1.11;

    Deck::createDeck(this);
    Deck::deck()->hide();

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos(0.11+7*dist_x, dist_y *i);
        target[i]->setType(Pile::KlondikeTarget);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->setPilePos(dist_x*i, 0);
        store[i]->setAddType(Pile::KlondikeStore);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setReservedSpace( QSizeF( 1.0, 3 * dist_y + 1.0 ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new YukonSolver( this ) );
    setNeededFutureMoves( 10 ); // it's a bit hard to judge as there are so many nonsense moves
}

void Yukon::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
}

void Yukon::deal() {
    for (int round = 0; round < 11; round++)
    {
        for (int j = 0; j < 7; j++)
        {
            bool doit = false;
            switch (j) {
            case 0:
                doit = (round == 0);
                break;
            default:
                doit = (round < j + 5);
            }
            if (doit)
                store[j]->add(Deck::deck()->nextCard(), round < j && j != 0);
        }
    }
}

static class LocalDealerInfo10 : public DealerInfo
{
public:
    LocalDealerInfo10() : DealerInfo(I18N_NOOP("Yukon"), 10) {}
    virtual DealerScene *createGame() const { return new Yukon(); }
} gfi10;

#include "yukon.moc"
