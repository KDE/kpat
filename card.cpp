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

static const char *suit_names[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
static const char *value_names[] = {"Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
                                    "Nine", "Ten", "Jack", "Queen", "King" };

Card::Card( Values v, Suits s, QCanvas* _parent )
    : QCanvasRectangle( _parent ),  _source(0), _suit( s ), _value( v ), scaleX(1.0), scaleY(1.0)
{
    _name = qstrdup(QString("%1 %2").arg(suit_names[s-1]).arg(value_names[v-1]).utf8());

    faceup = true;
    setSize( cardMap::CARDX, cardMap::CARDY );
    flipping = false;
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

int Card::realX() const
{
    if (animated())
        return destX;
    else
        return int(x());
}

int Card::realY() const
{
    if (animated())
        return destY;
    else
        return int(y());
}

bool Card::realFace() const
{
    if (animated()) {
        bool face = isFaceUp();
        kdDebug() << "realFace " << animSteps << " " << flipSteps <<  " " << face << endl;
        if ( animSteps >= flipSteps / 2-1 )
            return !face;
        else
            return face;
    } else
        return isFaceUp();
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
int Card::hz = 0;

void Card::setZ(int z)
{
    QCanvasRectangle::setZ(z);
    if (z > hz)
        hz = z;
}

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
    destZ = z();
    animSteps = flipSteps;
    setVelocity(dx/animSteps, dy/animSteps-flipLift);
    setAnimated(TRUE);
}

void Card::advance(int stage)
{
    kdDebug() << "advance " << name() << endl;
    if ( stage==1 ) {
	if ( animSteps-- <= 0 ) {
	    setAnimated(false);
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

void Card::animatedMove(int x2, int y2, int z2, int steps)
{
    destX = x2;
    destY = y2;
    destZ = z2;

    double x1 = x(), y1 = y(), dx = x2 - x1, dy = y2 - y1;
    setZ(hz++);

    if (steps) {
        // Ensure a good speed
        while ( fabs(dx/steps)+fabs(dy/steps) < 5.0 && steps > 4 )
            steps--;

        setAnimated(true);
        setVelocity(dx/steps, dy/steps);

        animSteps = steps;
    } else {
        // _really_ fast
        setAnimated(true);
        setAnimated(false);
    }
}

void Card::setAnimated(bool anim)
{
    if (animated() && !anim) {
        scaleX = 1.0;
        scaleY = 1.0;
        flipping = FALSE;
        setVelocity(0,0);
        move(destX,destY); // exact
        setZ(destZ);
    }
    QCanvasRectangle::setAnimated(anim);

}
