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

#include <math.h>
#include <assert.h>

#include <qpainter.h>

#include <kdebug.h>

#include "card.h"
#include "pile.h"
#include "cardmaps.h"


static const char  *suit_names[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
static const char  *rank_names[] = {"Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
                                     "Nine", "Ten", "Jack", "Queen", "King" };

// Run time type id
const int Card::RTTI = 1001;


Card::Card( Rank r, Suit s, QCanvas* _parent )
    : QCanvasRectangle( _parent ),
      m_suit( s ), m_rank( r ),
      m_source(0), scaleX(1.0), scaleY(1.0), tookDown(false)
{
    // Set the name of the card
    // FIXME: i18n()
    m_name = QString("%1 %2").arg(suit_names[s-1]).arg(rank_names[r-1]).utf8();

    // Default for the card is face up, standard size.
    m_faceup = true;
    setSize( cardMap::CARDX(), cardMap::CARDY() );

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;

    m_flipping  = false;
    m_animSteps = 0;
    m_flipSteps = 0;
}


Card::~Card()
{
    // If the card is in a pile, remove it from there.
    if (source())
	source()->remove(this);

    hide();
}


// ----------------------------------------------------------------
//              Member functions regarding graphics


// Return the pixmap of the card
//
QPixmap Card::pixmap() const
{
    return cardMap::self()->image( m_rank, m_suit );
}


// Turn the card if necessary.  If the face gets turned up, the card
// is activated at the same time.
//
void Card::turn( bool _faceup )
{
    if (m_faceup != _faceup) {
        m_faceup = _faceup;
        setActive(!isActive()); // abuse
    }
}

// Draw the card on the painter 'p'.
//
void Card::draw( QPainter &p )
{
    QPixmap  side;

    // Get the image to draw (front / back)
    if( isFaceUp() )
        side = cardMap::self()->image( m_rank, m_suit, isSelected());
    else
        side = cardMap::self()->backSide();

    // Rescale the image if necessary.
    if (scaleX <= 0.98 || scaleY <= 0.98) {
        QWMatrix  s;
        s.scale( scaleX, scaleY );
        side = side.xForm( s );
        int xoff = side.width() / 2;
        int yoff = side.height() / 2;
        p.drawPixmap( int(x() + cardMap::CARDX()/2 - xoff),
		      int(y() + cardMap::CARDY()/2 - yoff), side );
    } else
        p.drawPixmap( int(x()), int(y()), side );
}


void Card::moveBy(double dx, double dy)
{
    QCanvasRectangle::moveBy(dx, dy);
}


// Return the X of the cards real position.  This is the destination
// of the animation if animated, and the current X otherwise.
//
int Card::realX() const
{
    if (animated())
        return m_destX;
    else
        return int(x());
}


// Return the Y of the cards real position.  This is the destination
// of the animation if animated, and the current Y otherwise.
//
int Card::realY() const
{
    if (animated())
        return m_destY;
    else
        return int(y());
}


// Return the > of the cards real position.  This is the destination
// of the animation if animated, and the current Z otherwise.
//
int Card::realZ() const
{
    if (animated())
        return m_destZ;
    else
        return int(z());
}


// Return the "face up" status of the card.  
//
// This is the destination of the animation if animated and animation
// is more than half way, the original if animated and animation is
// less than half way, and the current "face up" status otherwise.
//

bool Card::realFace() const
{
    if (animated() && m_flipping) {
        bool face = isFaceUp();
        if ( m_animSteps >= m_flipSteps / 2 - 1 )
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


// Used to create an illusion of the card being lifted while flipped.
static const double flipLift = 1.2;

// The current maximum Z value.  This is used so that new cards always
// get placed on top of the old ones and don't get placed in the
// middle of a destination pile.
int  Card::Hz = 0;


void Card::setZ(double z)
{
    QCanvasRectangle::setZ(z);
    if (z > Hz)
        Hz = int(z);
}


// Start a move of the card using animation.
// 
// 'steps' is the number of steps the animation should take.
//
void Card::moveTo(int x2, int y2, int z2, int steps)
{
    m_destX = x2;
    m_destY = y2;
    m_destZ = z2;

    double  x1 = x();
    double  y1 = y();
    double  dx = x2 - x1;
    double  dy = y2 - y1;

    if (!dx && !dy) {
        setZ(z2);
        return;
    }
    setZ(Hz++);

    if (steps) {
        // Ensure a good speed
        while ( fabs(dx/steps)+fabs(dy/steps) < 5.0 && steps > 4 )
            steps--;

        setAnimated(true);
        setVelocity(dx/steps, dy/steps);

        m_animSteps = steps;

    } else {
        // _really_ fast
        setAnimated(true);
        setAnimated(false);
        emit stoped(this);
    }
}


// Animate a move to (x2, y2), and at the same time flip the card.
//
void Card::flipTo(int x2, int y2, int steps)
{
    // Check that we are not already animating.
    assert(!animated());

    int     x1 = (int)x();
    int     y1 = (int)y();
    double  dx = x2 - x1;
    double  dy = y2 - y1;

    // Mark this animation as a flip as well.
    m_flipping  = true;
    m_flipSteps = steps;

    // Set the target of the animation
    m_destX = x2;
    m_destY = y2;
    m_destZ = int(z());

    // Let the card be above all others during the animation.
    setZ(Hz++);

    m_animSteps = steps;
    setVelocity(dx/m_animSteps, dy/m_animSteps-flipLift);

    setAnimated(TRUE);
}


// Advance a card animation one step.  This function adds flipping of
// the card to the translation animation that QCanvasRectangle offers.
//
void Card::advance(int stage)
{
    if ( stage==1 ) {
	// If the animation is finished, emit stoped. (FIXME: name)
	if ( m_animSteps-- <= 0 ) {
	    setAnimated(false);
            emit stoped(this);
        } else {
	    // Animation is not finished. Check for flipping and add
	    // that animation to the simple translation.
	    if ( m_flipping ) {
		if ( m_animSteps > m_flipSteps / 2 ) {
		    // animSteps = flipSteps .. flipSteps/2 (flip up) -> 1..0
		    scaleX = ((double)m_animSteps/m_flipSteps-0.5)*2;
		} else {
		    // animSteps = flipSteps/2 .. 0 (flip down) -> 0..1
		    scaleX = 1-((double)m_animSteps/m_flipSteps)*2;
		}
		if ( m_animSteps == m_flipSteps / 2-1 ) {
		    setYVelocity(yVelocity()+flipLift*2);
		    turn( !isFaceUp() );
		}
	    }
	}
    }

    // Animate the translation of the card.
    QCanvasRectangle::advance(stage);
}


// Set 'animated' status to a new value, and set secondary values as
// well.
//
void Card::setAnimated(bool anim)
{
    // If no more animation, reset some other values as well.
    if (animated() && !anim) {
	// Reset all things that might have changed during the animation.
        scaleX = 1.0;
        scaleY = 1.0;
        m_flipping = FALSE;
        setVelocity(0, 0);

	// Move the card to its destination immediately.
        move(m_destX, m_destY);
        setZ(m_destZ);
    }

    QCanvasRectangle::setAnimated(anim);
}


void Card::setTakenDown(bool td)
{
    if (td)
        kdDebug(11111) << "took down " << name() << endl;
    tookDown = td;
}


bool Card::takenDown() const
{
    return tookDown;
}


// Get the card to the top.

void Card::getUp(int steps)
{
    m_destZ = int(z());
    m_destX = int(x());
    m_destY = int(y());
    setZ(Hz+1);

    // Animation
    m_animSteps = steps;
    setVelocity(0, 0);
    setAnimated(TRUE);
}

#include "card.moc"
