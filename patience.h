/*****************-*-C++-*-****************

  Patience.h  support classes for patience type card games

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


 *    Card -- basic class
 *
 *    cardPos -- position where cards can be placed
 *
 *    Deck -- 52 cards, shuffled
 *
 *    CardTable -- playing background
 *
 

 ****************************************************/

#ifndef PATIENCE_CLASS
#define PATIENCE_CLASS


#include <qevent.h> 
#include <qpushbt.h>
#include <qapp.h>
#include "card.h"


// The following classes are defined in this header:
class CardTable;
class cardPos;
class Deck;
class dealer;


/***************************************

  CardTable -- the playing surface

**************************************/
class CardTable: public QWidget {
  Q_OBJECT
public:
  CardTable(QWidget *parent=0, const char *name=0 );
  virtual ~CardTable();

protected:
  virtual void mouseMoveEvent (QMouseEvent*);
  virtual void mousePressEvent (QMouseEvent*);
};


/***************************************************************

  dealer -- abstract base class of all varieties of patience



  MORE WORK IS NEEDED -- especially on allocation/deallocation

***************************************************************/
class dealer: public CardTable {
  Q_OBJECT
public:
  dealer( QWidget *parent = 0, const char *name=0 );
  virtual ~dealer();

  QSize sizeHint() const;

public slots:
  virtual void restart() = 0;
  virtual void undo();
  virtual void show() = 0;

protected:
  void stopActivity();

private:
  dealer(dealer&) {};  // don't allow copies or assignments
  void operator=(dealer&) {};  // don't allow copies or assignments
};


/***************************************

  cardPos -- position on the table which can receive cards

**************************************/
class cardPos: public Card {
  Q_OBJECT
public:
  cardPos( int x, int y,  QWidget *parent=0, int type=0 )
    : Card( Empty, Empty, parent, type, TRUE )
  {     initMetaObject();  move(x,y); }

  virtual ~cardPos() {};
};

const int DeckType=0;


/***************************************

  Deck -- create and shuffle 52 cards

**************************************/
class Deck: public cardPos {
  Q_OBJECT
public:
  Deck( int x=0, int y=0, CardTable* parent=0, int m = 1 );
  virtual ~Deck();

  static const long n;

  void collectAndShuffle();
  void setSendBack(cardPos* c );

  Card* getCard();

private: // functions
  void makedeck();
  void addToDeck();
  void shuffle();
  void unlinkAll();

private:
  CardTable* f;
  int mult;
  Card** deck;
  cardPos* sendBackPos;
};

#endif
