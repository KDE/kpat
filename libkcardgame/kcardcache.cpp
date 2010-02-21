/*
 *  Copyright (C) 2008 Andreas Pakulat <apaku@gmx.de>
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include "kcardcache.h"
#include "kcardcache_p.h"

#include "carddeckinfo.h"

#include <KDebug>
#include <KPixmapCache>

#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QFileInfo>
#include <QtCore/QSizeF>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>

QString keyForPixmap( const QString & element, const QSize & s )
{
    return element + '@'+ QString::number( s.width() ) + 'x' + QString::number( s.height() );
}

QSvgRenderer* KCardCache2Private::renderer()
{
    if ( !svgRenderer )
    {
        kDebug() << "Loading front SVG renderer";
        svgRenderer = new QSvgRenderer( theme.graphicsFilePath() );
    }
    return svgRenderer;
}

void KCardCache2Private::stopThread()
{
    if ( loadThread && loadThread->isRunning() )
    {
        loadThread->kill();
        loadThread->wait();
    }
    delete loadThread;
    loadThread = 0;
}

void KCardCache2Private::submitRendering( const QString& key, const QImage& image )
{
    kDebug() << "Received render of" << key << "from rendering thread.";
    QPixmap pix = QPixmap::fromImage( image );
    cache->insert( key, pix );

    emit q->renderingDone( key.left( key.indexOf('@') ), pix );
}

LoadThread::LoadThread( KCardCache2Private * d, QSize size, const QString & theme, const QStringList & elements )
  : d( d ),
    size( size ),
    theme( theme ),
    elementsToRender( elements ),
    doKill( false ),
    killMutex( new QMutex )
{
}

void LoadThread::kill()
{
    QMutexLocker l(killMutex);
    doKill = true;
}

void LoadThread::run()
{
    {
        // Load the renderer even if we don't have any elements to render.
        QMutexLocker l( d->rendererMutex );
        d->renderer();
    }

    foreach( const QString& element, elementsToRender )
    {
        {
            QMutexLocker l( killMutex );
            if ( doKill )
                return;
        }

        QImage img = QImage( size, QImage::Format_ARGB32 );
        img.fill( Qt::transparent );
        QPainter p( &img );
        {
            QMutexLocker l( d->rendererMutex );
            d->renderer()->render( &p, element );
        }
        p.end();

        QString key = keyForPixmap( element, size );
        emit renderingDone( key, img );
    }
}

KCardCache2::KCardCache2( const KCardTheme & theme )
    : d( new KCardCache2Private( this ) )
{
    d->theme = theme;
    d->svgRenderer = 0;
    d->rendererMutex = new QMutex();
    d->loadThread = 0;

    d->cache = new KPixmapCache( QString( "kdegames-cards_%1" ).arg( theme.dirName() ) );

    QDateTime dt = theme.lastModified();
    if ( d->cache->timestamp() < dt.toTime_t() )
    {
        d->cache->discard();
        d->cache->setTimestamp( dt.toTime_t() );
    }
}

KCardCache2::~KCardCache2()
{
    if ( d->loadThread && d->loadThread->isRunning() )
        d->loadThread->kill();

    delete d->loadThread;
    delete d->cache;
    delete d->svgRenderer;
    delete d->rendererMutex;
}

void KCardCache2::setSize( const QSize& s )
{
    d->stopThread();

    if ( s != d->size )
        d->size = s;
}

QSize KCardCache2::size() const
{
    return d->size;
}


KCardTheme KCardCache2::theme() const
{
    return d->theme;
}


QPixmap KCardCache2::renderCard( const QString & element ) const
{
    QPixmap pix;
    if ( !d->theme.isValid() || d->size.isEmpty() )
        return pix;

    QString key = keyForPixmap( element , d->size );
    if ( !d->cache->find( key, pix ) )
    {
        kDebug() << "Renderering" << key << "in main thread.";

        pix = QPixmap( d->size );
        pix.fill( Qt::transparent );
        QPainter p( &pix );
        {
            QMutexLocker l( d->rendererMutex );
            if ( d->renderer()->elementExists( element ) )
            {
                d->renderer()->render( &p, element );
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
        if ( d->cache )
            d->cache->insert( key, pix );
    }

    return pix;
}


QPixmap KCardCache2::renderCardIfCached( const QString & element ) const
{
    QPixmap pix;
    if ( d->theme.isValid() && !d->size.isEmpty() )
        d->cache->find( keyForPixmap( element , d->size ), pix );
    return pix;
}


QSizeF KCardCache2::naturalSize( const QString & element ) const
{
    if( !d->theme.isValid() )
        return QSizeF();

    QPixmap pix;
    QString key = d->theme.dirName() + '_' + element + "_default";
    if ( d->cache && d->cache->find( key, pix ) )
        return pix.size();

    QSize size = QSize( 70, 100 ); // Sane default
    bool elementExists;
    {
        QMutexLocker( d->rendererMutex );
        elementExists = d->renderer()->elementExists( element );
        if ( elementExists )
            size = d->renderer()->boundsOnElement( element ).size().toSize();
    }

    if ( elementExists && d->cache )
        d->cache->insert( key, QPixmap( size ) );

    return size;
}


void KCardCache2::insertOther( const QString & key, const QPixmap & pix )
{
    d->cache->insert( key, pix );
}


bool KCardCache2::findOther (const QString & key, QPixmap & pix )
{
    return d->cache->find( key, pix );
}


QDateTime KCardCache2::timestamp() const
{
    if ( d->cache )
        return QDateTime::fromTime_t( d->cache->timestamp() );
    else
        return QDateTime();
}


void KCardCache2::invalidateCache()
{
    if ( d->cache )
        d->cache->discard();
}


void KCardCache2::loadInBackground( const QStringList & elements )
{
    d->stopThread();

    // We have to compile the list of elements to load here, because we can't
    // check the contents of the KPixmapCache from outside the GUI thread.
    QPixmap pix;
    QStringList unrenderedElements;
    foreach ( const QString & element, elements )
    {
        QString key = keyForPixmap( element, d->size );
        if ( d->cache && !d->cache->find( key, pix ) )
            unrenderedElements << element;
    }

    d->loadThread = new LoadThread( d, d->size, d->theme.dirName(), unrenderedElements );
    d->connect( d->loadThread, SIGNAL(renderingDone(QString,QImage)), SLOT(submitRendering(QString,QImage)), Qt::QueuedConnection );
    d->loadThread->start();
}


#include "kcardcache.moc"
#include "kcardcache_p.moc"
