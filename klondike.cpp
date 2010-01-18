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

#include "klondike.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "speeds.h"
#include "version.h"
#include "patsolve/klondikesolver.h"

#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KRandom>
#include <KSelectAction>



KlondikePile::KlondikePile( int _index, int _draw, const QString & objectName)
    : PatPile(_index, objectName), m_draw( _draw )
{
}

void KlondikePile::setDraws( int _draw )
{
    m_draw = _draw;
}

void KlondikePile::layoutCards( int duration )
{
    QList<Card*> cards = this->cards();

    if ( cards.isEmpty() )
        return;

    qreal cardWidth = cards.first()->pixmap().width();

    qreal divx = qMin<qreal>( ( maximumSpace().width() - cardWidth ) / ( 2 * spread().width() * cardWidth ), 1.0 );

    QPointF cardPos = pos();
    int z = zValue();
    for ( int i = 0; i < cards.size(); ++i )
    {
        ++z;
        if ( i < cards.size() - m_draw )
        {
            cards[i]->setZValue( z );
            cards[i]->animate( cardPos, z, 1, 0, cards[i]->realFace(), false, duration );
        }
        else
        {
            cards[i]->animate( cardPos, z, 1, 0, cards[i]->realFace(), false, duration );
            cardPos.rx() += divx * spread().width() * cardWidth;
        }
    }
}

Klondike::Klondike()
    : DealerScene( )
{
    // The units of the follwoing constants are pixels
    const qreal hspacing = 1.0 / 6 + 0.02; // horizontal spacing between card piles
    const qreal vspacing = 1.0 / 4; // vertical spacing between card piles

    setDeck( new CardDeck() );

    talon = new PatPile( 0, "talon" );
    talon->setPileRole(PatPile::Stock);
    talon->setPilePos(0, 0);
    connect(talon, SIGNAL(clicked(Card*)), SLOT(newCards()));
    // Give the talon a low Z value to keep it out of the way during there
    // deal animation.
    talon->setZValue( -52 );
    addPile(talon);

    KConfigGroup cg(KGlobal::config(), settings_group );
    EasyRules = cg.readEntry( "KlondikeEasy", true);

    pile = new KlondikePile( 13, EasyRules ? 1 : 3, "pile" );
    pile->setPileRole(PatPile::Waste);
    pile->setReservedSpace( QSizeF( 1.9, 1.0 ) );
    pile->setPilePos(1.0 + hspacing, 0);
    pile->setSpread( 0.33, 0 );
    addPile(pile);

    for( int i = 0; i < 7; i++ )
    {
        play[ i ] = new PatPile( i + 5, QString( "play%1" ).arg( i ));
        play[i]->setPileRole(PatPile::Tableau);
        play[i]->setPilePos((1.0 + hspacing) * i, 1.0 + vspacing);
        play[i]->setAutoTurnTop(true);
        play[i]->setReservedSpace( QSizeF( 1.0, 1.0 + play[i]->spread().height() * 7 ) );
        addPile(play[i]);
    }

    for( int i = 0; i < 4; i++ )
    {
        target[ i ] = new PatPile( i + 1, QString( "target%1" ).arg( i ) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setFoundation(true);
        target[i]->setPilePos((3 + i) * (1.0 + hspacing), 0);
        addPile(target[i]);
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

bool Klondike::checkAdd(const PatPile * pile, const CardList & oldCards, const CardList & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return checkAddAlternateColorDescendingFromKing(oldCards, newCards);
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    case PatPile::Waste:
    case PatPile::Stock:
    default:
        return false;
    }
}

bool Klondike::checkRemove(const PatPile * pile, const CardList & cards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return isAlternateColorDescending(cards);
    case PatPile::Foundation:
        return EasyRules && cards.first() == pile->top();
    case PatPile::Waste:
        return cards.first() == pile->top();
    case PatPile::Stock:
    default:
        return false;
    }
}


QList<QAction*> Klondike::configActions() const
{
    return QList<QAction*>() << options;
}

Card *Klondike::newCards()
{
    if ( talon->isEmpty() && pile->count() <= 1 )
        return 0;

    if ( deck()->hasAnimatedCards() )
    {
        if ( pile->top() )
            return pile->top();
        for (int i = 0; i < 7; ++i)
            if (play[i]->top())
                return play[i]->top();
    }

    if ( talon->isEmpty() )
    {
        // Move the cards from the pile back to the deck
        while ( !pile->isEmpty() )
        {
            Card *c = pile->top();
            c->completeAnimation();
            c->turn( false );
            talon->add(c);
        }
        talon->relayoutCards();

        redealt = true;
    }
    else
    {
        QList<Card*> flippedCards;
        for (int flipped = 0; flipped < pile->draw() && !talon->isEmpty(); ++flipped)
        {
            pile->add( talon->top() );
            flippedCards << pile->top();
        }
        pile->relayoutCards();
        int flipped = 0;
        foreach ( Card *c, flippedCards )
        {
            ++flipped;
            c->completeAnimation();
            QPointF destPos = c->realPos();
            c->setPos( talon->pos() );
            c->flipTo( destPos, DURATION_FLIP * ( 1 + flipped / 6.0) );
        }
    }

    onGameStateAlteredByUser();
    if ( talon->isEmpty() && pile->count() <= 1 )
       emit newCardsPossible( false );

    // we need to look that many steps in the future to see if we can lose
    setNeededFutureMoves( talon->count() + pile->count() );

    return pile->top() ? pile->top() : talon->top();
}

void Klondike::restart()
{
    deck()->returnAllCards();
    deck()->shuffle( gameNumber() );
    redealt = false;
    deal();
}

void Klondike::gameTypeChanged()
{
    stopDemo();

    if ( allowedToStartNewGame() )
    {
        setEasy( options->currentItem() == 0 );
        startNew( gameNumber() );
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
    emit newCardsPossible( !talon->isEmpty() || pile->count() > 1 );
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
    if ( _EasyRules != EasyRules )
    {
        EasyRules = _EasyRules;
        options->setCurrentItem( EasyRules ? 0 : 1 );

        int drawNumber = EasyRules ? 1 : 3;
        pile->setDraws( drawNumber );
        setSolver( new KlondikeSolver( this, drawNumber ) );

        KConfigGroup cg(KGlobal::config(), settings_group );
        cg.writeEntry( "KlondikeEasy", EasyRules);
        cg.sync();
    }
}


void Klondike::deal() {
    for(int round=0; round < 7; round++)
        for (int i = round; i < 7; i++ )
            addCardForDeal( play[i], deck()->takeCard(), (i == round), talon->pos());

    deck()->takeAllCards(talon);

    startDealAnimation();
}

bool Klondike::drop()
{
    bool pileempty = pile->isEmpty();
    if (!DealerScene::drop())
        return false;
    if (pile->isEmpty() && !pileempty)
        newCards();
    return true;
}

void Klondike::mapOldId(int id)
{
   switch (id) {
   case DealerInfo::KlondikeDrawThreeId :
       setEasy( false );
   case DealerInfo::KlondikeDrawOneId :
       setEasy( true );
   }
}


int Klondike::oldId() const
{
    if ( EasyRules )
        return DealerInfo::KlondikeDrawOneId;
    else
        return DealerInfo::KlondikeDrawThreeId;
}




static class KlondikeDealerInfo : public DealerInfo
{
public:
    KlondikeDealerInfo()
      : DealerInfo(I18N_NOOP("Klondike"), DealerInfo::KlondikeGeneralId)
    {
        addId(DealerInfo::KlondikeDrawThreeId);
        addId(DealerInfo::KlondikeDrawOneId);
    }

    virtual DealerScene *createGame() const
    {
        return new Klondike();
    }

    virtual const char * name( int id ) const
    {
        switch ( id )
        {
        case DealerInfo::KlondikeDrawOneId :
            return I18N_NOOP("Klondike (Draw 1)");
        case DealerInfo::KlondikeDrawThreeId :
            return I18N_NOOP("Klondike (Draw 3)");
        default :
            return m_name;
        }
    }
} klondikeDealerInfo;


#include "klondike.moc"
