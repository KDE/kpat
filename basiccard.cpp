
/******************************************************

  basicCard.cpp -- support classes for patience type card games

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

  ToDo:

  * Directions

*******************************************************/

#include <math.h>

#include <qcolor.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qdrawutil.h>

#include "basiccard.h"

// static member definitions
const char basicCard::vs[15] = "0A23456789TJQK";
const char basicCard::ss[6] =  "0CDHS";
cardMaps* basicCard::maps=0; //   The pictures...

cardMaps *cardmaps = 0;

const basicCard::Suits basicCard::Clubs    = 1;
const basicCard::Suits basicCard::Diamonds = 2;
const basicCard::Suits basicCard::Hearts   = 3;
const basicCard::Suits basicCard::Spades   = 4;

const basicCard::Values basicCard::Empty = 0;
const basicCard::Values basicCard::Ace   = 1;
const basicCard::Values basicCard::Two   = 2;
const basicCard::Values basicCard::Three = 3;
const basicCard::Values basicCard::Four  = 4;
const basicCard::Values basicCard::Five  = 5;
const basicCard::Values basicCard::Six   = 6;
const basicCard::Values basicCard::Seven = 7;
const basicCard::Values basicCard::Eight = 8;
const basicCard::Values basicCard::Nine  = 9;
const basicCard::Values basicCard::Ten   = 10;
const basicCard::Values basicCard::Jack  = 11;
const basicCard::Values basicCard::Queen = 12;
const basicCard::Values basicCard::King  = 13;


// end static member def
void basicCard::turn( bool _faceup )
{
  faceup = _faceup;

  update();
  //showCard();
  //if( nextPtr )
  //  nextPtr->turn( faceup );
}

void basicCard::rotate45( int _direction )
{
  if( _direction <= 2 &&
      _direction >= -1 &&
      _direction != direction )
  {
    direction = _direction;
    if( direction )
      if( direction % 2 )
      {
	int size = (int) ( ( maps->CARDX + maps->CARDY ) / 1.4142136 );
	resize( size, size );
      }
      else
      {
	resize( maps->CARDY, maps->CARDX );
      }
    else
    {
      resize( maps->CARDX, maps->CARDY );
    }
  }
}

static QColorGroup colgrp( Qt::black, Qt::white, Qt::darkGreen.light(), Qt::darkGreen.dark(), Qt::darkGreen, Qt::black, Qt::white );

void basicCard::paintEvent( QPaintEvent* _event )
{
  if( direction )
  {
    QPixmap pix( width(), height() );      // create pixmap

    pix.fill( backgroundColor() );         // initialize pixmap

    QPainter painter;
    painter.begin( &pix );

    if ( empty() )
    {
      if( direction == 1 ) painter.translate( 82.02, 0.0 );
      else if( direction == 2 ); // painter.translate( 116.0, 0.0 );
      else  if( direction == -1 ) painter.translate( 0.0, 57.27 );
      if( direction == 2 )
      {
	qDrawShadePanel( &painter, 0, 0, cardMaps::CARDY, cardMaps::CARDX, colgrp, TRUE );
      }
      else
      {
	painter.rotate( direction * 45.0 );
	qDrawShadePanel( &painter, 0, 0, cardMaps::CARDX, cardMaps::CARDY, colgrp, TRUE );
	//painter.drawRect( 0, 0, cardMaps::CARDX, cardMaps::CARDY );
      }
    }
    else
    {
      if( direction == 1 )
	painter.translate( maps->CARDX, 0.0 );
      else if( direction == 2 )
	painter.translate( maps->CARDY, 0.0 );
      else if( direction == -1 )
	painter.translate( 0.0, maps->CARDX / sqrt( 2 ) );
      painter.rotate( direction * 45.0 );
      if( FaceUp() )
	painter.drawPixmap( 0, 0, *maps->image( value,suit ) );
      else
	painter.drawPixmap( 0, 0, *maps->backSide() );
    }
    painter.end();

    // bitBlt( this, 0, 0, &pix, 0, 0, -1, -1 ); // copy pixmap to widget
    bitBlt( this, _event->rect().topLeft() , &pix, _event->rect() ); // copy pixmap to widget

  }
  else if( empty() )
  {
    QPixmap pix( width(), height());      // create pixmap

    pix.fill( backgroundColor() );         // initialize pixmap

    QPainter painter;
    painter.begin( &pix );
    painter.drawRect( 0, 0, width(), height());
    qDrawShadePanel( &painter, 0, 0, width(), height(), colgrp, TRUE);
    painter.end();

    bitBlt( this, _event->rect().topLeft() , &pix, _event->rect() ); // copy pixmap to widget
    // bitBlt( this, 0, 0, &pix, 0, 0, -1, -1 ); // copy pixmap to widget
  }
  else
  {
    if( FaceUp() )
      bitBlt( this, _event->rect().topLeft(), maps->image( value, suit ), _event->rect() ); // copy pixmap to widget
      // bitBlt( this, 0, 0, maps->image( value, suit ), 0, 0, -1, -1 );
    else
      bitBlt( this, _event->rect().topLeft(), maps->backSide(), _event->rect() ); // copy pixmap to widget
      // bitBlt( this, 0, 0, maps->backSide(), 0, 0, -1, -1 );
  }
}

void basicCard::showCard()
{
  if( parentWidget() )
    setBackgroundColor( parentWidget()->backgroundColor() );

  if( empty() ) {
    setFrameStyle( QFrame::Panel | QFrame::Plain );
  } else {
    setFrameStyle( QFrame::Panel | QFrame::Raised);
    setAlignment( AlignTop | AlignLeft );
    if( faceup )
    {
      //bitBlt( this, 0, 0, maps.image( Value() - 1, Suit() - 1 ), 0, 0, -1, -1 );
    }
    else
    {
      setBackgroundColor( darkRed );
    }
  }
}

basicCard::~basicCard()
{
}

basicCard::basicCard( Values _value, Suits _suit,  QWidget* _parent, bool _empty )
  : QLabel( _parent, 0 )
  , suit( _suit )
  , value( _value )
  , empty_flag( _empty )
  , direction( 0 )
{
  if( !maps )
  {
    maps = new cardMaps; //   The pictures...
    cardmaps = maps;
  }

  faceup = TRUE;
  resize( maps->CARDX, maps->CARDY );
  showCard();
}

QPixmap basicCard::pixmap() const
{
  return *maps->image( value,suit );
}

#include "basiccard.moc"
