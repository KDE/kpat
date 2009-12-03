/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#include "idiot.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "patsolve/idiotsolver.h"

#include <KLocale>

Idiot::Idiot( )
  : DealerScene( )
{
    // Since there are so few piles, we might as well set a big margin
    setLayoutMargin( 0.6 );

    setDeck( new CardDeck() );

    // Create the talon to the left.
    talon = new Pile( 0, "talon" );
    talon->setPilePos(0, 0);
    talon->setAddFlags(Pile::disallow);
    addPile(talon);

    const qreal distx = 1.1;

    // Create 4 piles where the cards will be placed during the game.
    for( int i = 0; i < 4; i++ ) {
        m_play[i] = new Pile( i + 1, QString( "play%1" ).arg( i ));
        m_play[i]->setAddFlags( Pile::addSpread );
        m_play[i]->setRemoveFlags( Pile::disallow | Pile::demoOK );
        m_play[i]->setPilePos(1.5 + distx * i, 0);
        m_play[i]->setReservedSpace( QSizeF( 1.0, 3.0 ) );
        addPile( m_play[i] );
    }

    // Create the discard pile to the right
    m_away = new Pile( 5, "away" );
    m_away->setTarget(true);
    m_away->setRemoveFlags(Pile::disallow);
    m_away->setPilePos(1.9 + distx * 4, 0);
    addPile(m_away);

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new IdiotSolver(this ) );
}


void Idiot::restart()
{
    deck()->returnAllCards();
    deck()->shuffle( gameNumber() );

    // Move the four top cards of the deck to the piles, faceup, spread out.
    for ( int i = 0; i < 4; ++i )
        m_play[ i ]->animatedAdd( deck()->takeCard(), true );

    deck()->takeAllCards( talon );

    emit newCardsPossible(true);
}

bool Idiot::cardClicked(Card *c)
{
    // If the deck is clicked, deal 4 more cards.
    if (c->source() == talon) {
        newCards();
        return true;
    }

    // Only the top card of a pile can be clicked.
    if (c != c->source()->top())
        return false;

    bool  didMove = true;
    finishSolver();
    solver()->translate_layout();
    int index = -1;
    for ( int i = 0; i < 4; i++ )
        if ( m_play[i] == c->source() )
        {
            index = i;
            break;
        }

    if ( index != -1 && static_cast<IdiotSolver*>( solver() )->canMoveAway(index) )
        // Add to 'm_away', face up, no spread
        m_away->animatedAdd(c, true );
    else if ( m_play[ 0 ]->isEmpty() )
        // Add to pile 1, face up, spread.
        m_play[0]->animatedAdd(c, true );
    else if ( m_play[ 1 ]->isEmpty() )
        // Add to pile 2, face up, spread.
        m_play[1]->animatedAdd(c, true );
    else if ( m_play[ 2 ]->isEmpty() )
        // Add to pile 3, face up, spread.
        m_play[2]->animatedAdd( c, true );
    else if ( m_play[ 3 ]->isEmpty() )
        // Add to pile 4, face up, spread.
        m_play[3]->animatedAdd(c, true );
    else
        didMove = false;

    if ( didMove )
        onGameStateAlteredByUser();

    return didMove;
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
        if (m_play[i]->cardsLeft() != 1 || m_play[i]->top()->rank() != Card::Ace)
            return false;
    }

    return true;
}


// This patience doesn't support double click.
//

bool Idiot::cardDoubleClicked(Card *)
{
    return false; // nothing - nada
}


// Deal 4 cards face up - one on each pile.
//
Card *Idiot::newCards()
{
    if ( talon->isEmpty() )
        return 0;

    if ( deck()->hasAnimatedCards() )
        for ( int i = 0; i < 4; ++i )
            if ( m_play[i]->top() )
                return m_play[i]->top();

    clearHighlightedItems();

    // Move the four top cards of the deck to the piles, faceup, spread out.
    for ( int i = 0; i < 4; ++i )
        m_play[ i ]->animatedAdd( talon->top(), true );

    onGameStateAlteredByUser();
    if ( talon->isEmpty() )
        emit newCardsPossible( false );

    return m_play[0]->top();
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
