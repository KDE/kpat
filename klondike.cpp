/***********************-*-C++-*-********

  klondike.cpp  implements a patience card game

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
// 7 positions, alternating red and black
//

****************************************/

#include <kapp.h>
#include "klondike.h"
#include <kmsgbox.h>
#include "global.h"

Klondike::Klondike( QWidget* parent, const char* name)
  : dealer(parent, name)
{
  initMetaObject();

  const int Pile   = 1;
  const int Play   = 2;
  const int Target = 3;

  Card::setAddFlags(Play, Card::addSpread | Card::several);
  Card::setRemoveFlags(Play, Card::several | Card::autoTurnTop 
		       | Card::wholeColumn);
  Card::setAddFun(Play, altStep);

  Card::setAddFlags(Target, Card::Default);
  Card::setRemoveFlags(Target, Card::disallow);
  Card::setAddFun(Target, &step1);

  Card::setAddFlags(Pile, Card::disallow);
  Card::setRemoveFlags(Pile, Card::Default);

  Card::setLegalMove(Play, Target);
  Card::setLegalMove(Play, Play);

  Card::setLegalMove(Pile, Play);
  Card::setLegalMove(Pile, Target);

  deck = new Deck (10, 80, this);

  // override standard Deck definitions
  Card::setAddFlags(DeckType, Card::disallow);
  Card::setRemoveFlags(DeckType, Card::disallow);

  EasyRules = TRUE;

  cb = new QComboBox( this, "comboBox" );
  //cb->insertItem( "Cheating" );
  cb->insertItem( i18n("Easy rules"), 0);
  cb->insertItem( i18n("Hard rules"), 1);
  cb->setGeometry( 10, 30, 120, 30 );
  cb->setAutoResize( TRUE );
  cb->setCurrentItem(0);
  cb->show();
  connect(cb, SIGNAL(activated(int)), 
	  SLOT(changeDiffLevel(int)) );

  pile = new cardPos(10, 230, this, Pile);

  for(int i=0; i<4; i++)
    target[i] = new cardPos(210+i*85, 10, this, Target);
  
  for(int i=0; i<7; i++) 
    play[i] = new cardPos(110+85*i, 150, this, Play);
  
  connect( deck , SIGNAL(nonMovableCardPressed(int)), 
	   SLOT(deal3()) );
  deal();
}

void Klondike::changeDiffLevel( int l ) {
  if ( EasyRules == (l == 0) ) 
    return;

  int r = KMsgBox::yesNo(this, i18n("Warning"), 
			 i18n("This will end the current game\n" \
			 "Are you sure you want to do this?"),
			 KMsgBox::EXCLAMATION);
  if(r == 2) {
    cb->setCurrentItem(1-cb->currentItem());
    return;
  }

  EasyRules = (bool)(l == 0);
  restart();
}

/*
  Check that there are 3 or more cards in pile (not including argument)
  assumes that p != 0
 */
static bool moreThan2(Card* p) {
  return p->next() && p->next()->next() && p->next()->next()->next();
}

void Klondike::show() {    
  int i;

  pile->show();

  for(i = 0; i < 4; i++)
    target[i]->show();
  
  for(i = 0; i < 7; i++)
    play[i]->show();
}

void Klondike::undo() {
  Card::undoLastMove();
}

void Klondike::restart() {
  deck->collectAndShuffle();  
  deal();
}

Klondike::~Klondike() {
  delete deck;
  delete pile;

  for(int i=0; i<4; i++)
    delete target[i];

  for(int i=0; i<7; i++)
    delete play[i];
}

void Klondike::deal3() {
  Card::dont_undo();

  if ( !EasyRules && !deck->next() 
       ||  EasyRules && !moreThan2(deck) && pile->next() ) 
    {
      redeal();
      return;
    }

  Card *p = deck->top();
  for(int i = 0; i<3 && (!p->empty()) ; i++) {
    Card* t = p->prev();
    p->remove();
    pile->add(p, FALSE, FALSE); // faceup, nospread
    p = t;
  }    
}


//  Add cards from  pile to deck, in reverse direction
void Klondike::redeal() {
  Card* olddeck = 0;

  if (EasyRules) {
    // the remaining cards in deck should be on top
    // of the new deck

    olddeck = deck->next();
    if (olddeck) 
      olddeck->remove();
  }

  Card* p = pile->top();
  while (!p->empty()) {
    Card* t = p->prev();
    p->remove();
    deck->add(p, TRUE, FALSE); // facedown, nospread
    p = t;
  }

  if (EasyRules)
    // put the cards from the old deck on top
    if (olddeck) 
      deck->add(olddeck);
}

void Klondike::deal() {
  for(int round=0; round < 7; round++)
    for (int i = round; i < 7; i++ )
      play[i]->add(deck->getCard(), i != round, TRUE);
}

bool Klondike::wholeBunch( const Card* c ) {
  if (c->prev()) 
    return c->prev()->empty()  || !c->prev()->FaceUp();
  else 
    return TRUE;	
}

bool Klondike::step1( const Card* c1, const Card* c2 ) {
  return (c2->Value() == c1->Value() + 1)
    && (c1->Suit() != Card::Empty ? c1->Suit() == c2->Suit() : TRUE);
}

bool Klondike::altStep( const Card* c1, const Card* c2) {
  if (c1->Suit() == Card::Empty) 
    return c2->Value() == Card::King;
  else
    return (c2->Value() == c1->Value() - 1) && c1->Red() != c2->Red();
}

QSize Klondike::sizeHint() const {
  return QSize(710, 476);
}

#include "klondike.moc"
