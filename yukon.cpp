/*
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include <KLocalizedString>


Yukon::Yukon( const DealerInfo * di )
  : DealerScene( di )
{
}


void Yukon::initialize()
{
    const qreal dist_x = 1.11;
    const qreal dist_y = 1.11;

    setDeckContents();

    for ( int i = 0; i < 4; ++i )
    {
        target[i] = new PatPile( this, i + 1, QString("target%1").arg(i) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(0.11+7*dist_x, dist_y *i);
        target[i]->setKeyboardSelectHint( KCardPile::NeverFocus );
        target[i]->setKeyboardDropHint( KCardPile::ForceFocusTop );
    }

    for ( int i = 0; i < 7; ++i )
    {
        store[i] = new PatPile( this, 5 + i, QString("store%1").arg(i) );
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(dist_x*i, 0);
        store[i]->setAutoTurnTop(true);
        store[i]->setBottomPadding( 3 * dist_y );
        store[i]->setHeightPolicy( KCardPile::GrowDown );
        store[i]->setKeyboardSelectHint( KCardPile::FreeFocus );
        store[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
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
            return newCards.first()->rank() == KCardDeck::King;
        else
            return newCards.first()->rank() == oldCards.last()->rank() - 1
                   && newCards.first()->color() != oldCards.last()->color();
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

void Yukon::restart( const QList<KCard*> & cards )
{
    QList<KCard*> cardList = cards;

    for ( int round = 0; round < 11; ++round )
    {
        for ( int j = 0; j < 7; ++j )
        {
            if ( ( j == 0 && round == 0 ) || ( j && round < j + 5 ) )
            {
                QPointF initPos = store[j]->pos();
                initPos.ry() += ((7 - j / 3.0) + round) * deck()->cardHeight();
                addCardForDeal( store[j], cardList.takeLast(), (round >= j || j == 0), initPos );
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
        return new Yukon( this );
    }
} yukonDealerInfo;



