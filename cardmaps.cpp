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

#include <qapp.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qwmatrix.h>
#include <qdrawutl.h>
#include <qapp.h>
#include <stdio.h>
#include "cardmaps.h"
#include "global.h"
#include <unistd.h>
#include <qdatetm.h>
#include <kconfig.h>
#include <config.h>
#include <kapp.h>

int cardMaps::CARDX;
int cardMaps::CARDY;

cardMaps::cardMaps( QObject* parent ) : QObject(parent, 0) {
  // create an animation window while loading pixmaps (this
  // may take a while (approx. 3 seconds on my AMD K6PR200)
  config->setGroup("General settings");
  bool animate = (bool)(config->readNumEntry("Animation", 1) != 0);
  QWidget *w = 0;
  QPixmap pm1;
  QPainter p;
  QTime t1, t2;

  if(animate) {
    t1 = QTime::currentTime();
    w = new QWidget(0, "", 
			     WStyle_Customize|WStyle_NoBorder|WStyle_Tool);
    pm1.load((PICDIR + "back1.bmp").data());
    QWidget *dt = qApp->desktop();
    w->setBackgroundColor(darkGreen);  
    w->setGeometry((dt->width() - 510)/2, (dt->height() - 180)/2, 
		 510, 180);
    w->show();
    qApp->processEvents();
    p.begin(w);
    
    p.drawText(0, 150, 510, 20, AlignCenter, 
	       i18n("please wait, loading cards..."));
    
    p.setFont(QFont("Times", 24));
    p.drawText(0, 0, 510, 40, AlignCenter, 
	       i18n("KPat - a Solitaire game"));  

    p.setPen(QPen(QColor(0, 0, 0), 4));
    p.setBrush(NoBrush);
    p.drawRect(0, 0, 510, 180);
    p.flush();
  }

  QString imgname;
  for(int idx = 1; idx < 53; idx++) {
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

    imgname.sprintf("%s/%d.bmp", PICDIR.data(), idx);
    img[rank][suit] = new QPixmap(imgname.data());

    if(img[rank][suit]->width() == 0 ||
       img[rank][suit]->height() == 0) {
      fprintf(stderr, 
	      i18n("kpat: PANIC, cannot load card pixmap \"%s\"\n"),
	      imgname.data());
      exit(1);
    }

    if(animate) {
      if(idx > 1)
	p.drawPixmap(10 + (idx-1)*8, 45, pm1);
      p.drawPixmap(10 + idx*8, 45, *img[rank][suit]);
      p.flush();
    }
  }

  if(animate) {
    p.end();
    t2 = QTime::currentTime();
    if(t1.msecsTo(t2) < 1500) 
      usleep((1500-t1.msecsTo(t2))*1000);
    delete w;
  }

  CARDX = img[0][0]->width();
  CARDY = img[0][0]->height();

  back = 0;
  setBackSide(0);
}

void cardMaps::setBackSide(QPixmap *pm) {
  // delete old background
  if(back != 0) {
    delete back;
    back = 0;
  }

  if(pm == 0) { // ok, let's use the default KDE background
    QColorGroup  mycolgroup( 
			    QApplication::palette()->normal().foreground(), 
			    QApplication::palette()->normal().background(),
			    lightGray,
			    QApplication::palette()->normal().dark(), 
			    QApplication::palette()->normal().mid(),
			    QApplication::palette()->normal().text(), 
			    QApplication::palette()->normal().base());

    back =  new QPixmap(CARDX, CARDY);
    back->fill( darkRed );         // initialize pixmap
    QPainter p;                           
    p.begin( back );                       
    QFont f( "times", 17, QFont::Black );
    f.setStyleHint( QFont::Times );
    p.setFont(f);
    QRect br = p.fontMetrics().boundingRect( "KDEI");
    p.rotate (45);
    int y = -CARDY;
    int x = -CARDX;
    while (y < 2 * CARDY){
      p.setPen(darkGray);
      p.drawText(x + 2, y + 2, "KDE");
      p.setPen(gray);
      p.drawText(x, y, "KDE");
      x += br.width();
      if (x > 2 * CARDX){
	x -= 3 * CARDX + br.width();
	y += (int) (br.height() * 1.5);
      }
    }

    //     p.setPen(white);
    //     for ( int y =  -CARDY ; y < 2*CARDY  ; y += 10) {
    // 	p.drawLine( 0 ,  y, CARDX , y + CARDX ); 
    // 	p.drawLine( 0 ,  y, CARDX , y - CARDX ); 
    //     }

    p.rotate( -45 );
    qDrawShadePanel(&p, 0, 0, CARDX, CARDY, mycolgroup);
    p.end();
  } else {
    back = new QPixmap(*pm);
    if(back->width() != CARDX ||
       back->height() != CARDY) {
      // scale to fit size
      QWMatrix wm;
      wm.scale(((float)(CARDX))/back->width(),
	       ((float)(CARDY))/back->height());
      *back = back->xForm(wm);
    }
  }  
}

QPixmap *cardMaps::backSide() {
  return back;
}

QPixmap* cardMaps::image( int value, int suit) const { 
  if ( 1 <= value && value <= 13 && 
       1 <= suit && suit <= 4 )
    return img[value-1][suit-1];
  else {
    fprintf(stderr, 
	    i18n("KPAT: access to invalid card %d, %d\n"), 
	    value, suit);
    return 0;
  }
}

#include "cardmaps.moc"
