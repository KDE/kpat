/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@gmail.com>
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

#include "cardscene.h"

#include "carddeck.h"
#include "pile.h"

#include <QtGui/QGraphicsSceneWheelEvent>
#include <QtGui/QPainter>

#include <cmath>


#define DEBUG_LAYOUT 0

CardScene::CardScene( QObject * parent )
  : QGraphicsScene( parent ),
    m_deck( 0 ),
    m_piles(),
    m_layoutMargin( 0.15 ),
    m_layoutSpacing( 0.15 ),
    m_contentSize(),
    m_sizeHasBeenSet( false )
{
}


CardScene::~CardScene()
{
    delete m_deck;

    foreach ( Pile * p, m_piles )
    {
        removePile( p );
        delete p;
    }
    m_piles.clear();

    Q_ASSERT( items().isEmpty() );
}


void CardScene::setDeck( CardDeck * deck )
{
    delete m_deck;
    m_deck = deck;
}


CardDeck * CardScene::deck() const
{
    return m_deck;
}


QList< Card* > CardScene::cardsBeingDragged() const
{
    return m_cardsBeingDragged;
}


void CardScene::addPile( Pile * pile )
{
    if ( pile->cardScene() )
        pile->cardScene()->removePile( pile );

    addItem( pile );
    foreach ( Card * c, pile->cards() )
        addItem( c );
    m_piles.append( pile );
}


void CardScene::removePile( Pile * pile )
{
    foreach ( Card * c, pile->cards() )
        removeItem( c );
    removeItem( pile );
    m_piles.removeAll( pile );
}


QList<Pile*> CardScene::piles() const
{
    return m_piles;
}


bool CardScene::checkAdd( int checkIndex, const Pile * pile, const QList<Card*> & cards ) const
{
    Q_UNUSED( checkIndex )
    Q_UNUSED( pile )
    Q_UNUSED( cards )
    return true;
}


bool CardScene::checkRemove( int checkIndex, const Pile * pile, const Card * card ) const
{
    Q_UNUSED( checkIndex )
    Q_UNUSED( pile )
    Q_UNUSED( card )
    return true;
}


void CardScene::setLayoutMargin( qreal margin )
{
    m_layoutMargin = margin;
}


qreal CardScene::layoutMargin() const
{
    return m_layoutMargin;
}


void CardScene::setLayoutSpacing( qreal spacing )
{
    m_layoutSpacing = spacing;
}


qreal CardScene::layoutSpacing() const
{
    return m_layoutSpacing;
}


void CardScene::resizeScene( const QSize & size )
{
    m_sizeHasBeenSet = true;
    setSceneRect( QRectF( sceneRect().topLeft(), size ) );
    relayoutScene();
}


void CardScene::relayoutScene()
{
    if ( !m_sizeHasBeenSet )
        return;

    QSizeF usedArea( 0, 0 );
    foreach ( const Pile * p, piles() )
    {
        QSizeF neededPileArea;
        if ( ( p->pilePos().x() >= 0 && p->reservedSpace().width() >= 0 )
             || ( p->pilePos().x() < 0 && p->reservedSpace().width() < 0 ) )
            neededPileArea.setWidth( qAbs( p->pilePos().x() + p->reservedSpace().width() ) );
        else
            neededPileArea.setWidth( qMax( qAbs( p->pilePos().x() ), qAbs( p->reservedSpace().width() ) ) );

        if ( ( p->pilePos().y() >= 0 && p->reservedSpace().height() >= 0 )
             || ( p->pilePos().y() < 0 && p->reservedSpace().height() < 0 ) )
            neededPileArea.setHeight( qAbs( p->pilePos().y() + p->reservedSpace().height() ) );
        else
            neededPileArea.setHeight( qMax( qAbs( p->pilePos().y() ), qAbs( p->reservedSpace().height() ) ) );

        usedArea = usedArea.expandedTo( neededPileArea );
    }

    // Add the border to the size of the contents
    QSizeF sizeToFit = usedArea + 2 * QSizeF( m_layoutMargin, m_layoutMargin );

    qreal scaleX = width() / ( deck()->cardWidth() * sizeToFit.width() );
    qreal scaleY = height() / ( deck()->cardHeight() * sizeToFit.height() );
    qreal n_scaleFactor = qMin( scaleX, scaleY );

    deck()->setCardWidth( n_scaleFactor * deck()->cardWidth() );

    m_contentSize = QSizeF( usedArea.width() * deck()->cardWidth(),
                            height() - 2 * m_layoutMargin * deck()->cardHeight() );

    qreal xOffset = ( width() - m_contentSize.width() ) / 2;
    qreal yOffset = ( height() - m_contentSize.height() ) / 2;

    setSceneRect( -xOffset, -yOffset, width(), height() );

    relayoutPiles();
}


void CardScene::relayoutPiles()
{
    if ( !m_sizeHasBeenSet )
        return;

    QSize s = m_contentSize.toSize();
    int cardWidth = deck()->cardWidth();
    int cardHeight = deck()->cardHeight();
    const qreal spacing = m_layoutSpacing * ( cardWidth + cardHeight ) / 2;

    foreach ( Pile * p, piles() )
    {
        QPointF layoutPos = p->pilePos();
        QPointF pixlePos = QPointF( layoutPos.x() * deck()->cardWidth(),
                                    layoutPos.y() * deck()->cardHeight() );
        if ( layoutPos.x() < 0 )
            pixlePos.rx() += m_contentSize.width() - deck()->cardWidth();
        if ( layoutPos.y() < 0 )
            pixlePos.ry() += m_contentSize.height() - deck()->cardHeight();

        p->setPos( pixlePos );
        p->updatePixmap();

        QSizeF maxSpace = deck()->cardSize();

        if ( p->reservedSpace().width() > 1 && s.width() > p->x() + cardWidth )
            maxSpace.setWidth( s.width() - p->x() );
        else if ( p->reservedSpace().width() < 0 && p->x() > 0 )
            maxSpace.setWidth( p->x() + cardWidth );

        if ( p->reservedSpace().height() > 1 && s.height() > p->y() + cardHeight )
            maxSpace.setHeight( s.height() - p->y() );
        else if ( p->reservedSpace().height() < 0 && p->y() > 0 )
            maxSpace.setHeight( p->y() + cardHeight );

        p->setMaximumSpace( maxSpace );
    }

    foreach ( Pile * p1, piles() )
    {
        if ( !p1->isVisible() || p1->reservedSpace() == QSizeF( 1, 1 ) )
            continue;

        QRectF p1Space( p1->pos(), p1->maximumSpace() );
        if ( p1->reservedSpace().width() < 0 )
            p1Space.moveRight( p1->x() + cardWidth );
        if ( p1->reservedSpace().height() < 0 )
            p1Space.moveBottom( p1->y() + cardHeight );

        foreach ( Pile * p2, piles() )
        {
            if ( p2 == p1 || !p2->isVisible() )
                continue;

            QRectF p2Space( p2->pos(), p2->maximumSpace() );
            if ( p2->reservedSpace().width() < 0 )
                p2Space.moveRight( p2->x() + cardWidth );
            if ( p2->reservedSpace().height() < 0 )
                p2Space.moveBottom( p2->y() + cardHeight );

            // If the piles spaces intersect we need to solve the conflict.
            if ( p1Space.intersects( p2Space ) )
            {
                if ( p1->reservedSpace().width() != 1 )
                {
                    if ( p2->reservedSpace().height() != 1 )
                    {
                        Q_ASSERT( p2->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        p2Space.setHeight( qMin( p1Space.top() - p2->y() - spacing, p2Space.height() ) );
                        //kDebug() << "1. reduced height of" << p2->objectName();
                    } else // if it's fixed height, we loose
                        if ( p1->reservedSpace().width() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( p2->reservedSpace().width() == 1 );
                            p1Space.setLeft( p2->x() + cardWidth + spacing);
                            //kDebug() << "2. reduced width of" << p1->objectName();
                        } else {
                            p1Space.setRight( p2->x() - spacing );
                            //kDebug() << "3. reduced width of" << p1->objectName();
                        }
                }

                if ( p1->reservedSpace().height() != 1 )
                {
                    if ( p2->reservedSpace().width() != 1 )
                    {
                        Q_ASSERT( p2->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        p2Space.setWidth( qMin( p1Space.right() - p2->x() - spacing, p2Space.width() ) );
                        //kDebug() << "4. reduced width of" << p2->objectName();
                    } else // if it's fixed height, we loose
                        if ( p1->reservedSpace().height() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( p2->reservedSpace().height() == 1 );
                            Q_ASSERT( false ); // TODO
                            p1Space.setLeft( p2->x() + cardWidth + spacing );
                            //kDebug() << "5. reduced width of" << p1->objectName();
                        } else if ( p2->y() >= 1  ) {
                            p1Space.setBottom( p2->y() - spacing );
                            //kDebug() << "6. reduced height of" << p1->objectName() << (*it2)->y() - 1 << myRect;
                        }
                }
                p2->setMaximumSpace( p2Space.size() );
            }
        }
        p1->setMaximumSpace( p1Space.size() );
    }

    foreach ( Pile * p, piles() )
        p->layoutCards( 0 );
}


void CardScene::setHighlightedItems( QList<QGraphicsItem*> items )
{
    QSet<QGraphicsItem*> s = QSet<QGraphicsItem*>::fromList( items );
    foreach ( QGraphicsItem * i, m_highlightedItems.subtract( s ) )
        setItemHighlight( i, false );
    foreach ( QGraphicsItem * i, s )
        setItemHighlight( i, true );
    m_highlightedItems = s;
}


void CardScene::clearHighlightedItems()
{
    foreach ( QGraphicsItem * i, m_highlightedItems )
        setItemHighlight( i, false );
    m_highlightedItems.clear();
}


QList<QGraphicsItem*> CardScene::highlightedItems() const
{
    return m_highlightedItems.toList();
}


void CardScene::setItemHighlight( QGraphicsItem * item, bool highlight )
{
    Card * card = qgraphicsitem_cast<Card*>( item );
    if ( card )
    {
        card->setHighlighted( highlight );
        return;
    }

    Pile * pile = qgraphicsitem_cast<Pile*>( item );
    if ( pile )
    {
        pile->setHighlighted( highlight );
        return;
    }
}


void CardScene::onGameStateAlteredByUser()
{
}


Pile * CardScene::targetPile()
{
    QSet<Pile*> targets;
    foreach ( QGraphicsItem * item, collidingItems( m_cardsBeingDragged.first() ) )
    {
        Pile * p = qgraphicsitem_cast<Pile*>( item );
        if ( !p )
        {
            Card * c = qgraphicsitem_cast<Card*>( item );
            if ( c )
                p = c->source();
        }
        if ( p )
            targets << p;
    }

    Pile * bestTarget = 0;
    qreal bestArea = 1;

    foreach ( Pile * p, targets )
    {
        if ( p != m_cardsBeingDragged.first()->source() && p->legalAdd( m_cardsBeingDragged ) )
        {
            QRectF targetRect = p->sceneBoundingRect();
            foreach ( Card *c, p->cards() )
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


bool CardScene::pileClicked( Pile * pile )
{
    if ( pile->cardClicked( 0 ) )
    {
        onGameStateAlteredByUser();
        return true;
    }
    return false;
}


bool CardScene::pileDoubleClicked( Pile * pile )
{
    if ( pile->cardDoubleClicked( 0 ) )
    {
        onGameStateAlteredByUser();
        return true;
    }
    return false;
}


bool CardScene::cardClicked( Card * card )
{
    if ( card->source()->cardClicked( card ) )
    {
        onGameStateAlteredByUser();
        return true;
    }
    return false;
}


bool CardScene::cardDoubleClicked( Card * card )
{
    if ( card->source()->cardDoubleClicked( card ) )
    {
        onGameStateAlteredByUser();
        return true;
    }
    return false;
}


void CardScene::mousePressEvent( QGraphicsSceneMouseEvent * e )
{
    // don't allow manual moves while animations are going on
    if ( deck()->hasAnimatedCards() )
        return;

    if ( e->button() == Qt::LeftButton )
    {
        Card *card = qgraphicsitem_cast<Card*>( itemAt( e->scenePos() ) );
        if ( !card || m_cardsBeingDragged.contains( card ) )
            return;

        m_dragStarted = false;
        m_cardsBeingDragged = card->source()->cardPressed( card );
        m_startOfDrag = e->scenePos();
    }
}


void CardScene::mouseMoveEvent( QGraphicsSceneMouseEvent * e )
{
    if ( m_cardsBeingDragged.isEmpty() )
        return;

    if ( !m_dragStarted )
    {
        bool overCard = m_cardsBeingDragged.first()->sceneBoundingRect().contains( e->scenePos() );
        QPointF delta = e->scenePos() - m_startOfDrag;
        qreal distanceSquared = delta.x() * delta.x() + delta.y() * delta.y();
        // Ignore the move event until we've moved at least 4 pixels or the
        // cursor leaves the card.
        if (distanceSquared > 16.0 || !overCard) {
            m_dragStarted = true;
            // If the cursor hasn't left the card, then continue the drag from
            // the current position, which makes it look smoother.
            if (overCard)
                m_startOfDrag = e->scenePos();
        }
    }

    if ( m_dragStarted )
    {
        foreach ( Card * card, m_cardsBeingDragged )
            card->setPos( card->pos() + e->scenePos() - m_startOfDrag );
        m_startOfDrag = e->scenePos();

        QList<QGraphicsItem*> toHighlight;
        Pile * dropPile = targetPile();
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


void CardScene::mouseReleaseEvent( QGraphicsSceneMouseEvent * e )
{
    if ( e->button() == Qt::LeftButton )
    {
        if ( !m_dragStarted )
        {
            if ( !m_cardsBeingDragged.isEmpty() )
            {
                m_cardsBeingDragged.first()->source()->moveCardsBack(m_cardsBeingDragged);
                m_cardsBeingDragged.clear();
            }

            QGraphicsItem * topItem = itemAt( e->scenePos() );
            if ( !topItem )
                return;

            Card * card = qgraphicsitem_cast<Card*>( topItem );
            if ( card && !card->animated() )
            {
                cardClicked( card );
                return;
            }

            Pile * pile = qgraphicsitem_cast<Pile*>( topItem );
            if ( pile )
            {
                pileClicked( pile );
                return;
            }
        }

        if ( m_cardsBeingDragged.isEmpty() )
            return;

        Pile * destination = targetPile();
        if ( destination )
        {
            m_cardsBeingDragged.first()->source()->moveCards( m_cardsBeingDragged, destination );
            onGameStateAlteredByUser();
        }
        else
        {
            m_cardsBeingDragged.first()->source()->moveCardsBack(m_cardsBeingDragged);
        }
        m_cardsBeingDragged.clear();
        m_dragStarted = false;
    }
}


void CardScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent * e )
{
    if ( deck()->hasAnimatedCards() )
        return;

    if ( !m_cardsBeingDragged.isEmpty() )
    {
        m_cardsBeingDragged.first()->source()->moveCardsBack( m_cardsBeingDragged );
        m_cardsBeingDragged.clear();
    }

    Card * c = qgraphicsitem_cast<Card*>( itemAt( e->scenePos() ) );
    if ( c )
        cardDoubleClicked( c );
}


void CardScene::wheelEvent( QGraphicsSceneWheelEvent * e )
{
    if ( e->modifiers() & Qt::ControlModifier )
    {
        qreal scaleFactor = pow( 2, e->delta() / qreal(10 * 120) );
        int newWidth = deck()->cardWidth() * scaleFactor;
        deck()->setCardWidth( newWidth );
        relayoutPiles();
    }
}


void CardScene::drawForeground ( QPainter * painter, const QRectF & rect )
{
    Q_UNUSED( rect )
    Q_UNUSED( painter )

#if DEBUG_LAYOUT
    if ( !m_sizeHasBeenSet )
        return;

    const int cardWidth = deck()->cardWidth();
    const int cardHeight = deck()->cardHeight();
    foreach ( const Pile *p, piles() )
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

        QRectF availbleRect;
        availbleRect.setSize( p->maximumSpace() );
        availbleRect.moveTopLeft( p->pos() );
        if ( p->reservedSpace().width() < 0 )
            availbleRect.moveRight( p->x() + cardWidth );
        if ( p->reservedSpace().height() < 0 )
            availbleRect.moveBottom( p->y() + cardHeight );

        painter->setPen( Qt::cyan );
        painter->drawRect( availbleRect );
        painter->setPen( QPen( Qt::red, 1, Qt::DotLine, Qt::FlatCap ) );
        painter->drawRect( reservedRect );
    }
#endif
}
