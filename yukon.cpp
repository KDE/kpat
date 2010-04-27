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

#include "yukon.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "patsolve/yukonsolver.h"

#include "Shuffle"

#include <KLocale>


void Yukon::initialize()
{
    const qreal dist_x = 1.11;
    const qreal dist_y = 1.11;

    static_cast<KStandardCardDeck*>( deck() )->setDeckContents();

    for (int i=0; i<4; i++) {
        target[i] = new PatPile(i+1, QString("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setFoundation(true);
        target[i]->setPilePos(0.11+7*dist_x, dist_y *i);
        addPile(target[i]);
    }

    for (int i=0; i<7; i++) {
        store[i] = new PatPile(5+i, QString("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setPilePos(dist_x*i, 0);
        store[i]->setAutoTurnTop(true);
        store[i]->setReservedSpace( 0, 0, 1, 3 * dist_y + 1.0 );
        addPile(store[i]);
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new YukonSolver( this ) );
    setNeededFutureMoves( 10 ); // it's a bit hard to judge as there are so many nonsense moves
}

bool Yukon::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    if (pile->pileRole() == PatPile::Tableau)
    {
        if (oldCards.isEmpty())
            return getRank( newCards.first() ) == KStandardCardDeck::King;
        else
            return getRank( newCards.first() ) == getRank( oldCards.last() ) - 1
                   && getIsRed( newCards.first() ) != getIsRed( oldCards.last() );
    }
    else
    {
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    }
}

bool Yukon::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau && cards.first()->isFaceUp();
}

void Yukon::restart()
{
    QList<KCard*> cards = shuffled( deck()->cards(), gameNumber() );

    for ( int round = 0; round < 11; ++round )
    {
        for ( int j = 0; j < 7; ++j )
        {
            if ( ( j == 0 && round == 0 ) || ( j && round < j + 5 ) )
            {
                QPointF initPos = store[j]->pos();
                initPos.ry() += ((7 - j / 3.0) + round) * deck()->cardHeight();
                addCardForDeal( store[j], cards.takeLast(), (round >= j || j == 0), initPos );
            }
        }
    }

    startDealAnimation();
}



static class YukonDealerInfo : public DealerInfo
{
public:
    YukonDealerInfo()
      : DealerInfo(I18N_NOOP("Yukon"), YukonId )
    {}

    virtual DealerScene *createGame() const
    {
        return new Yukon();
    }
} yukonDealerInfo;


#include "yukon.moc"
