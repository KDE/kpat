/******************************************************

  Card.cpp -- support classes for patience type card games

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

unimplemented flags: alsoFaceDown

*******************************************************/

#include "card.moc"
#include "qapp.h"

// static member definitions
int   Card::RemoveFlags[N_TYPES];
int   Card::AddFlags[N_TYPES];
bool  Card::LegalMove[N_TYPES][N_TYPES];
Card::addCheckPtr Card::addCheckFun[N_TYPES];
Card::removeCheckPtr Card::removeCheckFun[N_TYPES];

bool Card::moving = FALSE;
bool Card::resting = FALSE;
Card* Card::mov = 0;
Card* Card::source = 0;
Card* Card::justTurned = 0;
Card* Card::sendBackTo = 0;
QWidget* Card::widAtCurPos = 0;


const int Card::Default       = 0x0000;
const int Card::disallow      = 0x0001;
const int Card::several       = 0x0002; // default: move one card
const int Card::faceDown      = 0x0004; //  move/add cards facedown

// Add-flags
const int Card::addSpread     = 0x0100;
const int Card::addRotated    = 0x0600; // Note: cannot have Spread && Rotate
const int Card::minus45       = 0x0400; 
const int Card::plus45        = 0x0200; 
const int Card::plus90        = 0x0600; 

// Remove-flags
const int Card::alsoFaceDown  = 0x0100; 
const int Card::autoTurnTop   = 0x0200;
const int Card::noSendBack    = 0x0400;  
const int Card::wholeColumn   = 0x0800;  


// end static member def

void Card::clearAllFlags() {
  moving = FALSE;
  resting = FALSE;
  mov = 0;
  source = 0;
  justTurned = 0;
  sendBackTo = 0;

  for (int i=0; i<N_TYPES; i++) {
    RemoveFlags[i]=0;
    AddFlags[i] = 0;
    addCheckFun[i]=0;
    removeCheckFun[i]=0;
    for (int j=0; j<N_TYPES;j++)
      LegalMove[i][j]=0;
  }

}

const int SPREAD = 20;
const int DSPREAD = 5;

bool Card::legalAdd(Card *c) const  { 
  if (AddFlags[cardType] & disallow) return FALSE;
  if (! LegalMove[c->cardType][cardType]) return FALSE;
  if (! (AddFlags[cardType] & several) && c->nextPtr) return FALSE;

  if ( addCheckFun[cardType] ) return  addCheckFun[cardType]( this, c );
    
  return TRUE; 
} 


// this function isn't used yet
void Card::propagateCardClicked(Card *c)  {
  if (prev()) 
    prev()->propagateCardClicked(c);
  else {
    emit cardClicked(c);
    emit cardClicked(c->cardType);
  }
}

void Card::propagateNonMovableCardPressed(int cardType) {
  if (prev()) 
    prev()->propagateNonMovableCardPressed(cardType);
  else 
    emit nonMovableCardPressed(cardType);
}

bool Card::legalRemove() const { 
  if (!movable()) 
    return FALSE;

  if (RemoveFlags[cardType] & disallow) 
    return FALSE;

  if (! (RemoveFlags[cardType] & several) && nextPtr) 
    return FALSE;

  if (!FaceUp() && next() ) 
    return FALSE;

  if ( removeCheckFun[cardType] ) 
    return  removeCheckFun[cardType]( this );

  return TRUE; 
}

/* The old move function for cards and piles. Rather slow. */
void Card::moveTo(int x, int y) {
  QPoint p(x,y);
  moveTo (p);
} 

void Card::moveTo(const QPoint& p) {
  move(p);
  raise();
  if (nextPtr) {
    QPoint off(0, !empty() && AddFlags[cardType]&addSpread ? SPREAD : 0);
    nextPtr->moveTo(p+off); 
  }
}

/* This one is much faster for piles, since we only need to paint the
   visible part. */
void Card::quickMoveTo(const QPoint& p) {
  if (nextPtr){
    QPoint off(0, !empty() && AddFlags[cardType]&addSpread ? SPREAD : 0);
    nextPtr->quickMoveTo(p+off); 
  }
  move(p);
}

void Card::unrotate() {
  rotate45(0);

  if ( nextPtr ) 
    nextPtr->rotate45(0);
}

void Card::changeType(int type) {
  switch (AddFlags[type]&addRotated) {
  case minus45:
    rotate45( -1 );
    break;
  case plus45:
    rotate45( 1 );
    break;
  case plus90:
    rotate45( 2 );
    break;
  case 0:
    rotate45(0);
    break;
  }

  cardType = type;
  if ( nextPtr ) nextPtr->changeType(type);
}

/* add card to top of pile */
void Card::add( Card* c) {
  // Assume legal move  
  //  adjust flags

  if (nextPtr == 0) {
    c->changeType(cardType);
    nextPtr = c;
    c-> prevPtr = this;
    QPoint off(0,  !empty() && AddFlags[cardType]&addSpread ? 
	       !FaceUp() ? DSPREAD : SPREAD : 0); 
    c->moveTo (pos() + off); 
  } else
    nextPtr->add(c);
}

/* override cardtype (for initial deal ) */
void Card::add( Card* c, bool facedown , bool spread) {
  // NOTE: this duplicates code from Card::add(Card*)
  // "// new" signifies new or changed code
  if (nextPtr == 0) {
    c->turn(!facedown);          // new

    c->changeType(cardType);
    nextPtr = c;
    c-> prevPtr = this;
    QPoint off(0, !empty() && spread ? facedown || !FaceUp() ? DSPREAD : SPREAD : 0); // new
    c->moveTo (pos() + off); 
  } else
    nextPtr->add( c, facedown, spread ); // new
}

/*
  remove this card together with all subsequent cards in pile
 */
void Card::remove() {
  if (prevPtr) 
    prevPtr->nextPtr = 0;
  prevPtr = 0;
}

void Card::unlink() {
  if (prevPtr) 
    prevPtr->nextPtr = nextPtr;

  if (nextPtr) 
    nextPtr->prevPtr = prevPtr;

  prevPtr = nextPtr = 0;
}

void Card::startMove(const QPoint& fudge_arg) {
  ASSERT(!moving || !resting);
  if ( nextPtr && prevPtr && prevPtr->FaceUp() && prevPtr->movable() 
       && (wholeColumn & RemoveFlags[cardType]) ) {
    prevPtr->startMove(fudge_arg + QPoint(0, SPREAD) );
    return;
  }
  if ( legalRemove() ) {
    if ( !(faceDown & RemoveFlags[cardType]) ) 
      turn();

    moveTo(pos()); //raise cards
    moving = TRUE;
    unrotate();
    source = prevPtr;
    remove();
    mov = this;
    fudge = fudge_arg;
    if (fudge.x() > width()) 
      fudge.setX(width());
    if (fudge.y() > height()) 
      fudge.setY(height());
    if (fudge != fudge_arg)
      quickMoveTo(pos() + mapFromGlobal(QCursor::pos()) - fudge);
  } else { 
    if (RemoveFlags[cardType] & disallow) 
      propagateNonMovableCardPressed(cardType);  
    // should emit signals if illegalRemove
    else  
      if  (prev() && prev()->FaceUp())
	prev()->startMove(fudge_arg+ QPoint(0, SPREAD));
  }
}

void Card::turnTop() {
  ASSERT(!moving || !resting);
  if (!next()) {
    if ( !FaceUp() ) {
      // turn  card and place it correctly
      Card* base = prev();
      turn();  
      if (base) {
	remove();
	base->add(this);
	// ought to forbid sendback after this
	justTurned = source; // DANGEROUS 
      }
    }
  } else 
    next()->turnTop();
}

void Card::endMove() {
  ASSERT(!moving || !resting);
  if (resting) {
    if ( source && (RemoveFlags[source->cardType] & autoTurnTop ) ) 
      source->turnTop();
    resting = FALSE;
    return;
  }

  if (next()) 
    next()->endMove();
  else
    if (legalAdd(mov)) {
      moving = FALSE;
      add(mov);
      if ( source && (RemoveFlags[source->cardType] & autoTurnTop ) ) 
	source->turnTop();
    } 
    else {
      sendBack();
    }
}

void Card::restMove() {
  ASSERT(!moving || !resting);
  if (next()) 
    next()->restMove();
  else
    if (legalAdd(mov)&&this!=source) {
      moving = FALSE;
      resting = TRUE;
      add(mov);
    } 
}

void Card::stopMoving() {  
  ASSERT(!moving || !resting);
  resting = FALSE;
  if (moving) {
    // unconditional sendback
    moving = FALSE;
    source->add(mov);
  }
}

void Card::mousePressEvent (QMouseEvent* e) {
  ASSERT(!moving || !resting);
  if (e->button() == LeftButton ){ 
    if (moving)
      endMove();
    else if (resting) {
      endMove();
    }
    else
      startMove(e->pos());
  }
  else  if (e->button() == RightButton ) 
    emit rightButtonPressed(cardType);
}

void Card::mouseReleaseEvent (QMouseEvent*) {
  ASSERT(!moving || !resting);
  if (resting) {
    endMove();
  } else if (moving)
    sendBack();
}

void Card::stopMovingIfResting() {
  if ( resting )
    mov->endMove();
}


bool Card::sendBack() {
  ASSERT(!resting);
  if (moving && !(RemoveFlags[source->cardType] & noSendBack )
      && source != justTurned  ) {
    moving = FALSE;
    source->add(mov);
    return TRUE; // success
  } else if (moving && sendBackTo) {
    moving = FALSE;
    sendBackTo->add(mov);
    return TRUE;
  } else 
    return FALSE;
}

bool Card::undoLastMove() {
  if (moving) 
    return sendBack();
  else if (mov && source != justTurned ) {
    mov->remove();
    mov->changeType(source->cardType);
    mov->unrotate();
    moving = TRUE;
    sendBack();
    return TRUE;
  }
  else  
    return FALSE;
}

bool Card::movingCard(QWidget* w) {
  if (w == mov) {
    return TRUE;
  }
  if ( w->inherits( "Card" ) ) {
    Card *c = (Card*) w;
    return c->prev() && movingCard(c->prev());
  }
  return FALSE;
}

void Card::mouseMoveHandle(const QPoint &) {
  ASSERT(!moving || !resting);
  QWidget *tmp=0;
  if ((moving || resting) && mov)
    tmp = QApplication::widgetAt(
				 QCursor::pos() - mov->fudge + QPoint(cardMaps::CARDX/2, 0),
				 TRUE );
  else
    tmp = QApplication::widgetAt( QCursor::pos(), TRUE );
  if (tmp != widAtCurPos) {
    widAtCurPos = tmp;
    if (resting)
      {
	if ( !tmp || movingCard(tmp))
	  return;
	mov->remove();
	mov->changeType(source->cardType);
	mov->unrotate();
	resting = FALSE;
	moving = TRUE;
      }

    if ( moving && tmp && tmp->inherits( "Card" ) ) {
      Card *target = (Card*) tmp;
      target->restMove();
    }
  }
  if (moving)
    mov->quickMoveTo(mov->pos() + mov->mapFromGlobal(QCursor::pos()) - 
		     mov->fudge);
}

void Card::mouseMoveEvent (QMouseEvent* e) {
  mouseMoveHandle(e->pos() + pos());
}

Card::~Card() {
}

Card::Card( Values v, Suits s, QWidget *parent, int type, bool empt ) 
  : basicCard(v, s, parent, empt), cardType(type)
{
  initMetaObject();
  nextPtr=prevPtr=0;

  char name[3];
  name[0] = vsym();
  name[1] = ssym();
  name[2] = 0;
  setName(name);

  switch (AddFlags[type]&addRotated) {
  case minus45:
    rotate45( -1 );
    break;
  case plus45:
    rotate45( 1 );
    break;
  case plus90:
    rotate45( 2 );
    break;
  }
}

