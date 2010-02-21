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

#include <KDebug>
#include <KGlobal>

#include <QtCore/QCache>
#include <QtCore/QTimer>
#include <QtGui/QPainter>

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


    void cardStoppedAnimation( KCard * card )
    {
        Q_ASSERT( cardsWaitedFor.contains( card ) );
        cardsWaitedFor.remove( card );

        if ( cardsWaitedFor.isEmpty() )
            emit q->cardAnimationDone();
    };


    QPixmap requestPixmap( QString elementId, bool immediate )
    {
        if ( elementIds.isEmpty() )
            initializeMapping();

        QPixmap & stored = elementIdMapping[ elementId ].first;
        if ( stored.size() != currentCardSize )
        {
            if ( immediate || stored.isNull() )
            {
                stored = cache->renderCard( elementId );
            }
            else 
            {
                QPixmap pix = cache->renderCardIfCached( elementId );
                if ( !pix.isNull() )
                    stored = pix;
                else
                    stored = stored.scaled( currentCardSize );
            }
        }
        return stored;
    }


    void pixmapUpdated( const QString & element, const QPixmap & pix )
    {
        QPair<QPixmap,QList<KCard*> > & pair = elementIdMapping[ element ];
        pair.first = pix;
        foreach ( KCard * c, pair.second )
            c->update();
    }


    void loadInBackground()
    {
        if ( elementIds.isEmpty() )
            initializeMapping();

        cache->loadInBackground( elementIds );
    };


    void initializeMapping()
    {
        foreach ( KCard * c, cards )
        {
            QString elementId = q->elementName( c->id(), true );
            elementIdMapping[ elementId ].second.append( c );

            elementId = q->elementName( c->id(), false );
            elementIdMapping[ elementId ].second.append( c );
        }

        QHash<QString,QPair<QPixmap,QList<KCard*> > >::iterator it = elementIdMapping.begin();
        QHash<QString,QPair<QPixmap,QList<KCard*> > >::iterator end = elementIdMapping.end();
        while ( it != end )
        {
            elementIds << it.key();
            it.value().first = requestPixmap( it.key(), false );
            ++it;
        }
    }

public:
    KAbstractCardDeck * q;

    KCardCache2 * cache;
    QSizeF originalCardSize;
    QSize currentCardSize;

    QList<KCard*> cards;
    QSet<KCard*> cardsWaitedFor;

    QStringList elementIds;
    QHash<QString,QPair<QPixmap,QList<KCard*> > > elementIdMapping;
};



KAbstractCardDeck::KAbstractCardDeck( QList<quint32> ids, QObject * parent )
  : QObject( parent ),
    d( new KAbstractCardDeckPrivate( this ) )
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
            c->prepareGeometryChange();

        d->currentCardSize = newSize;
        d->cache->setSize( newSize );

        foreach ( KCard * c, d->cards )
            c->update();

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
    return d->cache->theme();
}


bool KAbstractCardDeck::hasAnimatedCards() const
{
    return !d->cardsWaitedFor.isEmpty();
}


void KAbstractCardDeck::paintCard( QPainter * painter, quint32 id, bool faceUp, qreal highlightedness )
{
    QPixmap pix = d->requestPixmap( elementName( id, faceUp ), false );

    if ( highlightedness > 0 )
    {
        QPainter p( &pix );
        p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
        p.setOpacity( 0.5 * highlightedness );
        p.fillRect( 0, 0, pix.width(), pix.height(), Qt::black );
    }

    painter->drawPixmap( 0, 0, pix );
}


#include "kabstractcarddeck.moc"
#include "moc_kabstractcarddeck.cpp"
