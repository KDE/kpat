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

cardMap::cardMap(const QColor &dim) : dimcolor(dim), _renderer( 0 )
{
    assert(!_self);

    card_width = 0;
    card_height = 0;
    _wantedCardSize = 72;

    kDebug(11111) << "cardMap\n";
    KConfig *config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    QString bg = cs.readEntry( "Back", KCardDialog::getDefaultDeck());
    setBackSide( bg, false);

    QString dir = cs.readEntry("Cards",  KCardDialog::getDefaultCardDir());
    setCardDir( dir );

    cms.setObject(_self, this);
//    kDebug(11111) << "card " << CARDX << " " << CARDY << endl;
}

bool cardMap::setCardDir( const QString &dir )
{
    delete _renderer;

    _renderer = new QSvgRenderer( KStandardDirs::locate( "data", "carddecks/svg-ornamental/ornamental.svg" ) );
    QPixmapCache::setCacheLimit(5 * 1024 * 1024);

    kDebug() << "setCardDir " << dir << endl;
    KConfig *config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    QWidget* w = 0;
    QPainter p;
    QTime t1, t2;

    QString imgname = KCardDialog::getCardPath(dir, 11);

    QImage image;
    image.load( imgname );
    if( image.isNull()) {
        kDebug(11111) << "cannot load card pixmap \"" << imgname << "\" in " << dir << "\n";
        p.end();
        delete w;
        return false;
    }

    int old_card_width = card_width;
    int old_card_height = card_height;

    card_width = image.width();
    card_height = image.height();

    setBackSide(back, true);

    for(int idx = 1; idx < 53; idx++)
    {
        // translate index to suit/rank
        // this is necessary since kpoker uses another
        // mapping in the pictures
        int rank = (idx - 1) / 4;
        if(rank != 0)
            rank = 13 - rank;
        int suit = 0;
        switch((idx - 1) % 4) {
            case 0:
                suit = 0;
                break;
            case 1:
                suit = 3;
                break;
            case 2:
                suit = 2;
                break;
            case 3:
                suit = 1;
                break;
        }

        imgname = KCardDialog::getCardPath(dir, idx);
        image.load(imgname);

        if( image.isNull() || image.width() != card_width || image.height() != card_height ) {
            kDebug(11111) << "cannot load card pixmap \"" << imgname << "\" in (" << idx << ") " << dir << "\n";
            p.end();
            delete w;
            card_width = old_card_width;
            card_height = old_card_height;

            setBackSide(back, true);
            return false;
        }

        img[rank][suit].normal = QPixmap::fromImage(image);
    }

    return true;
}

bool cardMap::setBackSide( const QPixmap &pm, bool scale )
{
    if (pm.isNull())
        return false;

    back = pm;

    if(scale && (back.width() != card_width ||
                 back.height() != card_height))
    {
        kDebug(11111) << "scaling back!!\n";
        // scale to fit size
        QMatrix wm;
        wm.scale(((float)(card_width))/back.width(),
                 ((float)(card_height))/back.height());
        back = back.transformed(wm);
    }

    return true;
}

int cardMap::CARDX() {
    return self()->card_width; // 72;
}

int cardMap::CARDY() {
    return self()->card_height; // 96;
}

QPixmap cardMap::backSide() const
{
    return back;
}

double cardMap::scaleFactor() const
{
    if ( !_scale ) {
        QRectF be = cardMap::self()->renderer()->boundsOnElement( "back" );
        _scale = wantedCardWidth() / be.width();
    }
    return _scale;
}

double cardMap::wantedCardHeight() const
{
    QRectF be = cardMap::self()->renderer()->boundsOnElement( "back" );
    return be.height() * _scale;
}

double cardMap::wantedCardWidth() const
{
    return _wantedCardSize;
}

void cardMap::setWantedCardWidth( double w )
{
    if ( w > 200 || w < 10 )
        return;

    _wantedCardSize = w;
    _scale = 0;
}

QPixmap cardMap::image( Card::Rank _rank, Card::Suit _suit) const
{
    if( 1 <= _rank && _rank <= 13
	&& 1 <= _suit && _suit <= 4 )
    {
        return img[ _rank - 1 ][ _suit - 1 ].normal;
    }
    else
    {
        kError() << "access to invalid card " << int(_rank) << ", " << int(_suit) << endl;
    }
    return 0;
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

