/*
 *  Copyright (C) 2009-2010 Parker Coates <coates@kde.org>
 *
 *  Original card caching:
 *  Copyright (C) 2008 Andreas Pakulat <apaku@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "kabstractcarddeck_p.h"

#include "common.h"
#include "kcardtheme.h"
#include "kcardpile.h"

#include <QDebug>
#include <KImageCache>

#include <QtCore/QTimer>
#include <QApplication>
#include <QPainter>
#include <QtSvg/QSvgRenderer>
#include <QGraphicsScene>

namespace
{
    const QString cacheNameTemplate( "libkcardgame-themes/%1" );
    const QString timeStampKey( "libkcardgame_timestamp" );
    const QString unscaledSizeKey( "libkcardgame_unscaledsize" );
    const QString lastUsedSizeKey( "libkcardgame_lastusedsize" );

    QString keyForPixmap( const QString & element, const QSize & s )
    {
        return element + '@' + QString::number( s.width() ) + 'x' + QString::number( s.height() );
    }
}


RenderingThread::RenderingThread( KAbstractCardDeckPrivate * d, QSize size, const QStringList & elements )
  : d( d ),
    m_size( size ),
    m_elementsToRender( elements ),
    m_haltFlag( false )
{
    connect( this, &RenderingThread::renderingDone, d, &KAbstractCardDeckPrivate::submitRendering, Qt::QueuedConnection );
}


void RenderingThread::halt()
{
    {
        QMutexLocker l( &m_haltMutex );
        m_haltFlag = true;
    }
    wait();
}


void RenderingThread::run()
{
    {
        // Load the renderer even if we don't have any elements to render.
        QMutexLocker l( &(d->rendererMutex) );
        d->renderer();
    }

    foreach ( const QString & element, m_elementsToRender )
    {
        {
            QMutexLocker l( &m_haltMutex );
            if ( m_haltFlag )
                return;
        }

        QString key = keyForPixmap( element, m_size );
        if ( !d->cache->contains( key ) )
        {
            //qDebug() << "Renderering" << key << "in rendering thread.";
            QImage img = d->renderCard( element, m_size );
            d->cache->insertImage( key, img );
            emit renderingDone( element, img );
        }
    }
}


KAbstractCardDeckPrivate::KAbstractCardDeckPrivate( KAbstractCardDeck * q )
  : QObject( q ),
    q( q ),
    animationCheckTimer( new QTimer( this ) ),
    cache( 0 ),
    svgRenderer( 0 ),
    thread( 0 )

{
    animationCheckTimer->setSingleShot( true );
    animationCheckTimer->setInterval( 0 );
    connect( animationCheckTimer, &QTimer::timeout, this, &KAbstractCardDeckPrivate::checkIfAnimationIsDone );
}


KAbstractCardDeckPrivate::~KAbstractCardDeckPrivate()
{
    deleteThread();
    delete cache;
    delete svgRenderer;
}


// Note that rendererMutex MUST be locked before calling this function.
QSvgRenderer * KAbstractCardDeckPrivate::renderer()
{
    if ( !svgRenderer )
    {
        QString thread = (qApp->thread() == QThread::currentThread()) ? "main" : "rendering";
        //qDebug() << QString("Loading card deck SVG in %1 thread").arg( thread );

        svgRenderer = new QSvgRenderer( theme.graphicsFilePath() );
    }
    return svgRenderer;
}


QImage KAbstractCardDeckPrivate::renderCard( const QString & element, const QSize & size )
{
    // Note that we don't use Format_ARGB32_Premultiplied as it sacrifices some
    // colour accuracy at low opacities for performance. Normally this wouldn't
    // be an issue, but in card games we often will have, say, 52 pixmaps
    // stacked on top of one another, which causes these colour inaccuracies to
    // add up to the point that they're very visible.
    QImage img( size, QImage::Format_ARGB32 );
    img.fill( Qt::transparent );
    QPainter p( &img );
    {
        QMutexLocker l( &rendererMutex );
        if ( renderer()->elementExists( element ) )
        {
            renderer()->render( &p, element );
        }
        else
        {
            qWarning() << "Could not find" << element << "in SVG.";
            p.fillRect( QRect( 0, 0, img.width(), img.height() ), Qt::white );
            p.setPen( Qt::red );
            p.drawLine( 0, 0, img.width(), img.height() );
            p.drawLine( img.width(), 0, 0, img.height() );
            p.end();
        }
    }
    p.end();

    return img;
}


QSizeF KAbstractCardDeckPrivate::unscaledCardSize()
{
    QSizeF size;

    if ( !theme.isValid() )
        return size;

    if ( !cacheFind( cache, unscaledSizeKey, &size ) )
    {
        {
            QMutexLocker l( &rendererMutex );
            size = renderer()->boundsOnElement( "back" ).size();
        }
        cacheInsert( cache, unscaledSizeKey, size );
    }

    return size;
}


QPixmap KAbstractCardDeckPrivate::requestPixmap( quint32 id, bool faceUp )
{
    if ( !theme.isValid() || !currentCardSize.isValid() )
        return QPixmap();

    QString elementId = q->elementName( id, faceUp );
    QHash<QString,CardElementData> & index = faceUp ? frontIndex : backIndex;

    QHash<QString,CardElementData>::iterator it = index.find( elementId );
    if ( it == index.end() )
        return QPixmap();

    QPixmap & stored = it.value().cardPixmap;
    if ( stored.size() != currentCardSize )
    {
        QString key = keyForPixmap( elementId , currentCardSize );
        if ( !cache->findPixmap( key, &stored ) )
        {
            if ( stored.isNull() )
            {
                //qDebug() << "Renderering" << key << "in main thread.";
                QImage img = renderCard( elementId, currentCardSize );
                cache->insertImage( key, img );
                stored = QPixmap::fromImage( img );
            }
            else
            {
                stored = stored.scaled( currentCardSize );
            }
        }
        Q_ASSERT( stored.size() == currentCardSize );
    }
    return stored;
}


void KAbstractCardDeckPrivate::deleteThread()
{
    if ( thread && thread->isRunning() )
        thread->halt();
    delete thread;
    thread = 0;
}


void KAbstractCardDeckPrivate::submitRendering( const QString & elementId, const QImage & image )
{
    QPixmap pix;

    // If the currentCardSize has changed since the rendering was performed,
    // we sadly just have to throw it away.
    if ( image.size() != currentCardSize )
        return;

    // The RenderingThread just put the image in the cache, but due to the
    // volatility of the cache there's no guarantee that it'll still be there
    // by the time this slot is called, in which case we convert the QImage
    // passed in the signal.
    if ( !cache->findPixmap( keyForPixmap( elementId, currentCardSize ), &pix ) )
        pix = QPixmap::fromImage( image );

    QHash<QString,CardElementData>::iterator it;
    it = frontIndex.find( elementId );
    if ( it != frontIndex.end() )
    {
        it.value().cardPixmap = pix;
        foreach ( KCard * c, it.value().cardUsers )
            c->setFrontPixmap( pix );
    }

    it = backIndex.find( elementId );
    if ( it != backIndex.end() )
    {
        it.value().cardPixmap = pix;
        foreach ( KCard * c, it.value().cardUsers )
            c->setBackPixmap( pix );
    }
}


void KAbstractCardDeckPrivate::cardStartedAnimation( KCard * card )
{
    Q_ASSERT( !cardsWaitedFor.contains( card ) );
    cardsWaitedFor.insert( card );
}


void KAbstractCardDeckPrivate::cardStoppedAnimation( KCard * card )
{
    Q_ASSERT( cardsWaitedFor.contains( card ) );
    cardsWaitedFor.remove( card );

    if ( cardsWaitedFor.isEmpty() )
        animationCheckTimer->start();
}


void KAbstractCardDeckPrivate::checkIfAnimationIsDone()
{
    if ( cardsWaitedFor.isEmpty() )
        emit q->cardAnimationDone();
}


KAbstractCardDeck::KAbstractCardDeck( const KCardTheme & theme, QObject * parent )
  : QObject( parent ),
    d( new KAbstractCardDeckPrivate( this ) )
{
    setTheme( theme );
}


KAbstractCardDeck::~KAbstractCardDeck()
{
    foreach ( KCard * c, d->cards )
        delete c;
    d->cards.clear();
}


void KAbstractCardDeck::setDeckContents( const QList<quint32> & ids )
{
    foreach ( KCard * c, d->cards )
        delete c;
    d->cards.clear();
    d->cardsWaitedFor.clear();

    QHash<QString,CardElementData> oldFrontIndex = d->frontIndex;
    d->frontIndex.clear();

    QHash<QString,CardElementData> oldBackIndex = d->backIndex;
    d->backIndex.clear();

    foreach ( quint32 id, ids )
    {
        KCard * c = new KCard( id, this );

        c->setObjectName( elementName( c->id() ) );

        connect( c, &KCard::animationStarted, d, &KAbstractCardDeckPrivate::cardStartedAnimation );
        connect( c, &KCard::animationStopped, d, &KAbstractCardDeckPrivate::cardStoppedAnimation );

        QString elementId = elementName( id, true );
        d->frontIndex[ elementId ].cardUsers.append( c );

        elementId = elementName( id, false );
        d->backIndex[ elementId ].cardUsers.append( c );

        d->cards << c;
    }

    QHash<QString,CardElementData>::iterator it;
    QHash<QString,CardElementData>::iterator end;
    QHash<QString,CardElementData>::const_iterator it2;
    QHash<QString,CardElementData>::const_iterator end2;

    end = d->frontIndex.end();
    end2 = oldFrontIndex.constEnd();
    for ( it = d->frontIndex.begin(); it != end; ++it )
    {
        it2 = oldFrontIndex.constFind( it.key() );
        if ( it2 != end2 )
            it.value().cardPixmap = it2.value().cardPixmap;
    }

    end = d->backIndex.end();
    end2 = oldBackIndex.constEnd();
    for ( it = d->backIndex.begin(); it != end; ++it )
    {
        it2 = oldBackIndex.constFind( it.key() );
        if ( it2 != end2 )
            it.value().cardPixmap = it2.value().cardPixmap;
    }
}


QList<KCard*> KAbstractCardDeck::cards() const
{
    return d->cards;
}


int KAbstractCardDeck::rankFromId( quint32 id ) const
{
    Q_UNUSED( id );
    return -1;
}


int KAbstractCardDeck::suitFromId( quint32 id ) const
{
    Q_UNUSED( id );
    return -1;
}


int KAbstractCardDeck::colorFromId( quint32 id ) const
{
    Q_UNUSED( id );
    return -1;
}


void KAbstractCardDeck::setCardWidth( int width )
{
    if ( width > 200 || width < 20 )
        return;

    int height = width * d->originalCardSize.height() / d->originalCardSize.width();
    QSize newSize( width, height );

    if ( newSize != d->currentCardSize )
    {
        d->deleteThread();

        d->currentCardSize = newSize;

        if ( !d->theme.isValid() )
            return;

        cacheInsert( d->cache, lastUsedSizeKey, d->currentCardSize );

        QStringList elementsToRender = d->frontIndex.keys() + d->backIndex.keys();
        d->thread = new RenderingThread( d, d->currentCardSize, elementsToRender );
        d->thread->start();
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


void KAbstractCardDeck::setTheme( const KCardTheme & theme )
{
    if ( theme != d->theme && theme.isValid() )
    {
        d->deleteThread();

        d->theme = theme;

        {
            QMutexLocker l( &(d->rendererMutex) );
            delete d->svgRenderer;
            d->svgRenderer = 0;
        }

        delete d->cache;

        QString cacheName = QString( cacheNameTemplate ).arg( theme.dirName() );
        d->cache = new KImageCache( cacheName, 3 * 1024 * 1024 );
        d->cache->setEvictionPolicy( KSharedDataCache::EvictLeastRecentlyUsed );

        // Enabling the pixmap cache has caused issues: we were getting back
        // different pixmaps than we had inserted. We keep a partial cache of the
        // pixmaps in KAbstractCardDeck already, so the builtin pixmap caching
        // doesn't really add that much benefit anyway.
        d->cache->setPixmapCaching( false );

        if ( d->cache->timestamp() < theme.lastModified().toTime_t() )
        {
            d->cache->clear();
            d->cache->setTimestamp( theme.lastModified().toTime_t() );
        }

        d->originalCardSize = d->unscaledCardSize();
        Q_ASSERT( !d->originalCardSize.isNull() );

        if ( !cacheFind( d->cache, lastUsedSizeKey, &(d->currentCardSize) ) )
        {
            qreal ratio = d->originalCardSize.height() / d->originalCardSize.width();
            d->currentCardSize = QSize( 10, 10 * ratio );
        }
    }
}


KCardTheme KAbstractCardDeck::theme() const
{
    return d->theme;
}


bool KAbstractCardDeck::hasAnimatedCards() const
{
    return !d->cardsWaitedFor.isEmpty();
}

void KAbstractCardDeck::stopAnimations() 
{
    foreach ( KCard * c, d->cardsWaitedFor )
       c->stopAnimation();
    d->cardsWaitedFor.clear();
}

QPixmap KAbstractCardDeck::cardPixmap( quint32 id, bool faceUp )
{
    return d->requestPixmap( id, faceUp );
}



