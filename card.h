/*****************-*-C++-*-****************



  Card.h -- movable  and stackable cards
            with check for legal  moves



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

 ****************************************************/


#ifndef PATIENCE_CARD
#define PATIENCE_CARD

#include <qpoint.h> 
#include "basiccard.h"

// The following classes are defined in other headers:
class CardTable;
class cardPos;
class Deck;
class dealer;

#define N_TYPES 16

class Card: public basicCard {
  friend class dealer;
  friend class CardTable;
  Q_OBJECT
public:

  //  Add- and remove-flags
  static const int Default;
  static const int disallow;
  static const int several; // default: move one card
  static const int faceDown; //  move/add cards facedown

  // Add-flags
  static const int addSpread;
  static const int addRotated; // Note: cannot have Spread && Rotate
  static const int minus45; 
  static const int plus45; 
  static const int plus90; 

  // Remove-flags
  static const int alsoFaceDown; 
  static const int autoTurnTop;
  static const int noSendBack;  
  static const int wholeColumn;  

  Card( Values v, Suits s,  QWidget *parent=0, int type=0, bool empty=FALSE);
  virtual ~Card();

  void add( Card* c);
  void add( Card* c, bool facedown, bool spread); // for initial deal 

  void moveTo (const QPoint&);
  void moveTo (int x, int y);
  void quickMoveTo (const QPoint&);

  bool legalAdd(Card *c) const;
  bool legalRemove() const;

  static void stopMovingIfResting();
  static bool sendBack();
  static bool undoLastMove();

signals:
  void cardClicked(Card*);
  void cardClicked(int cardtype);
  void nonMovableCardPressed(int cardtype);
  void rightButtonPressed(int cardtype);

private:
  void propagateCardClicked(Card*);
  void propagateNonMovableCardPressed(int cardtype);

public slots:
  void remove();
  void unlink(); // Should this be protected?
  void turnTop();
  void flipCard() { turn( !FaceUp() ); }

private:
  static int   RemoveFlags[N_TYPES];
  static int   AddFlags[N_TYPES];
  static bool  LegalMove[N_TYPES][N_TYPES];

  typedef bool (*addCheckPtr)(const Card*, const Card*);
  static addCheckPtr addCheckFun[N_TYPES];

  typedef bool (*removeCheckPtr)(const Card*);
  static  removeCheckPtr removeCheckFun[N_TYPES];

  int cardType;

protected:
  virtual void mouseMoveEvent (QMouseEvent*);
  virtual void mousePressEvent (QMouseEvent*);
  virtual void mouseReleaseEvent (QMouseEvent*);

  static void mouseMoveHandle(const QPoint&);
  static void stopMoving();
  static void clearAllFlags();

private:
  Card *nextPtr;
  Card *prevPtr; // doubly linked list

  static bool moving;   // Are we currently moving a card?
  static Card *mov;     // If so, which?
  static Card *source;  // And where did it come from?
  static Card *justTurned; // The last card that got turned over 
  static bool resting;  // Are the cards only temporarily placed?
  static Card *sendBackTo; // Where do we put the unwanted cards?

  static QWidget *widAtCurPos;

  static bool movingCard(QWidget*);
  QPoint  fudge;

private slots:
  void changeType(int type);
  void startMove(const QPoint&);
  void endMove();
  void unrotate();
  void restMove(); // place temporarily

public:
  int type() const {return cardType; }
  Card* next() const {return nextPtr; }
  Card* prev() const {return prevPtr; }
  Card* top() {	
    if (!next()) return this;
    else  return next()->top();
  }

  static void dont_undo() { if (!moving) {mov = source = 0; } }

  static void setRemoveFlags( int type, int flag ) { RemoveFlags[type] = flag; }
  static void setAddFlags( int type, int flag ) { AddFlags[type] = flag; }
  static void setLegalMove( int from, int to ) { LegalMove[from][to] = TRUE; }
  static void setSendBack( Card* c ) { sendBackTo = c; }
  static void setAddFun( int type, addCheckPtr f) 
  { addCheckFun[type] = f; }

  static void setRemoveFun( int type, removeCheckPtr f) 
  { removeCheckFun[type] = f; }
  
  //    static void setSafeMove() { moveSafe = TRUE; }
};

#endif
