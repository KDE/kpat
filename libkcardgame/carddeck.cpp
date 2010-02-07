/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <parker.coates@kdemail.net>
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

#include "pile.h"
#include "shuffle.h"

#include <KCardDeckInfo>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KSharedConfig>

#include <QtCore/QCache>
#include <QtCore/QTimer>
#include <QtGui/QGraphicsScene>

class CardDeckPrivateStatic
{
public:
    CardDeckPrivateStatic()
      : m_cacheCache( 3 )
    {}

    KCardCache2 * getCardCache( const QString & frontSide )
    {
        KCardCache2 * cache = m_cacheCache.take( frontSide );
        if ( !cache )
        {
            cache = new KCardCache2();
            cache->setTheme( frontSide );
        }
        return cache;
    };

    void stashCardCache( KCardCache2 * cache )
    {
        if ( cache )
            m_cacheCache.insert( cache->theme(), cache );
    };

private:
    QCache<QString,KCardCache2> m_cacheCache;
};

K_GLOBAL_STATIC( CardDeckPrivateStatic, cdps )




CardDeck::CardDeck()
  : m_allCards(),
    m_undealtCards(),
    m_cache( 0 ),
    m_originalCardSize( 1, 1 ),
    m_currentCardSize( 0, 0 ),
    m_cardsWaitedFor()
{
}


CardDeck::~CardDeck()
{
    returnAllCards();
    foreach ( Card * c, m_allCards )
        delete c;
    m_allCards.clear();
    m_undealtCards.clear();

    cdps->stashCardCache( m_cache );
}


QList< Card* > CardDeck::cards() const
{
    return m_allCards;
}


bool CardDeck::hasUndealtCards() const
{
    return !m_undealtCards.isEmpty();
}


Card * CardDeck::takeCard()
{
    if ( m_undealtCards.isEmpty() )
        return 0;

    return m_undealtCards.takeLast();
}


Card * CardDeck::takeCard( quint32 id )
{
    for ( QList<Card*>::iterator it = m_undealtCards.begin();
          it != m_undealtCards.end();
          ++it )
    {
        Card * c = *it;

        if ( c->data() == id )
        {
            m_undealtCards.erase( it );
            return c;
        }
    }
    return 0;
}


void CardDeck::takeAllCards( Pile * p )
{
    while ( !m_undealtCards.isEmpty() )
    {
        Card * c = m_undealtCards.takeFirst();
        c->setPos( p->pos() );
        c->turn( false );
        p->add( c );
    }
}


void CardDeck::returnCard( Card * c )
{
//FIXME
//     c->setTakenDown( false );
    if ( c->source() )
        c->source()->remove( c );
    if ( c->scene() )
        c->scene()->removeItem( c );
    m_undealtCards.append( c );
}


void CardDeck::returnAllCards()
{
    m_undealtCards.clear();
    foreach ( Card * c, m_allCards )
        returnCard( c );
}


// Shuffle all undealt cards
void CardDeck::shuffle( int gameNumber )
{
    m_undealtCards = shuffled( m_undealtCards, gameNumber );
}


void CardDeck::setCardWidth( int width )
{
    if ( width > 200 || width < 20 )
        return;

    int height = width * m_originalCardSize.height() / m_originalCardSize.width();
    QSize newSize( width, height );

    if ( newSize != m_currentCardSize )
    {
        m_currentCardSize = newSize;
        m_cache->setSize( newSize );
        foreach ( Card * c, m_allCards )
            c->updatePixmap();

        QTimer::singleShot( 200, this, SLOT(loadInBackground()) );;
    }
}


int CardDeck::cardWidth() const
{
    return m_currentCardSize.width();
}


void CardDeck::setCardHeight( int height )
{
    setCardWidth( height * m_originalCardSize.width() / m_originalCardSize.height() );
}


int CardDeck::cardHeight() const
{
    return m_currentCardSize.height();
}


QSize CardDeck::cardSize() const
{
    return m_currentCardSize;
}


QPixmap CardDeck::frontsidePixmap( quint32 id ) const
{
    return m_cache->renderCard( elementName( id, true ) );
}


QPixmap CardDeck::backsidePixmap( quint32 id ) const
{
    Q_UNUSED( id );
    return m_cache->renderCard( elementName( id, false ) );
}


void CardDeck::updateTheme( const KConfigGroup & cs )
{
    QString fronttheme = CardDeckInfo::frontTheme( cs );
    Q_ASSERT ( !fronttheme.isEmpty() );

    cdps->stashCardCache( m_cache );
    m_cache = cdps->getCardCache( fronttheme );

    m_originalCardSize = m_cache->naturalSize( "1_spade" );
    Q_ASSERT( !m_originalCardSize.isNull() );
    m_currentCardSize = m_originalCardSize.toSize();
}


bool CardDeck::hasAnimatedCards() const
{
    return !m_cardsWaitedFor.isEmpty();
}


void CardDeck::initializeCards( const QList<Card*> & cards )
{
    Q_ASSERT( m_allCards.isEmpty() );

    m_allCards = m_undealtCards = cards;
}


void CardDeck::loadInBackground()
{
    QSet<QString> elements;
    foreach ( const Card * c, m_allCards )
    {
        elements << elementName( c->data(), true );
        elements << elementName( c->data(), false );
    }

    m_cache->loadInBackground( elements.toList() );
}


void CardDeck::cardStartedAnimation( Card *card )
{
    Q_ASSERT( !m_cardsWaitedFor.contains( card ) );
    m_cardsWaitedFor.insert( card );
}


void CardDeck::cardStoppedAnimation( Card *card )
{
    Q_ASSERT( m_cardsWaitedFor.contains( card ) );
    m_cardsWaitedFor.remove( card );

    if ( m_cardsWaitedFor.isEmpty() )
        emit cardAnimationDone();
}

#include "carddeck.moc"
