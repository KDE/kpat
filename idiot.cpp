/*
   idiot.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation.

   This file is provided AS IS with no warranties of any kind.  The author
   shall have no liability with respect to the infringement of copyrights,
   trade secrets or any patents by this file or any part thereof.  In no
   event will the author be liable for any lost revenue or profits or
   other special, indirect and consequential damages.

   4 positions, remove lowest card(s) of suit
*/


#include "idiot.h"
#include "deck.h"
#include "patsolve/idiot.h"

#include <klocale.h>

Idiot::Idiot( )
  : DealerScene( )
{
    // Create the deck to the left.
    Deck::create_deck( this );
    Deck::deck()->setPilePos(1, 1);

    const int distx = 11;

    // Create 4 piles where the cards will be placed during the game.
    for( int i = 0; i < 4; i++ ) {
        m_play[i] = new Pile( i + 1, this);

        m_play[i]->setAddFlags( Pile::addSpread );
        m_play[i]->setRemoveFlags( Pile::disallow | Pile::demoOK );
        m_play[i]->setPilePos( 16 + distx * i, 1);
        m_play[i]->setObjectName( QString( "play%1" ).arg( i ) );
        m_play[i]->setReservedSpace( QSizeF( 10.0, 30.0 ) );
    }

    // Create the discard pile to the right
    m_away = new Pile( 5, this );
    m_away->setTarget(true);
    m_away->setRemoveFlags(Pile::disallow);
    m_away->setPilePos( 20 + distx * 4, 1);
    m_away->setObjectName( "away" );

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new IdiotSolver(this ) );
}


void Idiot::restart()
{
    Deck::deck()->collectAndShuffle();
    dealNext();
    emit dealPossible(true);
}

bool Idiot::cardClicked(Card *c)
{
    // If the deck is clicked, deal 4 more cards.
    if (c->source() == Deck::deck()) {
        dealNext();
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
    if (!Deck::deck()->isEmpty())
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

void Idiot::dealNext()
{
    unmarkAll();
    if ( Deck::deck()->isEmpty() )
        return;

    // Move the four top cards of the deck to the piles, faceup, spread out.
    for ( int i = 0; i < 4; ++i )
        m_play[ i ]->add( Deck::deck()->nextCard(), false );

    takeState();
    considerGameStarted();
    emit dealPossible( !Deck::deck()->isEmpty() );
}

Card *Idiot::demoNewCards()
{
    if ( Deck::deck()->isEmpty() )
        return 0;

    dealNext();

    return m_play[0]->top();
}

void Idiot::setGameState(const QString &)
{
    emit dealPossible( !Deck::deck()->isEmpty() );
}

static class LocalDealerInfo2 : public DealerInfo
{
public:
    LocalDealerInfo2() : DealerInfo(I18N_NOOP("Aces Up"), 2) {}
    virtual DealerScene *createGame() const { return new Idiot(); }
} ldi4;


#include "idiot.moc"
