/***********************-*-C++-*-********

  ten.cpp  implements a patience card game

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
// 8 positions, remove 3 cards when sum = 10, 20 or 30
//


****************************************/


#include <kapp.h>
#include <qpushbt.h>
#include "ten.h"
#include "global.h"

const int weight[14] = {0,1,2,3,4,5,6,7,8,9,10,10,10};

Ten::Ten( QWidget* parent, const char* name)
  : dealer(parent,name),timer(this)
{
  initMetaObject();

  pb = new QPushButton(i18n("Full Auto"), this );
  pb->setToggleButton( TRUE );
  pb->move(10,50);
  pb->adjustSize(); 
  connect( pb, SIGNAL( toggled(bool)) , SLOT( changeAuto(bool) ) );
  pb->show();

  for ( int Play   = 1; Play <= 8; Play++) {
    Card::setAddFlags(Play, Card::addSpread );
    Card::setRemoveFlags(Play, Card::disallow);
  }
  deck = new Deck (210, 10, this);

  // override standard Deck definitions
  Card::setAddFlags(DeckType, Card::disallow);
  Card::setRemoveFlags(DeckType, Card::disallow);

  for(int i=0; i<8; i++) {
    play[i] = new cardPos(10+100*i, 150, this, i+1);
    connect( play[i] , SIGNAL(nonMovableCardPressed(int)), 
	     SLOT(remove(int)) );
  }
  connect( deck, SIGNAL(nonMovableCardPressed(int)), 
	   SLOT(dealNext()) );
  deal();

  connect( &timer, SIGNAL(timeout()), SLOT(stepAuto()) );
}

void Ten::stepAuto() {
  if (lastPos >= 0) {
    if(!remove(lastPos+1))
      dealNext();
  } else {
    do {
      autoState++; 
    } while (autoState <=8 && !remove(autoState));
    if (autoState > 8) 
      dealNext();
  }
}

void Ten::changeAuto(bool b) {
  if (b) 
    startAuto();
  else 
    stopAuto();
}

void Ten::stopAuto() {
  timer.stop();
}

void Ten::startAuto() {
  timer.start(300);
}

void Ten::show() {
  for(int i=0; i<8; i++)
    play[i]-> show();
}

void Ten::undo() {
  Card::undoLastMove();
}

void Ten::restart() {
  deck->collectAndShuffle();
  deal();
}

Ten::~Ten()
{
  delete deck;
  for(int i = 0; i < 8; i++)
    delete play[i];
}

void Ten::dealNext()
{
  int i = lastPos;

  do { i = (i+1)%8;
  } while (!play[i]->next() && i != lastPos);

  Card* c=0;
  if (play[i]->next() && (c = deck->getCard()) ) {
    play[i]->add(c, FALSE, TRUE);

    lastPos = i;
  }
}

inline bool sumTen(Card *c1, Card *c2, Card *c3)
{
  return (weight[c1->Value()] + weight[c2->Value()] 
	  + weight[c3->Value()]) % 10 == 0;
}

inline void takeCards(Card *c1, Card *c2, Card *c3, Card *d)
{
  Card* t=c1->prev();

  c1->unlink();
  d->add(c1,TRUE,FALSE);
  c2->unlink();
  d->add(c2,TRUE,FALSE);
  c3->unlink();
  d->add(c3,TRUE,FALSE);

  Card* tt = t->next();
  if (t->empty() && tt) {
    tt->remove();
    t->add(tt);
  }
}

bool Ten::remove(int pile) {
  pile--;

  if (play[pile]->next() && play[pile]->next()->next()
      && play[pile]->next()->next()->next()) {

    Card* c1 = play[pile]->next();
    Card* c2 = c1->next();
    Card* c3 = c2->next();
    Card* c6 = c1->top();
    Card* c5 = c6->prev();
    Card* c4 = c5->prev();

    if (sumTen(c1,c2,c6))       
      takeCards(c1,c2,c6,deck);
    else if (sumTen(c1,c5,c6)) 
      takeCards(c1,c5,c6,deck);
    else if (sumTen(c4,c5,c6)) 
      takeCards(c4,c5,c6,deck);
    else if (sumTen(c1,c2,c3)) 
      takeCards(c1,c2,c3,deck);
    else  
      return FALSE;

    return TRUE;
  } else  
    return FALSE;
}

void Ten::deal() {
  for(int round=0; round < 3; round++) {
    for (int i = 0; i < 8; i++ ) {
      play[i]->add(deck->getCard(), FALSE, TRUE);
    }
  }
  lastPos = -1;
  autoState=0;
}

QSize Ten::sizeHint() const {
  return QSize(800, 476);
}

#include "ten.moc"
