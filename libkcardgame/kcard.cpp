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

#include "kcard.h"

#include "kabstractcarddeck.h"
#include "kcardpile.h"

#include <KDebug>

#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QGraphicsScene>
#include <QtGui/QPainter>


KCard::KCard( quint32 data, KAbstractCardDeck * deck )
  : QObject(),
    QGraphicsPixmapItem(),
    m_faceup( true ),
    m_highlighted( false ),
    m_data( data ),
    m_destFace( m_faceup ),
    m_destX( 0 ),
    m_destY( 0 ),
    m_destZ( 0 ),
    m_flippedness( m_faceup ? 1 : 0 ),
    m_highlightedness( m_highlighted ? 1 : 0 ),
    m_deck( deck ),
    m_source( 0 ),
    m_animation( 0 )
{
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    setTransformationMode( Qt::SmoothTransformation );

    m_fadeAnimation = new QPropertyAnimation( this, "highlightedness", this );
    m_fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    m_fadeAnimation->setKeyValueAt( 0, 0 );
    m_fadeAnimation->setKeyValueAt( 1, 1 );
}


KCard::~KCard()
{
    // If the card is in a pile, remove it from there.
    if ( source() )
        source()->remove( this );
    if ( scene() )
        scene()->removeItem( this );
}


bool KCard::isFaceUp() const
{
    return m_faceup;
}


quint32 KCard::data() const
{
    return m_data;
}


void KCard::raise()
{
    if ( zValue() < 1000 )
        setZValue( 1000 + zValue() );
}


void KCard::turn( bool faceUp )
{
    if ( m_faceup != faceUp || m_destFace != faceUp )
    {
        m_faceup = faceUp;
        m_destFace = faceUp;
        updatePixmap();
    }
    m_flippedness = m_faceup ? 1.0 : 0.0;
}


void KCard::flip()
{
    turn( !m_faceup );
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
void KCard::moveTo( QPointF pos2, qreal z2, int duration )
{
    animate( pos2, z2, 1, 0, isFaceUp(), true, duration );
}


bool KCard::animated() const
{
    return m_animation != 0;
}


// Return the  cards real position.  This is the destination
// of the animation if animated, and the current position otherwise.
QPointF KCard::realPos() const
{
    if (animated())
        return QPointF(m_destX, m_destY);
    else
        return pos();
}


// Return the > of the cards real position.  This is the destination
// of the animation if animated, and the current Z otherwise.
qreal KCard::realZ() const
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
bool KCard::realFace() const
{
    return m_destFace;
}


void KCard::setHighlighted( bool flag )
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


void KCard::setHighlightedness( qreal highlightedness )
{
    m_highlightedness = highlightedness;
    updatePixmap();
    return;
}


bool KCard::isHighlighted() const
{
    return m_highlighted;
}


qreal KCard::highlightedness() const
{
    return m_highlightedness;
}


void KCard::completeAnimation()
{
    if ( !m_animation )
        return;

    kDebug() << this->data();

    m_animation->disconnect( this );
    if ( m_animation->state() != QAbstractAnimation::Stopped )
        m_animation->setCurrentTime( m_animation->duration() );

    stopAnimation();
}


void KCard::stopAnimation()
{
    if ( !m_animation )
        return;

    delete m_animation;
    m_animation = 0;

    setZValue( m_destZ );

    emit animationStopped( this );
}











void KCard::updatePixmap()
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


void KCard::setFlippedness( qreal flippedness )
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


qreal KCard::flippedness() const
{
    return m_flippedness;
}


#include "kcard.moc"
