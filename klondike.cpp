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

#include "klondike.h"

#include "cardmaps.h"
#include "deck.h"
#include "version.h"
#include "view.h"
#include "patsolve/klondikesolver.h"

#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KRandom>
#include <KSelectAction>


KlondikePile::KlondikePile( int _index, int _draw, DealerScene* parent)
    : Pile(_index, parent), m_draw( _draw )
{
}

void KlondikePile::setDraws( int _draw )
{
    m_draw = _draw;
}

void KlondikePile::layoutCards( int duration )
{
    if ( m_cards.isEmpty() )
        return;

    qreal divx = qMin<qreal>( ( maximumSpace().width() - cardMap::self()->cardWidth() ) / ( 2 * spread() * cardMap::self()->cardWidth() ), 1.0 );

    QPointF cardPos = pos();
    int z = zValue();
    for ( int i = 0; i < m_cards.size(); ++i )
    {
        ++z;
        if ( i < m_cards.size() - m_draw )
        {
            m_cards[i]->setZValue( z );
            m_cards[i]->setPos( cardPos );
        }
        else
        {
            m_cards[i]->moveTo( cardPos.x(), cardPos.y(), z, dscene()->speedUpTime( duration ) );
            cardPos.rx() += divx * spread() * cardMap::self()->cardWidth();
        }
    }
}

Klondike::Klondike()
    : DealerScene( )
{
    // The units of the follwoing constants are pixels
    const qreal hspacing = 1.0 / 6 + 0.02; // horizontal spacing between card piles
    const qreal vspacing = 1.0 / 4; // vertical spacing between card piles

    Deck::createDeck(this);
    Deck::deck()->setPilePos(0, 0);
    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(newCards()));

    KConfigGroup cg(KGlobal::config(), settings_group );
    EasyRules = cg.readEntry( "KlondikeEasy", true);

    pile = new KlondikePile( 13, EasyRules ? 1 : 3, this);
    pile->setObjectName( "pile" );
    pile->setReservedSpace( QSizeF( 1.9, 1.0 ) );
    pile->setPilePos(1.0 + hspacing, 0);
    pile->setSpread( 0.33 );
    pile->setAddFlags( Pile::disallow );
    pile->setRemoveFlags(Pile::Default);

    for( int i = 0; i < 7; i++ )
    {
        play[ i ] = new Pile( i + 5, this);
        play[i]->setPilePos((1.0 + hspacing) * i, 1.0 + vspacing);
        play[i]->setAddType(Pile::KlondikeStore);
        play[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop | Pile::wholeColumn);
        play[i]->setObjectName( QString( "play%1" ).arg( i ) );
        play[i]->setReservedSpace( QSizeF( 1.0, 1.0 + play[i]->spread() * 7 ) );
    }

    for( int i = 0; i < 4; i++ )
    {
        target[ i ] = new Pile( i + 1, this );
        target[i]->setPilePos((3 + i) * (1.0 + hspacing), 0);
        target[i]->setAddType(Pile::KlondikeTarget);
        if (EasyRules) // change default
            target[i]->setRemoveFlags(Pile::Default);
        else
            target[i]->setRemoveType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new KlondikeSolver( this, pile->draw() ) );
    redealt = false;

    options = new KSelectAction(i18n("Klondike &Options"), this );
    options->addAction( i18n("Draw 1") );
    options->addAction( i18n("Draw 3") );
    options->setCurrentItem( EasyRules ? 0 : 1 );
    connect( options, SIGNAL(triggered(int)), SLOT(gameTypeChanged()) );
}

QList<QAction*> Klondike::configActions() const
{
    return QList<QAction*>() << options;
}

Card *Klondike::newCards()
{
    if ( Deck::deck()->isEmpty() && pile->cardsLeft() <= 1 )
        return 0;

    if ( pile->top() && pile->top()->animated() )
        return pile->top();

    if ( Deck::deck()->isEmpty() )
    {
        // Move the cards from the pile back to the deck
        redeal();
        return Deck::deck()->top();
    }

    for (int flipped = 0; flipped < pile->draw() && !Deck::deck()->isEmpty(); ++flipped) {
        Card *c = Deck::deck()->nextCard();
        pile->add(c, true); // facedown, nospread
        c->stopAnimation();
        // move back to flip
        c->setPos( Deck::deck()->pos() );
        c->flipTo( pile->x() + pile->spread() * flipped * cardMap::self()->cardWidth(), pile->y(), 200 + 80 * ( flipped + 1 ) );
    }

    takeState();
    considerGameStarted();
    if ( Deck::deck()->isEmpty() && pile->cardsLeft() <= 1 )
       emit newCardsPossible( false );

    // we need to look that many steps in the future to see if we can lose
    setNeededFutureMoves( Deck::deck()->cardsLeft() + pile->cardsLeft() );

    return pile->top();
}

void Klondike::restart()
{
    Deck::deck()->collectAndShuffle();
    redealt = false;
    deal();
}

void Klondike::gameTypeChanged()
{
    if ( demoActive() || isGameWon()  )
       return;

    if ( allowedToStartNewGame() )
    {
        setEasy( options->currentItem() == 0 );
        startNew( KRandom::random() );
    }
    else
    {
        // If we're not allowed, reset the option to
        // the current number of suits.
        options->setCurrentItem( EasyRules ? 0 : 1 );
    }
}

QString Klondike::getGameState()
{
    // getGameState() is called every time a card is moved, so we use it to
    // check if there are any cards left to deal. There might be a more elegant
    // to do this, though.
    emit newCardsPossible( !Deck::deck()->isEmpty() || pile->cardsLeft() > 1 );
    return QString();
}

QString Klondike::getGameOptions() const
{
    return QString::number( pile->draw() );
}

void Klondike::setGameOptions( const QString & options )
{
    setEasy( options.toInt() == 1 );
}

void Klondike::setEasy( bool _EasyRules )
{
    EasyRules = _EasyRules;
    pile->setDraws( EasyRules ? 1 : 3 );
    options->setCurrentItem( EasyRules ? 0 : 1 );

    for( int i = 0; i < 4; i++ ) {
        if (EasyRules) // change default
            target[i]->setRemoveFlags(Pile::Default);
        else
            target[i]->setRemoveType(Pile::KlondikeTarget);
    }
    KConfigGroup cg(KGlobal::config(), settings_group );
    cg.writeEntry( "KlondikeEasy", EasyRules);
    cg.sync();
}

//  Add cards from  pile to deck, in reverse direction
void Klondike::redeal() {

    CardList pilecards = pile->cards();
    if (EasyRules)
        // the remaining cards in deck should be on top
        // of the new deck
        pilecards += Deck::deck()->cards();

    for (int count = pilecards.count() - 1; count >= 0; --count)
    {
        Card *card = pilecards[count];
        card->stopAnimation();
        Deck::deck()->add(card, true); // facedown, nospread
    }

    redealt = true;
}

void Klondike::deal() {
    for(int round=0; round < 7; round++)
        for (int i = round; i < 7; i++ )
            play[i]->add(Deck::deck()->nextCard(), i != round && true);
}

bool Klondike::startAutoDrop()
{
    bool pileempty = pile->isEmpty();
    if (!DealerScene::startAutoDrop())
        return false;
    if (pile->isEmpty() && !pileempty)
        newCards();
    return true;
}

static class LocalDealerInfo0 : public DealerInfo
{
public:
    LocalDealerInfo0() : DealerInfo(I18N_NOOP("Klondike"), 18) { addOldId(0); addOldId(13); }
    virtual DealerScene *createGame() const { return new Klondike(); }
} ldi0;

void Klondike::mapOldId(int id)
{
   switch (id) {
   case 13: // draw 3
       setEasy( false );
   case 0: // draw 1
       setEasy( true );
   }
}

#include "klondike.moc"
