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

*******************************************************/

#include <qapplication.h>

#include "card.h"
#include <kdebug.h>
#include "cardmaps.h"
#include <qpainter.h>
#include <math.h>
#include "pile.h"
#include <qimage.h>
#include <assert.h>

void Card::draw( QPainter &p )
{
    QPixmap side;
    if( isFaceUp() )
        side = cardMap::self()->image( _value, _suit, selected());
    else
        side = cardMap::self()->backSide();

    if (scaleX <= 0.98 || scaleY <= 0.98) {
        QWMatrix s;
        s.scale( scaleX, scaleY );
        side = side.xForm( s );
        int xoff = side.width() / 2;
        int yoff = side.height() / 2;
        p.drawPixmap( x() + cardMap::CARDX/2 - xoff, y() + cardMap::CARDY/2 - yoff, side );
    } else
        p.drawPixmap( x(), y(), side );
}

Card::~Card()
{
    if (source()) source()->remove(this);
    delete [] _name;
    hide();
}

static const char *suit_names[] = {"None", "Clubs", "Diamonds", "Hearts", "Spades"};
static const char *value_names[] = {"Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
                                    "Nine", "Ten", "Jack", "Queen", "King" };

Card::Card( Values v, Suits s, QCanvas* _parent )
    : QCanvasRectangle( _parent ),  _source(0), _suit( s ), _value( v ), scaleX(1.0), scaleY(1.0)
{
    _name = qstrdup(QString("%1 %2").arg(suit_names[s]).arg(value_names[v]).utf8());

    faceup = true;
    setSize( cardMap::CARDX, cardMap::CARDY );
}

QPixmap Card::pixmap() const
{
    return cardMap::self()->image( _value, _suit );
}

// end static member def
void Card::turn( bool _faceup )
{
    if (faceup != _faceup) {
        faceup = _faceup;
        setActive(!active()); // abuse
    }
}

void Card::moveBy(double dx, double dy)
{
    QCanvasRectangle::moveBy(dx, dy);
}

void Card::enlargeCanvas(QCanvasRectangle *c)
{
    if (!c->visible())
        return;

    bool changed = false;
    QSize size = c->canvas()->size();
    if (c->x() + c->width() + 10 > size.width()) {
        size.setWidth(c->x() + c->width() + 10);
        changed = true;
    }
    if (c->y() + c->height() + 10 > size.height()) {
        size.setHeight(c->y() + c->height() + 10);
        changed = true;
    }

    if (changed)
        c->canvas()->resize(size.width(), size.height());
}

/// the following copyright is for the flipping code
/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Palmtop Environment.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

static const double flipLift = 1.5;

void Card::flipTo(int x2, int y2, int steps)
{
    assert(!animated());

    flipSteps = steps;

    int x1 = (int)x();
    int y1 = (int)y();
    double dx = x2 - x1;
    double dy = y2 - y1;

    flipping = TRUE;
    destX = x2;
    destY = y2;
    animSteps = flipSteps;
    setVelocity(dx/animSteps, dy/animSteps-flipLift);
    setAnimated(TRUE);
}

void Card::advance(int stage)
{
    if ( stage==1 ) {
	if ( animSteps-- <= 0 ) {
	    scaleX = 1.0;
	    scaleY = 1.0;
	    flipping = FALSE;
	    setVelocity(0,0);
	    setAnimated(FALSE);
	    move(destX,destY); // exact
	} else {
	    if ( flipping ) {
		if ( animSteps > flipSteps / 2 ) {
		    // animSteps = flipSteps .. flipSteps/2 (flip up) -> 1..0
		    scaleX = ((double)animSteps/flipSteps-0.5)*2;
		} else {
		    // animSteps = flipSteps/2 .. 0 (flip down) -> 0..1
		    scaleX = 1-((double)animSteps/flipSteps)*2;
		}
		if ( animSteps == flipSteps / 2-1 ) {
		    setYVelocity(yVelocity()+flipLift*2);
		    turn( !isFaceUp() );
		}
	    }
	}
    }
    QCanvasRectangle::advance(stage);
}


