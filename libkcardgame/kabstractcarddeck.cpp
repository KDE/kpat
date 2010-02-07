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

#include "kabstractcarddeck.h"

#include "kcardtheme.h"
#include "kcardpile.h"
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
            cache = new KCardCache2( KCardTheme( frontSide ) );
        }
        return cache;
    };

    void stashCardCache( KCardCache2 * cache )
    {
        if ( cache )
            m_cacheCache.insert( cache->theme().dirName(), cache );
    };

private:
    QCache<QString,KCardCache2> m_cacheCache;
};

K_GLOBAL_STATIC( CardDeckPrivateStatic, cdps )



class KAbstractCardDeckPrivate : public QObject
{
    Q_OBJECT

public:
    KAbstractCardDeckPrivate( KAbstractCardDeck * q )
      : q( q )
    {
    };

public slots:
    void cardStartedAnimation( KCard * card )
    {
        Q_ASSERT( !cardsWaitedFor.contains( card ) );
        cardsWaitedFor.insert( card );
    };


    void cardStoppedAnimation( KCard *card )
    {
        Q_ASSERT( cardsWaitedFor.contains( card ) );
        cardsWaitedFor.remove( card );

        if ( cardsWaitedFor.isEmpty() )
            emit q->cardAnimationDone();
    };


    void loadInBackground()
    {
        QSet<QString> elements;
        foreach ( const KCard * c, allCards )
        {
            elements << q->elementName( c->data(), true );
            elements << q->elementName( c->data(), false );
        }

        cache->loadInBackground( elements.toList() );
    };

public:
    KAbstractCardDeck * q;

    QList<KCard*> allCards;
    QList<KCard*> undealtCards;

    KCardCache2 * cache;
    QSizeF originalCardSize;
    QSize currentCardSize;

    QSet<KCard*> cardsWaitedFor;
};



KAbstractCardDeck::KAbstractCardDeck()
  : d( new KAbstractCardDeckPrivate( this ) )
{
    d->cache = 0;
    d->originalCardSize = QSize( 1, 1 );
    d->currentCardSize = QSize( 0, 0 );
}


KAbstractCardDeck::~KAbstractCardDeck()
{
    returnAllCards();
    foreach ( KCard * c, d->allCards )
        delete c;
    d->allCards.clear();
    d->undealtCards.clear();

    cdps->stashCardCache( d->cache );

    delete d;
}


QList<KCard*> KAbstractCardDeck::cards() const
{
    return d->allCards;
}


bool KAbstractCardDeck::hasUndealtCards() const
{
    return !d->undealtCards.isEmpty();
}


KCard * KAbstractCardDeck::takeCard()
{
    if ( d->undealtCards.isEmpty() )
        return 0;

    return d->undealtCards.takeLast();
}


KCard * KAbstractCardDeck::takeCard( quint32 id )
{
    for ( QList<KCard*>::iterator it = d->undealtCards.begin();
          it != d->undealtCards.end();
          ++it )
    {
        KCard * c = *it;

        if ( c->data() == id )
        {
            d->undealtCards.erase( it );
            return c;
        }
    }
    return 0;
}


void KAbstractCardDeck::takeAllCards( KCardPile * p )
{
    while ( !d->undealtCards.isEmpty() )
    {
        KCard * c = d->undealtCards.takeFirst();
        c->setPos( p->pos() );
        c->turn( false );
        p->add( c );
    }
}


void KAbstractCardDeck::returnCard( KCard * c )
{
//FIXME
//     c->setTakenDown( false );
    if ( c->source() )
        c->source()->remove( c );
    if ( c->scene() )
        c->scene()->removeItem( c );
    d->undealtCards.append( c );
}


void KAbstractCardDeck::returnAllCards()
{
    d->undealtCards.clear();
    foreach ( KCard * c, d->allCards )
        returnCard( c );
}


// Shuffle all undealt cards
void KAbstractCardDeck::shuffle( int gameNumber )
{
    d->undealtCards = shuffled( d->undealtCards, gameNumber );
}


void KAbstractCardDeck::setCardWidth( int width )
{
    if ( width > 200 || width < 20 )
        return;

    int height = width * d->originalCardSize.height() / d->originalCardSize.width();
    QSize newSize( width, height );

    if ( newSize != d->currentCardSize )
    {
        d->currentCardSize = newSize;
        d->cache->setSize( newSize );
        foreach ( KCard * c, d->allCards )
            c->updatePixmap();

        QTimer::singleShot( 200, d, SLOT(loadInBackground()) );;
    }
}


int KAbstractCardDeck::cardWidth() const
{
    return d->currentCardSize.width();
}


void KAbstractCardDeck::setCardHeight( int height )
{
    setCardWidth( height * d->originalCardSize.width() / d->originalCardSize.height() );
}


int KAbstractCardDeck::cardHeight() const
{
    return d->currentCardSize.height();
}


QSize KAbstractCardDeck::cardSize() const
{
    return d->currentCardSize;
}


QPixmap KAbstractCardDeck::frontsidePixmap( quint32 id ) const
{
    return d->cache->renderCard( elementName( id, true ) );
}


QPixmap KAbstractCardDeck::backsidePixmap( quint32 id ) const
{
    Q_UNUSED( id );
    return d->cache->renderCard( elementName( id, false ) );
}


void KAbstractCardDeck::updateTheme( const KConfigGroup & cs )
{
    QString fronttheme = CardDeckInfo::frontTheme( cs );
    Q_ASSERT ( !fronttheme.isEmpty() );

    cdps->stashCardCache( d->cache );
    d->cache = cdps->getCardCache( fronttheme );

    d->originalCardSize = d->cache->naturalSize( "back" );
    Q_ASSERT( !d->originalCardSize.isNull() );
    d->currentCardSize = d->originalCardSize.toSize();
}


bool KAbstractCardDeck::hasAnimatedCards() const
{
    return !d->cardsWaitedFor.isEmpty();
}


void KAbstractCardDeck::initializeCards( const QList<KCard*> & cards )
{
    Q_ASSERT( d->allCards.isEmpty() );

    d->allCards = d->undealtCards = cards;

    foreach ( const KCard * c, cards )
    {
        connect( c, SIGNAL(animationStarted(KCard*)), d, SLOT(cardStartedAnimation(KCard*)) );
        connect( c, SIGNAL(animationStopped(KCard*)), d, SLOT(cardStoppedAnimation(KCard*)) );
    }
}


#include "kabstractcarddeck.moc"
#include "moc_kabstractcarddeck.cpp"
