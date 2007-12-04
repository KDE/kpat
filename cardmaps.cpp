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

#include "cardmaps.h"

#include <stdio.h>
#include <unistd.h>

#include <qpainter.h>
//Added by qt3to4:
#include <QPixmap>
#include <QTime>

#include <kconfig.h>

#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include "version.h"
#include <qimage.h>
#include <krandom.h>
#include <kcarddialog.h>
#include <kglobalsettings.h>
#include <qfileinfo.h>
#include <QDir>
#include <assert.h>
#include <kglobal.h>
#include <QSvgRenderer>
#include <QPixmapCache>
#include <QThread>
#include <QMutex>
#include <kconfiggroup.h>
#ifdef __GNUC__
#warning cardmap should not really require to know the instance!
#endif
#include "view.h"
#include "dealer.h"
#include "ksvgrenderer.h"

cardMap *cardMap::_self = 0;

typedef QMap<QString, QImage> CardCache;

class cardMapPrivate;

class cardMapThread : public QThread
{
public:
    QImage renderCard( const QString &element);
    virtual void run();

    cardMapThread( cardMapPrivate *_cmp);

    ~cardMapThread() {
        delete m_renderer;
    }
    QSvgRenderer *renderer();
    void reset() {
        delete m_renderer;
        m_renderer = 0;
    }

    void finish() {
        m_shouldEnd = true;
    }
    QImage renderRaster( const QString &element );

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
    QString m_svg;
    QRect m_body;
};

QImage cardMapThread::renderRaster( const QString &element )
{
    const char *oldnames[] = { "back",
                               "1_club",
                               "1_spade",
                               "1_heart",
                               "1_diamond",
                               "king_club",
                               "king_spade",
                               "king_heart",
                               "king_diamond",
                               "queen_club",
                               "queen_spade",
                               "queen_heart",
                               "queen_diamond",
                               "jack_club",
                               "jack_spade",
                               "jack_heart",
                               "jack_diamond",
                               "10_club",
                               "10_spade",
                               "10_heart",
                               "10_diamond",
                               "9_club",
                               "9_spade",
                               "9_heart",
                               "9_diamond",
                               "8_club",
                               "8_spade",
                               "8_heart",
                               "8_diamond",
                               "7_club",
                               "7_spade",
                               "7_heart",
                               "7_diamond",
                               "6_club",
                               "6_spade",
                               "6_heart",
                               "6_diamond",
                               "5_club",
                               "5_spade",
                               "5_heart",
                               "5_diamond",
                               "4_club",
                               "4_spade",
                               "4_heart",
                               "4_diamond",
                               "3_club",
                               "3_spade",
                               "3_heart",
                               "3_diamond",
                               "2_club",
                               "2_spade",
                               "2_heart",
                               "2_diamond",
                               0 };

    int index = 0;
    while ( oldnames[index] && element != QString::fromLatin1( oldnames[index] ) )
        index++;
    Q_ASSERT( oldnames[index] );
    QString filename = KStandardDirs::locate( "data", QString( "carddecks/%1/%2.png" ).arg( d->m_cacheDir ).arg( index ) );
    if ( index == 0 )
        filename = KStandardDirs::locate( "data", "carddecks/decks/deck0.png" );
    QImage img;
    bool ret = img.load( filename, "PNG" );
    return img;
}

QImage cardMapThread::renderCard( const QString &element)
{
    QImage img;
    if ( d->m_svg.isEmpty() )
        img = renderRaster( element );
    else {
        img = QImage( ( int )cardMap::self()->wantedCardWidth(), ( int )(cardMap::self()->wantedCardHeight() + 1), QImage::Format_ARGB32 );
        img.fill( qRgba( 0, 0, 255, 0 ) );
        QPainter p( &img );
        m_renderer_mutex.lock();
        renderer()->render( &p, element );
        m_renderer_mutex.unlock();
        p.end();
    }

    QString filename = KStandardDirs::locateLocal( "data", QString( "carddecks/%1/%2/" ).
                                                   arg( d->m_cacheDir ).
                                                   arg( ( int )cardMap::self()->wantedCardWidth() ) +
                                                   element + ".png");
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

QSvgRenderer *cardMapThread::renderer()
{
    if ( m_renderer )
        return m_renderer;
    //kDebug() << "new renderer" << kBacktrace();
    Q_ASSERT( !d->m_svg.isEmpty() );

    m_renderer = new KSvgRenderer( d->m_svg );
    return m_renderer;
}

cardMap::cardMap() : QObject()
{
    d = new cardMapPrivate();

    d->m_thread = new cardMapThread(d);

    assert(!_self);
    kDebug(11111) << "cardMap\n";
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    d->_wantedCardWidth = cs.readEntry( "CardWith", 100 );
    updateTheme(cs);
    _self = this;
}

void cardMap::updateTheme(const KConfigGroup &cs)
{
    d->m_thread->disconnect();
    d->m_thread->finish();
    d->m_thread->reset();

    QString theme = cs.readEntry( "Theme", "Oxygen (SVG)" );
    d->m_cardDeck = KCardDialog::cardDir(theme);
    
    if( d->m_cardDeck.isEmpty() )
        KCardDialog::cardDir("Oxygen (SVG)");
    
    d->m_cardDeck += "index.desktop";

    KConfig fi( d->m_cardDeck, KConfig::SimpleConfig);
    KConfigGroup ficg(&fi, "KDE Backdeck");

    QString cache = d->m_cardDeck;
    cache = KStandardDirs::realFilePath( cache );
    cache = cache.left( cache.length() - strlen( "/index.desktop" ) );
    cache = cache.mid( cache.lastIndexOf( '/' ) + 1 );
    d->m_cacheDir = cache;

    d->_scale = 0;
    d->m_body = QRect();

    d->m_svg = ficg.readEntry( "SVG" );
    if ( !d->m_svg.isEmpty() )
        d->m_svg = QFileInfo(d->m_cardDeck).dir().absoluteFilePath(d->m_svg);

    d->m_backSize = ficg.readEntry("BackSize", QSizeF() );
    registerCard( "back" );
    if ( d->m_backSize.isNull() || !d->m_backSize.isValid() )
    {
        if ( !d->m_svg.isEmpty() )
        {
            d->m_backSize = d->m_thread->renderer()->boundsOnElement( "back" ).size();
        }
        kDebug(11111) << d->m_cardDeck << " " << d->m_backSize;
    }

    Q_ASSERT( !d->m_backSize.isNull() );
    if ( d->m_svg.isEmpty() )
        setWantedCardWidth( d->m_backSize.width() );

    if (PatienceView::instance() && PatienceView::instance()->dscene()) {
        PatienceView::instance()->dscene()->rescale(false);
        if ( !d->m_svg.isEmpty() )
            PatienceView::instance()->dscene()->relayoutPiles();
    }
}

cardMap::~cardMap()
{
    d->m_thread->finish();
    d->m_thread->wait();
    delete d->m_thread;
    delete d;
    if (_self == this)
      _self = 0;
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

void cardMap::triggerRescale()
{
    if ( d->m_thread->isRunning() )
    {
        d->m_thread->disconnect();
        d->m_thread->finish();
        connect( d->m_thread, SIGNAL( finished() ), SLOT( slotThreadEnded() ) );
    } else { // start directly
        slotThreadEnded();
    }
}

void cardMap::setWantedCardWidth( double w )
{
    if ( w > 200 || w < 10 || d->_wantedCardWidth == w )
        return;

    if ( d->m_svg.isEmpty() && w != d->m_backSize.width() )
        return;

    d->m_body = QRect();
    kDebug(11111) << "setWantedCardWidth" << w << " " << d->_wantedCardWidth;
    d->_wantedCardWidth = w;
    d->_scale = 0;
    if (PatienceView::instance()->dscene())
        PatienceView::instance()->dscene()->rescale(false);
    triggerRescale();
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

void cardMap::slotThreadFinished()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );
    cs.writeEntry( "CardWith", d->_wantedCardWidth );
    config->sync();
    if (  PatienceView::instance()->dscene() )
         PatienceView::instance()->dscene()->rescale(false);
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

QRect cardMap::opaqueRect() const { return d->m_body; }

QPixmap cardMap::renderCard( const QString &element )
{
    QImage img;
    d->m_cacheMutex.lock();
    if ( d->m_cache.contains( element ) )
        img = d->m_cache[element];
    d->m_cacheMutex.unlock();

    if ( img.isNull() )
    {
        QString filename = KStandardDirs::locate( "data", QString( "carddecks/%1/%2/" ).arg( d->m_cacheDir ).arg( ( int )cardMap::self()->wantedCardWidth() ) + element + ".png");
        if ( !filename.isEmpty() )
            img = QImage( filename );

        if ( img.isNull() ) {
            img = d->m_thread->renderCard( element );
            Q_ASSERT( !img.isNull() );
            //triggerRescale();
        }

        d->m_cacheMutex.lock();
        d->m_cache[element] = img;
        d->m_cacheMutex.unlock();

        if ( element == "back" && img.width() == cardMap::self()->wantedCardWidth()  )
        {
            d->m_body = QRect( QPoint( 0, 0 ), img.size() );
            while ( d->m_body.height() > 0 && qAlpha( img.pixel( d->m_body.left(), d->m_body.top() ) ) != 255 )
                d->m_body.setTop( d->m_body.top() + 1 );
            while ( d->m_body.height() > 0 && qAlpha( img.pixel( d->m_body.left(), d->m_body.bottom() - 1 ) ) != 255 )
                d->m_body.setBottom( d->m_body.bottom() - 1 );

            int left = 0;
            while ( left < img.width() && qAlpha( img.pixel( left, d->m_body.top() ) ) != 255 )
                left++;
            int right = 0;
            while ( right < img.width() && qAlpha( img.pixel( img.width() - right - 1, d->m_body.top() ) ) != 255 )
                right++;
        } else if ( element == "back" )
            d->m_body = QRect();
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
        kDebug(11111) << "exiting thread\n";
        return;
    }
    d->m_cacheMutex.lock();
    QStringList keys = d->m_cache.keys();
    d->m_cacheMutex.unlock();

    for ( QStringList::const_iterator it = keys.begin(); it != keys.end(); ++it )
    {
        if ( m_shouldEnd )
        {
            kDebug(11111) << "exiting thread\n";
            return;
        }
        QImage img = renderCard( *it );
        d->m_cacheMutex.lock();
        d->m_cache[*it] = img;
        d->m_cacheMutex.unlock();
    }
    kDebug(11111) << "returning from thread\n";
}

#include "cardmaps.moc"
