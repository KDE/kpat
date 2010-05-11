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


inline QRectF multRectSize( const QRectF & rect, const QSize & size )
{
    return QRectF( rect.x() * size.width(), 
                   rect.y() * size.height(),
                   rect.width() * size.width(),
                   rect.height() * size.height() );
}


inline QRectF divRectSize( const QRectF & rect, const QSize & size )
{
    return QRectF( rect.x() / size.width(), 
                   rect.y() / size.height(),
                   rect.width() / size.width(),
                   rect.height() / size.height() );
}


class KCardScenePrivate : public QObject
{
public:
    KCardScenePrivate( KCardScene * p );

    int calculateDuration( QPointF pos1, QPointF pos2, qreal velocity ) const;
    void changeFocus( int pileChange, int cardChange );
    void updateKeyboardFocus();

    KCardScene * const q;

    KAbstractCardDeck * deck;
    QList<KCardPile*> piles;
    QSet<QGraphicsItem*> highlightedItems;

    QList<KCard*> cardsBeingDragged;
    QPointF startOfDrag;
    bool dragStarted;

    KCardScene::SceneAlignment alignment;
    qreal layoutMargin;
    qreal layoutSpacing;
    QSizeF contentSize;

    bool keyboardMode;
    int keyboardPileIndex;
    int keyboardCardIndex;
    QGraphicsItem * keyboardFocusItem;

    bool sizeHasBeenSet;
};


KCardScenePrivate::KCardScenePrivate( KCardScene * p )
  : QObject( p ),
    q( p )
{
}


int KCardScenePrivate::calculateDuration( QPointF pos1, QPointF pos2, qreal velocity ) const
{
    QPointF delta = pos2 - pos1;
    qreal distance = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
    qreal cardUnit = ( deck->cardWidth() + deck->cardHeight() ) / 2.0;
    qreal unitDistance = distance / cardUnit;

    return 1000 * unitDistance / velocity;
}


void KCardScenePrivate::changeFocus( int pileChange, int cardChange )
{
    if ( !keyboardMode )
    {
        q->setKeyboardModeActive( true );
        return;
    }

    if ( pileChange )
    {
        KCardPile * pile;
        KCardPile::KeyboardFocusHint hint;
        do
        {
            keyboardPileIndex += pileChange;
            if ( keyboardPileIndex < 0 )
                keyboardPileIndex = piles.size() - 1;
            else if ( keyboardPileIndex >= piles.size() )
                keyboardPileIndex = 0;

            pile = piles.at( keyboardPileIndex );
            hint = cardsBeingDragged.isEmpty()
                 ? pile->keyboardSelectHint()
                 : pile->keyboardDropHint();
        }
        while ( hint == KCardPile::NeverFocus );

        if ( !pile->isEmpty() )
        {
            if ( hint == KCardPile::AutoFocusTop || hint == KCardPile::ForceFocusTop )
            {
                keyboardCardIndex = pile->count() - 1;
            }
            else if ( hint == KCardPile::AutoFocusDeepestRemovable )
            {
                keyboardCardIndex = pile->count() - 1;
                while ( keyboardCardIndex > 0 && q->allowedToRemove( pile, pile->at( keyboardCardIndex - 1 ) ) )
                    --keyboardCardIndex;
            }
            else if ( hint == KCardPile::AutoFocusDeepestFaceUp )
            {
                keyboardCardIndex = pile->count() - 1;
                while ( keyboardCardIndex > 0 && pile->at( keyboardCardIndex - 1 )->isFaceUp() )
                    --keyboardCardIndex;
            }
            else if ( hint == KCardPile::AutoFocusBottom )
            {
                keyboardCardIndex = 0;
            }
        }
    }

    if ( cardChange )
    {
        KCardPile * pile = piles.at( keyboardPileIndex );
        if ( cardChange < 0 && keyboardCardIndex >= pile->count() )
        {
            keyboardCardIndex = qMax( 0, pile->count() - 2 );
        }
        else
        {
            keyboardCardIndex += cardChange;
            if ( keyboardCardIndex < 0 )
                keyboardCardIndex =  pile->count() - 1;
            else if ( keyboardCardIndex >= pile->count() )
                keyboardCardIndex = 0;
        }
    }

    updateKeyboardFocus();
}


void KCardScenePrivate::updateKeyboardFocus()
{
    q->setItemHighlight( keyboardFocusItem, false );

    if ( !keyboardMode )
        return;

    KCardPile * pile = piles.at( keyboardPileIndex );
    KCardPile::KeyboardFocusHint hint = cardsBeingDragged.isEmpty()
                                      ? pile->keyboardSelectHint()
                                      : pile->keyboardDropHint();

    if ( !cardsBeingDragged.isEmpty()
         && cardsBeingDragged.first()->pile() == pile )
    {
        int index = pile->indexOf( cardsBeingDragged.first() );
        if ( index == 0 )
            keyboardFocusItem = pile;
        else
            keyboardFocusItem = pile->at( index - 1 );
    }
    else if ( pile->isEmpty() )
    {
        keyboardFocusItem = pile;
    }
    else if ( keyboardCardIndex >= pile->count() || hint == KCardPile::ForceFocusTop )
    {
        keyboardFocusItem = pile->top();
    }
    else
    {
        keyboardFocusItem = pile->at( keyboardCardIndex );
    }

    q->setItemHighlight( keyboardFocusItem, true );

    QPointF delta = keyboardFocusItem->pos() - startOfDrag;
    startOfDrag = keyboardFocusItem->pos();
    foreach ( KCard * c, cardsBeingDragged )
        c->setPos( c->pos() + delta );
}


KCardScene::KCardScene( QObject * parent )
  : QGraphicsScene( parent ),
    d( new KCardScenePrivate( this ) )
{
    d->deck = 0;
    d->alignment = AlignHCenter | AlignHSpread;
    d->layoutMargin = 0.15;
    d->layoutSpacing = 0.15;
    d->keyboardMode = false;
    d->keyboardPileIndex = 0;
    d->keyboardCardIndex = 0;
    d->keyboardFocusItem = 0;
    d->sizeHasBeenSet = false;
}


KCardScene::~KCardScene()
{
    foreach ( KCardPile * p, d->piles )
    {
        removePile( p );
        delete p;
    }
    d->piles.clear();

    Q_ASSERT( items().isEmpty() );
}


void KCardScene::setDeck( KAbstractCardDeck * deck )
{
    if ( d->deck )
        disconnect( d->deck, SIGNAL(cardAnimationDone()), this, SIGNAL(cardAnimationDone()) );

    d->deck = deck;

    if ( d->deck )
        connect( d->deck, SIGNAL(cardAnimationDone()), this, SIGNAL(cardAnimationDone()) );
}


KAbstractCardDeck * KCardScene::deck() const
{
    return d->deck;
}


bool KCardScene::isCardAnimationRunning() const
{
    return d->deck && d->deck->hasAnimatedCards();
}


QList<KCard*> KCardScene::cardsBeingDragged() const
{
    return d->cardsBeingDragged;
}


void KCardScene::addPile( KCardPile * pile )
{
    KCardScene * origScene = dynamic_cast<KCardScene*>( pile->scene() );
    if ( origScene )
        origScene->removePile( pile );

    addItem( pile );
    foreach ( KCard * c, pile->cards() )
        addItem( c );
    d->piles.append( pile );
}


void KCardScene::removePile( KCardPile * pile )
{
    foreach ( KCard * c, pile->cards() )
        removeItem( c );
    removeItem( pile );
    d->piles.removeAll( pile );
}


QList<KCardPile*> KCardScene::piles() const
{
    return d->piles;
}


void KCardScene::setSceneAlignment( KCardScene::SceneAlignment alignment )
{
    if ( alignment != d->alignment )
    {
        d->alignment = alignment;
        relayoutScene();
    }
}


KCardScene::SceneAlignment KCardScene::sceneAlignment() const
{
    return d->alignment;
}


void KCardScene::setLayoutMargin( qreal margin )
{
    if ( margin != d->layoutMargin )
    {
        d->layoutMargin = margin;
        relayoutScene();
    }
}


qreal KCardScene::layoutMargin() const
{
    return d->layoutMargin;
}


void KCardScene::setLayoutSpacing( qreal spacing )
{
    if ( spacing != d->layoutSpacing )
    {
        d->layoutSpacing = spacing;
        relayoutScene();
    }
}


qreal KCardScene::layoutSpacing() const
{
    return d->layoutSpacing;
}


QRectF KCardScene::contentArea() const
{
    return QRectF( QPoint( 0, 0 ), d->contentSize );
}


void KCardScene::resizeScene( const QSize & size )
{
    d->sizeHasBeenSet = true;
    setSceneRect( QRectF( sceneRect().topLeft(), size ) );
    relayoutScene();
}


void KCardScene::relayoutScene()
{
    if ( !d->sizeHasBeenSet || !d->deck  )
        return;

    qreal usedWidth = 1;
    qreal usedHeight = 1;
    qreal extraWidth = 0;
    qreal extraHeight = 0;
    foreach ( const KCardPile * p, piles() )
    {
        if ( p->layoutPos().x() >= 0 )
            usedWidth = qMax( usedWidth, p->layoutPos().x() + p->reservedSpace().right() );
        else
            extraWidth = qMax( extraWidth, p->reservedSpace().width() );

        if ( p->layoutPos().y() >= 0 )
            usedHeight = qMax( usedHeight, p->layoutPos().y() + p->reservedSpace().bottom() );
        else
            extraHeight = qMax( extraHeight, p->reservedSpace().height() );
    }
    if ( extraWidth )
    {
        qreal hSpacing = d->layoutSpacing * (1 + d->deck->cardHeight() / d->deck->cardWidth()) / 2;
        usedWidth += hSpacing + extraWidth;
    }
    if ( extraHeight )
    {
        qreal vSpacing = d->layoutSpacing * (1 + d->deck->cardWidth() / d->deck->cardHeight()) / 2;
        usedHeight += vSpacing + extraHeight;
    }

    // Add the border to the size of the contents
    QSizeF sizeToFit( usedWidth + 2 * d->layoutMargin, usedHeight+ 2 * d->layoutMargin );

    qreal scaleX = width() / ( d->deck->cardWidth() * sizeToFit.width() );
    qreal scaleY = height() / ( d->deck->cardHeight() * sizeToFit.height() );
    qreal n_scaleFactor = qMin( scaleX, scaleY );

    d->deck->setCardWidth( n_scaleFactor * d->deck->cardWidth() );

    qreal usedPixelWidth = usedWidth * d->deck->cardWidth();
    qreal usedPixelHeight = usedHeight * d->deck->cardHeight();
    qreal pixelHMargin = d->layoutMargin * d->deck->cardWidth();
    qreal pixelVMargin = d->layoutMargin * d->deck->cardHeight();

    qreal contentWidth;
    qreal xOffset;
    if ( d->alignment & AlignLeft )
    {
        contentWidth = usedPixelWidth;
        xOffset = pixelHMargin;
    }
    else if ( d->alignment & AlignRight )
    {
        contentWidth = usedPixelWidth;
        xOffset = width() - contentWidth - pixelHMargin;
    }
    else if ( d->alignment & AlignHCenter )
    {
        contentWidth = usedPixelWidth;
        xOffset = ( width() - contentWidth ) / 2;
    }
    else
    {
        contentWidth = width() - 2 * d->layoutMargin * d->deck->cardWidth();
        xOffset = pixelHMargin;
    }

    qreal contentHeight;
    qreal yOffset;
    if ( d->alignment & AlignTop )
    {
        contentHeight = usedPixelHeight;
        yOffset = pixelVMargin;
    }
    else if ( d->alignment & AlignBottom )
    {
        contentHeight = usedPixelHeight;
        yOffset = height() - contentHeight - pixelVMargin;
    }
    else if ( d->alignment & AlignVCenter )
    {
        contentHeight = usedPixelHeight;
        yOffset = ( height() - contentHeight ) / 2;
    }
    else
    {
        contentHeight = height() - 2 * d->layoutMargin * d->deck->cardHeight();
        yOffset = pixelVMargin;
    }

    d->contentSize = QSizeF( contentWidth, contentHeight );
    setSceneRect( -xOffset, -yOffset, width(), height() );

    relayoutPiles( 0 );
}


void KCardScene::relayoutPiles( int duration )
{
    if ( !d->sizeHasBeenSet || !d->deck )
        return;

    QSize cardSize = d->deck->cardSize();
    const qreal spacing = d->layoutSpacing * ( cardSize.width() + cardSize.height() ) / 2;

    QList<KCardPile*> visiblePiles;
    QHash<KCardPile*,QRectF> reserve;
    QHash<KCardPile*,QRectF> areas;
    foreach ( KCardPile * p, piles() )
    {
        QPointF layoutPos = p->layoutPos();
        if ( layoutPos.x() < 0 )
            layoutPos.rx() += d->contentSize.width() / cardSize.width() - 1;
        if ( layoutPos.y() < 0 )
            layoutPos.ry() += d->contentSize.height() / cardSize.height() - 1;

        p->setPos( layoutPos.x() * cardSize.width(), layoutPos.y() * cardSize.height() );
        p->setAvailableSpace( p->reservedSpace().translated( layoutPos ) );
        p->setGraphicSize( cardSize );

        if ( p->isVisible() )
        {
            visiblePiles << p;
            QRectF r = multRectSize( p->reservedSpace().translated( layoutPos ), cardSize );
            areas[p] = reserve[p] = r;
        }
    }

    // Grow piles down
    foreach ( KCardPile * p1, visiblePiles )
    {
        if ( p1->reservedSpace().height() > 1 )
        {
            areas[p1].setBottom( d->contentSize.height() );
            foreach ( KCardPile * p2, visiblePiles )
            {
                if ( p2 != p1 && areas[p1].intersects( areas[p2] ) )
                {
                    if ( p2->reservedSpace().y() < 0 )
                        areas[p1].setBottom( (reserve[p1].bottom() + reserve[p2].top() - spacing ) / 2 );
                    else
                        areas[p1].setBottom( reserve[p2].top() - spacing );
                }
            }
        }
    }

    // Grow piles up
    foreach ( KCardPile * p1, visiblePiles )
    {
        if ( p1->reservedSpace().y() < 0 )
        {
            areas[p1].setTop( 0 );
            foreach ( KCardPile * p2, visiblePiles )
            {
                if ( p2 != p1 && areas[p1].intersects( areas[p2] ) )
                {
                    if ( p2->reservedSpace().height() > 1 )
                        areas[p1].setTop( (reserve[p1].top() + reserve[p2].bottom() + spacing ) / 2 );
                    else
                        areas[p1].setTop( reserve[p2].bottom() + spacing );
                }
            }
        }
    }

    // Grow piles right
    foreach ( KCardPile * p1, visiblePiles )
    {
        if ( p1->reservedSpace().width() > 1 )
        {
            areas[p1].setRight( d->contentSize.width() );
            foreach ( KCardPile * p2, visiblePiles )
            {
                if ( p2 != p1 && areas[p1].intersects( areas[p2] ) )
                {
                    if ( p2->reservedSpace().x() < 0 )
                        areas[p1].setRight( (reserve[p1].right() + reserve[p2].left() - spacing ) / 2 );
                    else
                        areas[p1].setRight( reserve[p2].left() - spacing );
                }
            }
        }
    }

    // Grow piles left
    foreach ( KCardPile * p1, visiblePiles )
    {
        if ( p1->reservedSpace().x() < 0 )
        {
            areas[p1].setLeft( 0 );
            foreach ( KCardPile * p2, visiblePiles )
            {
                 Q_ASSERT( p2 == p1 || !reserve[p1].intersects( reserve[p2] ) );

                if ( p2 != p1 && areas[p1].intersects( areas[p2] ) )
                {
                    if ( p2->reservedSpace().width() > 1 )
                        areas[p1].setLeft( (reserve[p1].left() + reserve[p2].right() + spacing ) / 2 );
                    else
                        areas[p1].setLeft( reserve[p2].right() + spacing );
                }
            }
        }
    }

    foreach ( KCardPile * p, piles() )
    {
        p->setAvailableSpace( divRectSize( areas[p].translated( -p->pos() ), cardSize ) );
        p->layoutCards( duration );
    }
}


void KCardScene::setHighlightedItems( QList<QGraphicsItem*> items )
{
    QSet<QGraphicsItem*> s = QSet<QGraphicsItem*>::fromList( items );
    foreach ( QGraphicsItem * i, d->highlightedItems.subtract( s ) )
        setItemHighlight( i, false );
    foreach ( QGraphicsItem * i, s )
        setItemHighlight( i, true );
    d->highlightedItems = s;
}


void KCardScene::clearHighlightedItems()
{
    foreach ( QGraphicsItem * i, d->highlightedItems )
        setItemHighlight( i, false );
    d->highlightedItems.clear();
}


QList<QGraphicsItem*> KCardScene::highlightedItems() const
{
    return d->highlightedItems.toList();
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

    KCardPile * source = cards.first()->pile();

    foreach ( KCard * c, cards )
    {
        Q_ASSERT( c->pile() == source );
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
    moveCardToPile( card, pile, d->calculateDuration( origPos, estimatedDestPos, velocity ) );

    card->completeAnimation();
    QPointF destPos = card->pos();
    card->setPos( origPos );

    int duration = d->calculateDuration( origPos, destPos, velocity );
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
    moveCardToPile( card, pile, d->calculateDuration( origPos, estimatedDestPos, velocity ) );

    card->completeAnimation();
    QPointF destPos = card->pos();
    card->setPos( origPos );
    card->setFaceUp( origFaceUp );

    int duration = d->calculateDuration( origPos, destPos, velocity );
    card->animate( destPos, card->zValue(), 1.0, 0.0, !origFaceUp, true, duration );
}


void KCardScene::keyboardFocusLeft()
{
    d->changeFocus( -1, 0 );
}


void KCardScene::keyboardFocusRight()
{
    d->changeFocus( +1, 0 );
}


void KCardScene::keyboardFocusUp()
{
    d->changeFocus( 0, -1 );
}


void KCardScene::keyboardFocusDown()
{
    d->changeFocus( 0, +1 );
}


void KCardScene::keyboardFocusCancel()
{
    setKeyboardModeActive( false );
}


void KCardScene::keyboardFocusSelect()
{
    if ( !isKeyboardModeActive() )
    {
        setKeyboardModeActive( true );
        return;
    }

    if ( d->cardsBeingDragged.isEmpty() )
    {
        KCardPile * pile = d->piles.at( d->keyboardPileIndex );
        if ( pile->isEmpty() )
            return;

        if ( d->keyboardCardIndex >= pile->count() )
            d->keyboardCardIndex = pile->count() - 1;

        KCard * card = pile->at( d->keyboardCardIndex );
        d->cardsBeingDragged = card->pile()->topCardsDownTo( card );
        if ( allowedToRemove( card->pile(), d->cardsBeingDragged.first() ) )
        {
            d->startOfDrag = d->keyboardCardIndex > 0
                          ? pile->at( d->keyboardCardIndex - 1 )->pos()
                          : pile->pos();

            QPointF offset = d->startOfDrag - card->pos() + QPointF( d->deck->cardWidth(), d->deck->cardHeight() ) / 10.0;
            foreach ( KCard * c, d->cardsBeingDragged )
            {
                c->stopAnimation();
                c->raise();
                c->setPos( c->pos() + offset );
            }
            d->dragStarted = true;
            d->updateKeyboardFocus();
        }
        else
        {
            d->cardsBeingDragged.clear();
        }
    }
    else
    {
        KCardPile * destination = targetPile();
        if ( destination )
        {
            moveCardsToPile( d->cardsBeingDragged, destination, cardMoveDuration );
        }
        else
        {
            d->cardsBeingDragged.first()->pile()->layoutCards( cardMoveDuration );
        }
        d->cardsBeingDragged.clear();
        d->dragStarted = false;
    }
}


void KCardScene::setKeyboardModeActive( bool keyboardMode )
{
    if ( !d->keyboardMode && keyboardMode )
    {
        clearHighlightedItems();
        d->keyboardMode = true;
        d->updateKeyboardFocus();
    }
    else if ( d->keyboardMode && !keyboardMode )
    {
        if ( !d->cardsBeingDragged.isEmpty() )
            d->cardsBeingDragged.first()->pile()->layoutCards( cardMoveDuration );
        d->cardsBeingDragged.clear();

        d->keyboardMode = false;
        d->updateKeyboardFocus();
    }
}


bool KCardScene::isKeyboardModeActive() const
{
    return d->keyboardMode;
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
    foreach ( QGraphicsItem * item, collidingItems( d->cardsBeingDragged.first() ) )
    {
        KCardPile * p = qgraphicsitem_cast<KCardPile*>( item );
        if ( !p )
        {
            KCard * c = qgraphicsitem_cast<KCard*>( item );
            if ( c )
                p = c->pile();
        }
        if ( p )
            targets << p;
    }

    KCardPile * bestTarget = 0;
    qreal bestArea = 1;

    foreach ( KCardPile * p, targets )
    {
        if ( p != d->cardsBeingDragged.first()->pile() && allowedToAdd( p, d->cardsBeingDragged ) )
        {
            QRectF targetRect = p->sceneBoundingRect();
            foreach ( KCard *c, p->cards() )
                targetRect |= c->sceneBoundingRect();

            QRectF intersection = targetRect & d->cardsBeingDragged.first()->sceneBoundingRect();
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
    if ( isKeyboardModeActive() )
        setKeyboardModeActive( false );

    QGraphicsItem * item = itemAt( e->scenePos() );
    KCard * card = qgraphicsitem_cast<KCard*>( item );
    KCardPile * pile = qgraphicsitem_cast<KCardPile*>( item );

    if ( e->button() == Qt::LeftButton && ( card || pile ) )
    {
        e->accept();

        if ( card
             && !isCardAnimationRunning()
             && !d->cardsBeingDragged.contains( card ) )
        {
            QList<KCard*> cards = card->pile()->topCardsDownTo( card );

            if ( allowedToRemove( card->pile(), cards.first() ) )
            {
                d->cardsBeingDragged = cards;
                foreach ( KCard * c, d->cardsBeingDragged )
                {
                    c->stopAnimation();
                    c->raise();
                }
            }

            d->dragStarted = false;
            d->startOfDrag = e->scenePos();
        }
    }
    else
    {
        QGraphicsScene::mousePressEvent( e );
    }
}


void KCardScene::mouseMoveEvent( QGraphicsSceneMouseEvent * e )
{
    if ( !d->cardsBeingDragged.isEmpty() )
    {
        e->accept();

        if ( !d->dragStarted )
        {
            bool overCard = d->cardsBeingDragged.first()->sceneBoundingRect().contains( e->scenePos() );
            QPointF delta = e->scenePos() - d->startOfDrag;
            qreal distanceSquared = delta.x() * delta.x() + delta.y() * delta.y();
            // Ignore the move event until we've moved at least 4 pixels or the
            // cursor leaves the card.
            if ( distanceSquared > 16.0 || !overCard )
            {
                d->dragStarted = true;
                // If the cursor hasn't left the card, then continue the drag from
                // the current position, which makes it look smoother.
                if ( overCard )
                    d->startOfDrag = e->scenePos();
            }
        }

        if ( d->dragStarted )
        {
            foreach ( KCard * card, d->cardsBeingDragged )
                card->setPos( card->pos() + e->scenePos() - d->startOfDrag );
            d->startOfDrag = e->scenePos();

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

    if ( e->button() == Qt::LeftButton && !d->dragStarted && !d->cardsBeingDragged.isEmpty() )
    {
        d->cardsBeingDragged.first()->pile()->layoutCards( cardMoveDuration );
        d->cardsBeingDragged.clear();
    }

    if ( e->button() == Qt::LeftButton && !d->cardsBeingDragged.isEmpty() )
    {
        e->accept();

        KCardPile * destination = targetPile();
        if ( destination )
            moveCardsToPile( d->cardsBeingDragged, destination, cardMoveDuration );
        else
            d->cardsBeingDragged.first()->pile()->layoutCards( cardMoveDuration );
        d->cardsBeingDragged.clear();
        d->dragStarted = false;
    }
    else if ( card && !isCardAnimationRunning() )
    {
        e->accept();
        if ( e->button() == Qt::LeftButton )
        {
            emit cardClicked( card );
            if ( card->pile() )
                emit card->pile()->clicked( card );
        }
        else if ( e->button() == Qt::RightButton )
        {
            emit cardRightClicked( card );
            if ( card->pile() )
                emit card->pile()->rightClicked( card );
        }
    }
    else if ( pile && !isCardAnimationRunning() )
    {
        e->accept();
        if ( e->button() == Qt::LeftButton )
        {
            emit pileClicked( pile );
            emit pile->clicked( 0 );
        }
        else if ( e->button() == Qt::RightButton )
        {
            emit pileRightClicked( pile );
            emit pile->rightClicked( 0 );
        }
    }
    else
    {
        QGraphicsScene::mouseReleaseEvent( e );
    }
}


void KCardScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent * e )
{
    QGraphicsItem * topItem = itemAt( e->scenePos() );
    KCard * card = qgraphicsitem_cast<KCard*>( topItem );
    KCardPile * pile = qgraphicsitem_cast<KCardPile*>( topItem );

    if ( !d->cardsBeingDragged.isEmpty() )
    {
        d->cardsBeingDragged.first()->pile()->layoutCards( cardMoveDuration );
        d->cardsBeingDragged.clear();
    }

    if ( card && e->button() == Qt::LeftButton && !isCardAnimationRunning() )
    {
        e->accept();
        emit cardDoubleClicked( card );
        if ( card->pile() )
            emit card->pile()->doubleClicked( card );
    }
    else if ( pile && e->button() == Qt::LeftButton && !isCardAnimationRunning() )
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
    if ( d->deck && e->modifiers() & Qt::ControlModifier )
    {
        e->accept();

        qreal scaleFactor = pow( 2, e->delta() / qreal(10 * 120) );
        int newWidth = d->deck->cardWidth() * scaleFactor;
        d->deck->setCardWidth( newWidth );
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
    if ( !d->sizeHasBeenSet || !d->deck  )
        return;

    painter->setPen( Qt::yellow );
    painter->drawRect( 0, 0, d->contentSize.width(), d->contentSize.height() );

    foreach ( const KCardPile * p, piles() )
    {
        if ( !p->isVisible() )
            continue;

        QRectF reservedRect = multRectSize( p->reservedSpace(), d->deck->cardSize() );
        reservedRect.translate( p->pos() );

        QRectF availableRect = multRectSize( p->availableSpace(), d->deck->cardSize() );

        painter->setPen( Qt::cyan );
        painter->drawRect( availableRect );
        painter->setPen( QPen( Qt::red, 1, Qt::DotLine, Qt::FlatCap ) );
        painter->drawRect( reservedRect );
    }
#endif
}


#include "kcardscene.moc"
