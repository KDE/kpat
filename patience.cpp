
/******************************************************

  Patience.cpp -- support classes for patience type card games

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

NB!

unimplemented flags: alsoFaceDown




*******************************************************/

#include "patience.h"
#include "rnd.h"
#include <stdio.h>


const long Deck::n = 52;


/*
 *    General support class for patience type card games
 *
 *    Card -- basic class
 *
 *    CardTable -- place to put cards
 *
 */
CardTable::CardTable( QWidget *parent, const char *name ) 
  :QWidget(parent, name)
{
  initMetaObject();

  setBackgroundColor(darkGreen);
  setMouseTracking(TRUE);
  show();
}

CardTable::~CardTable() {
}

void CardTable::mouseMoveEvent (QMouseEvent* e) {
  Card::mouseMoveHandle(e->pos());
}

void CardTable::mousePressEvent (QMouseEvent*) {
  Card::stopMovingIfResting();
}

Card* Deck::getCard() {
  Card* c;

  c = next();  // Dealing from bottom of deck ....
  if (c) c->unlink();
  return c;
}


Deck::Deck( int x, int y, CardTable* parent, int m ) 
  : cardPos(x, y, parent, DeckType), f(parent), mult (m)
{ 
  deck = new Card * [mult*n];
  CHECK_PTR (deck);

  initMetaObject(); 
  makedeck(); 
  shuffle(); 
  addToDeck(); 
}


void Deck::makedeck() {
  int i=0;

  setAddFlags(DeckType, Card::disallow);
  setRemoveFlags(DeckType, Card::noSendBack);
  show();
  for ( Card::Suits s = Card::Clubs; s <=  Card::Spades ; s++)
    for ( Card::Values  v = Card::Ace; v <= Card::King; v++)
      for ( int m = 0; m < mult; m++)
        (deck[i++] = new Card(v, s, f))->show();
}

Deck::~Deck() {
  unlinkAll();
  clearAllFlags();
  
  for (int i=0; i < mult*n; i++)
    delete deck[i];
}

void Deck::collectAndShuffle() { 
  unlinkAll(); 
  shuffle(); 
  addToDeck(); 
}


void Deck::setSendBack(cardPos* c ) { 
  sendBackPos = c; 
}


// Shuffle deck, assuming all cards are in deck[]
void Deck::shuffle() {
  //  Something is rotten...
  Rnd R;

  Card* t;
  long z;
  for (int i = mult*n-1; i >= 1; i--) {
    z = R.rInt(i);
    t = deck[z]; 
    deck[z] = deck[i]; 
    deck[i] = t;
  }
}

// add cards in deck[] to Deck
void Deck::addToDeck() {
  Card *c = this;
  for (int i = 0; i < mult*n; i++) {
    c->add( deck[i], TRUE, FALSE );
    c = deck[i];
  }
}

// unlink all cards 
void Deck::unlinkAll() {
  Card::stopMoving();
  
  for (int i=0; i < mult*n; i++)
    deck[i]->unlink();
}


dealer::dealer( QWidget *parent , const char *name )
  :CardTable(parent, name)
{
  initMetaObject();
}

dealer::~dealer() {
}

void dealer::stopActivity() { 
  Card::stopMoving(); 
}

QSize dealer::sizeHint() const {
  // just a dummy size
  fprintf(stderr, "kpat: dealer::sizeHint() called!\n");
  return QSize(100, 100);
}

void dealer::undo() { 
  Card::undoLastMove(); 
}

#include "patience.moc"
