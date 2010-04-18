/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <parker.coates@kdemail.net>
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

#include "kcardscene.h"

#include "kabstractcarddeck.h"
#include "kcardpile.h"

#include <KDebug>

#include <QtGui/QGraphicsSceneWheelEvent>
#include <QtGui/QPainter>

#include <cmath>


#define DEBUG_LAYOUT 0


const int cardMoveDuration = 230;


KCardScene::KCardScene( QObject * parent )
  : QGraphicsScene( parent ),
    m_deck( 0 ),
    m_piles(),
    m_alignment( AlignHCenter | AlignHSpread ),
    m_layoutMargin( 0.15 ),
    m_layoutSpacing( 0.15 ),
    m_contentSize(),
    m_sizeHasBeenSet( false )
{
}


KCardScene::~KCardScene()
{
    foreach ( KCardPile * p, m_piles )
    {
        removePile( p );
        delete p;
    }
    m_piles.clear();

    Q_ASSERT( items().isEmpty() );
}


void KCardScene::setDeck( KAbstractCardDeck * deck )
{
    m_deck = deck;
}


KAbstractCardDeck * KCardScene::deck() const
{
    return m_deck;
}


QList<KCard*> KCardScene::cardsBeingDragged() const
{
    return m_cardsBeingDragged;
}


void KCardScene::addPile( KCardPile * pile )
{
    KCardScene * origScene = dynamic_cast<KCardScene*>( pile->scene() );
    if ( origScene )
        origScene->removePile( pile );

    addItem( pile );
    foreach ( KCard * c, pile->cards() )
        addItem( c );
    m_piles.append( pile );
}


void KCardScene::removePile( KCardPile * pile )
{
    foreach ( KCard * c, pile->cards() )
        removeItem( c );
    removeItem( pile );
    m_piles.removeAll( pile );
}


QList<KCardPile*> KCardScene::piles() const
{
    return m_piles;
}


void KCardScene::setSceneAlignment( KCardScene::SceneAlignment alignment )
{
    if ( alignment != m_alignment )
    {
        m_alignment = alignment;
        relayoutScene();
    }
}


KCardScene::SceneAlignment KCardScene::sceneAlignment() const
{
    return m_alignment;
}


void KCardScene::setLayoutMargin( qreal margin )
{
    if ( margin != m_layoutMargin )
    {
        m_layoutMargin = margin;
        relayoutScene();
    }
}


qreal KCardScene::layoutMargin() const
{
    return m_layoutMargin;
}


void KCardScene::setLayoutSpacing( qreal spacing )
{
    if ( spacing != m_layoutSpacing )
    {
        m_layoutSpacing = spacing;
        relayoutScene();
    }
}


qreal KCardScene::layoutSpacing() const
{
    return m_layoutSpacing;
}


QRectF KCardScene::contentArea() const
{
    return QRectF( QPoint( 0, 0 ), m_contentSize );
}


void KCardScene::resizeScene( const QSize & size )
{
    m_sizeHasBeenSet = true;
    setSceneRect( QRectF( sceneRect().topLeft(), size ) );
    relayoutScene();
}


void KCardScene::relayoutScene()
{
    if ( !m_sizeHasBeenSet || !m_deck  )
        return;

    QSizeF usedArea( 0, 0 );
    foreach ( const KCardPile * p, piles() )
    {
        QSizeF neededPileArea;
        if ( ( p->pilePos().x() >= 0 && p->requestedSpace().width() >= 0 )
             || ( p->pilePos().x() < 0 && p->requestedSpace().width() < 0 ) )
            neededPileArea.setWidth( qAbs( p->pilePos().x() + p->requestedSpace().width() ) );
        else
            neededPileArea.setWidth( qMax( qAbs( p->pilePos().x() ), qAbs( p->requestedSpace().width() ) ) );

        if ( ( p->pilePos().y() >= 0 && p->requestedSpace().height() >= 0 )
             || ( p->pilePos().y() < 0 && p->requestedSpace().height() < 0 ) )
            neededPileArea.setHeight( qAbs( p->pilePos().y() + p->requestedSpace().height() ) );
        else
            neededPileArea.setHeight( qMax( qAbs( p->pilePos().y() ), qAbs( p->requestedSpace().height() ) ) );

        usedArea = usedArea.expandedTo( neededPileArea );
    }

    // Add the border to the size of the contents
    QSizeF sizeToFit = usedArea + 2 * QSizeF( m_layoutMargin, m_layoutMargin );

    qreal scaleX = width() / ( m_deck->cardWidth() * sizeToFit.width() );
    qreal scaleY = height() / ( m_deck->cardHeight() * sizeToFit.height() );
    qreal n_scaleFactor = qMin( scaleX, scaleY );

    m_deck->setCardWidth( n_scaleFactor * m_deck->cardWidth() );

    qreal usedPixelWidth = usedArea.width() * m_deck->cardWidth();
    qreal usedPixelHeight = usedArea.height() * m_deck->cardHeight();
    qreal pixelHMargin = m_layoutMargin * m_deck->cardWidth();
    qreal pixelVMargin = m_layoutMargin * m_deck->cardHeight();

    qreal contentWidth;
    qreal xOffset;
    if ( m_alignment & AlignLeft )
    {
        contentWidth = usedPixelWidth;
        xOffset = pixelHMargin;
    }
    else if ( m_alignment & AlignRight )
    {
        contentWidth = usedPixelWidth;
        xOffset = width() - contentWidth - pixelHMargin;
    }
    else if ( m_alignment & AlignHCenter )
    {
        contentWidth = usedPixelWidth;
        xOffset = ( width() - contentWidth ) / 2;
    }
    else
    {
        contentWidth = width() - 2 * m_layoutMargin * m_deck->cardWidth();
        xOffset = pixelHMargin;
    }

    qreal contentHeight;
    qreal yOffset;
    if ( m_alignment & AlignTop )
    {
        contentHeight = usedPixelHeight;
        yOffset = pixelVMargin;
    }
    else if ( m_alignment & AlignBottom )
    {
        contentHeight = usedPixelHeight;
        yOffset = height() - contentHeight - pixelVMargin;
    }
    else if ( m_alignment & AlignVCenter )
    {
        contentHeight = usedPixelHeight;
        yOffset = ( height() - contentHeight ) / 2;
    }
    else
    {
        contentHeight = height() - 2 * m_layoutMargin * m_deck->cardHeight();
        yOffset = pixelVMargin;
    }

    m_contentSize = QSizeF( contentWidth, contentHeight );
    setSceneRect( -xOffset, -yOffset, width(), height() );

    relayoutPiles( 0 );
}


void KCardScene::relayoutPiles( int duration )
{
    if ( !m_sizeHasBeenSet || !m_deck )
        return;

    QSize s = m_contentSize.toSize();
    int cardWidth = m_deck->cardWidth();
    int cardHeight = m_deck->cardHeight();
    const qreal spacing = m_layoutSpacing * ( cardWidth + cardHeight ) / 2;

    foreach ( KCardPile * p, piles() )
    {
        QPointF layoutPos = p->pilePos();
        QPointF pixlePos = QPointF( layoutPos.x() * cardWidth,
                                    layoutPos.y() * cardHeight );
        if ( layoutPos.x() < 0 )
            pixlePos.rx() += m_contentSize.width() - cardWidth;
        if ( layoutPos.y() < 0 )
            pixlePos.ry() += m_contentSize.height() - cardHeight;

        p->setPos( pixlePos );
        p->setGraphicSize( m_deck->cardSize() );

        QSizeF maxSpace = m_deck->cardSize();

        if ( p->requestedSpace().width() > 1 && s.width() > p->x() + cardWidth )
            maxSpace.setWidth( s.width() - p->x() );
        else if ( p->requestedSpace().width() < 0 && p->x() > 0 )
            maxSpace.setWidth( p->x() + cardWidth );

        if ( p->requestedSpace().height() > 1 && s.height() > p->y() + cardHeight )
            maxSpace.setHeight( s.height() - p->y() );
        else if ( p->requestedSpace().height() < 0 && p->y() > 0 )
            maxSpace.setHeight( p->y() + cardHeight );

        p->setAvailableSpace( maxSpace );
    }

    foreach ( KCardPile * p1, piles() )
    {
        if ( !p1->isVisible() || p1->requestedSpace() == QSizeF( 1, 1 ) )
            continue;

        QRectF p1Space( p1->pos(), p1->availableSpace() );
        if ( p1->requestedSpace().width() < 0 )
            p1Space.moveRight( p1->x() + cardWidth );
        if ( p1->requestedSpace().height() < 0 )
            p1Space.moveBottom( p1->y() + cardHeight );

        foreach ( KCardPile * p2, piles() )
        {
            if ( p2 == p1 || !p2->isVisible() )
                continue;

            QRectF p2Space( p2->pos(), p2->availableSpace() );
            if ( p2->requestedSpace().width() < 0 )
                p2Space.moveRight( p2->x() + cardWidth );
            if ( p2->requestedSpace().height() < 0 )
                p2Space.moveBottom( p2->y() + cardHeight );

            // If the piles spaces intersect we need to solve the conflict.
            if ( p1Space.intersects( p2Space ) )
            {
                if ( p1->requestedSpace().width() != 1 )
                {
                    if ( p2->requestedSpace().height() != 1 )
                    {
                        Q_ASSERT( p2->requestedSpace().height() > 0 );
                        // if it's growing too, we win
                        p2Space.setHeight( qMin( p1Space.top() - p2->y() - spacing, p2Space.height() ) );
                        //kDebug() << "1. reduced height of" << p2->objectName();
                    } else // if it's fixed height, we loose
                        if ( p1->requestedSpace().width() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( p2->requestedSpace().width() == 1 );
                            p1Space.setLeft( p2->x() + cardWidth + spacing);
                            //kDebug() << "2. reduced width of" << p1->objectName();
                        } else {
                            p1Space.setRight( p2->x() - spacing );
                            //kDebug() << "3. reduced width of" << p1->objectName();
                        }
                }

                if ( p1->requestedSpace().height() != 1 )
                {
                    if ( p2->requestedSpace().width() != 1 )
                    {
                        Q_ASSERT( p2->requestedSpace().height() > 0 );
                        // if it's growing too, we win
                        p2Space.setWidth( qMin( p1Space.right() - p2->x() - spacing, p2Space.width() ) );
                        //kDebug() << "4. reduced width of" << p2->objectName();
                    } else // if it's fixed height, we loose
                        if ( p1->requestedSpace().height() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( p2->requestedSpace().height() == 1 );
                            Q_ASSERT( false ); // TODO
                            p1Space.setLeft( p2->x() + cardWidth + spacing );
                            //kDebug() << "5. reduced width of" << p1->objectName();
                        } else if ( p2->y() >= 1  ) {
                            p1Space.setBottom( p2->y() - spacing );
                            //kDebug() << "6. reduced height of" << p1->objectName() << (*it2)->y() - 1 << myRect;
                        }
                }
                p2->setAvailableSpace( p2Space.size() );
            }
        }
        p1->setAvailableSpace( p1Space.size() );
    }

    foreach ( KCardPile * p, piles() )
        p->layoutCards( duration );
}


void KCardScene::setHighlightedItems( QList<QGraphicsItem*> items )
{
    QSet<QGraphicsItem*> s = QSet<QGraphicsItem*>::fromList( items );
    foreach ( QGraphicsItem * i, m_highlightedItems.subtract( s ) )
        setItemHighlight( i, false );
    foreach ( QGraphicsItem * i, s )
        setItemHighlight( i, true );
    m_highlightedItems = s;
}


void KCardScene::clearHighlightedItems()
{
    foreach ( QGraphicsItem * i, m_highlightedItems )
        setItemHighlight( i, false );
    m_highlightedItems.clear();
}


QList<QGraphicsItem*> KCardScene::highlightedItems() const
{
    return m_highlightedItems.toList();
}


void KCardScene::setItemHighlight( QGraphicsItem * item, bool highlight )
{
    KCard * card = qgraphicsitem_cast<KCard*>( item );
    if ( card )
    {
        card->setHighlighted( highlight );
        return;
    }

    KCardPile * pile = qgraphicsitem_cast<KCardPile*>( item );
    if ( pile )
    {
        pile->setHighlighted( highlight );
        return;
    }
}


void KCardScene::moveCardsToPile( QList<KCard*> cards, KCardPile * pile, int duration )
{
    if ( cards.isEmpty() )
        return;

    KCardPile * source = cards.first()->source();

    foreach ( KCard * c, cards )
    {
        Q_ASSERT( c->source() == source );
        pile->add( c );
        c->raise();
    }

    source->layoutCards( duration );

    pile->layoutCards( duration );
}


void KCardScene::moveCardToPile( KCard * card, KCardPile * pile, int duration )
{
    moveCardsToPile( QList<KCard*>() << card, pile, duration );
}


void KCardScene::moveCardToPileAtSpeed( KCard * card, KCardPile * pile, qreal velocity )
{
    QPointF origPos = card->pos();

    QPointF estimatedDestPos = pile->isEmpty() ? pile->pos() : pile->top()->pos();
    moveCardToPile( card, pile, calculateDuration( origPos, estimatedDestPos, velocity ) );

    card->completeAnimation();
    QPointF destPos = card->pos();
    card->setPos( origPos );

    int duration = calculateDuration( origPos, destPos, velocity );
    card->animate( destPos, card->zValue(), 1, 0, card->isFaceUp(), true, duration );
}


void KCardScene::flipCardsToPile( QList<KCard*> cards, KCardPile * pile, int duration )
{
    QList<KCard*> revCards;
    QList<bool> origFaces;
    QList<QPointF> origPositions;
    QList<qreal> origZValues;

    for ( int i = cards.size() - 1; i >= 0; --i )
    {
        KCard * c = cards.at( i );
        revCards << c;
        origFaces << c->isFaceUp();
        origZValues << c->zValue();
        origPositions << c->pos();
    }

    moveCardsToPile( revCards, pile, duration );

    for ( int i = 0; i < revCards.size(); ++i )
    {
        KCard * c = revCards.at( i );

        c->completeAnimation();
        c->setFaceUp( origFaces.at( i ) );
        QPointF destPos = c->pos();
        c->setPos( origPositions.at( i ) );
        qreal destZValue = c->zValue();

        // This is a bit of a hack. It means we preserve the z ordering of face
        // up cards, but feel free to mess about with face down ones. This may
        // need to be smarter in the future.
        if ( c->isFaceUp() )
            c->setZValue( origZValues.at( i ) );

        c->animate( destPos, destZValue, 1, 0, !c->isFaceUp(), true, duration );
    }
}


void KCardScene::flipCardToPile( KCard * card, KCardPile * pile, int duration )
{
    flipCardsToPile( QList<KCard*>() << card, pile, duration );
}


void KCardScene::flipCardToPileAtSpeed( KCard * card, KCardPile * pile, qreal velocity )
{
    QPointF origPos = card->pos();
    bool origFaceUp = card->isFaceUp();

    QPointF estimatedDestPos = pile->isEmpty() ? pile->pos() : pile->top()->pos();
    moveCardToPile( card, pile, calculateDuration( origPos, estimatedDestPos, velocity ) );

    card->completeAnimation();
    QPointF destPos = card->pos();
    card->setPos( origPos );
    card->setFaceUp( origFaceUp );

    int duration = calculateDuration( origPos, destPos, velocity );
    card->animate( destPos, card->zValue(), 1.0, 0.0, !origFaceUp, true, duration );
}


bool KCardScene::allowedToAdd( const KCardPile * pile, const QList<KCard*> & cards ) const
{
    Q_UNUSED( pile )
    Q_UNUSED( cards )
    return true;
}


bool KCardScene::allowedToRemove( const KCardPile * pile, const KCard * card ) const
{
    Q_UNUSED( pile )
    Q_UNUSED( card )
    return true;
}


KCardPile * KCardScene::targetPile()
{
    QSet<KCardPile*> targets;
    foreach ( QGraphicsItem * item, collidingItems( m_cardsBeingDragged.first() ) )
    {
        KCardPile * p = qgraphicsitem_cast<KCardPile*>( item );
        if ( !p )
        {
            KCard * c = qgraphicsitem_cast<KCard*>( item );
            if ( c )
                p = c->source();
        }
        if ( p )
            targets << p;
    }

    KCardPile * bestTarget = 0;
    qreal bestArea = 1;

    foreach ( KCardPile * p, targets )
    {
        if ( p != m_cardsBeingDragged.first()->source() && allowedToAdd( p, m_cardsBeingDragged ) )
        {
            QRectF targetRect = p->sceneBoundingRect();
            foreach ( KCard *c, p->cards() )
                targetRect |= c->sceneBoundingRect();

            QRectF intersection = targetRect & m_cardsBeingDragged.first()->sceneBoundingRect();
            qreal area = intersection.width() * intersection.height();
            if ( area > bestArea )
            {
                bestTarget = p;
                bestArea = area;
            }
        }
    }

    return bestTarget;
}


void KCardScene::mousePressEvent( QGraphicsSceneMouseEvent * e )
{
    QGraphicsItem * item = itemAt( e->scenePos() );
    KCard * card = qgraphicsitem_cast<KCard*>( item );
    KCardPile * pile = qgraphicsitem_cast<KCardPile*>( item );

    if ( e->button() == Qt::LeftButton && ( card || pile ) )
    {
        e->accept();

        if ( card
             && !m_deck->hasAnimatedCards()
             && !m_cardsBeingDragged.contains( card ) )
        {
            QList<KCard*> cards = card->source()->topCardsDownTo( card );

            if ( allowedToRemove( card->source(), cards.first() ) )
            {
                m_cardsBeingDragged = cards;
                foreach ( KCard * c, m_cardsBeingDragged )
                {
                    c->stopAnimation();
                    c->raise();
                }
            }

            m_dragStarted = false;
            m_startOfDrag = e->scenePos();
        }
    }
    else
    {
        kDebug() << "Calling QGraphicsScene::mousePressEvent:" << e->isAccepted();
        QGraphicsScene::mousePressEvent( e );
    }
}


void KCardScene::mouseMoveEvent( QGraphicsSceneMouseEvent * e )
{
    if ( !m_cardsBeingDragged.isEmpty() )
    {
        e->accept();

        if ( !m_dragStarted )
        {
            bool overCard = m_cardsBeingDragged.first()->sceneBoundingRect().contains( e->scenePos() );
            QPointF delta = e->scenePos() - m_startOfDrag;
            qreal distanceSquared = delta.x() * delta.x() + delta.y() * delta.y();
            // Ignore the move event until we've moved at least 4 pixels or the
            // cursor leaves the card.
            if ( distanceSquared > 16.0 || !overCard )
            {
                m_dragStarted = true;
                // If the cursor hasn't left the card, then continue the drag from
                // the current position, which makes it look smoother.
                if ( overCard )
                    m_startOfDrag = e->scenePos();
            }
        }

        if ( m_dragStarted )
        {
            foreach ( KCard * card, m_cardsBeingDragged )
                card->setPos( card->pos() + e->scenePos() - m_startOfDrag );
            m_startOfDrag = e->scenePos();

            QList<QGraphicsItem*> toHighlight;
            KCardPile * dropPile = targetPile();
            if ( dropPile )
            {
                if ( dropPile->isEmpty() )
                    toHighlight << dropPile;
                else
                    toHighlight << dropPile->top();
            }

            setHighlightedItems( toHighlight );
        }
    }
    else
    {
        QGraphicsScene::mouseMoveEvent( e );
    }
}


void KCardScene::mouseReleaseEvent( QGraphicsSceneMouseEvent * e )
{
    QGraphicsItem * topItem = itemAt( e->scenePos() );
    KCard * card = qgraphicsitem_cast<KCard*>( topItem );
    KCardPile * pile = qgraphicsitem_cast<KCardPile*>( topItem );

    if ( e->button() == Qt::LeftButton && !m_dragStarted && !m_cardsBeingDragged.isEmpty() )
    {
        m_cardsBeingDragged.first()->source()->layoutCards( cardMoveDuration );
        m_cardsBeingDragged.clear();
    }

    if ( e->button() == Qt::LeftButton && !m_cardsBeingDragged.isEmpty() )
    {
        e->accept();

        KCardPile * destination = targetPile();
        if ( destination )
            moveCardsToPile( m_cardsBeingDragged, destination, cardMoveDuration );
        else
            m_cardsBeingDragged.first()->source()->layoutCards( cardMoveDuration );
        m_cardsBeingDragged.clear();
        m_dragStarted = false;
    }
    else if ( card && !m_deck->hasAnimatedCards() )
    {
        e->accept();
        if ( e->button() == Qt::LeftButton )
        {
            emit cardClicked( card );
            if ( card->source() )
                emit card->source()->clicked( card );
        }
    }
    else if ( pile && !m_deck->hasAnimatedCards() )
    {
        e->accept();
        if ( e->button() == Qt::LeftButton )
        {
            emit pileClicked( pile );
            emit pile->clicked( 0 );
        }
    }
    else
    {
        kDebug() << "Calling QGraphicsScene::mouseReleaseEvent:" << e->isAccepted();
        QGraphicsScene::mouseReleaseEvent( e );
    }
}


void KCardScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent * e )
{
    QGraphicsItem * topItem = itemAt( e->scenePos() );
    KCard * card = qgraphicsitem_cast<KCard*>( topItem );
    KCardPile * pile = qgraphicsitem_cast<KCardPile*>( topItem );

    if ( !m_cardsBeingDragged.isEmpty() )
    {
        m_cardsBeingDragged.first()->source()->layoutCards( cardMoveDuration );
        m_cardsBeingDragged.clear();
    }

    if ( card && e->button() == Qt::LeftButton && !m_deck->hasAnimatedCards() )
    {
        e->accept();
        emit cardDoubleClicked( card );
        if ( card->source() )
            emit card->source()->doubleClicked( card );
    }
    else if ( pile && e->button() == Qt::LeftButton && !m_deck->hasAnimatedCards() )
    {
        e->accept();
        emit pileDoubleClicked( pile );
        emit pile->doubleClicked( 0 );
    }
    else
    {
        QGraphicsScene::mouseDoubleClickEvent( e );
    }
}


void KCardScene::wheelEvent( QGraphicsSceneWheelEvent * e )
{
    if ( m_deck && e->modifiers() & Qt::ControlModifier )
    {
        e->accept();

        qreal scaleFactor = pow( 2, e->delta() / qreal(10 * 120) );
        int newWidth = m_deck->cardWidth() * scaleFactor;
        m_deck->setCardWidth( newWidth );
        relayoutPiles();
    }
    else
    {
        QGraphicsScene::wheelEvent( e );
    }
}


void KCardScene::drawForeground ( QPainter * painter, const QRectF & rect )
{
    Q_UNUSED( rect )
    Q_UNUSED( painter )

#if DEBUG_LAYOUT
    if ( !m_sizeHasBeenSet || !m_deck  )
        return;

    painter->setPen( Qt::yellow );
    painter->drawRect( 0, 0, m_contentSize.width(), m_contentSize.height() );

    const int cardWidth = m_deck->cardWidth();
    const int cardHeight = m_deck->cardHeight();
    foreach ( const Pile * p, piles() )
    {
        if ( !p->isVisible() )
            continue;

        QRectF reservedRect;
        reservedRect.moveTopLeft( p->pos() );
        reservedRect.setWidth( qAbs( p->reservedSpace().width() * cardWidth ) );
        reservedRect.setHeight( qAbs( p->reservedSpace().height() * cardHeight ) );
        if ( p->reservedSpace().width() < 0 )
            reservedRect.moveRight( p->x() + cardWidth );
        if ( p->reservedSpace().height() < 0 )
            reservedRect.moveBottom( p->y() + cardHeight );

        QRectF availableRect;
        availableRect.setSize( p->maximumSpace() );
        availableRect.moveTopLeft( p->pos() );
        if ( p->reservedSpace().width() < 0 )
            availableRect.moveRight( p->x() + cardWidth );
        if ( p->reservedSpace().height() < 0 )
            availableRect.moveBottom( p->y() + cardHeight );

        painter->setPen( Qt::cyan );
        painter->drawRect( availableRect );
        painter->setPen( QPen( Qt::red, 1, Qt::DotLine, Qt::FlatCap ) );
        painter->drawRect( reservedRect );
    }
#endif
}


int KCardScene::calculateDuration( QPointF pos1, QPointF pos2, qreal velocity ) const
{
    QPointF delta = pos2 - pos1;
    qreal distance = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
    qreal cardUnit = ( deck()->cardWidth() + deck()->cardHeight() ) / 2.0;
    qreal unitDistance = distance / cardUnit;

    return 1000 * unitDistance / velocity;
}


#include "kcardscene.moc"
