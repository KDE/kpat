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
//Added by qt3to4:
#include <QPixmap>
#include <QBrush>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QStyleOptionGraphicsItem>

#include <kdebug.h>

#include "card.h"
#include "pile.h"
#include "cardmaps.h"
#include "dealer.h"

static const char  *suit_names[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
static const char  *rank_names[] = {"Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
                                     "Nine", "Ten", "Jack", "Queen", "King" };

// Run time type id
const int Card::my_type = Dealer::CardTypeId;


Card::Card( Rank r, Suit s, QGraphicsScene* _parent )
    : QGraphicsRectItem( QRectF( 0, 0, cardMap::CARDX(), cardMap::CARDY() ), 0, _parent ),
      m_suit( s ), m_rank( r ),
      m_source(0), tookDown(false), animation( 0 )
{
    // Set the name of the card
    m_name = QString("%1 %2").arg(suit_names[s-1]).arg(rank_names[r-1]).toUtf8();

    // Default for the card is face up, standard size.
    m_faceup = true;

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;

    setPen(QPen(Qt::NoPen));
    setAcceptsHoverEvents( true );

    setFlag( QGraphicsRectItem::ItemIsSelectable, true );
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
        QBrush b = brush();
        if ( m_faceup )
            b.setTexture( cardMap::self()->image( m_rank, m_suit, isSelected() ) );
        else
            b.setTexture( cardMap::self()->backSide() );
        setBrush( b );
    }
}

void Card::flip()
{
    turn( !m_faceup );
}

void Card::moveBy(double dx, double dy)
{
    QGraphicsRectItem::moveBy(dx, dy);
}


// Return the X of the cards real position.  This is the destination
// of the animation if animated, and the current X otherwise.
//
qreal Card::realX() const
{
    if (animated())
        return m_destX;
    else
        return x();
}


// Return the Y of the cards real position.  This is the destination
// of the animation if animated, and the current Y otherwise.
//
qreal Card::realY() const
{
    if (animated())
        return m_destY;
    else
        return y();
}


// Return the > of the cards real position.  This is the destination
// of the animation if animated, and the current Z otherwise.
//
int Card::realZ() const
{
    if (animated())
        return m_destZ;
    else
        return int(zValue());
}


// Return the "face up" status of the card.
//
// This is the destination of the animation if animated and animation
// is more than half way, the original if animated and animation is
// less than half way, and the current "face up" status otherwise.
//

bool Card::realFace() const
{
#warning FIXME - this needs some caching
    return isFaceUp();
#if 0
    if (animated() && m_flipping) {
        bool face = isFaceUp();
        if ( m_animSteps >= m_flipSteps / 2 - 1 )
            return !face;
        else
            return face;
    } else
        return isFaceUp();
#endif
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


void Card::setZValue(double z)
{
    QGraphicsRectItem::setZValue(z);
    if (z > Hz)
        Hz = int(z);
}


// Start a move of the card using animation.
//
// 'steps' is the number of steps the animation should take.
//
void Card::moveTo(qreal x2, qreal y2, int z2, int steps)
{
    stopAnimation();

    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation;
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setPosAt(1, QPointF( x2, y2 ));

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setCurveShape(QTimeLine::EaseInCurve);
    timeLine->setLoopCount(1);
    timeLine->setDuration( 530 );
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );

    m_destX = x2;
    m_destY = y2;
    m_destZ = z2;

    double  x1 = x();
    double  y1 = y();

    double  dx = x2 - x1;
    double  dy = y2 - y1;

    if (!dx && !dy) {
        setZValue(z2);
        return;
    }
    setZValue(Hz++);
}

// Animate a move to (x2, y2), and at the same time flip the card.
//
void Card::flipTo(int x2, int y2, int steps)
{
    stopAnimation();

    qreal  x1 = x();
    qreal  y1 = y();
    qreal  dx = x2 - x1;
    qreal  dy = y2 - y1;

    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setScaleAt( 0, 1, 1.0 );
    animation->setScaleAt( 0.5, 0.0, 1.0 );
    animation->setScaleAt( 1, 1, 1.0 );
    QPointF hp = pos();
    hp.setX( ( x1 + x2 + boundingRect().width() ) / 2 );
    if ( y1 != y2 )
        hp.setY( ( y1 + y2 + boundingRect().height() ) / 2 );
    animation->setPosAt(0.5, hp );
    animation->setPosAt(1, QPointF( x2, y2 ));

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->setDuration( 400 );
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );
    connect( timeLine, SIGNAL( valueChanged( qreal ) ), SLOT( flipAnimationChanged( qreal ) ) );

    // Set the target of the animation
    m_destX = x2;
    m_destY = y2;
    m_destZ = int(zValue());

    // Let the card be above all others during the animation.
    setZValue(Hz++);
}


void Card::flipAnimationChanged( qreal r)
{
    if ( r > 0.5 && !isFaceUp() ) {
        flip();
    }
}

void Card::setTakenDown(bool td)
{
    if (td)
        kDebug(11111) << "took down " << name() << endl;
    tookDown = td;
}


bool Card::takenDown() const
{
    return tookDown;
}

void Card::stopAnimation()
{
    if ( !animation )
        return;
    QGraphicsItemAnimation *old_animation = animation;
    animation = 0;
    if ( old_animation->timeLine()->state() == QTimeLine::Running )
        old_animation->timeLine()->setCurrentTime(3000);
    old_animation->timeLine()->stop();
    setPos( m_destX, m_destY );
    setZValue( m_destZ );
    update();
    emit stoped( this );
}

bool  Card::animated() const
{
    return animation != 0;
}

void Card::hoverEnterEvent ( QGraphicsSceneHoverEvent * event )
{
    if ( animated() || !isFaceUp() )
        return;

    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setScaleAt( 0, 1, 1.0 );
    animation->setScaleAt( 0.5, 1.1, 1.1 );
    qreal x2 = pos().x() + boundingRect().width() / 2 - boundingRect().width() * 1.1 / 2;
    qreal y2 = pos().y() + boundingRect().height() / 2 - boundingRect().height() * 1.1 / 2;
    animation->setPosAt( 0.5, QPointF( x2, y2 ) );
    animation->setPosAt( 1, pos() );
    animation->setScaleAt( 1, 1, 1 );

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->setDuration( 1000 );
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );

    m_destZ = zValue();
    m_destX = x();
    m_destY = y();

    //kDebug() << "hoverEnterEvent\n";
}

void Card::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event )
{
    //kDebug() << "hoverLeaveEvent\n";
}

void Card::hoverMoveEvent ( QGraphicsSceneHoverEvent * event )
{
    //kDebug() << "hoverMoveEvent\n";
}

void Card::dragEnterEvent ( QGraphicsSceneDragDropEvent * event ) {
    kDebug() << "dragEnterEvent\n";
}
void Card::dragLeaveEvent ( QGraphicsSceneDragDropEvent * event ) {
    kDebug() << "dragLeaveEvent\n";

}
void Card::dragMoveEvent ( QGraphicsSceneDragDropEvent * event ) {
    kDebug() << "dragMoveEvent\n";
}

void Card::mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * event ) {
    kDebug() << "mouseDoubleClickEvent\n";

}
void Card::mouseMoveEvent ( QGraphicsSceneMouseEvent * event ) {
        kDebug() << "mouseMoveEvent\n";
}

void Card::mousePressEvent ( QGraphicsSceneMouseEvent * event ) {
        kDebug() << "mousePressEvent\n";

}
void Card::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event ) {
    kDebug() << "mouseReleaseEvent\n";
}

// Get the card to the top.

void Card::getUp(int steps)
{
    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);

    timeLine->setDuration( 1500 );
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );

    m_destZ = zValue();
    m_destX = x();
    m_destY = y();
    setZValue(Hz+1);
}

void Card::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *widget )
{
    kDebug() << "paint " << ( option->state & QStyle::State_Selected ) << " " << name() << endl;
    if (scene()->mouseGrabberItem() == this) {
        painter->setOpacity(.8);
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(brush());
    painter->drawRect(rect());

    if (option->state & QStyle::State_Selected) {
        painter->setBrush( QColor( 40, 40, 40, 127 ));
        painter->drawRect(rect() );
    }

}

#include "card.moc"
