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

cardMap *cardMap::_self = 0;
static KStaticDeleter<cardMap> cms;

typedef QMap<QString, QImage> CardCache;

class cardMapPrivate
{
public:
    QSvgRenderer *_renderer;
    double _wantedCardWidth;
    mutable double m_aspectRatio;
    mutable double _scale;
    CardCache m_cache;
};

cardMap::cardMap()
{
    d = new cardMapPrivate();
    d->m_aspectRatio = 1;

    assert(!_self);

    d->_wantedCardWidth = 80;

    kDebug(11111) << "cardMap\n";
    KConfig *config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    d->_renderer = new QSvgRenderer( KStandardDirs::locate( "data", "carddecks/svg-ornamental/ornamental.svg" ) );
    QPixmapCache::setCacheLimit(5 * 1024 * 1024);

    cms.setObject(_self, this);
//    kDebug(11111) << "card " << CARDX << " " << CARDY << endl;
}

cardMap::~cardMap()
{
    delete d->_renderer;
    delete d;
}

double cardMap::scaleFactor() const
{
    if ( !d->_scale ) {
        QRectF be = d->_renderer->boundsOnElement( "back" );
        d->_scale = wantedCardWidth() / be.width();
        d->m_aspectRatio = be.width() / be.height();
    }
    return d->_scale;
}

double cardMap::wantedCardHeight() const
{
    return d->_wantedCardWidth / d->m_aspectRatio;
}

double cardMap::wantedCardWidth() const
{
    return d->_wantedCardWidth;
}

void cardMap::setWantedCardWidth( double w )
{
    if ( w > 200 || w < 10 )
        return;

    d->_wantedCardWidth = w;
    d->_scale = 0;
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

QPixmap cardMap::renderCard( const QString &element )
{
    QImage img;
    if ( !d->m_cache.contains( element ) )
    {
        img = QImage( ( int )cardMap::self()->wantedCardWidth(), ( int )cardMap::self()->wantedCardHeight(), QImage::Format_ARGB32 );
        img.fill( qRgba( 0, 0, 255, 0 ) );
        QPainter p( &img );
        d->_renderer->render( &p, element, QRectF( 0, 0, img.width(), img.height() ) );
        p.end();
        d->m_cache[element] = img;
    } else
        img = d->m_cache[element];

    QMatrix matrix;
    matrix.scale( cardMap::self()->wantedCardWidth() / img.width(),
                  cardMap::self()->wantedCardHeight() / img.height() );

    return QPixmap::fromImage( img ).transformed(matrix);
}
