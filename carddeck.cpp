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

#include "carddeck.h"

#include "dealer.h"
#include "version.h"

#include <KCardDeckInfo>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KSharedConfig>


const unsigned int NumberOfCards = 52;
static QHash<AbstractCard::Rank,KCardInfo::Card> rankList;
static QHash<AbstractCard::Suit,KCardInfo::Suit> suitList;

CardDeck *CardDeck::my_deck = 0;

CardDeck * CardDeck::self()
{
    if ( !my_deck )
        my_deck = new CardDeck();
    return my_deck;
}


CardDeck::CardDeck()
    : Pile( 0, 0 ),
    m_originalCardSize(1, 1),
    m_currentCardSize(0, 0),
    m_isInitialized(false),
    m_dscene( 0 )
{
    setObjectName( "deck" );

    setAddFlags(Pile::disallow);
    setRemoveFlags(Pile::disallow);

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    updateTheme(cs);
    setCardWidth( cs.readEntry( "CardWidth", 100 ) );
    suitList[AbstractCard::Clubs] = KCardInfo::Club;
    suitList[AbstractCard::Spades] = KCardInfo::Spade;
    suitList[AbstractCard::Diamonds] = KCardInfo::Diamond;
    suitList[AbstractCard::Hearts] = KCardInfo::Heart;
    rankList[AbstractCard::Two] = KCardInfo::Two;
    rankList[AbstractCard::Three] = KCardInfo::Three;
    rankList[AbstractCard::Four] = KCardInfo::Four;
    rankList[AbstractCard::Five] = KCardInfo::Five;
    rankList[AbstractCard::Six] = KCardInfo::Six;
    rankList[AbstractCard::Seven] = KCardInfo::Seven;
    rankList[AbstractCard::Eight] = KCardInfo::Eight;
    rankList[AbstractCard::Nine] = KCardInfo::Nine;
    rankList[AbstractCard::Ten] = KCardInfo::Ten;
    rankList[AbstractCard::Jack] = KCardInfo::Jack;
    rankList[AbstractCard::Queen] = KCardInfo::Queen;
    rankList[AbstractCard::King] = KCardInfo::King;
    rankList[AbstractCard::Ace] = KCardInfo::Ace;
}


CardDeck::~CardDeck()
{
    qDeleteAll( m_allCards );
    m_allCards.clear();
    m_cards.clear();
}

// ----------------------------------------------------------------

void CardDeck::setDeckProperties( uint m, uint s )
{
    if ( m == mult && s == suits )
        return;

    mult = m;
    suits = s;

    // Delete current cards
    collectCards();
    foreach ( Card *c, m_allCards )
    {
        m_dscene->removeItem( c );
        delete c;
    }
    m_allCards.clear();
    m_cards.clear();

    Card::Suit suitList[4] = { Card::Clubs, Card::Diamonds, Card::Hearts, Card::Spades };
    for ( uint m = 0; m < mult; ++m )
        for ( int r = Card::Ace; r <= Card::King; ++r )
            for ( int s = 3; s >= 0 ; --s )
                m_allCards << new Card( Card::Rank(r), suitList[3 - (s % suits)], m_dscene );

    kDebug() << m_allCards.size();

    collectAndShuffle();
}

void CardDeck::setScene( DealerScene * dealer )
{
    if ( dealer == m_dscene )
        return;

    if ( m_dscene )
    {
        disconnect( m_dscene );
        foreach ( Card * c, m_allCards ) {
            m_dscene->removeItem( c );
        }
        m_dscene->removePile( this );
    }

    if ( dealer )
    {
        foreach ( Card * c, m_allCards ) {
            dealer->addItem( c );
        }
        dealer->addPile( this );
    }

    m_dscene = dealer;
}

void CardDeck::updatePixmaps()
{
    foreach (Card * c, m_allCards)
        c->setPixmap();
}

void CardDeck::collectAndShuffle()
{
    collectCards();
    shuffle();
}


Card* CardDeck::nextCard()
{
    if (m_cards.isEmpty())
        return 0;

    return m_cards.last();
}


// ----------------------------------------------------------------


static int pseudoRandomSeed = 0;

static void pseudoRandom_srand(int seed)
{
    pseudoRandomSeed=seed;
}


// Documented as in
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q28150
//

static int pseudoRandom_random() {
    pseudoRandomSeed = 214013*pseudoRandomSeed+2531011;
    return (pseudoRandomSeed >> 16) & 0x7fff;
}


// Shuffle deck, assuming all cards are in m_cards

void CardDeck::shuffle()
{
    Q_ASSERT((uint)m_cards.count() == mult*NumberOfCards);

    Q_ASSERT(dscene()->gameNumber() >= 0);
    pseudoRandom_srand(dscene()->gameNumber());

    //kDebug(11111) << "first card" << m_cards[0]->name() << " " << dscene()->gameNumber();

    Card* t;
    int z;
    int left = mult*NumberOfCards;
    for (uint i = 0; i < mult*NumberOfCards; i++) {
        z = pseudoRandom_random() % left;
        t = m_cards[z];
        m_cards[z] = m_cards[left-1];
        m_cards[left-1] = t;
        left--;
    }
}


// add cards in deck[] to Deck
void CardDeck::collectCards()
{
    clear();

    foreach (Card * c, m_allCards) {
        c->setTakenDown(false);
        add( c, true );
        if ( isVisible() )
            c->setPos( QPointF( x(), y() ) );
        else
            c->setPos( QPointF( 2000, 2000 ) );
    }
}

void CardDeck::updateTheme(const KConfigGroup &cs)
{
    QString fronttheme = CardDeckInfo::frontTheme( cs );
    QString backtheme = CardDeckInfo::backTheme( cs );
    Q_ASSERT ( !backtheme.isEmpty() );
    Q_ASSERT ( !fronttheme.isEmpty() );

    m_cache.setFrontTheme( fronttheme );
    m_cache.setBackTheme( backtheme );

    m_originalCardSize = m_cache.defaultFrontSize( KCardInfo( KCardInfo::Spade, KCardInfo::Ace ) );
    Q_ASSERT( !m_originalCardSize.isNull() );
    m_currentCardSize = m_originalCardSize.toSize();
}

QSize CardDeck::cardSize() const
{
    return m_currentCardSize;
}

int CardDeck::cardWidth() const
{
    return m_currentCardSize.width();
}

int CardDeck::cardHeight() const
{
    return m_currentCardSize.height();
}

void CardDeck::setCardWidth( int width )
{
    if ( width > 200 || width < 10 )
        return;

    int height = width * m_originalCardSize.height() / m_originalCardSize.width();
    QSize newSize( width, height );

    if ( newSize != m_currentCardSize )
    {
        KConfigGroup cs( KGlobal::config(), settings_group );
        cs.writeEntry( "CardWidth", m_currentCardSize.width() );

        m_currentCardSize = newSize;
        m_cache.setSize( newSize );
        foreach (Card * c, m_allCards)
            c->setPixmap();

        QTimer::singleShot( 200, this, SLOT(loadInBackground()) );;
    }
}

QPixmap CardDeck::renderBackside( int variant )
{
    QPixmap pix = m_cache.backside( variant );
    return pix;
}

QPixmap CardDeck::renderFrontside( AbstractCard::Rank r, AbstractCard::Suit s )
{
    QPixmap pix = m_cache.frontside( KCardInfo( suitList[s], rankList[r] ) );
    return pix;
}

void CardDeck::loadInBackground()
{
    m_cache.loadTheme( KCardCache::LoadFrontSide | KCardCache::Load52Cards );
}

#include "carddeck.moc"