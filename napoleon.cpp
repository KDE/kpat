/* 
  napoleon.cpp  implements a patience card game

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

 */

#include "napoleon.h"
#include <qevent.h> 
#include <qpainter.h>
#include "patience.h"

Napoleon::Napoleon( QWidget* parent, const char* name)
  :dealer(parent,name)
{
  initMetaObject();

  const int Store   = 1;
  const int Store2  = 2;
  const int Target  = 3;
  const int Target2 = 4;
  const int Centre  = 5;
  const int Pile    = 6;

  Card::setAddFlags(Pile, Card::Default);
  Card::setRemoveFlags(Pile, Card::Default);

  Card::setAddFlags(Store, Card::Default);
  Card::setRemoveFlags(Store, Card::Default);
  Card::setAddFun(Store, &justOne);

  Card::setAddFlags(Store2, Card::plus90);
  Card::setRemoveFlags(Store2, Card::Default);
  Card::setAddFun(Store2, &justOne);

  Card::setAddFlags(Target,  Card::plus45);
  Card::setRemoveFlags(Target, Card::disallow);
  Card::setAddFun(Target, &Ustep1);

  Card::setAddFlags(Target2, Card::minus45);
  Card::setRemoveFlags(Target2, Card::disallow);
  Card::setAddFun(Target2, &Ustep1);

  Card::setAddFlags(Centre, Card::Default);
  Card::setRemoveFlags(Centre, Card::disallow);
  Card::setAddFun(Centre, &Dstep1);

  Card::setLegalMove(Store, Target);
  Card::setLegalMove(Store, Target2);
  Card::setLegalMove(Store, Centre);

  Card::setLegalMove(Store2, Target);
  Card::setLegalMove(Store2, Target2);
  Card::setLegalMove(Store2, Centre);

  Card::setLegalMove(Pile, Target);
  Card::setLegalMove(Pile, Target2);
  Card::setLegalMove(Pile, Centre);
  Card::setLegalMove(Pile, Store);
  Card::setLegalMove(Pile, Store2);

  Card::setLegalMove(DeckType, Target);
  Card::setLegalMove(DeckType, Target2);
  Card::setLegalMove(DeckType, Store);
  Card::setLegalMove(DeckType, Store2);
  Card::setLegalMove(DeckType, Centre);
  Card::setLegalMove(DeckType, Pile);

  deck = new Deck (500, 290, this);

  // override standard Deck definitions
  Card::setAddFlags(DeckType, Card::disallow);

  pile = new cardPos(400, 290, this, Pile);
  Card::setSendBack(pile);    

  target[0] = new cardPos(10,  10,  this, Target2);
  target[1] = new cardPos(250, 10,  this, Target);
  target[2] = new cardPos(250, 268, this, Target2);
  target[3] = new cardPos(10,  268, this, Target);

  store[0] = new cardPos(160, 10,  this, Store);
  store[1] = new cardPos(272, 167,  this, Store2);
  store[2] = new cardPos(160, 290, this, Store);
  store[3] = new cardPos(10,  167,  this, Store2);

  centre = new cardPos(160, 150, this, Centre);

  //    connect( deck , SIGNAL(nonMovableCardPressed(int)), SLOT(deal1()) );
  deal();
}

Napoleon::~Napoleon() {
  delete deck;
  delete pile;
  delete centre;

  for( int i=0; i<4; i++)
    delete target[i];

  for( int i=0; i<4; i++)
    delete store[i];
}

void Napoleon::show() {
  deck->show();
  pile->show();
  centre->show();

  for( int i=0; i<4; i++)
    target[i]->show();

  for( int i=0; i<4; i++)
    store[i]->show();
}

void Napoleon::restart() {
  deck->collectAndShuffle();
  deal();
}

bool Napoleon::Ustep1( const Card* c1, const Card* c2) {
  if (c1->Suit() == Card::Empty) 
    return c2->Value() == Card::Seven;
  else
    return (c2->Value() == c1->Value() + 1);
}

bool Napoleon::Dstep1( const Card* c1, const Card* c2) {
  if (c1->Suit() == Card::Empty) 
    return c2->Value() == Card::Six;
    
  if (c1->Value() == Card::Ace)
    return (c2->Value() == Card::Six);
  else  
    return (c2->Value() == c1->Value() - 1);
}


bool Napoleon::justOne( const Card* c1, const Card* ) {
  return (c1->Suit() == Card::Empty);
}

void Napoleon::deal() {
  for (int i=0; i<4; i++)
    store[i]->add(deck->getCard(), FALSE, FALSE);
}

void Napoleon::deal1() {
  Card::dont_undo();
  pile->add(deck->getCard(), FALSE, FALSE);
}

QSize Napoleon::sizeHint() const {
  return QSize(600, 476);
}

#include "napoleon.moc"
