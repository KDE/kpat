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
#include <qcolor.h>
#include <qwmatrix.h>
#include <qdrawutil.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qdatetime.h>

#include <kconfig.h>
#include <kglobal.h>
#include <kiconloader.h>

#include <config.h>
#include "cardmaps.h"
#include <stdlib.h>

#include <config.h>
#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>
#include "version.h"
#include <kstaticdeleter.h>
#include <qimage.h>
#include <kimageeffect.h>
#include <kcarddialog.h>
#include <assert.h>

int cardMap::CARDX = 72;
int cardMap::CARDY = 96;

cardMap *cardMap::_self = 0;
static KStaticDeleter<cardMap> cms;

cardMap::cardMap(const QColor &dim) : dimcolor(dim)
{
    assert(!_self);

    kdDebug(11111) << "cardMap\n";
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );

    QString bg = config->readEntry( "Back", KCardDialog::getRandomDeck());
    if (!setBackSide( bg )) {
        bg = KCardDialog::getRandomDeck();
        config->writeEntry( "Back", bg);
        setBackSide( bg );
    }

    QString dir = config->readEntry("Cards",  KCardDialog::getRandomCardDir());
    if (!setCardDir( dir )) {
        dir = KCardDialog::getRandomCardDir();
        config->writeEntry("Cards", dir);
        setCardDir( dir );
    }

    _self = cms.setObject(this);
//    kdDebug(11111) << "card " << CARDX << " " << CARDY << endl;
}

bool cardMap::setCardDir( const QString &dir)
{
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );

    // create an animation window while loading pixmaps (this
    // may take a while (approx. 3 seconds on my AMD K6PR200)
    bool animate = config->readBoolEntry( "Animation", true);

    QWidget* w = 0;
    QPainter p;
    QTime t1, t2;

    if( animate ) {
        t1 = QTime::currentTime();
        w = new QWidget( 0, "", Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_Tool );
        QWidget* dt = qApp->desktop();
        w->setBackgroundColor( Qt::darkGreen );
        w->setGeometry( ( dt->width() - 510 ) / 2, ( dt->height() - 180 ) / 2, 510, 180);
        w->show();
        qApp->processEvents();

        p.begin( w );
        p.drawText(0, 150, 510, 20, Qt::AlignCenter,
                   i18n("please wait, loading cards..."));

        p.setFont(QFont("Times", 24));
        p.drawText(0, 0, 510, 40, Qt::AlignCenter,
                   i18n("KPatience - a Solitaire game"));

        p.setPen(QPen(QColor(0, 0, 0), 4));
        p.setBrush(Qt::NoBrush);
        p.drawRect(0, 0, 510, 180);
        p.flush();
    }

    QString imgname;

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

        QImage image;
        image.load(imgname);

        if( image.isNull()) {
            kdDebug(11111) << "cannot load card pixmap \"" << imgname << "\" in (" << idx << ") " << dir << "\n";
            p.end();
            delete w;
            return false;
        }
        img[rank][suit].normal.convertFromImage(image);
        KImageEffect::fade(image, 0.4, dimcolor);
        img[rank][suit].inverted.convertFromImage(image);

        if( animate )
        {
            if( idx > 1 )
                p.drawPixmap( 10 + ( idx - 1 ) * 8, 45, back );
            p.drawPixmap( 10 + idx * 8, 45, img[ rank ][ suit ].normal );
            p.flush();
        }
    }

    if( animate )
    {
        p.end();
        t2 = QTime::currentTime();
        if(t1.msecsTo(t2) < 1500)
            usleep((1500-t1.msecsTo(t2))*1000);
        delete w;
    }

    return true;
}

bool cardMap::setBackSide( const QPixmap &pm )
{
    if (pm.isNull())
        return false;

    back = pm;

    if (!CARDX || !CARDY) {
        CARDX = back.width();
        CARDY = back.height();
    }

    if(back.width() != CARDX ||
       back.height() != CARDY)
    {
        kdDebug(11111) << "scaling back¡!!\n";
        // scale to fit size
        QWMatrix wm;
        wm.scale(((float)(CARDX))/back.width(),
                 ((float)(CARDY))/back.height());
        back = back.xForm(wm);
    }

    return true;
}

QPixmap cardMap::backSide() const
{
    return back;
}

QPixmap cardMap::image( Card::Values _value, Card::Suits _suit, bool inverted) const
{
    if( 1 <= _value &&
        _value <= 13 &&
        1 <= _suit &&
        _suit <= 4 )
    {
        if (inverted)
            return img[ _value - 1 ][ _suit - 1 ].inverted;
        else
            return img[ _value - 1 ][ _suit - 1 ].normal;
    }
    else
    {
        kdError() << "access to invalid card " << int(_value) << ", " << int(_suit) << endl;
    }
    return 0;
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

