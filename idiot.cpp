/***********************-*-C++-*-********

  idiot.cpp  implements a patience card game

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
// 4 positions, remove lowest card(s) of suit 
//


****************************************/

#include <qapp.h>
#include "idiot.h"

Idiot::Idiot( QWidget* parent, const char* name)
  : dealer(parent,name)
{
  initMetaObject();

  for ( int Play   = 1; Play <= 4; Play++) {
    Card::setAddFlags(Play, Card::addSpread );
    Card::setRemoveFlags(Play, Card::disallow);
  }

  deck = new Deck (210, 10, this);

  // override standard Deck definition
  Card::setAddFlags(DeckType, Card::disallow);
  Card::setRemoveFlags(DeckType, Card::disallow);

  away = new cardPos(-100, -100, this, 5);
  Card::setAddFlags(5, Card::disallow);
  Card::setRemoveFlags(5, Card::disallow);

  for(int i=0; i<4; i++) {
    play[i] = new cardPos(10+100*i, 150, this, i+1);
    connect( play[i] , SIGNAL(nonMovableCardPressed(int)), 
	     SLOT(handle(int)) );
  }

  connect( deck , SIGNAL(nonMovableCardPressed(int)), 
	   SLOT(deal()) );

  deal();
}

void Idiot::show() {
  for(int i=0; i<4; i++) {
    play[i]-> show() ;
  }
}

void Idiot::undo() {
  Card::undoLastMove();
}

void Idiot::restart() {
  deck->collectAndShuffle();

  deal();
}

Idiot::~Idiot() {
  delete deck;

  for(int i=0; i<4; i++)
    delete play[i];
}

inline bool higher(Card *c1, Card *c2) {
  return c1->Suit()==c2->Suit() && c1->ValueA() < c2->ValueA();
}

inline void moveCard(Card *c1, Card *c2) {
  c1->remove();
  c2->add(c1);
}

bool Idiot::handle(int pile) {
  pile--;

  if (!play[pile]) 
    return FALSE;

  Card *c = play[pile]->top();
  bool Ok = TRUE;

  if ( higher(c, play[0]->top()) ||
       higher(c, play[1]->top()) ||
       higher(c, play[2]->top()) ||
       higher(c, play[3]->top()) )
    moveCard(c,away);
  else if (!play[0]->next())
    moveCard(c,play[0]);
  else if (!play[1]->next())
    moveCard(c,play[1]);
  else if (!play[2]->next())
    moveCard(c,play[2]);
  else if (!play[3]->next())
    moveCard(c,play[3]);
  else  
    Ok = FALSE;

  return Ok;
}

void Idiot::deal() {
  if (deck->next())
    for (int i = 0; i < 4; i++ )
      play[i]->add(deck->getCard(), FALSE, TRUE);
}

QSize Idiot::sizeHint() const {
  return QSize(540, 476);
}

#include "idiot.moc"
