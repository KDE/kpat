/***********************-*-C++-*-********

  computation.h  implements a patience card game

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

//
// This one was discussed on the newsgroup rec.games.abstract
//
****************************************/

#include <kapp.h>
#include <qcombo.h>
#include "rnd.h"
#include "computation.h"
#include "global.h"

const int Computation::Store = 1;
const int Computation::Target1 = 2;
const int Computation::Target2 = 3;
const int Computation::Target3 = 4;
const int Computation::Target4 = 5;

Card *Computation::getCardByValue( char v ) {
  Card* p;

  p = &d;
  while (p && p->Value() != v)
    p = p->next();

  if (p) {
    p->unlink();
    return p;
  }
  return 0;
}

void Computation::changeDiffLevel( int l ) {
  //if areYouSure() ....
  diffLevel = l - 4;
  restart();
}

void Computation::initByDiffLevel() {
  int used[4] = {0, 0, 0, 0};
  Rnd rand;
  int i;
    
  int n = (5 - diffLevel) % 5;   // How many cards do we give?

  while ( n-- ){
    do {
      i = rand.rInt(4);
    } while ( used[i] );
    used[i] = 1;

    Card* p = getCardByValue( i+1 );
    p->turnTop();
    switch (i+1) {
    case 1: p01.add(p); break; 
    case 2: p02.add(p); break; 
    case 3: p03.add(p); break; 
    case 4: p04.add(p); break; 
      // default: panic
    }
  }
}

Computation::Computation( QWidget *parent, const char *name )
  :dealer( parent, name), 

   d(10, 100, this),
   p01(110, 10, this, Target1),
   p02(210, 10, this, Target2),
   p03(310, 10, this, Target3),
   p04(410, 10, this, Target4),
   p11(110, 150, this, Store),
   p12(210, 150, this, Store),
   p13(310, 150, this, Store),
   p14(410, 150, this, Store)

{ 
  initMetaObject(); 

  QComboBox *cb = new QComboBox( this, "comboBox" );
  cb->insertItem( i18n("Easy 4") );
  cb->insertItem( i18n("Easy 3") );
  cb->insertItem( i18n("Easy 2") );
  cb->insertItem( i18n("Easy 1") );
  cb->insertItem( i18n("Normal") );
  cb->insertItem( i18n("Hard 1") );
  cb->insertItem( i18n("Hard 2") );
  cb->insertItem( i18n("Hard 3") );
  cb->insertItem( i18n("Hard 4") );
  cb->insertItem( i18n("Hard 5") );

  cb->setCurrentItem(4);
  diffLevel = 0;
    
  cb->setGeometry( 10, 30, 120, 30 );
  cb->setAutoResize( TRUE );
  cb->show();
  connect( cb, SIGNAL(activated(int)), SLOT(changeDiffLevel(int)) );

  Card::setAddFlags(Store, Card::addSpread);
  Card::setRemoveFlags(Store, Card::several);
  Card::setAddFun(Store, 0);

  Card::setAddFlags(Target1, Card::several);
  Card::setRemoveFlags(Target1, Card::disallow);
  Card::setAddFun(Target1, &step1);

  Card::setAddFlags(Target2, Card::several);
  Card::setRemoveFlags(Target2, Card::disallow);
  Card::setAddFun(Target2, &step2);

  Card::setAddFlags(Target3, Card::several);
  Card::setRemoveFlags(Target3, Card::disallow);
  Card::setAddFun(Target3, &step3);

  Card::setAddFlags(Target4, Card::several);
  Card::setRemoveFlags(Target4, Card::disallow);
  Card::setAddFun(Target4, &step4);

  Card::setLegalMove(DeckType, Store);

  Card::setLegalMove(DeckType, Target1);
  Card::setLegalMove(Store, Target1);

  Card::setLegalMove(DeckType, Target2);
  Card::setLegalMove(Store, Target2);

  Card::setLegalMove(DeckType, Target3);
  Card::setLegalMove(Store, Target3);

  Card::setLegalMove(DeckType, Target4);
  Card::setLegalMove(Store, Target4);
}

Computation::~Computation() {
}

void Computation::restart() {
  //diffLevel++;
  d.collectAndShuffle();
  initByDiffLevel();
  show();
}

void Computation::show() {
  d.show();   
  p01.show();
  p02.show();
  p03.show();
  p04.show();
  p11.show();
  p12.show();
  p13.show();
  if (diffLevel <= 0) {
    p14.show();
    p14.setEnabled(TRUE);
  }
  else {
    p14.hide();
    p14.setEnabled(FALSE);
  }
}

bool Computation::step1( const Card* c1, const Card* c2)  {
  return (c2->Value() % 13 == (c1->Value() + 1) % 13)
    && (c2->next() ? step1( c2, c2->next()) : TRUE);
}

bool Computation::step2( const Card* c1, const Card* c2) {
  return (c2->Value() % 13 == (c1->Value() + 2) % 13)
    && (c2->next() ? step2( c2, c2->next()) : TRUE);
}

bool Computation::step3( const Card* c1, const Card* c2) {
  return (c2->Value() % 13 == (c1->Value() + 3) % 13)
    && (c2->next() ? step3( c2, c2->next()) : TRUE);
}

bool Computation::step4( const Card* c1, const Card* c2) {
  return (c2->Value() % 13 == (c1->Value() + 4) % 13)
    && (c2->next() ? step4( c2, c2->next()) : TRUE);
}

QSize Computation::sizeHint() const {
  return QSize(540, 476);
}

#include "computation.moc"
