/***********************-*-C++-*-********

  ten.h  implements a patience card game

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

#ifndef P_SUM_TEN
#define P_SUM_TEN

#include "patience.h"
#include <qtimer.h>

class QPushButton;

class Ten: public dealer {
  Q_OBJECT
public:
  Ten( QWidget* parent=0, const char* name=0);
  virtual ~Ten();
  virtual void show();
  QSize sizeHint() const;

public slots:
  virtual void restart();
  virtual void undo();
  void deal();
  void dealNext();

private slots:
  bool remove(int pile);
  void stepAuto();
  void stopAuto();
  void startAuto();
  void changeAuto(bool);

private:
  cardPos* play[8];
  int lastPos;
  int autoState;
  QTimer timer;
  Deck *deck;
  QPushButton *pb;
};

#endif
