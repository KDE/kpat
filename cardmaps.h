/***********************-*-C++-*-********

  cardmaps.h  defines pixmaps for playing cards

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

#ifndef P_HACK_CARDMAP
#define P_HACK_CARDMAP

#include <qbitmap.h>
#include <qobject.h>

class cardMaps: public QObject {
  Q_OBJECT
public:
  cardMaps( QObject* parent = 0 );

  static int CARDX;
  static int CARDY;

  QPixmap *image( int value, int suit) const;
  QPixmap *backSide();
  void setBackSide(QPixmap *);

private:
  QPixmap *img[13][4];
  QPixmap *back;
};

#endif
