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
#include <qgraphicssvgitem.h>
#include <QSvgRenderer>
#include <QPixmapCache>
#include <kdebug.h>

#include "card.h"
#include "pile.h"
#include "cardmaps.h"
#include "dealer.h"

static const char  *suit_names[] = {"club", "diamond", "heart", "spade"};
static const char  *rank_names[] = {"1", "2", "3", "4", "5", "6", "7", "8",
                                     "9", "10", "jack", "queen", "king" };

// Run time type id
const int Card::my_type = Dealer::CardTypeId;

Card::Card( Rank r, Suit s, QGraphicsScene *_parent )
    : QGraphicsSvgItem( ),
      m_suit( s ), m_rank( r ),
      m_source(0), tookDown(false), animation( 0 ), m_highlighted( false )
{
    _parent->addItem( this );

    setSharedRenderer( cardMap::self()->renderer() );
    QString element = QString( "%1_%2" ).arg( rank_names[m_rank-1] ).arg( suit_names[m_suit-1] );
    setElementId( element );

    update();

    // Set the name of the card
    m_name = QString("%1 %2").arg(suit_names[s-1]).arg(rank_names[r-1]).toUtf8();
    m_hoverTimer = new QTimer(this);
    m_hovered = false;
    // Default for the card is face up, standard size.
    m_faceup = true;

    setCachingEnabled( false );

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;

    setAcceptsHoverEvents( true );

    //m_hoverTimer->setSingleShot(true);
    //m_hoverTimer->setInterval(50);
    connect(m_hoverTimer, SIGNAL(timeout()),
            this, SLOT(zoomInAnimation()));
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


void Card::update()
{
    QMatrix matrix;
    double scale = cardMap::self()->scaleFactor();
    matrix.scale( scale, scale );
    setMatrix( matrix, false );
}

// Turn the card if necessary.  If the face gets turned up, the card
// is activated at the same time.
//
void Card::turn( bool _faceup )
{
    kDebug() << "turn " << _faceup << " " << m_faceup << endl;

    if (m_faceup != _faceup) {
        m_faceup = _faceup;
        //QBrush b = brush();
        if ( m_faceup ) {
            m_normalPixmap = cardMap::self()->image( m_rank, m_suit );
            m_movePixmap = m_normalPixmap;
            QPixmap trans( m_normalPixmap.width(), m_normalPixmap.height() );
	    int gra = 210;
            trans.fill( QColor(gra, gra, gra) );
            m_movePixmap.setAlphaChannel(trans);
            setElementId( QString( "%1_%2" ).arg( rank_names[m_rank-1] ).arg( suit_names[m_suit-1] ) );
        } else {
            setElementId( QLatin1String( "back" ) );
            m_normalPixmap = cardMap::self()->backSide();
        }
    }
}

void Card::flip()
{
    turn( !m_faceup );
}

void Card::moveBy(double dx, double dy)
{
    QGraphicsSvgItem::moveBy(dx, dy);
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
qreal Card::realZ() const
{
    if (animated())
        return m_destZ;
    else
        return zValue();
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
    QGraphicsSvgItem::setZValue(z);
    if (z > Hz)
        Hz = int(z);
}


// Start a move of the card using animation.
//
// 'steps' is the number of steps the animation should take.
//
void Card::moveTo(qreal x2, qreal y2, qreal z2, int duration)
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
    timeLine->setDuration( duration );
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
void Card::flipTo(int x2, int y2)
{
    stopAnimation();

    qreal  x1 = x();
    qreal  y1 = y();

    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    double scale = cardMap::self()->scaleFactor();
    animation->setScaleAt( 0, scale, scale );
    animation->setScaleAt( 0.5, 0.0, scale );
    animation->setScaleAt( 1, scale, scale );
    QPointF hp = pos();
    hp.setX( ( x1 + x2 + boundingRect().width() ) / 2 );
    if ( y1 != y2 )
        hp.setY( ( y1 + y2 + boundingRect().height() ) / 2 );
    kDebug() << "hp " << pos() << " " << hp << " " << QPointF( x2, y2 ) << endl;
    animation->setPosAt(0.5, hp );
    animation->setPosAt(1, QPointF( x2, y2 ));

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->setDuration( 700 );
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
        kDebug() << "flipAnimationChanged " << endl;
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

void Card::setHighlighted( bool flag ) {
    m_highlighted = flag;
    QGraphicsSvgItem::update();
}

void Card::stopAnimation()
{
    if ( !animation )
        return;
    kDebug() << "stopAnimation " << m_destX << " " << m_destY << endl;
    QGraphicsItemAnimation *old_animation = animation;
    animation = 0;
    if ( old_animation->timeLine()->state() == QTimeLine::Running )
        old_animation->timeLine()->setCurrentTime(3000);
    old_animation->timeLine()->stop();
    setPos( m_destX, m_destY );
    setZValue( m_destZ );
    update();
    QGraphicsSvgItem::update();
    emit stoped( this );
}

bool  Card::animated() const
{
    return animation != 0;
}

void Card::hoverEnterEvent ( QGraphicsSceneHoverEvent *  )
{
    if ( animated() || !isFaceUp() )
        return;

    return;

    m_hoverTimer->start(200);
    //zoomIn(400);
    //kDebug() << "hoverEnterEvent\n";
}

void Card::hoverLeaveEvent ( QGraphicsSceneHoverEvent * )
{
    m_hoverTimer->stop();
    stopAnimation();

    if ( !isFaceUp() || !m_hovered)
        return;

    m_hovered = false;

    //zoomOut(200);
}

void Card::mousePressEvent ( QGraphicsSceneMouseEvent * ) {
    kDebug() << "mousePressEvent\n";
    if ( !isFaceUp() )
        return;
    m_hoverTimer->stop();
    stopAnimation();
    zoomOut(100);
    m_hovered = false;
    //TODO
    //setPixmap(m_movePixmap);
}

void Card::mouseReleaseEvent ( QGraphicsSceneMouseEvent * ) {
    kDebug() << "mouseReleaseEvent\n";
    //TODO
    //setPixmap(m_normalPixmap);
}

// Get the card to the top.

void Card::getUp()
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
                 QWidget *w)
{
//    painter->setRenderHint(QPainter::Antialiasing);
    //painter->setRenderHint(QPainter::SmoothPixmapTransform);
    if (scene()->mouseGrabberItem() == this) {
        //painter->setOpacity(.8);
    }
    QGraphicsSvgItem::paint(painter, option, w);

    if ( isHighlighted() ) {
        painter->setBrush( QColor( 40, 40, 40, 127 ));
        painter->setPen( Qt::NoPen );
        painter->drawRect(boundingRect());
    }

}

void Card::zoomIn(int t)
{
    m_hovered = true;
    QTimeLine *timeLine = new QTimeLine( t, this );

    m_originalPosition = pos();
    animation = new QGraphicsItemAnimation( this );
    animation->setItem( this );
    animation->setTimeLine( timeLine );
    QPointF dest =  QPointF( pos().x() - boundingRect().width() / 3,
                             pos().y() - boundingRect().height() / 8 );
    animation->setPosAt( 1, dest );
    double scale = cardMap::self()->scaleFactor();
    animation->setScaleAt( 1, scale * 1.1, scale * 1.1 );
    animation->setRotationAt( 1, -20 );
    //qreal x2 = pos().x() + boundingRect().width() / 2 - boundingRect().width() * 1.1 / 2;
    //qreal y2 = pos().y() + boundingRect().height() / 2 - boundingRect().height() * 1.1 / 2;
    //animation->setScaleAt( 1, 1, 1 );

    timeLine->setUpdateInterval( 1000 / 25 );
    timeLine->setFrameRange( 0, 100 );
    timeLine->setLoopCount( 1 );
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );

    m_destZ = zValue();
    m_destX = x();
    m_destY = y();
}

void Card::zoomOut(int t)
{
    QTimeLine *timeLine = new QTimeLine( t, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setRotationAt( 0, -20 );
    animation->setRotationAt( 0.5, -10 );
    animation->setRotationAt( 1, 0 );
    double scale = cardMap::self()->scaleFactor();
    animation->setScaleAt( 0, 1.1 * scale, 1.1 * scale );
    animation->setScaleAt( 1, scale, scale );
    //animation->setPosAt( 1, m_originalPosition );
    //qreal x2 = pos().x() + boundingRect().width() / 2 - boundingRect().width() * 1.1 / 2;
    //qreal y2 = pos().y() + boundingRect().height() / 2 - boundingRect().height() * 1.1 / 2;
    //animation->setScaleAt( 1, 1, 1 );

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );

    m_destZ = zValue();
    m_destX = x();
    m_destY = y();
}

void Card::zoomInAnimation()
{
    m_hoverTimer->stop();
    zoomIn(400);
}

void Card::zoomOutAnimation()
{
    zoomOut(100);
}

#include "card.moc"
