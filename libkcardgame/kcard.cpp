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

#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QGraphicsScene>
#include <QtGui/QPainter>


KCardPrivate::KCardPrivate( KCard * card )
  : QObject( card ),
    q( card )
{
}


void KCardPrivate::updatePixmap()
{
    QPixmap pix;
    if( flippedness > 0.5 )
        pix = deck->frontsidePixmap( data );
    else
        pix = deck->backsidePixmap( data );

    qreal highlightOpacity = fadeAnimation->state() == QAbstractAnimation::Running
                             ? highlightValue
                             : ( highlighted ? 1 : 0 );

    if ( highlightOpacity > 0 )
    {
        QPainter p( &pix );
        p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
        p.setOpacity( 0.5 * highlightOpacity );
        p.fillRect( 0, 0, pix.width(), pix.height(), Qt::black );
    }

    q->setPixmap( pix );
}


void KCardPrivate::setHighlightedness( qreal highlightedness )
{
    highlightValue = highlightedness;
    updatePixmap();
    return;
}


qreal KCardPrivate::highlightedness() const
{
    return highlightValue;
}


KCard::KCard( quint32 data, KAbstractCardDeck * deck )
  : QObject(),
    QGraphicsPixmapItem(),
    d( new KCardPrivate( this ) )
{
    d->data = data;
    d->deck = deck;

    d->faceUp = true;
    d->flippedness = d->faceUp ? 1 : 0;
    d->highlighted = false;
    d->highlightValue = d->highlighted ? 1 : 0;

    d->source = 0;

    d->animation = 0;

    d->fadeAnimation = new QPropertyAnimation( d, "highlightedness", d );
    d->fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    d->fadeAnimation->setKeyValueAt( 0, 0 );
    d->fadeAnimation->setKeyValueAt( 1, 1 );

    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    setTransformationMode( Qt::SmoothTransformation );
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


quint32 KCard::data() const
{
    return d->data;
}


void KCard::setSource( KCardPile * pile )
{
    d->source = pile;
}


KCardPile * KCard::source() const
{
    return d->source;
}


void KCard::turn( bool faceUp )
{
    qreal flippedness = faceUp ? 1.0 : 0.0;
    if ( d->faceUp != faceUp || d->flippedness != flippedness )
    {
        d->faceUp = faceUp;
        d->flippedness = flippedness;
        d->updatePixmap();
    }
}


bool KCard::isFaceUp() const
{
    return d->faceUp;
}


void KCard::animate( QPointF pos2, qreal z2, qreal scale2, qreal rotation2, bool faceup2, bool raised, int duration )
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

    if ( faceup2 != d->faceUp )
    {
        QPropertyAnimation * flip = new QPropertyAnimation( this, "flippedness" );
        flip->setKeyValueAt( 0, d->faceUp ? 1.0 : 0.0 );
        flip->setKeyValueAt( 1, faceup2 ? 1.0 : 0.0 );
        flip->setDuration( duration );
        aniGroup->addAnimation( flip );
        d->faceUp = faceup2;
    }

    if ( raised )
        raise();

    if ( aniGroup->animationCount() == 0 )
    {
        setZValue( z2 );

        delete aniGroup;
    }
    else
    {
        d->destZ = z2;

        d->animation = aniGroup;
        connect( d->animation, SIGNAL(finished()), SLOT(stopAnimation()) );
        d->animation->start();
        emit animationStarted( this );
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


void KCard::setFlippedness( qreal flippedness )
{
    if ( flippedness == d->flippedness )
        return;

    bool changePixmap = ( flippedness >= 0.5 && d->flippedness < 0.5 )
                        || ( flippedness <= 0.5 && d->flippedness > 0.5 );

    d->flippedness = flippedness;

    if ( changePixmap )
        d->updatePixmap();

    qreal xOffset = pixmap().width() * ( 0.5 - qAbs( flippedness - 0.5 ) );
    qreal xScale = qAbs( 2 * flippedness - 1 );

    setTransform( QTransform().translate( xOffset, 0 ).scale( xScale, 1 ) );
}


qreal KCard::flippedness() const
{
    return d->flippedness;
}


#include "kcard_p.moc"
#include "kcard.moc"
