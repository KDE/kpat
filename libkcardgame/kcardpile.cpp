/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "kcardpile.h"

#include "kabstractcarddeck.h"
#include "kcardscene.h"

#include <QDebug>

#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QPainter>

#include <cmath>

class KCardPilePrivate : public QObject
{
    Q_OBJECT

    Q_PROPERTY( qreal highlightedness READ highlightedness WRITE setHighlightedness )

public:
    KCardPilePrivate( KCardPile * q );

    void setHighlightedness( qreal highlightedness );
    qreal highlightedness() const;

    KCardPile * q;

    QList<KCard*> cards;

    bool autoTurnTop;
    bool highlighted;

    QSize graphicSize;
    QPointF layoutPos;
    QPointF spread;

    qreal topPadding;
    qreal rightPadding;
    qreal bottomPadding;
    qreal leftPadding;

    KCardPile::WidthPolicy widthPolicy;
    KCardPile::HeightPolicy heightPolicy;

    KCardPile::KeyboardFocusHint selectHint;
    KCardPile::KeyboardFocusHint dropHint;

    qreal highlightValue;

    QPropertyAnimation * fadeAnimation;
};


KCardPilePrivate::KCardPilePrivate( KCardPile * q )
  : QObject( q ),
    q( q )
{
}


void KCardPilePrivate::setHighlightedness( qreal highlightedness )
{
    highlightValue = highlightedness;
    q->update();
}


qreal KCardPilePrivate::highlightedness() const
{
    return highlightValue;
}


KCardPile::KCardPile( KCardScene * cardScene )
  : QGraphicsObject(),
    d( new KCardPilePrivate( this ) )
{
    d->autoTurnTop = false;
    d->highlighted = false;
    d->highlightValue = 0;
    d->spread = QPointF( 0, 0 );

    d->topPadding = 0;
    d->rightPadding = 0;
    d->bottomPadding = 0;
    d->leftPadding = 0;

    d->widthPolicy = FixedWidth;
    d->heightPolicy = FixedHeight;

    d->fadeAnimation = new QPropertyAnimation( d, "highlightedness", d );
    d->fadeAnimation->setDuration( 150 );
    d->fadeAnimation->setKeyValueAt( 0, 0 );
    d->fadeAnimation->setKeyValueAt( 1, 1 );

    setZValue( 0 );
    QGraphicsItem::setVisible( true );

    if ( cardScene )
        cardScene->addPile( this );
}


KCardPile::~KCardPile()
{
    foreach ( KCard * c, d->cards )
        c->setPile( 0 );

    KCardScene * cardScene = dynamic_cast<KCardScene*>( scene() );
    if ( cardScene )
        cardScene->removePile( this );
}


int KCardPile::type() const
{
    return KCardPile::Type;
}


QRectF KCardPile::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), d->graphicSize );
}


void KCardPile::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    Q_UNUSED( option );
    Q_UNUSED( widget );

    paintGraphic( painter, d->highlightValue );
}


QList<KCard*> KCardPile::cards() const
{
    return d->cards;
}


int KCardPile::count() const
{
    return d->cards.count();
}


bool KCardPile::isEmpty() const
{
    return d->cards.isEmpty();
}


int KCardPile::indexOf( const KCard * card ) const
{
    return d->cards.indexOf( const_cast<KCard*>( card ) );
}


KCard * KCardPile::at( int index ) const
{
    if ( index < 0 || index >= d->cards.size() )
        return 0;
    return d->cards.at( index );
}


KCard * KCardPile::topCard() const
{
    if ( d->cards.isEmpty() )
        return 0;

    return d->cards.last();
}


QList<KCard*> KCardPile::topCards( int depth ) const
{
    if ( depth <= 0 )
        return QList<KCard*>();

    if ( depth > count() )
        return d->cards;

    return d->cards.mid( count() - depth );
}


QList<KCard*> KCardPile::topCardsDownTo( const KCard * card ) const
{
    int index = d->cards.indexOf( const_cast<KCard*>( card ) );
    if ( index == -1 )
        return QList<KCard*>();
    return d->cards.mid( index );
}


void KCardPile::setLayoutPos( QPointF pos )
{
    d->layoutPos = pos;
}


void KCardPile::setLayoutPos( qreal x,  qreal y )
{
    setLayoutPos( QPointF( x, y ) );
}


QPointF KCardPile::layoutPos() const
{
    return d->layoutPos;
}


void KCardPile::setWidthPolicy( WidthPolicy policy )
{
    d->widthPolicy = policy;
}


KCardPile::WidthPolicy KCardPile::widthPolicy() const
{
    return d->widthPolicy;
}


void KCardPile::setHeightPolicy( HeightPolicy policy )
{
    d->heightPolicy = policy;
}


KCardPile::HeightPolicy KCardPile::heightPolicy() const
{
    return d->heightPolicy;
}


void KCardPile::setPadding( qreal topPadding, qreal rightPadding, qreal bottomPadding, qreal leftPadding )
{
    setTopPadding( topPadding );
    setRightPadding( rightPadding );
    setBottomPadding( bottomPadding );
    setLeftPadding( leftPadding);
}


void KCardPile::setTopPadding( qreal padding )
{
    d->topPadding = padding;
}


qreal KCardPile::topPadding() const
{
    return d->topPadding;
}


void KCardPile::setRightPadding( qreal padding )
{
    d->rightPadding = padding;
}


qreal KCardPile::rightPadding() const
{
    return d->rightPadding;
}


void KCardPile::setBottomPadding( qreal padding )
{
    d->bottomPadding = padding;
}


qreal KCardPile::bottomPadding() const
{
    return d->bottomPadding;
}


void KCardPile::setLeftPadding( qreal padding )
{
    d->leftPadding = padding;
}


qreal KCardPile::leftPadding() const
{
    return d->leftPadding;
}


void KCardPile::setSpread( QPointF spread )
{
    d->spread = spread;
}


void KCardPile::setSpread( qreal width, qreal height )
{
    setSpread( QPointF( width, height ) );
}


QPointF KCardPile::spread() const
{
    return d->spread;
}


void KCardPile::setAutoTurnTop( bool autoTurnTop )
{
    d->autoTurnTop = autoTurnTop;
}


bool KCardPile::autoTurnTop() const
{
    return d->autoTurnTop;
}


void KCardPile::setKeyboardSelectHint( KeyboardFocusHint hint )
{
    d->selectHint = hint;
}


KCardPile::KeyboardFocusHint KCardPile::keyboardSelectHint() const
{
    return d->selectHint;
}


void KCardPile::setKeyboardDropHint( KeyboardFocusHint hint )
{
    d->dropHint = hint;
}


KCardPile::KeyboardFocusHint KCardPile::keyboardDropHint() const
{
    return d->dropHint;
}


void KCardPile::setVisible( bool visible )
{
    if ( visible != isVisible() )
    {
        QGraphicsItem::setVisible( visible );
        foreach ( KCard * c, d->cards )
            c->setVisible( visible );
    }
}


void KCardPile::setHighlighted( bool highlighted )
{
    if ( highlighted != d->highlighted )
    {
        d->highlighted = highlighted;
        d->fadeAnimation->setDirection( highlighted
                                       ? QAbstractAnimation::Forward
                                       : QAbstractAnimation::Backward );
        if ( d->fadeAnimation->state() != QAbstractAnimation::Running )
            d->fadeAnimation->start();
    }
}


bool KCardPile::isHighlighted() const
{
    return d->highlighted;
}


void KCardPile::add( KCard * card )
{
    insert( d->cards.size(), card );
}


void KCardPile::insert( int index, KCard * card )
{
    if ( card->scene() != scene() )
        scene()->addItem( card );

    if ( card->pile() )
        card->pile()->remove( card );

    card->setPile( this );
    card->setVisible( isVisible() );

    d->cards.insert( index, card );
}


void KCardPile::remove( KCard * card )
{
    Q_ASSERT( d->cards.contains( card ) );
    d->cards.removeAll( card );
    card->setPile( 0 );
}


void KCardPile::clear()
{
    foreach ( KCard *card, d->cards )
        remove( card );
}


void KCardPile::swapCards( int index1, int index2 )
{
    if ( index1 == index2 )
        return;

    KCard * temp = d->cards.at( index1 );
    d->cards[ index1 ] = d->cards.at( index2 );
    d->cards[ index2 ] = temp;
}


void KCardPile::paintGraphic( QPainter * painter, qreal highlightedness )
{
    int penWidth = boundingRect().width() / 40;
    int topLeft = penWidth / 2;
    int bottomRight = topLeft - penWidth;
    painter->setPen( QPen( Qt::black, penWidth ) );
    painter->setBrush( QColor( 0, 0, 0, 64 * highlightedness ) );
    painter->drawRect( boundingRect().adjusted( topLeft, topLeft, bottomRight, bottomRight ) );
}


QList<QPointF> KCardPile::cardPositions() const
{
    QList<QPointF> positions;
    for ( int i = 0; i < count(); ++i )
        positions << i * spread();
    return positions;
}


void KCardPile::setGraphicSize( QSize size )
{
    if ( size != d->graphicSize )
    {
        prepareGeometryChange();
        d->graphicSize = size;
        update();
    }
}


#include "kcardpile.moc"
#include "moc_kcardpile.cpp"
