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

#define NUM_OF_COLORS   4
#define CARDS_PER_COLOR 13

class cardMaps: public QObject
{
  Q_OBJECT

public:

  cardMaps( QObject* _parent = 0 );

  static int CARDX;
  static int CARDY;

  QPixmap* image( int _value, int _suit ) const;
  QPixmap* backSide();
  void setBackSide( QPixmap* _pix );

private:

  QPixmap* img[ CARDS_PER_COLOR ][ NUM_OF_COLORS ];
  QPixmap* back;
};

#endif
