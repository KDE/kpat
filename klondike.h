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

#ifndef P_KLONDIKE
#define P_KLONDIKE

#include "patience.h"
#include <qcombo.h>

class Klondike : public dealer {
  Q_OBJECT
public:
  Klondike( QWidget* parent=0, const char* name=0);
  virtual ~Klondike();

  virtual void show();
  QSize sizeHint() const;

public slots:
  void changeDiffLevel(int lvl);
  virtual void restart();
  
  void redeal(); // put pile back into deck
  void deal();
  void deal3(); // move up to 3 cards from deck to pile
  virtual void undo();

  //signals:
  //void setDiffLevel();

private:
  static bool step1( const Card* c1, const Card* c2);
  static bool altStep( const Card* c1, const Card* c2);
  static bool wholeBunch( const Card* c);

  bool EasyRules;

  cardPos* play[7];
  cardPos* target[4];

  cardPos *pile;
  Deck* deck;
  QComboBox *cb;
};

#endif


