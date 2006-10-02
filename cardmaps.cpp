/***********************-*-C++-*-********

  cardmaps.cpp  defines pixmaps for playing cards

     Copyright (C) 1995  Paul Olav Tvete

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

****************************************/

#include <stdio.h>
#include <unistd.h>

#include <qpainter.h>
//Added by qt3to4:
#include <QPixmap>
#include <QTime>

#include <kconfig.h>

#include <config.h>
#include "cardmaps.h"
#include <stdlib.h>

#include <config.h>
#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include "version.h"
#include <kstaticdeleter.h>
#include <qimage.h>
#include <kimageeffect.h>
#include <kcarddialog.h>
#include <kglobalsettings.h>
#include <assert.h>
#include <kglobal.h>
#include <QSvgRenderer>
#include <QPixmapCache>
#include <QThread>
#include <QMutex>
#include "dealer.h"
#include "ksvgrenderer.h"

cardMap *cardMap::_self = 0;
static KStaticDeleter<cardMap> cms;

typedef QMap<QString, QImage> CardCache;

class cardMapPrivate;

class cardMapThread : public QThread
{
public:
    QImage renderCard( const QString &element)
    {
        QImage img = QImage( ( int )cardMap::self()->wantedCardWidth(), ( int )cardMap::self()->wantedCardHeight(), QImage::Format_ARGB32 );
        img.fill( qRgba( 0, 0, 255, 0 ) );
        QPainter p( &img );
        m_renderer_mutex.lock();
        m_renderer->render( &p, element, QRectF( 0, 0, img.width(), img.height() ) );
        m_renderer_mutex.unlock();
        p.end();
        return img;
    }
    virtual void run();

    cardMapThread( cardMapPrivate *_cmp);

    ~cardMapThread() {
        delete m_renderer;
    }
    void setRenderer( QSvgRenderer *_renderer ) {
        delete m_renderer;
        m_renderer = _renderer;
    }
    void finish() {
        m_shouldEnd = true;
    }
private:
    QSvgRenderer *m_renderer;
    QMutex m_renderer_mutex;
    cardMapPrivate *d;
    bool m_shouldEnd;
};

class cardMapPrivate
{
public:

    double _wantedCardWidth;
    mutable double _scale;
    cardMapThread *m_thread;
    CardCache m_cache;
    QMutex m_cacheMutex;
    QSizeF m_backSize;
};

cardMap::cardMap() : QObject()
{
    d = new cardMapPrivate();

    d->m_thread = new cardMapThread(d);

    assert(!_self);
    KConfig *config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    d->_wantedCardWidth = config->readEntry( "CardWith", 70 );

    kDebug(11111) << "cardMap\n";

    QSvgRenderer *renderer = new KSvgRenderer( KStandardDirs::locate( "data", "carddecks/svg-ornamental/ornamental.svgz" ) );
    d->m_thread->setRenderer( renderer );
    d->m_backSize = renderer->boundsOnElement( "back" ).size();
    QPixmapCache::setCacheLimit(5 * 1024 * 1024);

    cms.setObject(_self, this);
//    kDebug(11111) << "card " << CARDX << " " << CARDY << endl;
}

cardMap::~cardMap()
{
    d->m_thread->finish();
    d->m_thread->wait();
    delete d->m_thread;
    delete d;
}

double cardMap::scaleFactor() const
{
    if ( !d->_scale ) {
        d->_scale = wantedCardWidth() / d->m_backSize.width();
    }
    return d->_scale;
}

double cardMap::wantedCardHeight() const
{
    return d->_wantedCardWidth / d->m_backSize.width() * d->m_backSize.height();
}

double cardMap::wantedCardWidth() const
{
    return d->_wantedCardWidth;
}

void cardMap::setWantedCardWidth( double w )
{
    kDebug() << "setWantedCardWidth " << w << endl;
    if ( w > 200 || w < 10 )
        return;

    d->_wantedCardWidth = w;
    d->_scale = 0;
    if ( d->m_thread->isRunning() )
    {
        d->m_thread->disconnect();
        d->m_thread->finish();
        connect( d->m_thread, SIGNAL( finished() ), SLOT( slotThreadEnded() ) );
    } else // start directly
        slotThreadEnded();
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

void cardMap::slotThreadFinished()
{
    KConfig *config = KGlobal::config();
    KConfigGroup cs(config, settings_group );
    config->writeEntry( "CardWith", d->_wantedCardWidth );
    config->sync();
    Dealer::instance()->dscene()->rescale();
}

void cardMap::slotThreadEnded()
{
    d->m_thread->start(QThread::IdlePriority);
    d->m_thread->disconnect();
    connect( d->m_thread, SIGNAL( finished() ), SLOT( slotThreadFinished() ) );
}

QPixmap cardMap::renderCard( const QString &element )
{
    QImage img;
    d->m_cacheMutex.lock();
    if ( !d->m_cache.contains( element ) )
    {
        img = d->m_thread->renderCard( element );
        d->m_cache[element] = img;
    } else
        img = d->m_cache[element];
    d->m_cacheMutex.unlock();

    QMatrix matrix;
    matrix.scale( cardMap::self()->wantedCardWidth() / img.width(),
                  cardMap::self()->wantedCardHeight() / img.height() );

    return QPixmap::fromImage( img ).transformed(matrix);
}

cardMapThread::cardMapThread( cardMapPrivate *_cmp )
{
    d = _cmp;
    m_renderer = 0;
}

void cardMapThread::run()
{
    m_shouldEnd = false;
    msleep( 150 );
    if ( m_shouldEnd )
    {
        kDebug() << "exiting thread\n";
        return;
    }
    d->m_cacheMutex.lock();
    QStringList keys = d->m_cache.keys();
    d->m_cacheMutex.unlock();

    for ( QStringList::const_iterator it = keys.begin(); it != keys.end(); ++it )
    {
        if ( m_shouldEnd )
        {
            kDebug() << "exiting thread\n";
            return;
        }
        QImage img = renderCard( *it );
        d->m_cacheMutex.lock();
        d->m_cache[*it] = img;
        d->m_cacheMutex.unlock();
    }
    kDebug() << "returning from thread\n";
}

#include "cardmaps.moc"
