/***********************-*-C++-*-********

  computation.h  implements a patience card game

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
// This one was discussed on the newsgroup rec.games.abstract
//


****************************************/

#ifndef P_COMPUTATION_H
#define P_COMPUTATION_H

#include "patience.h"
#include <qscrbar.h>

class Computation : public dealer {
  Q_OBJECT
public:
  Computation(  QWidget *parent = 0, const char *name=0 );
  virtual ~Computation();

  virtual void restart();
  virtual void show();
  QSize sizeHint() const;

public slots:
  void changeDiffLevel(int lvl);

private:
  Card *getCardByValue( char v );
  void initByDiffLevel();
  static bool step1( const Card* c1, const Card* c2);
  static bool step2( const Card* c1, const Card* c2);
  static bool step3( const Card* c1, const Card* c2);
  static bool step4( const Card* c1, const Card* c2);

  Deck d;

  cardPos p01;
  cardPos p02;
  cardPos p03;
  cardPos p04;
  cardPos p11;
  cardPos p12;
  cardPos p13;
  cardPos p14;

  int diffLevel;

  static const int Store;
  static const int Target1;
  static const int Target2;
  static const int Target3;
  static const int Target4;
};

#endif
