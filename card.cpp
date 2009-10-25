/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of 
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "card.h"

#include "carddeck.h"
#include "dealer.h"
#include "pile.h"
#include "speeds.h"

#include <KDebug>

#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QTimeLine>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QStyleOptionGraphicsItem>


AbstractCard::AbstractCard( Rank r, Suit s )
    : m_suit( s ), m_rank( r ), m_faceup( true )
{
}

Card::Card( Rank r, Suit s )
    : QObject(), AbstractCard( r, s ), QGraphicsPixmapItem(),
      m_source(0), tookDown(false), animation( 0 ),
      m_highlighted( false ), m_isZoomed( false )
{
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    setTransformationMode( Qt::SmoothTransformation );

    QString suitName;
    switch( m_suit )
    {
        case Clubs :    suitName = "Clubs";    break;
        case Diamonds : suitName = "Diamonds"; break;
        case Hearts :   suitName = "Hearts";   break;
        case Spades :   suitName = "Spades";   break;
        default :       suitName = "???";      break;
    }
    setObjectName( suitName + QString::number( m_rank ) );

    m_destFace = isFaceUp();
    m_flippedness = m_faceup ? 1.0 : 0.0;

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;

    m_spread = QSizeF( 0, 0 );
}

Card::~Card()
{
    // If the card is in a pile, remove it from there.
    if (source())
        source()->remove(this);
    if (scene())
        scene()->removeItem(this);
}

// ----------------------------------------------------------------
//              Member functions regarding graphics


void Card::updatePixmap()
{
    if ( !source() || !source()->dscene() || !source()->dscene()->cardDeck())
        return;

    CardDeck * deck = source()->dscene()->cardDeck();

    if( m_faceup )
    {
        QPixmap pix = deck->frontsidePixmap( m_rank, m_suit );
        if ( m_highlighted )
        {
            QPainter p( &pix );
            p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
            p.setOpacity( 0.5 );
            p.fillRect( 0, 0, pix.width(), pix.height(), Qt::black );
        }
        setPixmap( pix );
    }
    else
    {
        setPixmap( deck->backsidePixmap() );
    }

    m_boundingRect = QRectF( QPointF( 0, 0 ), pixmap().size() );
}

// Turn the card if necessary.  If the face gets turned up, the card
// is activated at the same time.
void Card::turn( bool _faceup )
{
    if ( m_faceup != _faceup ) {
        m_faceup = _faceup;
        m_destFace = _faceup;
        updatePixmap();
    }
    m_flippedness = m_faceup ? 1.0 : 0.0;
}

void Card::flip()
{
    turn( !m_faceup );
}

void Card::setFlippedness( qreal flippedness )
{
    if ( flippedness == m_flippedness )
        return;

    if ( ( flippedness >= 0.5 && m_flippedness < 0.5 )
         || ( flippedness <= 0.5 && m_flippedness > 0.5 ) )
    {
        m_faceup = m_destFace;
        updatePixmap();
    }

    m_flippedness = flippedness;

    qreal xOffset = pixmap().width() * ( 0.5 - qAbs( flippedness - 0.5 ) );
    qreal xScale = qAbs( 2 * flippedness - 1 );

    setTransform( QTransform().translate( xOffset, 0 ).scale( xScale, 1 ) );
}

qreal Card::flippedness() const
{
    return m_flippedness;
}


// Return the X of the cards real position.  This is the destination
// of the animation if animated, and the current X otherwise.
qreal Card::realX() const
{
    if (animated())
        return m_destX;
    else
        return x();
}


// Return the Y of the cards real position.  This is the destination
// of the animation if animated, and the current Y otherwise.
qreal Card::realY() const
{
    if (animated())
        return m_destY;
    else
        return y();
}


// Return the > of the cards real position.  This is the destination
// of the animation if animated, and the current Z otherwise.
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
bool Card::realFace() const
{
    return m_destFace;
}

// The current maximum Z value.  This is used so that new cards always
// get placed on top of the old ones and don't get placed in the
// middle of a destination pile.
qreal  Card::Hz = 0;

void Card::setZValue(qreal z)
{
    QGraphicsPixmapItem::setZValue(z);
    if (z > Hz)
        Hz = z;
}


// Start a move of the card using animation.
//
// 'steps' is the number of steps the animation should take.
void Card::moveTo(qreal x2, qreal y2, qreal z2, int duration)
{
    stopAnimation();

    if ( qAbs( x2 - x() ) < 2 && qAbs( y2 - y() ) < 1 )
    {
        setPos( QPointF( x2, y2 ) );
        setZValue( z2 );
        return;
    }

    QPropertyAnimation * an = new QPropertyAnimation(this, "pos");
    an->setDuration( duration );
    an->setKeyValueAt( 0, pos() );
    an->setKeyValueAt( 1, QPointF( x2, y2 ));
    animation = an;

    connect( animation, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_destX = x2;
    m_destY = y2;
    m_destZ = z2;

    if (qAbs( x2 - x() ) < 1 && qAbs( y2 - y() ) < 1) {
        setZValue(z2);
        return;
    }
    // if ( fabs( z2 - zValue() ) >= 1 )
        setZValue(Hz++);

    animation->start();
}

// Animate a move to (x2, y2), and at the same time flip the card.
void Card::flipTo(qreal x2, qreal y2, int duration)
{
    stopAnimation();

    QPropertyAnimation * flip = new QPropertyAnimation( this, "flippedness" );
    flip->setKeyValueAt( 0, isFaceUp() ? 1 : 0 );
    flip->setKeyValueAt( 1, isFaceUp() ? 0 : 1 );
    flip->setDuration( duration );

    QPropertyAnimation * slide = new QPropertyAnimation( this, "pos" );
    slide->setKeyValueAt( 0, pos() );
    slide->setKeyValueAt( 1, QPointF( x2, y2 ) );
    slide->setDuration( duration );

    QParallelAnimationGroup * aniGroup = new QParallelAnimationGroup( this );
    aniGroup->addAnimation( flip );
    aniGroup->addAnimation( slide );
    animation = aniGroup;
    connect( animation, SIGNAL(finished()), SLOT(stopAnimation()) );

    // Set the target of the animation
    m_destX = x2;
    m_destY = y2;
    m_destZ = zValue();
    m_destFace = !m_faceup;

    // Let the card be above all others during the animation.
    setZValue(Hz++);

    animation->start();
}


void Card::setTakenDown(bool td)
{
    tookDown = td;
}

bool Card::takenDown() const
{
    return tookDown;
}

void Card::setHighlighted( bool flag ) {
    if ( m_highlighted != flag )
    {
        m_highlighted = flag;
        updatePixmap();
    }
}

void Card::stopAnimation()
{
    if ( !animation )
        return;

    if ( animation->state() != QAbstractAnimation::Stopped )
        animation->setCurrentTime( animation->duration() );

    delete animation;
    animation = 0;

    setPos( QPointF( m_destX, m_destY ) );
    setZValue( m_destZ );
    if ( source() )
        setSpread( source()->cardOffset(this) );

    emit stopped( this );
}

bool  Card::animated() const
{
    return animation != 0;
}

void Card::mousePressEvent ( QGraphicsSceneMouseEvent *ev )
{
    if ( this == source()->top() )
        return; // no way this is meaningful

    if ( ev->button() == Qt::RightButton && !animated() )
        zoomInAnimation();
}

void Card::mouseReleaseEvent ( QGraphicsSceneMouseEvent * ev )
{
    if ( this == source()->top() )
        return; // no way this is meaningful

    if ( m_isZoomed && ev->button() == Qt::RightButton )
        zoomOutAnimation();
}

void Card::zoomInAnimation()
{
    stopAnimation();

    m_originalPosition = pos();
    QPointF dest( pos().x() + boundingRect().width() / 3,
                  pos().y() - boundingRect().height() / 4 );

    QPropertyAnimation * zoom = new QPropertyAnimation( this, "scale" );
    zoom->setKeyValueAt( 0, 1.0 );
    zoom->setKeyValueAt( 1, 1.1 );
    zoom->setDuration( DURATION_FANCYSHOW );

    QPropertyAnimation * rotate = new QPropertyAnimation( this, "rotation" );
    rotate->setKeyValueAt( 0, 0 );
    rotate->setKeyValueAt( 1, 20 );
    rotate->setDuration( DURATION_FANCYSHOW );

    QPropertyAnimation * slide = new QPropertyAnimation( this, "pos" );
    slide->setKeyValueAt( 0, pos() );
    slide->setKeyValueAt( 1, dest );
    slide->setDuration( DURATION_FANCYSHOW );

    QParallelAnimationGroup * aniGroup = new QParallelAnimationGroup( this );
    aniGroup->addAnimation( zoom );
    aniGroup->addAnimation( rotate );
    aniGroup->addAnimation( slide );
    animation = aniGroup;
    connect( animation, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_isZoomed = true;

    m_destZ = zValue();
    m_destX = dest.x();
    m_destY = dest.y();

    animation->start();
}

void Card::zoomOutAnimation()
{
    stopAnimation();

    QPropertyAnimation * zoom = new QPropertyAnimation( this, "scale" );
    zoom->setKeyValueAt( 0, 1.1 );
    zoom->setKeyValueAt( 1, 1.0 );
    zoom->setDuration( DURATION_FANCYSHOW );

    QPropertyAnimation * rotate = new QPropertyAnimation( this, "rotation" );
    rotate->setKeyValueAt( 0, 20 );
    rotate->setKeyValueAt( 1, 0 );
    rotate->setDuration( DURATION_FANCYSHOW );

    QPropertyAnimation * slide = new QPropertyAnimation( this, "pos" );
    slide->setKeyValueAt( 0, pos() );
    slide->setKeyValueAt( 1, m_originalPosition );
    slide->setDuration( DURATION_FANCYSHOW );

    QParallelAnimationGroup * aniGroup = new QParallelAnimationGroup( this );
    aniGroup->addAnimation( zoom );
    aniGroup->addAnimation( rotate );
    aniGroup->addAnimation( slide );
    animation = aniGroup;
    connect( animation, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_isZoomed = false;

    m_destZ = zValue();
    m_destX = m_originalPosition.x();
    m_destY = m_originalPosition.y();

    animation->start();
}

QRectF Card::boundingRect() const
{
    return m_boundingRect;
}

QSizeF Card::spread() const
{
    return m_spread;
}

void Card::setSpread(const QSizeF& spread)
{
    if (source() && m_spread != spread)
        source()->tryRelayoutCards();
    m_spread = spread;
}

QString gettime()
{
    static struct timeval tv2 = { -1, -1};
    struct timeval tv;
    gettimeofday( &tv, 0 );
    if ( tv2.tv_sec == -1 )
        gettimeofday( &tv2, 0 );
    return QString::number( ( tv.tv_sec - tv2.tv_sec ) * 1000 + ( tv.tv_usec -tv2.tv_usec ) / 1000 );
}

#include "card.moc"
