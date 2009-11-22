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


#define DEBUG_LAYOUT 0

CardScene::CardScene( QObject * parent )
  : PatienceGraphicsScene( parent ),
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


void CardScene::addPile( Pile * pile )
{
    if ( pile->dscene() )
        pile->dscene()->removePile( pile );

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


QSizeF CardScene::contentSize() const
{
    return m_contentSize;
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
        p->rescale();

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


void CardScene::setHighlightedItems( QList<HighlightableItem*> items )
{
    QSet<HighlightableItem*> s = QSet<HighlightableItem*>::fromList( items );
    foreach ( HighlightableItem * i, m_highlightedItems.subtract( s ) )
        i->setHighlighted( false );
    foreach ( HighlightableItem * i, s )
        i->setHighlighted( true );
    m_highlightedItems = s;
}


void CardScene::clearHighlightedItems()
{
    foreach ( HighlightableItem * i, m_highlightedItems )
        i->setHighlighted( false );
    m_highlightedItems.clear();
}


QList< HighlightableItem* > CardScene::highlightedItems() const
{
    return m_highlightedItems.toList();
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
