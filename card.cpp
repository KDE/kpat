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

#include "card.h"

#include "cardmaps.h"
#include "dealer.h"
#include "pile.h"

#include <KDebug>

#include <QtCore/QTimeLine>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QStyleOptionGraphicsItem>

#include <cmath>


AbstractCard::AbstractCard( Rank r, Suit s )
    : m_suit( s ), m_rank( r ), m_faceup( false )
{
    // Default for the card is face up, standard size.
    m_faceup = true;
}

Card::Card( Rank r, Suit s, QGraphicsScene *_parent )
    : QObject(), AbstractCard( r, s ), QGraphicsPixmapItem(),
      m_source(0), tookDown(false), animation( 0 ),
      m_highlighted( false ), m_moving( false ), m_isZoomed( false ), m_isSeen( Unknown )
{
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    _parent->addItem( this );

    // Set the name of the card
    m_hoverTimer = new QTimer(this);
    m_hovered = false;
    m_destFace = isFaceUp();

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;

    m_spread = QSizeF( 0, 0 );

    setAcceptsHoverEvents( true );

    //m_hoverTimer->setSingleShot(true);
    //m_hoverTimer->setInterval(50);
    connect(m_hoverTimer, SIGNAL(timeout()),
            this, SLOT(zoomInAnimation()));

#if 0
    m_shadow = new QGraphicsPixmapItem();
    scene()->addItem( m_shadow );
#endif
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


void Card::setPixmap()
{
    if( m_faceup )
        QGraphicsPixmapItem::setPixmap( cardMap::self()->renderFrontside( m_rank, m_suit ) );
    else
        QGraphicsPixmapItem::setPixmap( cardMap::self()->renderBackside() );
    m_boundingRect = QRectF(QPointF(0,0), pixmap().size());
    m_isSeen = Unknown;
    return;

    QImage img = pixmap().toImage().mirrored( false, true );
    QImage acha( img.size(), QImage::Format_RGB32 );
    acha.fill( Qt::white );
    QPainter p( &acha );
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, 200));
    linearGrad.setColorAt(0.6, Qt::black);
    linearGrad.setColorAt( 1, Qt::black);
    linearGrad.setColorAt(0, Qt::white);
    p.fillRect( QRect( QPoint( 0,0 ), img.size() ), linearGrad );
    p.end();
    img.setAlphaChannel( acha );
    m_shadow->setPixmap( QPixmap::fromImage( img ) );
}

// Turn the card if necessary.  If the face gets turned up, the card
// is activated at the same time.
//
void Card::turn( bool _faceup )
{
    if (m_faceup != _faceup ) {
        m_faceup = _faceup;
        m_destFace = _faceup;
        setPixmap();
    }
}

void Card::flip()
{
    turn( !m_faceup );
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
    return m_destFace;
}

// The current maximum Z value.  This is used so that new cards always
// get placed on top of the old ones and don't get placed in the
// middle of a destination pile.
qreal  Card::Hz = 0;

void Card::setZValue(double z)
{
    QGraphicsPixmapItem::setZValue(z);
    if (z > Hz)
        Hz = z;
}


// Start a move of the card using animation.
//
// 'steps' is the number of steps the animation should take.
//
void Card::moveTo(qreal x2, qreal y2, qreal z2, int duration)
{
    //kDebug(11111) << "Card::moveTo" << x2 << " " << y2 << " " << duration << " " << kBacktrace();
    if ( fabs( x2 - x() ) < 2 && fabs( y2 - y() ) < 1 )
    {
        setPos( QPointF( x2, y2 ) );
        setZValue( z2 );
        m_isSeen = Unknown;
        return;
    }
    stopAnimation();
    m_isSeen = CardVisible; // avoid surprises

    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation(this);
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setPosAt(0, pos() );
    animation->setPosAt(1, QPointF( x2, y2 ));

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setCurveShape(QTimeLine::LinearCurve);
    timeLine->setLoopCount(1);
    timeLine->setDuration( duration );
    timeLine->start();

    connect( timeLine, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_destX = x2;
    m_destY = y2;
    m_destZ = z2;

    if (fabs( x2 - x() ) < 1 && fabs( y2 - y() ) < 1) {
        setZValue(z2);
        return;
    }
    // if ( fabs( z2 - zValue() ) >= 1 )
        setZValue(Hz++);
}

// Animate a move to (x2, y2), and at the same time flip the card.
//
void Card::flipTo(qreal x2, qreal y2, int duration)
{
    stopAnimation();

    qreal  x1 = x();
    qreal  y1 = y();

    QTimeLine *timeLine = new QTimeLine( 1000, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setScaleAt( 0, 1, 1 );
    animation->setScaleAt( 0.5, 0.0, 1 );
    animation->setScaleAt( 1, 1, 1 );
    QPointF hp = pos();
    hp.setX( ( x1 + x2 + boundingRect().width() ) / 2 );
    //kDebug(11111) << "flip" << name() << " " << x1 << " " << x2 << " " << y1 << " " << y2;
    if ( fabs( y1 - y2) > 2 )
        hp.setY( ( y1 + y2 + boundingRect().height() ) / 20 );
    //kDebug(11111) << "hp" << pos() << " " << hp << " " << QPointF( x2, y2 );
    animation->setPosAt(0.5, hp );
    animation->setPosAt(1, QPointF( x2, y2 ));

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->setDuration( duration );
    timeLine->start();

    connect( timeLine, SIGNAL(finished()), SLOT(stopAnimation()) );
    connect( timeLine, SIGNAL(valueChanged(qreal)), SLOT(flipAnimationChanged(qreal)) );

    // Set the target of the animation
    m_destX = x2;
    m_destY = y2;
    m_destZ = zValue();

    // Let the card be above all others during the animation.
    setZValue(Hz++);

    m_destFace = !m_faceup;
}


void Card::flipAnimationChanged( qreal r)
{
    if ( r > 0.5 && !isFaceUp() ) {
        flip();
        Q_ASSERT( m_destFace == m_faceup );
    }
}

void Card::setTakenDown(bool td)
{
/*    if (td)
      kDebug(11111) << "took down" << name();*/
    tookDown = td;
}

void Card::testVisibility()
{
    // check if we can prove we're not visible
    QList<QGraphicsItem *> list = scene()->items( mapToScene( m_boundingRect ), Qt::ContainsItemBoundingRect );
    for ( QList<QGraphicsItem*>::Iterator it = list.begin(); it != list.end(); ++it )
    {
        Card *c = dynamic_cast<Card*>( *it );
        if ( !c )
            continue;
        if ( c == this )
        {
            m_isSeen = CardVisible;
            return;
        }

        //kDebug(11111) << c->name() << "covers" << name() << " " << c->mapToScene( c->boundingRect() ).boundingRect() << " " << mapToScene( boundingRect() ).boundingRect() << " " << zValue() << " " << c->zValue();
        c->m_hiddenCards.append( this );
        m_isSeen = CardHidden;
        return;
    }
    m_isSeen = CardVisible;
}

bool Card::takenDown() const
{
    return tookDown;
}

void Card::setHighlighted( bool flag ) {
    m_highlighted = flag;
    update();
}

void Card::stopAnimation()
{
    if ( !animation )
        return;

    //kDebug(11111) << gettime() << "stopAnimation" << name();
    QGraphicsItemAnimation *old_animation = animation;
    animation = 0;
    if ( old_animation->timeLine()->state() == QTimeLine::Running )
        old_animation->timeLine()->setCurrentTime(old_animation->timeLine()->duration() + 1);
    old_animation->timeLine()->stop();
    old_animation->deleteLater();
    setPos( QPointF( m_destX, m_destY ) );
    setZValue( m_destZ );
    if ( source() )
        setSpread( source()->cardOffset(this) );

    m_isSeen = Unknown;
    emit stopped( this );
}

bool  Card::animated() const
{
    return animation != 0;
}

void Card::hoverEnterEvent ( QGraphicsSceneHoverEvent *  )
{
    if ( animated() || !isFaceUp() )
        return;

    m_hovered = true;
    source()->tryRelayoutCards();
    return;

    m_hoverTimer->start(200);
    //zoomIn(400);
    //kDebug(11111) << "hoverEnterEvent\n";
}

void Card::hoverLeaveEvent ( QGraphicsSceneHoverEvent * )
{
    if ( !isFaceUp() || !m_hovered)
        return;

    m_hovered = false;

    source()->tryRelayoutCards();
    m_hoverTimer->stop();

    //zoomOut(200);
}

void Card::mousePressEvent ( QGraphicsSceneMouseEvent *ev ) {
    //kDebug(11111) << "mousePressEvent\n";
    if ( !isFaceUp() )
        return;
    if ( this == source()->top() )
        return; // no way this is meaningful

    if ( ev->button() == Qt::RightButton && !animated() && !source()->dscene()->isMoving( this ) )
    {
        m_hoverTimer->stop();
        stopAnimation();
        zoomIn(400);
        m_hovered = false;
        m_moving = true;
        m_isSeen = CardVisible;
    }
}

void Card::mouseReleaseEvent ( QGraphicsSceneMouseEvent * ev ) {
    //kDebug(11111) << "mouseReleaseEvent\n";
    m_moving = false;

    if ( !isFaceUp() )
        return;
    if ( this == source()->top() )
        return; // no way this is meaningful

    if ( m_isZoomed && ev->button() == Qt::RightButton )
    {
        m_hoverTimer->stop();
        stopAnimation();
        zoomOut(400);
        m_hovered = false;
        m_moving = true;
        m_isSeen = CardVisible;
    }
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

    connect( timeLine, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_destZ = zValue();
    m_destX = x();
    m_destY = y();
    setZValue(Hz+1);
}

void Card::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *)
{
    if ( m_isSeen == Unknown )
        testVisibility();
    if ( m_isSeen == CardHidden )
        return;

    painter->save();
//    painter->setRenderHint(QPainter::Antialiasing);
    //painter->setRenderHint(QPainter::SmoothPixmapTransform);
    if (scene()->mouseGrabberItem() == this) {
        //painter->setOpacity(.8);
    }
    QRectF exposed = option->exposedRect;
//    exposed = exposed.adjusted(-1, -1, 2, 2);
    Q_ASSERT( !pixmap().isNull() );

    //painter->drawPixmap(exposed, pixmap(), exposed );
    //kDebug(11111) << "exposed" << exposed;

    QRect body = cardMap::self()->opaqueRect();
    QMatrix m = painter->combinedMatrix();
    bool isi = qFuzzyCompare( m.m11(), 1 ) &&
               qFuzzyCompare( m.m22(), 1 ) &&
               qFuzzyCompare( m.m12(), 0 ) &&
               qFuzzyCompare( m.m21(), 0 );
    painter->setRenderHint( QPainter::SmoothPixmapTransform, !isi );
    painter->setRenderHint( QPainter::Antialiasing, !isi );

    if ( body.isValid() && isi )
    {
        painter->setCompositionMode(QPainter::CompositionMode_Source);

        //body = exposed.intersected( body );//.adjusted(-1, -1, 2, 2);
        painter->drawPixmap( body.topLeft(), pixmap(), body );
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver );
        QRectF hori( QPointF( 0, 0 ), QSizeF( pixmap().width(), body.top() ) );
        if ( exposed.intersects( hori ) )
            painter->drawPixmap( hori.topLeft(), pixmap(), hori );
        hori.moveTop( body.bottom() );
        hori.setHeight( pixmap().height() - body.bottom() );
        if ( exposed.intersects( hori ) )
            painter->drawPixmap( hori.topLeft(), pixmap(), hori );
        QRectF vert( QPointF( 0, body.top() ),
                     QSizeF( body.left(),  body.height() ) );
        if ( vert.width() != 0 && exposed.intersects( vert ) )
            painter->drawPixmap( vert.topLeft(), pixmap(), vert );
        vert.moveLeft( body.right() );
        vert.setWidth( pixmap().width() - body.right() );
        if ( vert.width() != 0 && exposed.intersects( vert ) )
            painter->drawPixmap( vert.topLeft(), pixmap(), vert );
    } else
         painter->drawPixmap( QPointF( 0, 0 ), pixmap() );

    if ( isHighlighted() ) {
        painter->setRenderHint( QPainter::SmoothPixmapTransform, true );
        painter->setRenderHint( QPainter::Antialiasing, true );

        painter->setCompositionMode(QPainter::CompositionMode_SourceOver );
        QPixmap pix(pixmap().size());
        pix.fill(Qt::black);

        QPixmap shadow = pixmap();
        QPainter px(&shadow);
        px.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        px.drawPixmap(0, 0, pix);
        px.end();
        painter->setOpacity(.5);

        painter->drawPixmap(exposed, shadow, exposed );
    }
    painter->restore();
}

void Card::zoomIn(int t)
{
    m_hovered = true;

    QTimeLine *timeLine = new QTimeLine( t, this );
    m_originalPosition = pos();

    animation = new QGraphicsItemAnimation( this );
    animation->setItem( this );
    animation->setTimeLine( timeLine );
    QPointF dest =  QPointF( pos().x() + boundingRect().width() / 3,
                             pos().y() - boundingRect().height() / 4 );
    animation->setPosAt( 1, dest );
    animation->setScaleAt( 1, 1.1, 1.1 );
    animation->setRotationAt( 1, 20 );
    //qreal x2 = pos().x() + boundingRect().width() / 2 - boundingRect().width() * 1.1 / 2;
    //qreal y2 = pos().y() + boundingRect().height() / 2 - boundingRect().height() * 1.1 / 2;
    //animation->setScaleAt( 1, 1, 1 );

    timeLine->setUpdateInterval( 1000 / 25 );
    timeLine->setFrameRange( 0, 100 );
    timeLine->setLoopCount( 1 );
    timeLine->start();

    m_isZoomed = true;

    connect( timeLine, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_destZ = zValue();
    m_destX = dest.x();
    m_destY = dest.y();
}

void Card::zoomOut(int t)
{
    QTimeLine *timeLine = new QTimeLine( t, this );

    animation = new QGraphicsItemAnimation( this );
    animation->setItem(this);
    animation->setTimeLine(timeLine);
    animation->setRotationAt( 0, 20 );
    animation->setRotationAt( 0.5, 10 );
    animation->setRotationAt( 1, 0 );
    animation->setScaleAt( 0, 1.1, 1.1 );
    animation->setScaleAt( 1, 1.0, 1.0 );
    animation->setPosAt( 1, m_originalPosition );
    //qreal x2 = pos().x() + boundingRect().width() / 2 - boundingRect().width() * 1.1 / 2;
    //qreal y2 = pos().y() + boundingRect().height() / 2 - boundingRect().height() * 1.1 / 2;
    //animation->setScaleAt( 1, 1, 1 );

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->start();

    m_isZoomed = false;

    connect( timeLine, SIGNAL(finished()), SLOT(stopAnimation()) );

    m_destZ = zValue();
    m_destX = m_originalPosition.x();
    m_destY = m_originalPosition.y();
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
    if (m_spread != spread)
        source()->tryRelayoutCards();
    m_spread = spread;
}

void Card::setPos( const QPointF &pos )
{
    QGraphicsPixmapItem::setPos( pos );
    m_isSeen = Unknown;
    for ( QList<Card*>::Iterator it = m_hiddenCards.begin();
          it != m_hiddenCards.end(); ++it )
    {
        ( *it )->m_isSeen = Unknown;
    }
    m_hiddenCards.clear();
#if 0
    m_shadow->setZValue( -1 );
    m_shadow->setPos( pos + QPointF( 0, pixmap().height() ) );
    if ( source() )
        m_shadow->setVisible( ( source()->top() == this ) && realFace() );
#endif
}

bool Card::collidesWithItem ( const QGraphicsItem * other,
                              Qt::ItemSelectionMode mode ) const
{
    if ( this == other )
        return true;
    const Card *othercard = dynamic_cast<const Card*>( other );
    if ( !othercard )
        return QGraphicsPixmapItem::collidesWithItem( other, mode );
    bool col = sceneBoundingRect().intersects( othercard->sceneBoundingRect() );
    return col;
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
