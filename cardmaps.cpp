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

#include <kconfig.h>

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
#include <kglobalsettings.h>
#include <assert.h>

cardMap *cardMap::_self = 0;
static KStaticDeleter<cardMap> cms;

cardMap::cardMap(const QColor &dim) : dimcolor(dim)
{
    assert(!_self);

    card_width = 0;
    card_height = 0;

    kdDebug(11111) << "cardMap\n";
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );

    QString bg = config->readEntry( "Back", KCardDialog::getDefaultDeck());
    setBackSide( bg, false);

    QString dir = config->readEntry("Cards",  KCardDialog::getDefaultCardDir());
    setCardDir( dir );

    cms.setObject(_self, this);
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

    QString imgname = KCardDialog::getCardPath(dir, 11);

    QImage image;
    image.load(imgname);
    if( image.isNull()) {
        kdDebug(11111) << "cannot load card pixmap \"" << imgname << "\" in " << dir << "\n";
        p.end();
        delete w;
        return false;
    }

    int old_card_width = card_width;
    int old_card_height = card_height;

    card_width = image.width();
    card_height = image.height();

    const int diff_x_between_cards = QMAX(card_width / 9, 1);
    QString wait_message = i18n("please wait, loading cards...");
    QString greeting = i18n("KPatience - a Solitaire game");

    const int greeting_width = 20 + diff_x_between_cards * 52 + card_width;

    if( animate ) {
        t1 = QTime::currentTime();
        w = new QWidget( 0, "", Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_Tool );
        QRect dg = KGlobalSettings::splashScreenDesktopGeometry();
        w->setBackgroundColor( Qt::darkGreen );
        w->setGeometry( dg.left() + ( dg.width() - greeting_width ) / 2, dg.top() + ( dg.height() - 180 ) / 2, greeting_width, 180);
        w->show();
        qApp->processEvents();

        p.begin( w );
        p.drawText(0, 150, greeting_width, 20, Qt::AlignCenter,
                   wait_message );

        p.setFont(QFont("Times", 24));
        p.drawText(0, 0, greeting_width, 40, Qt::AlignCenter,
                   greeting);

        p.setPen(QPen(QColor(0, 0, 0), 4));
        p.setBrush(Qt::NoBrush);
        p.drawRect(0, 0, greeting_width, 180);
        p.flush();
    }

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
            kdDebug(11111) << "cannot load card pixmap \"" << imgname << "\" in (" << idx << ") " << dir << "\n";
            p.end();
            delete w;
            card_width = old_card_width;
            card_height = old_card_height;

            setBackSide(back, true);
            return false;
        }

        img[rank][suit].normal.convertFromImage(image);
        KImageEffect::fade(image, 0.4, dimcolor);
        img[rank][suit].inverted.convertFromImage(image);

        if( animate )
        {
            if( idx > 1 )
                p.drawPixmap( 10 + ( idx - 1 ) * diff_x_between_cards, 45, back );
            p.drawPixmap( 10 + idx * diff_x_between_cards, 45, img[ rank ][ suit ].normal );
            p.flush();
        }
    }

    if( animate )
    {
        const int time_to_see = 900;
        p.end();
        t2 = QTime::currentTime();
        if(t1.msecsTo(t2) < time_to_see)
          usleep((time_to_see-t1.msecsTo(t2))*1000);
        delete w;
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
        kdDebug(11111) << "scaling back!!\n";
        // scale to fit size
        QWMatrix wm;
        wm.scale(((float)(card_width))/back.width(),
                 ((float)(card_height))/back.height());
        back = back.xForm(wm);
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

QPixmap cardMap::image( Card::Rank _rank, Card::Suit _suit, bool inverted) const
{
    if( 1 <= _rank && _rank <= 13 
	&& 1 <= _suit && _suit <= 4 )
    {
        if (inverted)
            return img[ _rank - 1 ][ _suit - 1 ].inverted;
        else
            return img[ _rank - 1 ][ _suit - 1 ].normal;
    }
    else
    {
        kdError() << "access to invalid card " << int(_rank) << ", " << int(_suit) << endl;
    }
    return 0;
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

