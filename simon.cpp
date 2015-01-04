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

#include "simon.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "patsolve/simonsolver.h"

#include <KLocalizedString>


Simon::Simon( const DealerInfo * di )
  : DealerScene( di )
{
}


void Simon::initialize()
{
    setDeckContents();

    const qreal dist_x = 1.11;

    for ( int i = 0; i < 4; ++i )
    {
        target[i] = new PatPile( this, i + 1, QString( "target%1" ).arg( i ) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos((i+3)*dist_x, 0);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint( KCardPile::NeverFocus );
        target[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    for ( int i = 0; i < 10; ++i )
    {
        store[i] = new PatPile( this, 5 + i, QString( "store%1" ).arg( i ) );
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(dist_x*i, 1.2);
        store[i]->setBottomPadding( 2.5 );
        store[i]->setHeightPolicy( KCardPile::GrowDown );
        store[i]->setZValue( 0.01 * i );
        store[i]->setKeyboardSelectHint( KCardPile::AutoFocusDeepestRemovable );
        store[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new SimonSolver( this ) );
    //setNeededFutureMoves( 1 ); // could be some nonsense moves
}

void Simon::restart( const QList<KCard*> & cards )
{
    QList<KCard*> cardList = cards;

    QPointF initPos( 0, -deck()->cardHeight() );

    for ( int piles = 9; piles >= 3; --piles )
        for ( int j = 0; j < piles; ++j )
            addCardForDeal( store[j], cardList.takeLast(), true, initPos );

    for ( int j = 0; j < 10; ++j )
        addCardForDeal( store[j], cardList.takeLast(), true, initPos );

    Q_ASSERT( cardList.isEmpty() );

    startDealAnimation();
}

bool Simon::checkPrefering(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    return pile->pileRole() == PatPile::Tableau
           && !oldCards.isEmpty()
           && oldCards.last()->suit() == newCards.first()->suit();
}

bool Simon::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    if (pile->pileRole() == PatPile::Tableau)
    {
        return oldCards.isEmpty()
               || oldCards.last()->rank() == newCards.first()->rank() + 1;
    }
    else
    {
        return oldCards.isEmpty()
               && newCards.first()->rank() == KCardDeck::King
               && newCards.last()->rank() == KCardDeck::Ace;
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
        return new Simon( this );
    }
} simonDealerInfo;


