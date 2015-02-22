/*
 * Copyright (C) 2001-2009 Stephan Kulow <coolo@kde.org>
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

#include "golf.h"

#include "dealerinfo.h"
#include "speeds.h"
#include "patsolve/golfsolver.h"

#include <KLocalizedString>


Golf::Golf( const DealerInfo * di )
  : DealerScene( di )
{
}


void Golf::initialize()
{
    const qreal dist_x = 1.11;
    const qreal smallNeg = -1e-6;

    setDeckContents();

    talon = new PatPile( this, 0, "talon" );
    talon->setPileRole(PatPile::Stock);
    talon->setLayoutPos(0, smallNeg);
    talon->setSpread(0, 0);
    talon->setKeyboardSelectHint( KCardPile::NeverFocus );
    talon->setKeyboardDropHint( KCardPile::NeverFocus );
    connect( talon, &KCardPile::clicked, this, &DealerScene::drawDealRowOrRedeal );

    waste = new PatPile( this, 8, "waste" );
    waste->setPileRole(PatPile::Foundation);
    waste->setLayoutPos(1.1, smallNeg);
    waste->setSpread(0.12, 0);
    waste->setRightPadding( 5 * dist_x );
    waste->setWidthPolicy( KCardPile::GrowRight );
    waste->setKeyboardSelectHint( KCardPile::NeverFocus );
    waste->setKeyboardDropHint( KCardPile::AutoFocusTop );

    for( int r = 0; r < 7; ++r )
    {
        stack[r] = new PatPile( this, 1 + r, QString("stack%1").arg(r) );
        stack[r]->setPileRole(PatPile::Tableau);
        stack[r]->setLayoutPos(r*dist_x,0);
        // Manual tweak of the pile z values to make some animations better.
        stack[r]->setZValue((7-r)/100.0);
        stack[r]->setBottomPadding( 1.3 );
        stack[r]->setHeightPolicy( KCardPile::GrowDown );
        stack[r]->setKeyboardSelectHint( KCardPile::AutoFocusTop );
        stack[r]->setKeyboardDropHint( KCardPile::NeverFocus );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new GolfSolver( this ) );

    connect( this, &KCardScene::cardClicked, this, &DealerScene::tryAutomaticMove );
}


bool Golf::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    return pile->pileRole() == PatPile::Foundation
           && ( newCards.first()->rank() == oldCards.last()->rank() + 1
                || newCards.first()->rank() == oldCards.last()->rank() - 1 );
}


bool Golf::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau
           && cards.first() == pile->topCard();
}


void Golf::restart( const QList<KCard*> & cards )
{
    QList<KCard*> cardList = cards;

    for ( int i = 0; i < 5; ++i )
        for ( int r = 0; r < 7; ++r )
            addCardForDeal( stack[r], cardList.takeLast(), true, stack[6]->pos() );

    while ( !cardList.isEmpty() )
    {
        KCard * c = cardList.takeFirst();
        c->setPos( talon->pos() );
        c->setFaceUp( false );
        talon->add( c );
    }

    startDealAnimation();

    flipCardToPile(talon->topCard(), waste, DURATION_MOVE);

    emit newCardsPossible( true );
}


bool Golf::newCards()
{
    if ( talon->isEmpty() )
         return false;

    flipCardToPile(talon->topCard(), waste, DURATION_MOVE);

    if ( talon->isEmpty() )
        emit newCardsPossible( false );

    return true;
}


bool Golf::drop()
{
    for ( int i = 0; i < 7; ++i )
        if ( !stack[i]->isEmpty() )
            return false;

    if ( !talon->isEmpty() )
    {
        flipCardToPile( talon->topCard(), waste, DURATION_MOVE );
        takeState();
        return true;
    }

    return false;
}


void Golf::setGameState( const QString & state )
{
    Q_UNUSED( state );
    emit newCardsPossible( !talon->isEmpty() );
}


static class GolfDealerInfo : public DealerInfo
{
public:
    GolfDealerInfo()
      : DealerInfo(I18N_NOOP("Golf"), GolfId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Golf( this );
    }
} golfDealerInfo;



