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
#include "pile.h"
#include "../speeds.h"

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


Card::Card( Rank r, Suit s, CardDeck * deck )
  : AbstractCard( (s << 4) + r ),
    m_deck( deck ),
    m_source( 0 ),
    m_animation( 0 ),
    m_takenDown( false ),
    m_highlighted( false )
{
    Q_ASSERT( m_deck );

    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    setTransformationMode( Qt::SmoothTransformation );

    QString suitName;
    switch( suit() )
    {
        case Clubs :    suitName = "Clubs";    break;
        case Diamonds : suitName = "Diamonds"; break;
        case Hearts :   suitName = "Hearts";   break;
        case Spades :   suitName = "Spades";   break;
        default :       suitName = "???";      break;
    }
    setObjectName( suitName + QString::number( rank() ) );

    m_destFace = isFaceUp();
    m_flippedness = m_faceup ? 1 : 0;
    m_highlightedness = m_highlighted ? 1 : 0;

    m_fadeAnimation = new QPropertyAnimation( this, "highlightedness", this );
    m_fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    m_fadeAnimation->setKeyValueAt( 0, 0 );
    m_fadeAnimation->setKeyValueAt( 1, 1 );

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;
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
    QPixmap pix;
    if( m_faceup )
        pix = m_deck->frontsidePixmap( data() );
    else
        pix = m_deck->backsidePixmap( data() );

    qreal highlightOpacity = m_fadeAnimation->state() == QAbstractAnimation::Running
                             ? m_highlightedness
                             : ( m_highlighted ? 1 : 0 );

    if ( highlightOpacity > 0 )
    {
        QPainter p( &pix );
        p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
        p.setOpacity( 0.5 * highlightOpacity );
        p.fillRect( 0, 0, pix.width(), pix.height(), Qt::black );
    }

    setPixmap( pix );
}

// Turn the card if necessary.  If the face gets turned up, the card
// is activated at the same time.
void Card::turn( bool _faceup )
{
    if ( m_faceup != _faceup || m_destFace != _faceup )
    {
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

void Card::raise()
{
    if ( zValue() < 1000 )
        setZValue( 1000 + zValue() );
}

// Return the  cards real position.  This is the destination
// of the animation if animated, and the current position otherwise.
QPointF Card::realPos() const
{
    if (animated())
        return QPointF(m_destX, m_destY);
    else
        return pos();
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

void Card::animate( QPointF pos2, qreal z2, qreal scale2, qreal rotation2, bool faceup2, bool raised, int duration )
{
    stopAnimation();

    if ( duration <= 0 )
    {
        setPos( pos2 );
        setZValue( z2 );
        setScale( scale2 );
        setRotation( rotation2 );
        turn( faceup2 );
        return;
    }

    QParallelAnimationGroup * aniGroup = new QParallelAnimationGroup( this );

    if ( qAbs( pos2.x() - x() ) > 2 || qAbs( pos2.y() - y() ) > 2 )
    {
        QPropertyAnimation * slide = new QPropertyAnimation( this, "pos" );
        slide->setKeyValueAt( 0, pos() );
        slide->setKeyValueAt( 1, pos2 );
        slide->setDuration( duration );
        aniGroup->addAnimation( slide );
    }
    else
    {
        setPos( pos2 );
    }

    if ( qAbs( scale2 - scale() ) > 0.05 )
    {
        QPropertyAnimation * resize = new QPropertyAnimation( this, "scale" );
        resize->setKeyValueAt( 0, scale() );
        resize->setKeyValueAt( 1, scale2 );
        resize->setDuration( duration );
        aniGroup->addAnimation( resize );
    }
    else
    {
        setScale( scale2 );
    }

    if ( qAbs( rotation2 - rotation() ) > 2 )
    {
        QPropertyAnimation * spin = new QPropertyAnimation( this, "rotation" );
        spin->setKeyValueAt( 0, rotation() );
        spin->setKeyValueAt( 1, rotation2 );
        spin->setDuration( duration );
        aniGroup->addAnimation( spin );
    }
    else
    {
        setRotation( rotation2 );
    }

    if ( faceup2 != m_faceup )
    {
        QPropertyAnimation * flip = new QPropertyAnimation( this, "flippedness" );
        flip->setKeyValueAt( 0, m_faceup ? 1.0 : 0.0 );
        flip->setKeyValueAt( 1, faceup2 ? 1.0 : 0.0 );
        flip->setDuration( duration );
        aniGroup->addAnimation( flip );
    }

    if ( raised )
        raise();

    m_destX = pos2.x();
    m_destY = pos2.y();
    m_destZ = z2;
    m_destFace = faceup2;

    if ( aniGroup->animationCount() == 0 )
    {
        delete aniGroup;
        setZValue( z2 );
    }
    else
    {
        m_animation = aniGroup;
        connect( m_animation, SIGNAL(finished()), SLOT(stopAnimation()) );
        m_animation->start();
        emit animationStarted( this );
    }
}


// Start a move of the card using animation.
void Card::moveTo( QPointF pos2, qreal z2, int duration )
{
    animate( pos2, z2, 1, 0, isFaceUp(), true, duration );
}

// Animate a move to (x2, y2), and at the same time flip the card.
void Card::flipTo( QPointF pos2, int duration )
{
    animate( pos2, zValue(), 1.0, 0.0, !isFaceUp(), true, duration );
}

void Card::flipToPile( Pile * pile, int duration )
{
    Q_ASSERT( pile );

    QPointF origPos = pos();

    pile->add(this);
    pile->layoutCards( DURATION_RELAYOUT );
    completeAnimation();
    QPointF destPos = realPos();

    setPos( origPos );
    flipTo( destPos, duration );
}

void Card::setTakenDown(bool td)
{
    m_takenDown = td;
}

bool Card::takenDown() const
{
    return m_takenDown;
}

void Card::setHighlighted( bool flag )
{
    if ( flag != m_highlighted )
    {
        m_highlighted = flag;

        m_fadeAnimation->setDirection( flag
                                       ? QAbstractAnimation::Forward
                                       : QAbstractAnimation::Backward );

        if ( m_fadeAnimation->state() != QAbstractAnimation::Running )
            m_fadeAnimation->start();
    }
}

void Card::setHighlightedness( qreal highlightedness )
{
    m_highlightedness = highlightedness;
    updatePixmap();
    return;
}

bool Card::isHighlighted() const
{
    return m_highlighted;
}

qreal Card::highlightedness() const
{
    return m_highlightedness;
}

void Card::completeAnimation()
{
    if ( !m_animation )
        return;

    m_animation->disconnect( this );
    if ( m_animation->state() != QAbstractAnimation::Stopped )
        m_animation->setCurrentTime( m_animation->duration() );

    stopAnimation();
}

void Card::stopAnimation()
{
    if ( !m_animation )
        return;

    delete m_animation;
    m_animation = 0;

    setZValue( m_destZ );

    emit animationStopped( this );
}

bool  Card::animated() const
{
    return m_animation != 0;
}

#include "card.moc"
