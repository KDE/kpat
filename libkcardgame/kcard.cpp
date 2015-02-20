/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "kcard_p.h"

#include "kabstractcarddeck.h"
#include "kcardpile.h"

#include <QDebug>

#include <QtCore/QPropertyAnimation>
#include <QPainter>

#include <cmath>


namespace
{
    const qreal raisedZValue = 10000;
}


KCardAnimation::KCardAnimation( KCardPrivate * d,
                                int duration,
                                QPointF pos,
                                qreal rotation,
                                bool faceUp )
  : QAbstractAnimation( d ),
    d( d ),
    m_duration( duration ),
    m_x0( d->q->x() ),
    m_y0( d->q->y() ),
    m_rotation0( d->q->rotation() ),
    m_flippedness0( d->flippedness() ),
    m_xDelta( pos.x() - m_x0 ),
    m_yDelta( pos.y() - m_y0 ),
    m_rotationDelta( rotation - m_rotation0 ),
    m_flippednessDelta( (faceUp ? 1.0 : 0.0) - m_flippedness0 )
{
    qreal w = d->deck->cardWidth();
    qreal h = d->deck->cardHeight();
    qreal diagSquared = w * w + h * h;
    qreal distSquared = m_xDelta * m_xDelta + m_yDelta * m_yDelta;

    m_flipProgressFactor = qMax<qreal>( 1, sqrt( distSquared / diagSquared ) );
}


int KCardAnimation::duration() const
{
    return m_duration;
}


void KCardAnimation::updateCurrentTime( int msec )
{
    qreal progress = qreal(msec) / m_duration;
    qreal flipProgress = qMin<qreal>( 1, progress * m_flipProgressFactor );

    d->q->setPos( m_x0 + m_xDelta * progress, m_y0 + m_yDelta * progress );
    d->q->setRotation( m_rotation0 + m_rotationDelta * progress );
    d->setFlippedness( m_flippedness0 + m_flippednessDelta * flipProgress );
}


KCardPrivate::KCardPrivate( KCard * card )
  : QObject( card ),
    q( card )
{
}


void KCardPrivate::setFlippedness( qreal flippedness )
{
    if ( flippedness == flipValue )
        return;

    if ( flipValue < 0.5 && flippedness >= 0.5 )
        q->setPixmap( frontPixmap );
    else if ( flipValue >= 0.5 && flippedness < 0.5 )
        q->setPixmap( backPixmap );

    flipValue = flippedness;

    qreal xOffset = deck->cardWidth() * ( 0.5 - qAbs( flippedness - 0.5 ) );
    qreal xScale = qAbs( 2 * flippedness - 1 );

    q->setTransform( QTransform().translate( xOffset, 0 ).scale( xScale, 1 ) );
}


qreal KCardPrivate::flippedness() const
{
    return flipValue;
}


void KCardPrivate::setHighlightedness( qreal highlightedness )
{
    highlightValue = highlightedness;
    q->update();
}


qreal KCardPrivate::highlightedness() const
{
    return highlightValue;
}


KCard::KCard( quint32 id, KAbstractCardDeck * deck )
  : QObject(),
    QGraphicsPixmapItem(),
    d( new KCardPrivate( this ) )
{
    d->id = id;
    d->deck = deck;

    d->faceUp = true;
    d->flipValue = d->faceUp ? 1 : 0;
    d->highlighted = false;
    d->highlightValue = d->highlighted ? 1 : 0;

    d->pile = 0;

    d->animation = 0;

    d->fadeAnimation = new QPropertyAnimation( d, "highlightedness", d );
    d->fadeAnimation->setDuration( 150 );
    d->fadeAnimation->setKeyValueAt( 0, 0 );
    d->fadeAnimation->setKeyValueAt( 1, 1 );
}


KCard::~KCard()
{
    stopAnimation();

    // If the card is in a pile, remove it from there.
    if ( pile() )
        pile()->remove( this );
}


int KCard::type() const
{
    return KCard::Type;
}


quint32 KCard::id() const
{
    return d->id;
}


int KCard::rank() const
{
    return d->deck->rankFromId( d->id );
}


int KCard::suit() const
{
    return d->deck->suitFromId( d->id );
}


int KCard::color() const
{
    return d->deck->colorFromId( d->id );
}


void KCard::setPile( KCardPile * pile )
{
    d->pile = pile;
}


KCardPile * KCard::pile() const
{
    return d->pile;
}


void KCard::setFaceUp( bool faceUp )
{
    qreal flippedness = faceUp ? 1.0 : 0.0;
    if ( d->faceUp != faceUp || d->flipValue != flippedness )
    {
        d->faceUp = faceUp;
        d->setFlippedness( flippedness );
    }
}


bool KCard::isFaceUp() const
{
    return d->faceUp;
}


void KCard::animate( QPointF pos, qreal z, qreal rotation, bool faceUp, bool raised, int duration )
{
    stopAnimation();

    if ( duration > 0
         && ( qAbs( pos.x() - x() ) > 2
              || qAbs( pos.y() - y() ) > 2
              || qAbs( rotation - this->rotation() ) > 2
              || faceUp != d->faceUp ) )
    {
        if ( raised )
            raise();

        d->destZ = z;
        d->faceUp = faceUp;

        d->animation = new KCardAnimation( d, duration, pos, rotation, faceUp );
        connect(d->animation, &KCardAnimation::finished, this, &KCard::stopAnimation);
        d->animation->start();
        emit animationStarted( this );
    }
    else
    {
        setPos( pos );
        setZValue( z );
        setRotation( rotation );
        setFaceUp( faceUp );
    }
}


bool KCard::isAnimated() const
{
    return d->animation != 0;
}


void KCard::raise()
{
    if ( zValue() < raisedZValue )
        setZValue( raisedZValue + zValue() );
}


void KCard::setHighlighted( bool flag )
{
    if ( flag != d->highlighted )
    {
        d->highlighted = flag;

        d->fadeAnimation->setDirection( flag
                                        ? QAbstractAnimation::Forward
                                        : QAbstractAnimation::Backward );

        if ( d->fadeAnimation->state() != QAbstractAnimation::Running )
            d->fadeAnimation->start();
    }
}


bool KCard::isHighlighted() const
{
    return d->highlighted;
}


void KCard::completeAnimation()
{
    if ( !d->animation )
        return;

    d->animation->disconnect( this );
    if ( d->animation->state() != QAbstractAnimation::Stopped )
        d->animation->setCurrentTime( d->animation->duration() );

    stopAnimation();
}


void KCard::stopAnimation()
{
    if ( !d->animation )
        return;

    delete d->animation;
    d->animation = 0;

    setZValue( d->destZ );

    emit animationStopped( this );
}


void KCard::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    Q_UNUSED( option );
    Q_UNUSED( widget );

    if ( pixmap().size() != d->deck->cardSize() )
    {
        QPixmap newPix = d->deck->cardPixmap( d->id, d->faceUp );
        if ( d->faceUp )
            setFrontPixmap( newPix );
        else
            setBackPixmap( newPix );

        // Changing the pixmap will call update() and force a repaint, so we
        // might as well return early.
        return;
    }

    // Enable smooth pixmap transformation only if the card is rotated. We
    // don't really need it otherwise and it slows down our flip animations.
    painter->setRenderHint( QPainter::SmoothPixmapTransform, int(rotation()) % 90 );

    QPixmap pix = pixmap();

    if ( d->highlightValue > 0 )
    {
        QPainter p( &pix );
        p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
        p.fillRect( 0, 0, pix.width(), pix.height(), QColor::fromRgbF( 0, 0, 0, 0.5 * d->highlightValue ) );
    }

    painter->drawPixmap( 0, 0, pix );
}


void KCard::setFrontPixmap( const QPixmap & pix )
{
    d->frontPixmap = pix;
    if ( d->flipValue >= 0.5 )
        setPixmap( d->frontPixmap );
}


void KCard::setBackPixmap( const QPixmap & pix )
{
    d->backPixmap = pix;
    if ( d->flipValue < 0.5 )
        setPixmap( d->backPixmap );
}


void KCard::setPixmap( const QPixmap & pix )
{
    QGraphicsPixmapItem::setPixmap( pix );
}




