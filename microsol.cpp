/***********************-*-C++-*-********

  microsol.cpp  implements a patience card game

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
// 6 positions, alternating red and black
//


****************************************/

#include <qcombo.h>
#include <qapp.h>
#include <kmsgbox.h>
#include "microsol.h"

MicroSolitaire::MicroSolitaire( QWidget* parent, const char* name)
  : dealer(parent,name)
{
  initMetaObject();

  const int Pile   = 1;
  const int Play   = 2;
  const int Target = 3;

  Card::setAddFlags(Play, Card::addSpread | Card::several);
  Card::setRemoveFlags(Play, Card::several | Card::autoTurnTop);
  Card::setAddFun(Play, altStep);

  Card::setAddFlags(Target, Card::Default);
  Card::setRemoveFlags(Target, Card::Default );
  Card::setAddFun(Target, &step1);

  Card::setAddFlags(Pile, Card::disallow);
  Card::setRemoveFlags(Pile, Card::Default);

  Card::setLegalMove(Play, Target);
  Card::setLegalMove(Play, Play);
  Card::setLegalMove(Target, Play);

  Card::setLegalMove(Pile, Play);
  Card::setLegalMove(Pile, Target);

  deck = new Deck (10, 80, this);

  // override standard Deck definitions
  Card::setAddFlags(DeckType, Card::disallow);
  Card::setRemoveFlags(DeckType, Card::disallow);

  pile = new cardPos(10, 230, this, Pile);

  for(int i=0; i<4; i++){
    target[i] = new cardPos(210+i*100, 10, this, Target);
  }
  for(int i=0; i<6; i++) {
    play[i] = new cardPos(110+100*i, 150, this, Play);
  }
  connect( deck , SIGNAL(nonMovableCardPressed(int)), SLOT(dealNext()) );
  deal();
}

void MicroSolitaire::show() {
  pile->show();

  for(int i=0; i<4; i++)
    target[i]-> show() ;
  
  for(int i=0; i<6; i++)
    play[i]-> show() ;
}

void MicroSolitaire::undo() {
  Card::undoLastMove();
}

void MicroSolitaire::restart() {
  deck->collectAndShuffle();
  deal();
}

MicroSolitaire::~MicroSolitaire() {
  delete deck;
  delete pile;

  for(int i=0; i<4; i++)
    delete target[i];

  for(int i=0; i<6; i++)
    delete play[i];
}

// Deal one card
void MicroSolitaire::dealNext() {
  Card::dont_undo();

  if ( !deck->next() ) {
    redeal();
    return;
  }

  Card* p = deck->top();
  p->remove();
  pile->add(p, FALSE, FALSE); // faceup, nospread
}

//  Add cards from  pile to deck, in reverse direction
void MicroSolitaire::redeal() {
  Card* p = pile->top();
  while (!p->empty()) {
    Card* t = p->prev();
    p->remove();
    deck->add(p, TRUE, FALSE); // facedown, nospread
    p = t;
  }
}

void MicroSolitaire::deal() {
  for(int round=0; round < 6; round++)
    for (int i = round; i < 6; i++ )
      play[i]->add(deck->getCard(), i != round, TRUE);
}

bool MicroSolitaire::wholeBunch( const Card* c ) {
  if (c->prev()) 
    return   c->prev()->empty()  || !c->prev()->FaceUp();
  else 
    return TRUE;
}

bool MicroSolitaire::step1( const Card* c1, const Card* c2 ) {
  return (c2->Value() == c1->Value() + 1)
    && (c1->Suit() != Card::Empty ? c1->Suit() == c2->Suit() : TRUE);
}

bool MicroSolitaire::altStep( const Card* c1, const Card* c2)  {
  if (c1->Suit() == Card::Empty) 
    return c2->Value() == Card::King;
  else
    return (c2->Value() == c1->Value() - 1) && c1->Red() != c2->Red();
}

QSize MicroSolitaire::sizeHint() const {
  return QSize(700, 476);
}


#include "microsol.moc"
