/***********************-*-C++-*-********

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

****************************************/


#ifndef P_NAPOLEON
#define P_NAPOLEON

#include "patience.h"

class Napoleon : public dealer {
  Q_OBJECT
public:
  Napoleon (QWidget* parent=0, const char* name=0);
  virtual ~Napoleon();

  virtual void restart();
  virtual void show();
  QSize sizeHint() const;

public slots:
  void deal1();

private:
  void deal();

  static bool Ustep1( const Card* c1, const Card* c2);
  static bool Dstep1( const Card* c1, const Card* c2);    
  static bool justOne( const Card* c1, const Card* c2);

  cardPos* pile;
  cardPos* target[4];
  cardPos* centre;
  cardPos* store[4];
  Deck* deck;
};

#endif
