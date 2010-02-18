/*
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include <KDebug>

#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>


KCardAnimation::KCardAnimation( KCardPrivate * d,
                                int duration,
                                QPointF pos,
                                qreal rotation,
                                qreal scale,
                                bool faceUp )
  : QAbstractAnimation( d ),
    d( d ),
    m_duration( duration ),
    m_x0( d->q->x() ),
    m_y0( d->q->y() ),
    m_rotation0( d->q->rotation() ),
    m_scale0( d->q->scale() ),
    m_flippedness0( d->flippedness() ),
    m_x1( pos.x() ),
    m_y1( pos.y() ),
    m_rotation1( rotation ),
    m_scale1( scale ),
    m_flippedness1( faceUp ? 1.0 : 0.0 )
{
}


int KCardAnimation::duration() const
{
    return m_duration;
}


void KCardAnimation::updateCurrentTime( int msec )
{
    qreal progress = qreal(msec) / m_duration;

    d->q->setPos( m_x0 + (m_x1 - m_x0) * progress, m_y0 + (m_y1 - m_y0) * progress );
    d->q->setRotation( m_rotation0 + (m_rotation1 - m_rotation0) * progress );
    d->q->setScale( m_scale0 + (m_scale1 - m_scale0) * progress );
    d->setFlippedness( m_flippedness0 + (m_flippedness1 - m_flippedness0) * progress );
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
    return;
}


qreal KCardPrivate::highlightedness() const
{
    return highlightValue;
}


KCard::KCard( quint32 id, KAbstractCardDeck * deck )
  : QObject(),
    QGraphicsItem(),
    d( new KCardPrivate( this ) )
{
    d->id = id;
    d->deck = deck;

    d->faceUp = true;
    d->flipValue = d->faceUp ? 1 : 0;
    d->highlighted = false;
    d->highlightValue = d->highlighted ? 1 : 0;

    d->source = 0;

    d->animation = 0;

    d->fadeAnimation = new QPropertyAnimation( d, "highlightedness", d );
    d->fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    d->fadeAnimation->setKeyValueAt( 0, 0 );
    d->fadeAnimation->setKeyValueAt( 1, 1 );
}


KCard::~KCard()
{
    // If the card is in a pile, remove it from there.
    if ( source() )
        source()->remove( this );
}


int KCard::type() const
{
    return KCard::Type;
}


QRectF KCard::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), d->deck->cardSize() );
}


quint32 KCard::id() const
{
    return d->id;
}


void KCard::setSource( KCardPile * pile )
{
    d->source = pile;
}


KCardPile * KCard::source() const
{
    return d->source;
}


void KCard::setFaceUp( bool faceUp )
{
    qreal flippedness = faceUp ? 1.0 : 0.0;
    if ( d->faceUp != faceUp || d->flipValue != flippedness )
    {
        d->faceUp = faceUp;
        d->flipValue = flippedness;
        update();
    }
}


bool KCard::isFaceUp() const
{
    return d->faceUp;
}


void KCard::animate( QPointF pos, qreal z, qreal scale, qreal rotation, bool faceUp, bool raised, int duration )
{
    stopAnimation();

    if ( duration > 0
         && ( qAbs( pos.x() - x() ) > 2
              || qAbs( pos.y() - y() ) > 2
              || qAbs( scale - this->scale() ) > 0.05
              || qAbs( rotation - this->rotation() ) > 2
              || faceUp != d->faceUp ) )
    {
        if ( raised )
            raise();

        d->destZ = z;
        d->faceUp = faceUp;

        d->animation = new KCardAnimation( d, duration, pos, rotation, scale, faceUp );
        connect( d->animation, SIGNAL(finished()), SLOT(stopAnimation()) );
        d->animation->start();
        emit animationStarted( this );
    }
    else
    {
        setPos( pos );
        setZValue( z );
        setScale( scale );
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
    if ( zValue() < 1000 )
        setZValue( 1000 + zValue() );
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

    update();
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

    d->deck->paintCard( painter, d->id, d->flipValue > 0.5, d->highlightValue );
}


#include "kcard_p.moc"
#include "kcard.moc"
