/***********************-*-C++-*-********

  grandf.cpp  implements a patience card game

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
// 7 positions, all cards on table, follow suit
// ( I don't know a name for this one, but I learned it from my grandfather.)

****************************************/


#ifndef P_GRANDF_7
#define P_GRANDF_7

#include "patience.h"

class Grandf : public dealer {
  Q_OBJECT
public:
  Grandf( QWidget* parent=0, const char* name=0);
  virtual ~Grandf();

  virtual void show();
  virtual void undo();

  QSize sizeHint() const;

public slots:
  void redeal();
  void deal();
  virtual void restart();
  virtual void hint();

private: // functions
  void collect();
  static bool step1( const Card* c1, const Card* c2);
  static bool Sstep1( const Card* c1, const Card* c2);

private:
  cardPos* store[7];
  cardPos* target[4];
  Deck *deck;
  QPushButton rb;
  int numberOfDeals;
};

#endif
