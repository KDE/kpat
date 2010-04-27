/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#include "idiot.h"

#include "dealerinfo.h"
#include "patsolve/idiotsolver.h"

#include "Shuffle"

#include <KLocale>


void Idiot::initialize()
{
    setSceneAlignment( AlignHCenter | AlignVCenter );

    static_cast<KStandardCardDeck*>( deck() )->setDeckContents();

    // Create the talon to the left.
    talon = new PatPile( 0, "talon" );
    talon->setPileRole(PatPile::Stock);
    talon->setPilePos(0, 0);
    talon->setSpread(0, 0);
    addPile(talon);

    const qreal distx = 1.1;

    // Create 4 piles where the cards will be placed during the game.
    for( int i = 0; i < 4; i++ ) {
        m_play[i] = new PatPile( i + 1, QString( "play%1" ).arg( i ));
        m_play[i]->setPileRole(PatPile::Tableau);
        m_play[i]->setPilePos(1.5 + distx * i, 0);
        m_play[i]->setReservedSpace( 0, 0, 1, 3 );
        addPile( m_play[i] );
    }

    // Create the discard pile to the right
    m_away = new PatPile( 5, "away" );
    m_away->setPileRole(PatPile::Waste);
    m_away->setFoundation(true);
    m_away->setPilePos(1.9 + distx * 4, 0);
    m_away->setSpread(0, 0);
    addPile(m_away);

    connect( this, SIGNAL(cardClicked(KCard*)), this, SLOT(handleCardClick(KCard*)) );

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new IdiotSolver(this ) );
}


void Idiot::restart()
{
    QList<KCard*> cards = shuffled( deck()->cards(), gameNumber() );

    while ( !cards.isEmpty() )
    {
        KCard * c = cards.takeFirst();
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
    case PatPile::Waste:
        return canMoveAway( newCards.first() );
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
           && cards.first() == pile->top()
           && ( canMoveAway( cards.first() )
                || m_play[0]->isEmpty()
                || m_play[1]->isEmpty()
                || m_play[2]->isEmpty()
                || m_play[3]->isEmpty() );
}

bool Idiot::canMoveAway(const KCard * card) const
{
    if ( card->pile() == talon || card->pile() == m_away )
        return false;

    if ( card != card->pile()->top() )
        return false;

    for ( int i = 0; i < 4; ++i )
    {
        KCard * c = m_play[i]->top();
        if ( c && c != card && getSuit( c ) == getSuit( card )
             && ( getRank( c ) == KStandardCardDeck::Ace
                  || ( getRank( card ) != KStandardCardDeck::Ace
                       && getRank( c ) > getRank( card ) ) ) )
            return true;
    }

    return false;
}



void Idiot::handleCardClick( KCard * card )
{
    // Only the top card of a pile can be clicked.
    if ( card != card->pile()->top())
        return;

    KCardPile * destination = 0;
    if ( card->pile() == talon )
        newCards();
    else if ( canMoveAway( card) )
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
        moveCardToPile( card, destination, DURATION_MOVE );
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
        if (m_play[i]->count() != 1 || getRank( m_play[i]->top() ) != KStandardCardDeck::Ace)
            return false;
    }

    return true;
}


// Deal 4 cards face up - one on each pile.
//
KCard *Idiot::newCards()
{
    if ( talon->isEmpty() )
        return 0;

    if ( deck()->hasAnimatedCards() )
        for ( int i = 0; i < 4; ++i )
            if ( m_play[i]->top() )
                return m_play[i]->top();

    clearHighlightedItems();

    dealRow();

    if ( talon->isEmpty() )
        emit newCardsPossible( false );

    return m_play[0]->top();
}


void Idiot::dealRow()
{
    Q_ASSERT(talon->count() >= 4);

    for ( int i = 0; i < 4; ++i )
    {
        KCard * c = talon->top();
        flipCardToPileAtSpeed( c, m_play[i], DEAL_SPEED );

        // Fudge the z values so that cards don't appear to pop through one another.
        c->setZValue( c->zValue() + i );
    }
}


void Idiot::setGameState(const QString &)
{
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
        return new Idiot();
    }
} idiotDealerInfo;


#include "idiot.moc"
