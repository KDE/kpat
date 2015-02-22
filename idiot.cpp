/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#include "idiot.h"

#include "dealerinfo.h"
#include "patsolve/idiotsolver.h"

#include <KLocalizedString>


Idiot::Idiot( const DealerInfo * di )
  : DealerScene( di )
{
}


void Idiot::initialize()
{
    setSceneAlignment( AlignHCenter | AlignVCenter );

    setDeckContents();

    // Create the talon to the left.
    talon = new PatPile( this, 0, "talon" );
    talon->setPileRole(PatPile::Stock);
    talon->setLayoutPos(0, 0);
    talon->setSpread(0, 0);
    talon->setKeyboardSelectHint( KCardPile::NeverFocus );
    talon->setKeyboardDropHint( KCardPile::NeverFocus );
    connect(talon, &PatPile::clicked, this, &Idiot::newCards);

    const qreal distx = 1.1;

    // Create 4 piles where the cards will be placed during the game.
    for( int i = 0; i < 4; ++i )
    {
        m_play[i] = new PatPile( this, i + 1, QString( "play%1" ).arg( i ));
        m_play[i]->setPileRole(PatPile::Tableau);
        m_play[i]->setLayoutPos(1.5 + distx * i, 0);
        m_play[i]->setBottomPadding( 2 );
        m_play[i]->setHeightPolicy( KCardPile::GrowDown );
        m_play[i]->setKeyboardSelectHint( KCardPile::AutoFocusTop );
        m_play[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    // Create the discard pile to the right
    m_away = new PatPile( this, 5, "away" );
    m_away->setPileRole(PatPile::Foundation);
    m_away->setLayoutPos(1.9 + distx * 4, 0);
    m_away->setSpread(0, 0);
    m_away->setKeyboardSelectHint(KCardPile::NeverFocus);
    m_away->setKeyboardDropHint(KCardPile::ForceFocusTop);

    connect(this, &Idiot::cardClicked, this, &Idiot::tryAutomaticMove);

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new IdiotSolver(this ) );
}


void Idiot::restart( const QList<KCard*> & cards )
{
    foreach ( KCard * c, cards )
    {
        c->setPos( talon->pos() );
        c->setFaceUp( false );
        talon->add( c );
    }

    dealRow();

    emit newCardsPossible(true);
}


bool Idiot::drop()
{
    return false;
}


bool Idiot::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch ( pile->pileRole() )
    {
    case PatPile::Foundation:
        return newCards.size() == 1 && canMoveAway( newCards.first() );
    case PatPile::Tableau:
        return oldCards.isEmpty() && newCards.size() == 1;
    case PatPile::Stock:
    default:
        return false;
    }
}

bool Idiot::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau
           && cards.first() == pile->topCard()
           && ( canMoveAway( cards.first() )
                || m_play[0]->isEmpty()
                || m_play[1]->isEmpty()
                || m_play[2]->isEmpty()
                || m_play[3]->isEmpty() );
}

bool Idiot::canMoveAway(const KCard * card) const
{
    Q_ASSERT( card->pile() != talon );
    Q_ASSERT( card->pile() != m_away );
    Q_ASSERT( card == card->pile()->topCard() );

    for ( int i = 0; i < 4; ++i )
    {
        KCard * c = m_play[i]->topCard();
        if ( c
             && c != card
             && c->suit() == card->suit()
             && ( c->rank() == KCardDeck::Ace
                  || ( card->rank() != KCardDeck::Ace
                       && c->rank() > card->rank() ) ) )
            return true;
    }

    return false;
}



bool Idiot::tryAutomaticMove( KCard * card )
{
    if ( !isCardAnimationRunning()
         && card
         && card->pile()
         && card == card->pile()->topCard()
         && card->pile() != talon
         && card->pile() != m_away )
    {
        KCardPile * destination = 0;
        if ( canMoveAway( card ) )
            destination = m_away;
        else if ( m_play[0]->isEmpty() )
            destination = m_play[0];
        else if ( m_play[1]->isEmpty() )
            destination = m_play[1];
        else if ( m_play[2]->isEmpty() )
            destination = m_play[2];
        else if ( m_play[3]->isEmpty() )
            destination = m_play[3];

        if ( destination )
        {
            moveCardToPile( card, destination, DURATION_MOVE );
            return true;
        }
    }
    return false;
}


// The game is won when:
//  1. all cards are dealt.
//  2. all piles contain exactly one ace.
//  3. the rest of the cards are thrown away (follows automatically from 1, 2.
//
bool Idiot::isGameWon() const
{
    // Criterium 1.
    if (!talon->isEmpty())
        return false;

    // Criterium 2.
    for (int i = 0; i < 4; i++) {
        if (m_play[i]->count() != 1 || m_play[i]->topCard()->rank() != KCardDeck::Ace)
            return false;
    }

    return true;
}


// Deal 4 cards face up - one on each pile.
bool Idiot::newCards()
{
    if ( talon->isEmpty() )
        return false;

    dealRow();

    if ( talon->isEmpty() )
        emit newCardsPossible( false );

    return true;
}


void Idiot::dealRow()
{
    Q_ASSERT(talon->count() >= 4);

    for ( int i = 0; i < 4; ++i )
    {
        KCard * c = talon->topCard();
        flipCardToPileAtSpeed( c, m_play[i], DEAL_SPEED );

        // Fudge the z values so that cards don't appear to pop through one another.
        c->setZValue( c->zValue() + i );
    }
}


void Idiot::setGameState( const QString & state )
{
    Q_UNUSED( state );
    emit newCardsPossible( !talon->isEmpty() );
}



static class IdiotDealerInfo : public DealerInfo
{
public:
    IdiotDealerInfo()
      : DealerInfo(I18N_NOOP("Aces Up"), AcesUpId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Idiot( this );
    }
} idiotDealerInfo;



