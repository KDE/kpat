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
const int Card::my_type = DealerScene::CardTypeId;

Card::Card( Rank r, Suit s, QGraphicsScene *_parent )
    : QObject(), QGraphicsPixmapItem(),
      m_suit( s ), m_rank( r ),
      m_source(0), tookDown(false), animation( 0 ),
      m_highlighted( false ), m_moving( false )
{
    _parent->addItem( this );

    // Set the name of the card
    m_name = QString("%1 %2").arg(suit_names[s-1]).arg(rank_names[r-1]).toUtf8();
    m_hoverTimer = new QTimer(this);
    m_hovered = false;
    // Default for the card is face up, standard size.
    m_faceup = false;
    m_destFace = true;

    m_destX = 0;
    m_destY = 0;
    m_destZ = 0;

    cardMap::self()->registerCard( QString( "%1_%2" ).arg( rank_names[m_rank-1] ).arg( suit_names[m_suit-1] ) );

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
    if ( name() == "club 1" && false )
        kDebug() << this << " Card::setPixmap " << m_elementId << " " << cardMap::self()->wantedCardWidth() << " " << kBacktrace() << endl;

    QGraphicsPixmapItem::setPixmap( cardMap::self()->renderCard( m_elementId ) );
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
    if (m_faceup != _faceup || m_elementId.isNull() ) {
        m_faceup = _faceup;
        m_destFace = _faceup;
        if ( m_faceup ) {
            setElementId( QString( "%1_%2" ).arg( rank_names[m_rank-1] ).arg( suit_names[m_suit-1] ) );
        } else {
            setElementId( QLatin1String( "back" ) );
        }
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
    if ( fabs( x2 - x() ) < 2 && fabs( y2 - y() ) < 1 )
    {
        setPos( QPointF( x2, y2 ) );
        setZValue( z2 );
        return;
    }
    if ( name() == "diamond 01" )
        kDebug() << "moveTo " << name() << " " << x2 << " " << y2 << " " << z2 << " " << pos() << " " << zValue() << " " << duration << " " << kBacktrace() << endl;
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
    //kDebug() << "flip " << name() << " " << x1 << " " << x2 << " " << y1 << " " << y2 << endl;
    if ( fabs( y1 - y2) > 2 )
        hp.setY( ( y1 + y2 + boundingRect().height() ) / 20 );
    //kDebug() << "hp " << pos() << " " << hp << " " << QPointF( x2, y2 ) << endl;
    animation->setPosAt(0.5, hp );
    animation->setPosAt(1, QPointF( x2, y2 ));

    timeLine->setUpdateInterval(1000 / 25);
    timeLine->setFrameRange(0, 100);
    timeLine->setLoopCount(1);
    timeLine->setDuration( duration );
    timeLine->start();

    connect( timeLine, SIGNAL( finished() ), SLOT( stopAnimation() ) );
    connect( timeLine, SIGNAL( valueChanged( qreal ) ), SLOT( flipAnimationChanged( qreal ) ) );

    // Set the target of the animation
    m_destX = x2;
    m_destY = y2;
    m_destZ = int(zValue());

    // Let the card be above all others during the animation.
    setZValue(Hz++);

    m_destFace = !m_faceup;
}


void Card::flipAnimationChanged( qreal r)
{
    if ( r > 0.5 && !isFaceUp() ) {
        flip();
        assert( m_destFace == m_faceup );
    }
}

void Card::setTakenDown(bool td)
{
/*    if (td)
      kDebug(11111) << "took down " << name() << endl;*/
    tookDown = td;
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

    if ( !sender() || !sender()->isA( "QTimeLine" ) )
        if ( name() == "diamond 01" )
            kDebug() << "stopAnimation " << name() << " " << m_destX << " " << m_destY << " " << animation->timeLine()->duration() << " " << kBacktrace() << endl;
    QGraphicsItemAnimation *old_animation = animation;
    animation = 0;
    if ( old_animation->timeLine()->state() == QTimeLine::Running )
        old_animation->timeLine()->setCurrentTime(old_animation->timeLine()->duration() + 1);
    old_animation->timeLine()->stop();
    setPos( QPointF( m_destX, m_destY ) );
    setZValue( m_destZ );
    if ( source() )
        setSpread( source()->cardOffset(this) );

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

    m_hovered = true;
    source()->tryRelayoutCards();
    return;

    m_hoverTimer->start(200);
    //zoomIn(400);
    //kDebug() << "hoverEnterEvent\n";
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

void Card::mousePressEvent ( QGraphicsSceneMouseEvent * ) {
    kDebug() << "mousePressEvent\n";
    if ( !isFaceUp() )
        return;
    if ( this == source()->top() )
        return; // no way this is meaningful
    m_hoverTimer->stop();
    stopAnimation();
    zoomOut(100);
    m_hovered = false;
    m_moving = true;
}

void Card::mouseReleaseEvent ( QGraphicsSceneMouseEvent * ) {
    kDebug() << "mouseReleaseEvent\n";
    m_moving = false;
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
                 QWidget *)
{
//    painter->setRenderHint(QPainter::Antialiasing);
    //painter->setRenderHint(QPainter::SmoothPixmapTransform);
    if (scene()->mouseGrabberItem() == this) {
        //painter->setOpacity(.8);
    }
    painter->setRenderHint( QPainter::SmoothPixmapTransform, false );
    painter->setRenderHint( QPainter::Antialiasing, false );
    QRectF exposed = option->exposedRect;
//    exposed = exposed.adjusted(-1, -1, 2, 2);
    Q_ASSERT( !pixmap().isNull() );
    painter->drawPixmap(exposed, pixmap(), exposed );

    if ( isHighlighted() ) {
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
    animation->setScaleAt( 1, 1.1, 1.1 );
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
    animation->setScaleAt( 0, 1.1, 1.1 );
    animation->setScaleAt( 1, 1.0, 1.0 );
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

void Card::setElementId( const QString & element )
{
    if (element == m_elementId)
        return;
    m_elementId = element;
    setPixmap();
}

QRectF Card::boundingRect() const
{
   return QRectF(QPointF(0,0), pixmap().size());
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
#if 0
    m_shadow->setZValue( -1 );
    m_shadow->setPos( pos + QPointF( 0, pixmap().height() ) );
    if ( source() )
        m_shadow->setVisible( ( source()->top() == this ) && realFace() );
#endif
}

#include "card.moc"
