/*****************-*-C++-*-****************

  basicCard -- displayable cards with values

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

**************************************/

#ifndef P_BASIC_CARD
#define P_BASIC_CARD


#include <qlabel.h>
#include "cardmaps.h"

class basicCard : public QLabel { 
  Q_OBJECT
public:
  typedef char Suits;
  typedef char Values;

  static const Suits Clubs;
  static const Suits Diamonds;
  static const Suits Hearts;
  static const Suits Spades;

  static const Values Empty;
  static const Values Ace;
  static const Values Two;
  static const Values Three;
  static const Values Four;
  static const Values Five;
  static const Values Six;
  static const Values Seven;
  static const Values Eight;
  static const Values Nine;
  static const Values Ten;
  static const Values Jack;
  static const Values Queen;
  static const Values King;

  basicCard( Values v, Suits s,  QWidget *parent=0,  bool empty=FALSE); 
  virtual ~basicCard();

  const QPixmap & pixmap();

private:
  static const char vs[15]; 
  static const char ss[6];  
  static cardMaps *maps; //   The pictures...
  //   Note: we have to use a pointer
  //   because of Qt restrictions.
  //   ( QPainter objects needs a
  //   running QApplication. )
  //   

  Suits suit;
  Values value;
  bool faceup;
  bool empty_flag;
  int direction;

protected:
  void paintEvent( QPaintEvent * );
  void showCard(); 
  void turn(bool faceup = TRUE); 
  void rotate45(int d);

public:
  Suits Suit() const {return suit;}     
  Values  Value() const {return value;} 
  Values  ValueA() const {return value==Ace?14:value;} 

  bool Red() const { return suit==Diamonds || suit==Hearts; }
  bool Black() const { return suit==Clubs || suit==Spades; }

  bool FaceUp() const {return faceup;}  

  Values vsym() const { return vs[value]; }
  Suits ssym() const { return ss[suit]; }

  virtual bool empty() const { return empty_flag; }
  virtual bool movable() const {return !empty(); }

  int rotated() { return direction; }
};

extern cardMaps *cardmaps;

#endif
