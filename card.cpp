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

#include <qapplication.h>

#include "card.h"

// static member definitions
int   Card::RemoveFlags[ N_TYPES ];
int   Card::AddFlags[ N_TYPES ];
bool  Card::LegalMove[ N_TYPES ][ N_TYPES ];
Card::addCheckPtr Card::addCheckFun[ N_TYPES ];
Card::removeCheckPtr Card::removeCheckFun[ N_TYPES ];

bool Card::moving             = FALSE;
bool Card::resting            = FALSE;
Card* Card::mov               = 0;
Card* Card::source            = 0;
Card* Card::justTurned        = 0;
Card* Card::sendBackTo        = 0;
QWidget* Card::widAtCurPos    = 0;

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

const int SPREAD = 20;
const int DSPREAD = 5;

void Card::clearAllFlags()
{
  moving = FALSE;
  resting = FALSE;
  mov = 0;
  source = 0;
  justTurned = 0;
  sendBackTo = 0;

  for( int index1 = 0; index1 < N_TYPES; index1++ )
  {
    RemoveFlags[ index1 ] = 0;
    AddFlags[ index1 ] = 0;
    addCheckFun[ index1 ] = 0;
    removeCheckFun[ index1 ] = 0;
    for( int index2 = 0; index2 < N_TYPES; index2++ )
      LegalMove[ index1 ][ index2 ] = 0;
  }

}

bool Card::legalAdd( Card* _card ) const
{ 
  if( AddFlags[ cardType ] & disallow )
    return FALSE;

  if( !LegalMove[ _card->cardType ][ cardType ] )
    return FALSE;

  if( !( AddFlags[ cardType ] & several ) &&
      _card->nextPtr )
    return FALSE;

  if( addCheckFun[ cardType ] )
    return addCheckFun[ cardType ]( this, _card );

  return TRUE; 
} 

// this function isn't used yet
void Card::propagateCardClicked( Card* _card )
{
  if( prev() ) 
  {
    prev()->propagateCardClicked( _card );
  }
  else
  {
    emit cardClicked( _card );
    emit cardClicked( _card->cardType );
  }
}

void Card::propagateNonMovableCardPressed( int _cardType )
{
  if( prev() ) 
  {
    prev()->propagateNonMovableCardPressed( _cardType );
  }
  else 
  {
    emit nonMovableCardPressed( _cardType );
  }
}

bool Card::legalRemove() const
{ 
  if( !movable() ) 
    return FALSE;

  if( RemoveFlags[ cardType ] & disallow ) 
    return FALSE;

  if( !( RemoveFlags[ cardType ] & several ) && nextPtr ) 
    return FALSE;

  if( !FaceUp() && next() ) 
    return FALSE;

  if( removeCheckFun[ cardType ] ) 
    return removeCheckFun[ cardType ]( this );

  return TRUE; 
}

/* The old move function for cards and piles. Rather slow. */
void Card::moveTo( int _x, int _y )
{
  QPoint point( _x, _y );
  moveTo ( point );
} 

void Card::moveTo( const QPoint& _point )
{
  move( _point );
  raise();
  if( nextPtr )
  {
    QPoint off( 0, !empty() && AddFlags[ cardType ] & addSpread ? SPREAD : 0 );
    nextPtr->moveTo( _point + off ); 
  }
}

/* This one is much faster for piles, since we only need to paint the
   visible part. */
void Card::quickMoveTo( const QPoint& _point )
{
  if( nextPtr )
  {
    QPoint off( 0, !empty() && AddFlags[ cardType ] & addSpread ? SPREAD : 0 );
    nextPtr->quickMoveTo( _point + off ); 
  }
  move( _point );
}

void Card::unrotate()
{
  rotate45( 0 );

  if( nextPtr ) 
  {
    nextPtr->rotate45( 0 );
  }
}

void Card::changeType( int _type )
{
  switch( AddFlags[ _type ] & addRotated )
  {
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

  cardType = _type;
  if( nextPtr )
  {
    nextPtr->changeType( _type );
  }
}

/* add card to top of pile */
void Card::add( Card* _card )
{
  // Assume legal move  
  // adjust flags

  if( !nextPtr )
  {
    _card->changeType( cardType );
    nextPtr = _card;
    _card->prevPtr = this;
    QPoint off( 0, !empty() && AddFlags[ cardType ] & addSpread ? 
	       !FaceUp() ? DSPREAD : SPREAD : 0); 
    _card->moveTo( pos() + off ); 
  }
  else
  {
    nextPtr->add( _card );
  }
}

/* override cardtype (for initial deal ) */
void Card::add( Card* _card, bool _facedown, bool _spread )
{
  // NOTE: this duplicates code from Card::add( Card* )
  // "// new" signifies new or changed code
  if( !nextPtr )
  {
    _card->turn( !_facedown );          // new

    _card->changeType( cardType );
    nextPtr = _card;
    _card->prevPtr = this;
    QPoint off( 0, !empty() && _spread ? _facedown || !FaceUp() ? DSPREAD : SPREAD : 0 ); // new
    _card->moveTo( pos() + off ); 
  }
  else
  {
    nextPtr->add( _card, _facedown, _spread ); // new
  }
}

/*
  remove this card together with all subsequent cards in pile
 */
void Card::remove()
{
  if( prevPtr ) 
  {
    prevPtr->nextPtr = 0;
  }
  prevPtr = 0;
}

void Card::unlink()
{
  if( prevPtr ) 
    prevPtr->nextPtr = nextPtr;

  if( nextPtr ) 
    nextPtr->prevPtr = prevPtr;

  prevPtr = 0;
  nextPtr = 0;
}

void Card::startMove( const QPoint& _fudge_arg )
{
  ASSERT( !moving || !resting );

  if( nextPtr &&
      prevPtr && 
      prevPtr->FaceUp() &&
      prevPtr->movable() &&
      (wholeColumn & RemoveFlags[ cardType ] ) )
  {
    prevPtr->startMove( _fudge_arg + QPoint( 0, SPREAD ) );
    return;
  }
  if( legalRemove() )
  {
    if( !( faceDown & RemoveFlags[ cardType ] ) ) 
      turn();

    moveTo( pos() ); //raise cards
    moving = TRUE;
    unrotate();
    source = prevPtr;
    remove();
    mov = this;
    fudge = _fudge_arg;
    if( fudge.x() > width() ) 
      fudge.setX( width() );
    if( fudge.y() > height() ) 
      fudge.setY( height() );
    if( fudge != _fudge_arg )
      quickMoveTo( pos() + mapFromGlobal( QCursor::pos() ) - fudge );
  }
  else
  { 
    if( RemoveFlags[ cardType ] & disallow ) 
      propagateNonMovableCardPressed( cardType );  
    // should emit signals if illegalRemove
    else  
      if( prev() && prev()->FaceUp() )
	prev()->startMove( _fudge_arg + QPoint( 0, SPREAD ) );
  }
}

void Card::turnTop()
{
  ASSERT( !moving || !resting );

  if( !next() )
  {
    if( !FaceUp() )
    {
      // turn card and place it correctly
      Card* base = prev();
      turn();  
      if( base )
      {
	remove();
	base->add( this );
	// ought to forbid sendback after this
	justTurned = source; // DANGEROUS 
      }
    }
  }
  else next()->turnTop();
}

void Card::endMove()
{
  ASSERT( !moving || !resting );

  if( resting )
  {
    if( source && ( RemoveFlags[ source->cardType ] & autoTurnTop ) ) 
    {
      source->turnTop();
    }
    resting = FALSE;
    return;
  }

  if( next() ) 
  {
    next()->endMove();
  }
  else
  {
    if( legalAdd( mov ) )
    {
      moving = FALSE;
      add( mov );
      if ( source && ( RemoveFlags[ source->cardType ] & autoTurnTop ) ) 
	source->turnTop();
    } 
    else
    {
      sendBack();
    }
  }
}

void Card::restMove()
{
  ASSERT( !moving || !resting );

  if( next() )
  { 
    next()->restMove();
  }
  else
  {
    if( legalAdd( mov ) && this != source )
    {
      moving = FALSE;
      resting = TRUE;
      add( mov );
    }
  }
}

void Card::stopMoving()
{  
  ASSERT( !moving || !resting );

  resting = FALSE;
  if( moving )
  {
    // unconditional sendback
    moving = FALSE;
    source->add( mov );
  }
}

void Card::mousePressEvent( QMouseEvent* _event )
{
  ASSERT( !moving || !resting );

  if( _event->button() == LeftButton )
  { 
    if( moving )
    {
      endMove();
    }
    else if( resting )
    {
      endMove();
    }
    else
    {
      startMove( _event->pos() );
    }
  }
  else if( _event->button() == RightButton ) 
  {
    emit rightButtonPressed( cardType );
  }
}

void Card::mouseReleaseEvent( QMouseEvent* )
{
  ASSERT( !moving || !resting );

  if( resting )
  {
    endMove();
  }
  else if( moving )
  {
    sendBack();
  }
}

void Card::stopMovingIfResting()
{
  if( resting )
  {
    mov->endMove();
  }
}


bool Card::sendBack()
{
  ASSERT( !resting );

  if( moving && 
      !( RemoveFlags[ source->cardType ] & noSendBack ) &&
      source != justTurned )
  {
    moving = FALSE;
    source->add( mov );
    return TRUE; // success
  }
  else if( moving && sendBackTo )
  {
    moving = FALSE;
    sendBackTo->add( mov );
    return TRUE;
  }
  else return FALSE;
}

bool Card::undoLastMove()
{
  if( moving ) 
    return sendBack();
  else if( mov && source != justTurned )
  {
    mov->remove();
    mov->changeType( source->cardType );
    mov->unrotate();
    moving = TRUE;
    sendBack();
    return TRUE;
  }
  else return FALSE;
}

bool Card::movingCard( QWidget* _widget )
{
  if( _widget == mov )
  {
    return TRUE;
  }
  if( _widget->inherits( "Card" ) )
  {
    Card* card = (Card*) _widget;
    return card->prev() && movingCard( card->prev() );
  }
  return FALSE;
}

void Card::mouseMoveHandle( const QPoint & )
{
  ASSERT( !moving || !resting );

  QWidget* tmp = 0;
  if( ( moving || resting ) && mov )
    tmp = QApplication::widgetAt( QCursor::pos() - mov->fudge + QPoint( cardMaps::CARDX / 2, 0 ), TRUE );
  else
    tmp = QApplication::widgetAt( QCursor::pos(), TRUE );
  if( tmp != widAtCurPos )
  {
    widAtCurPos = tmp;
    if( resting )
    {
      if( !tmp || movingCard( tmp ) )
        return;
      mov->remove();
      mov->changeType( source->cardType );
      mov->unrotate();
      resting = FALSE;
      moving = TRUE;
    }

    if( moving && tmp && tmp->inherits( "Card" ) )
    {
      Card* target = (Card*) tmp;
      target->restMove();
    }
  }
  if( moving )
    mov->quickMoveTo( mov->pos() + mov->mapFromGlobal( QCursor::pos() ) - mov->fudge );
}

void Card::mouseMoveEvent( QMouseEvent* _event )
{
  mouseMoveHandle( _event->pos() + pos() );
}

Card::~Card()
{
}

Card::Card( Values _value, Suits _suit, QWidget* _parent, int _type, bool _empty ) 
  : basicCard( _value, _suit, _parent, _empty )
  , cardType( _type )
{
  nextPtr = 0;
  prevPtr = 0;

  char name[ 3 ];
  name[ 0 ] = vsym();
  name[ 1 ] = ssym();
  name[ 2 ] = 0;
  setName( name );

  switch( AddFlags[ _type ] & addRotated )
  {
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

#include "card.moc"

