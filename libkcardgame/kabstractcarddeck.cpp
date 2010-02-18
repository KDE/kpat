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

#include "kcard_p.h"
#include "kcardtheme.h"
#include "kcardpile.h"
#include "shuffle.h"

#include <KCardDeckInfo>
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


    void pixmapUpdated( const QString & element, const QPixmap & pix )
    {
        foreach ( KCard * c, elementMapping.values( element ) )
            c->setFrontsidePixmap( pix );
    }


    void loadInBackground()
    {
        if ( frontsideElements.isEmpty() )
        {
            foreach ( KCard * c, cards )
                elementMapping.insert( q->elementName( c->id(), true ), c );
            frontsideElements = elementMapping.uniqueKeys();
        }

        cache->loadInBackground( frontsideElements );
    };

public:
    KAbstractCardDeck * q;

    QList<KCard*> cards;

    KCardCache2 * cache;
    QSizeF originalCardSize;
    QSize currentCardSize;

    QSet<KCard*> cardsWaitedFor;

    QList<QString> frontsideElements;
    QMultiHash<QString,KCard*> elementMapping;
};



KAbstractCardDeck::KAbstractCardDeck( QList<quint32> ids )
  : d( new KAbstractCardDeckPrivate( this ) )
{
    d->cache = 0;
    d->originalCardSize = QSize( 1, 1 );
    d->currentCardSize = QSize( 0, 0 );

    foreach ( quint32 id, ids )
    {
        KCard * c = new KCard( id, this );
        connect( c, SIGNAL(animationStarted(KCard*)), d, SLOT(cardStartedAnimation(KCard*)) );
        connect( c, SIGNAL(animationStopped(KCard*)), d, SLOT(cardStoppedAnimation(KCard*)) );
        d->cards << c;
    }
}


KAbstractCardDeck::~KAbstractCardDeck()
{
    foreach ( KCard * c, d->cards )
        delete c;
    d->cards.clear();

    cdps->stashCardCache( d->cache );

    delete d;
}


QList<KCard*> KAbstractCardDeck::cards() const
{
    return d->cards;
}


void KAbstractCardDeck::setCardWidth( int width )
{
    if ( width > 200 || width < 20 )
        return;

    int height = width * d->originalCardSize.height() / d->originalCardSize.width();
    QSize newSize( width, height );
    QSize oldSize = d->currentCardSize;

    if ( newSize != d->currentCardSize )
    {
        foreach ( KCard * c, d->cards )
        {
            c->prepareGeometryChange();
        }

        d->currentCardSize = newSize;
        d->cache->setSize( newSize );

        foreach ( KCard * c, d->cards )
        {
            QString element = elementName( c->id(), true );
            QPixmap pix;
            if ( oldSize.isEmpty() )
                pix = d->cache->renderCard( element );
            else 
                pix = d->cache->renderCardIfCached( element );

            if ( pix.isNull() )
                pix = c->d->frontside.scaled( newSize );

            c->setFrontsidePixmap( pix );
        }

        QTimer::singleShot( 0, d, SLOT(loadInBackground()) );;
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


void KAbstractCardDeck::updateTheme( const KCardTheme & theme )
{
    Q_ASSERT ( theme.isValid() );

    cdps->stashCardCache( d->cache );
    d->cache = cdps->getCardCache( theme.dirName() );
    connect( d->cache, SIGNAL(renderingDone(QString,QPixmap)), d, SLOT(pixmapUpdated(QString,QPixmap)) );

    d->originalCardSize = d->cache->naturalSize( "back" );
    Q_ASSERT( !d->originalCardSize.isNull() );
    d->currentCardSize = d->originalCardSize.toSize();
}


KCardTheme KAbstractCardDeck::theme() const
{
    return KCardTheme( d->cache->theme() );
}


bool KAbstractCardDeck::hasAnimatedCards() const
{
    return !d->cardsWaitedFor.isEmpty();
}


#include "kabstractcarddeck.moc"
#include "moc_kabstractcarddeck.cpp"
