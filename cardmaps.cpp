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
#include <krandom.h>
#include <kimageeffect.h>
#include <kcarddialog.h>
#include <kglobalsettings.h>
#include <qfileinfo.h>
#include <QDir>
#include <assert.h>
#include <ksimpleconfig.h>
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
        renderer()->render( &p, element, QRectF( 0, 0, img.width(), img.height() ) );
        m_renderer_mutex.unlock();
        p.end();
        QString filename = KStandardDirs::locateLocal( "data", "carddecks/svg-nicu-white/83/" + element + ".png");
        QFile m( filename );
        if ( m.open(  QIODevice::WriteOnly ) )
        {
            bool ret = img.save( &m, "PNG" );
            m.close();
            if ( !ret )
               m.remove();
        }

        return img;
    }
    virtual void run();

    cardMapThread( cardMapPrivate *_cmp);

    ~cardMapThread() {
        delete m_renderer;
    }
    QSvgRenderer *renderer();

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
    QString m_cacheDir;
    QString m_cardDeck;
};

QSvgRenderer *cardMapThread::renderer()
{
    if ( m_renderer )
        return m_renderer;
    kDebug() << "new renderer\n";
    m_renderer = new KSvgRenderer( d->m_cardDeck );
    return m_renderer;
}

QString cardMap::pickRandom()
{
    QStringList list = KGlobal::dirs()->findAllResources("data", "carddecks/*/index.desktop");
    QStringList svgs;
    for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
       KSimpleConfig fi(*it, true);
       fi.setGroup("KDE Backdeck");
       QString svg = fi.readEntry("SVG");
       if (!svg.isNull()) {
         svg = QFileInfo(*it).dir().absoluteFilePath(svg);
         assert( QFile::exists(svg) );
         svgs.append(*it);
       }
    }
    assert(svgs.size());
    int hit = KRandom::random() % svgs.size();
    kDebug() << svgs << " " << hit << " " << svgs.size() << " " << svgs[hit] << endl;
    return svgs[ hit ];
}

cardMap::cardMap() : QObject()
{
    d = new cardMapPrivate();

    d->m_thread = new cardMapThread(d);

    assert(!_self);
    KConfig *config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    d->_wantedCardWidth = config->readEntry( "CardWith", 100 );

    kDebug(11111) << "cardMap\n";

    d->m_cardDeck = KStandardDirs::locate("data", "carddecks/svg-nicu-white/index.desktop");
    // d->m_cardDeck = pickRandom();

    KSimpleConfig fi(d->m_cardDeck, true);
    fi.setGroup("KDE Backdeck");
    d->m_backSize = fi.readEntry("BackSize", QSizeF( ));
    if ( !d->m_backSize.isValid() )
        d->m_backSize = d->m_thread->renderer()->boundsOnElement( "back" ).size();
    kDebug() << "back " << d->m_backSize << endl;
    d->m_cardDeck = QFileInfo(d->m_cardDeck).dir().absoluteFilePath(fi.readEntry( "SVG" ));

    d->m_cacheDir = KGlobal::dirs()->findResourceDir( "data", "carddecks/svg-nicu-white/83/" );
    kDebug() << "cacheDir " << d->m_cacheDir << endl;
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
    kDebug() << "setWantedCardWidth " << w << " " << d->_wantedCardWidth << endl;

    if ( w > 200 || w < 10 || d->_wantedCardWidth == w )
        return;

    d->_wantedCardWidth = w;
    d->_scale = 0;
    Dealer::instance()->dscene()->rescale(false);
    if ( d->m_thread->isRunning() )
    {
        d->m_thread->disconnect();
        d->m_thread->finish();
        connect( d->m_thread, SIGNAL( finished() ), SLOT( slotThreadEnded() ) );
    } else { // start directly
        slotThreadEnded();
    }
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
    Dealer::instance()->dscene()->rescale(false);
}

void cardMap::slotThreadEnded()
{
    d->m_thread->start(QThread::IdlePriority);
    d->m_thread->disconnect();
    connect( d->m_thread, SIGNAL( finished() ), SLOT( slotThreadFinished() ) );
}

void cardMap::registerCard( const QString &element )
{
    d->m_cacheMutex.lock();
    d->m_cache[element] = QImage();
    d->m_cacheMutex.unlock();
}

QPixmap cardMap::renderCard( const QString &element )
{
    QImage img;
    d->m_cacheMutex.lock();
    if ( d->m_cache.contains( element ) )
        img = d->m_cache[element];
    d->m_cacheMutex.unlock();

    if ( img.isNull() )
    {
        QString filename = KStandardDirs::locate( "data", "carddecks/svg-nicu-white/83/" + element + ".png");
        if ( !filename.isNull() )
            img = QImage( filename );
        if ( img.isNull() )
            img = d->m_thread->renderCard( element );

        d->m_cacheMutex.lock();
        d->m_cache[element] = img;
        d->m_cacheMutex.unlock();
    }

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
