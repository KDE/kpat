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

#include "deck.h"
#include "patsolve/idiotsolver.h"

#include <KLocale>

Idiot::Idiot( )
  : DealerScene( )
{
    // Since there are so few piles, we might as well set a big margin
    setLayoutMargin( 0.6 );

    // Create the deck to the left.
    Deck::self()->setScene( this );
    Deck::self()->setDeckProperties(1, 4);
    Deck::self()->setPilePos(0, 0);

    const qreal distx = 1.1;

    // Create 4 piles where the cards will be placed during the game.
    for( int i = 0; i < 4; i++ ) {
        m_play[i] = new Pile( i + 1, this);
        m_play[i]->setAddFlags( Pile::addSpread );
        m_play[i]->setRemoveFlags( Pile::disallow | Pile::demoOK );
        m_play[i]->setPilePos(1.5 + distx * i, 0);
        m_play[i]->setObjectName( QString( "play%1" ).arg( i ) );
        m_play[i]->setReservedSpace( QSizeF( 1.0, 3.0 ) );
    }

    // Create the discard pile to the right
    m_away = new Pile( 5, this );
    m_away->setTarget(true);
    m_away->setRemoveFlags(Pile::disallow);
    m_away->setPilePos(1.9 + distx * 4, 0);
    m_away->setObjectName( "away" );

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new IdiotSolver(this ) );
}


void Idiot::restart()
{
    Deck::self()->collectAndShuffle();

    // Move the four top cards of the deck to the piles, faceup, spread out.
    for ( int i = 0; i < 4; ++i )
        m_play[ i ]->add( Deck::self()->nextCard(), false );

    emit newCardsPossible(true);
}

bool Idiot::cardClicked(Card *c)
{
    // If the deck is clicked, deal 4 more cards.
    if (c->source() == Deck::self()) {
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
        m_away->add(c, false );
    else if ( m_play[ 0 ]->isEmpty() )
	// Add to pile 1, face up, spread.
        m_play[0]->add(c, false );
    else if ( m_play[ 1 ]->isEmpty() )
	// Add to pile 2, face up, spread.
        m_play[1]->add(c, false );
    else if ( m_play[ 2 ]->isEmpty() )
	// Add to pile 3, face up, spread.
        m_play[2]->add( c, false );
    else if ( m_play[ 3 ]->isEmpty() )
	// Add to pile 4, face up, spread.
        m_play[3]->add(c, false );
    else
	didMove = false;

    return true; // may be a lie, but no one cares
}

// The game is won when:
//  1. all cards are dealt.
//  2. all piles contain exactly one ace.
//  3. the rest of the cards are thrown away (follows automatically from 1, 2.
//
bool Idiot::isGameWon() const
{
    // Criterium 1.
    if (!Deck::self()->isEmpty())
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

bool Idiot::cardDblClicked(Card *)
{
    return false; // nothing - nada
}


// Deal 4 cards face up - one on each pile.
//
Card *Idiot::newCards()
{
    if ( Deck::self()->isEmpty() )
        return 0;

    if ( waiting() )
        for ( int i = 0; i < 4; ++i )
            if ( m_play[i]->top() )
                return m_play[i]->top();

    unmarkAll();

    // Move the four top cards of the deck to the piles, faceup, spread out.
    for ( int i = 0; i < 4; ++i )
        m_play[ i ]->add( Deck::self()->nextCard(), false );

    takeState();
    considerGameStarted();
    if ( Deck::self()->isEmpty() )
        emit newCardsPossible( false );

    return m_play[0]->top();
}

void Idiot::setGameState(const QString &)
{
    emit newCardsPossible( !Deck::self()->isEmpty() );
}

static class LocalDealerInfo2 : public DealerInfo
{
public:
    LocalDealerInfo2() : DealerInfo(I18N_NOOP("Aces Up"), 2) {}
    virtual DealerScene *createGame() const { return new Idiot(); }
} ldi4;


#include "idiot.moc"
