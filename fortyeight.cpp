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

#include "fortyeight.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "speeds.h"
#include "patsolve/fortyeightsolver.h"

#include <KLocalizedString>


Fortyeight::Fortyeight( const DealerInfo* di )
  : DealerScene( di )
{
}


void Fortyeight::initialize()
{
    const qreal dist_x = 1.11;
    const qreal smallNeg = -1e-6;

    setDeckContents( 2 );

    talon = new PatPile( this, 0, "talon" );
    talon->setPileRole(PatPile::Stock);
    talon->setLayoutPos( 7 * dist_x, smallNeg );
    talon->setZValue(20);
    talon->setSpread(0, 0);
    talon->setKeyboardSelectHint( KCardPile::NeverFocus );
    talon->setKeyboardDropHint( KCardPile::NeverFocus );
    connect( talon, &KCardPile::clicked, this, &DealerScene::drawDealRowOrRedeal );

    pile = new PatPile( this, 20, "pile" );
    pile->setPileRole(PatPile::Waste);
    pile->setLayoutPos( 6 * dist_x, smallNeg );
    pile->setLeftPadding( 6 * dist_x );
    pile->setWidthPolicy( KCardPile::GrowLeft );
    pile->setSpread(-0.21, 0);
    pile->setKeyboardSelectHint( KCardPile::AutoFocusTop );
    pile->setKeyboardDropHint( KCardPile::NeverFocus );

    for ( int i = 0; i < 8; ++i )
    {
        target[i] = new PatPile( this, 9 + i, QString( "target%1" ).arg( i ) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(dist_x*i, 0);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint( KCardPile::NeverFocus );
        target[i]->setKeyboardDropHint( KCardPile::ForceFocusTop );
    }

    for ( int i = 0; i < 8; ++i )
    {
        stack[i] = new PatPile( this, 1 + i, QString( "stack%1" ).arg( i ) );
        stack[i]->setPileRole(PatPile::Tableau);
        stack[i]->setLayoutPos(dist_x*i, 1.1 );
        stack[i]->setAutoTurnTop(true);
        stack[i]->setSpread(0, 0.25);
        stack[i]->setBottomPadding( 1.75 );
        stack[i]->setHeightPolicy( KCardPile::GrowDown );
        stack[i]->setKeyboardSelectHint( KCardPile::FreeFocus );
        stack[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new FortyeightSolver( this ) );
}


void Fortyeight::restart( const QList<KCard*> & cards )
{
    lastdeal = false;

    QList<KCard*> cardList = cards;

    for ( int r = 0; r < 4; ++r )
    {
        for ( int column = 0; column < 8; ++column )
        {
            QPointF initPos = stack[column]->pos() - QPointF( 0, 2 * deck()->cardHeight() );
            addCardForDeal( stack[column], cardList.takeLast(), true, initPos );
        }
    }

    while ( !cardList.isEmpty() )
    {
        KCard * c = cardList.takeFirst();
        c->setPos( talon->pos() );
        c->setFaceUp( false );
        talon->add( c );
    }

    startDealAnimation();

    flipCardToPile( talon->topCard(), pile, DURATION_MOVE );

    emit newCardsPossible( true );
}


bool Fortyeight::newCards()
{
    if ( talon->isEmpty() )
    {
        if ( lastdeal )
        {
            return false;
        }
        else
        {
            lastdeal = true;
            flipCardsToPile( pile->cards(), talon, DURATION_MOVE );
        }
    }
    else
    {
        flipCardToPile( talon->topCard(), pile, DURATION_MOVE );
        setKeyboardFocus( pile->topCard() );
    }

    if ( talon->isEmpty() && lastdeal )
        emit newCardsPossible( false );

    return true;
}


void Fortyeight::cardsDroppedOnPile( const QList<KCard*> & cards, KCardPile * pile )
{
    if ( cards.size() <= 1 )
    {
        DealerScene::moveCardsToPile( cards, pile, DURATION_MOVE );
        return;
    }

    QList<KCardPile*> freePiles;
    for ( int i = 0; i < 8; ++i )
        if ( stack[i]->isEmpty() && stack[i] != pile )
            freePiles << stack[i];

    multiStepMove( cards, pile, freePiles, QList<KCardPile*>(), DURATION_MOVE );
}


bool Fortyeight::canPutStore( const KCardPile * pile, const QList<KCard*> &cards ) const
{
    int frees = 0;
    for (int i = 0; i < 8; i++)
        if (stack[i]->isEmpty()) frees++;

    if ( pile->isEmpty()) // destination is empty
        frees--;

    if (int( cards.count()) > 1<<frees)
        return false;

    // ok if the target is empty
    if ( pile->isEmpty())
        return true;

    KCard *c = cards.first(); // we assume there are only valid sequences

    return pile->topCard()->suit() == c->suit()
      && pile->topCard()->rank() == c->rank() + 1;
}

bool Fortyeight::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch ( pile->pileRole() )
    {
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    case PatPile::Tableau:
      return canPutStore(pile, newCards);
    case PatPile::Stock:
    case PatPile::Waste:
    default:
        return false;
    }
}


bool Fortyeight::checkRemove( const PatPile * pile, const QList<KCard*> & cards) const
{
    switch ( pile->pileRole() )
    {
    case PatPile::Waste:
      return cards.first() == pile->topCard();
    case PatPile::Tableau:
      return isSameSuitDescending(cards);
    case PatPile::Foundation:
    case PatPile::Stock:
    default:
        return false;
    }
}


QString Fortyeight::getGameState() const
{
    return QString::number(lastdeal);
}


void Fortyeight::setGameState( const QString & state )
{
    lastdeal = state.toInt();
    emit newCardsPossible( !lastdeal || !talon->isEmpty() );
}


static class FortyEightDealerInfo : public DealerInfo
{
public:
    FortyEightDealerInfo()
      : DealerInfo(I18N_NOOP("Forty & Eight"), FortyAndEightId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Fortyeight( this );
    }
} fortyEightDealerInfo;



