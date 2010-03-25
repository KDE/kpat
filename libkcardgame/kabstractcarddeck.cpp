/*
 *  Copyright (C) 2009-2010 Parker Coates <parker.coates@kdemail.org>
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

#include "kcardtheme.h"
#include "kcardpile.h"
#include "shuffle.h"

#include <KDebug>
#include <KPixmapCache>

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>

namespace
{
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
    connect( this, SIGNAL(renderingDone(QString,QImage)), d, SLOT(submitRendering(QString,QImage)), Qt::QueuedConnection );
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

    foreach( const QString& element, m_elementsToRender )
    {
        {
            QMutexLocker l( &m_haltMutex );
            if ( m_haltFlag )
                return;
        }

        QString key = keyForPixmap( element, m_size );
        kDebug() << "Renderering" << key << "in rendering thread.";

        QImage img = QImage( m_size, QImage::Format_ARGB32 );
        img.fill( Qt::transparent );
        QPainter p( &img );
        {
            QMutexLocker l( &(d->rendererMutex) );
            d->renderer()->render( &p, element );
        }
        p.end();

        emit renderingDone( key, img );
    }
}


KAbstractCardDeckPrivate::KAbstractCardDeckPrivate( KAbstractCardDeck * q )
  : QObject( q ),
    q( q ),
    cache( 0 ),
    svgRenderer( 0 ),
    thread( 0 )
{
};


KAbstractCardDeckPrivate::~KAbstractCardDeckPrivate()
{
    deleteThread();
    delete cache;
}


// Note that rendererMutex MUST be locked before calling this function.
QSvgRenderer * KAbstractCardDeckPrivate::renderer()
{
    if ( !svgRenderer )
    {
        QString thread = (qApp->thread() == QThread::currentThread()) ? "main" : "rendering";
        kDebug() << QString("Loading card deck SVG in %1 thread").arg( thread );

        svgRenderer = new QSvgRenderer( theme.graphicsFilePath(), this );
    }
    return svgRenderer;
}


QPixmap KAbstractCardDeckPrivate::renderCard( const QString & element )
{
    QPixmap pix;
    if ( !theme.isValid() || currentCardSize.isEmpty() )
        return pix;

    QString key = keyForPixmap( element , currentCardSize );
    if ( !cache->find( key, pix ) )
    {
        kDebug() << "Renderering" << key << "in main thread.";

        pix = QPixmap( currentCardSize );
        pix.fill( Qt::transparent );
        QPainter p( &pix );
        {
            QMutexLocker l( &rendererMutex );
            if ( renderer()->elementExists( element ) )
            {
                renderer()->render( &p, element );
            }
            else
            {
                kWarning() << "Could not find" << element << "in SVG.";
                p.fillRect( QRect( 0, 0, pix.width(), pix.height() ), Qt::white );
                p.setPen( Qt::red );
                p.drawLine( 0, 0, pix.width(), pix.height() );
                p.drawLine( pix.width(), 0, 0, pix.height() );
                p.end();
            }
        }
        p.end();

        cache->insert( key, pix );
    }

    return pix;
}


QSizeF KAbstractCardDeckPrivate::unscaledCardSize()
{
    if( !theme.isValid() )
        return QSizeF();

    const QString element = "back";

    QPixmap pix;
    QString key = theme.dirName() + '_' + element + "_default";
    if ( cache->find( key, pix ) )
        return pix.size();

    QSize size = QSize( 70, 100 ); // Sane default
    bool elementExists;
    {
        QMutexLocker l( &rendererMutex );
        elementExists = renderer()->elementExists( element );
        if ( elementExists )
            size = renderer()->boundsOnElement( element ).size().toSize();
    }

    if ( elementExists )
        cache->insert( key, QPixmap( size ) );

    return size;
}


QPixmap KAbstractCardDeckPrivate::requestPixmap( QString elementId, bool immediate )
{
    QPixmap & stored = elementIdMapping[ elementId ].first;
    if ( stored.size() != currentCardSize )
    {
        if ( immediate || stored.isNull() )
        {
            stored = renderCard( elementId );
        }
        else
        {
            QPixmap pix;
            if ( cache->find( keyForPixmap( elementId , currentCardSize ), pix ) )
                stored = pix;
            else
                stored = stored.scaled( currentCardSize );
        }
    }
    return stored;
}


void KAbstractCardDeckPrivate::updateCardSize( const QSize & size )
{
    currentCardSize = size;

    cache->insert( "lastUsedSize", QPixmap( currentCardSize ) );

    foreach ( KCard * c, cards )
        c->update();

    deleteThread();

    // We have to compile the list of elements to load here, because we can't
    // check the contents of the KPixmapCache from outside the GUI thread.
    QPixmap pix;
    QStringList unrenderedElements;
    CardPixmapHash::const_iterator it = elementIdMapping.constBegin();
    CardPixmapHash::const_iterator end = elementIdMapping.constEnd();
    for ( ; it != end; ++it )
    {
        QString key = keyForPixmap( it.key(), currentCardSize );
        if ( !cache->find( key, pix ) )
            unrenderedElements << it.key();
    }

    thread = new RenderingThread( this, currentCardSize, unrenderedElements );
    thread->start();
}


void KAbstractCardDeckPrivate::deleteThread()
{
    if ( thread && thread->isRunning() )
        thread->halt();
    delete thread;
    thread = 0;
}


void KAbstractCardDeckPrivate::submitRendering( const QString & key, const QImage & image )
{
    QString elementId = key.left( key.indexOf('@') );
    QPair<QPixmap,QList<KCard*> > & pair = elementIdMapping[ elementId ];

    pair.first = QPixmap::fromImage( image );
    cache->insert( key, pair.first );

    foreach ( KCard * c, pair.second )
        c->update();
}


void KAbstractCardDeckPrivate::cardStartedAnimation( KCard * card )
{
    Q_ASSERT( !cardsWaitedFor.contains( card ) );
    cardsWaitedFor.insert( card );
};


void KAbstractCardDeckPrivate::cardStoppedAnimation( KCard * card )
{
    Q_ASSERT( cardsWaitedFor.contains( card ) );
    cardsWaitedFor.remove( card );

    if ( cardsWaitedFor.isEmpty() )
        emit q->cardAnimationDone();
};


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


void KAbstractCardDeck::setDeckContents( QList<quint32> ids )
{
    foreach ( KCard * c, d->cards )
        delete c;
    d->cards.clear();
    d->cardsWaitedFor.clear();

    QHash<QString,QPair<QPixmap,QList<KCard*> > > oldMapping = d->elementIdMapping;
    d->elementIdMapping.clear();

    foreach ( quint32 id, ids )
    {
        KCard * c = new KCard( id, this );

        c->setObjectName( elementName( c->id() ) );

        connect( c, SIGNAL(animationStarted(KCard*)), d, SLOT(cardStartedAnimation(KCard*)) );
        connect( c, SIGNAL(animationStopped(KCard*)), d, SLOT(cardStoppedAnimation(KCard*)) );

        QString elementId = elementName( id, true );
        d->elementIdMapping[ elementId ].second.append( c );

        elementId = elementName( id, false );
        d->elementIdMapping[ elementId ].second.append( c );

        d->cards << c;
    }

    if ( d->currentCardSize.isValid() )
    {
        QHash<QString,QPair<QPixmap,QList<KCard*> > >::iterator it = d->elementIdMapping.begin();
        QHash<QString,QPair<QPixmap,QList<KCard*> > >::iterator end = d->elementIdMapping.end();
        while ( it != end )
        {
            if ( oldMapping.contains( it.key() ) )
                it.value().first = oldMapping[ it.key() ].first;

            it.value().first = d->requestPixmap( it.key(), false );

            ++it;
        }
    }
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

    if ( newSize != d->currentCardSize )
    {
        foreach ( KCard * c, d->cards )
            c->prepareGeometryChange();

        d->updateCardSize( newSize );
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
    Q_ASSERT ( theme.isValid() );

    d->deleteThread();

    d->theme = theme;

    {
        QMutexLocker l( &(d->rendererMutex) );
        delete d->svgRenderer;
        d->svgRenderer = 0;
    }

    delete d->cache;
    d->cache = new KPixmapCache( QString( "kdegames-cards_%1" ).arg( theme.dirName() ) );
    if ( d->cache->timestamp() < theme.lastModified().toTime_t() )
    {
        d->cache->discard();
        d->cache->setTimestamp( theme.lastModified().toTime_t() );
    }

    d->originalCardSize = d->unscaledCardSize();
    Q_ASSERT( !d->originalCardSize.isNull() );

    QPixmap pix( 10, 10 * d->originalCardSize.height() / d->originalCardSize.width() );
    d->cache->find( "lastUsedSize", pix );
    d->currentCardSize = pix.size();
}


KCardTheme KAbstractCardDeck::theme() const
{
    return d->theme;
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

    painter->setRenderHint( QPainter::SmoothPixmapTransform );
    painter->drawPixmap( 0, 0, pix );
}


#include "kabstractcarddeck.moc"
#include "kabstractcarddeck_p.moc"
