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


#ifndef KCARDSCENE_H
#define KCARDSCENE_H

class KCard;
class KAbstractCardDeck;
class HighlightableItem;
#include "libkcardgame_export.h"
class KCardPile;

#include <QtCore/QSet>
#include <QtGui/QGraphicsScene>

class LIBKCARDGAME_EXPORT KCardScene : public QGraphicsScene
{
public:
    enum SceneAlignmentFlag
    {
        AlignLeft = 0x0001,
        AlignRight = 0x0002,
        AlignHCenter = 0x0004,
        AlignHSpread = 0x0008,
        AlignTop = 0x0010,
        AlignBottom = 0x0020,
        AlignVCenter = 0x0040,
        AlignVSpread = 0x0080
    };
    Q_DECLARE_FLAGS(SceneAlignment, SceneAlignmentFlag)

    KCardScene( QObject * parent = 0 );
    ~KCardScene();

    void setDeck( KAbstractCardDeck * deck );
    KAbstractCardDeck * deck() const;

    QList<KCard*> cardsBeingDragged() const;

    virtual void resizeScene( const QSize & size );
    virtual void relayoutScene();
    virtual void relayoutPiles( int duration = 0);

    void addPile( KCardPile * pile );
    void removePile( KCardPile * pile );
    QList<KCardPile*> piles() const;

    void setSceneAlignment( SceneAlignment alignment );
    SceneAlignment sceneAlignment() const;
    void setLayoutMargin( qreal margin );
    qreal layoutMargin() const;
    void setLayoutSpacing( qreal spacing );
    qreal layoutSpacing() const;

    void setHighlightedItems( QList<QGraphicsItem*> items );
    void clearHighlightedItems();
    QList<QGraphicsItem*> highlightedItems() const;

    virtual void moveCardsToPile( QList<KCard*> cards, KCardPile * pile, int duration );
    void moveCardToPile( KCard * card, KCardPile * pile, int duration );
    void moveCardToPileAtSpeed( KCard * card, KCardPile * pile, qreal velocity );
    void flipCardToPile( KCard * card, KCardPile * pile, int duration );
    void flipCardToPileAtSpeed( KCard * card, KCardPile * pile, qreal velocity );

protected:
    virtual void onGameStateAlteredByUser();

    virtual bool allowedToAdd( const KCardPile * pile, const QList<KCard*> & cards ) const;
    virtual bool allowedToRemove( const KCardPile * pile, const KCard * card ) const;
    virtual KCardPile * targetPile();

    virtual void setItemHighlight( QGraphicsItem * item, bool highlight );

    virtual bool pileClicked( KCardPile * pile );
    virtual bool pileDoubleClicked( KCardPile * pile );
    virtual bool cardClicked( KCard * card );
    virtual bool cardDoubleClicked( KCard * card );

    virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * e );
    virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * e );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * e );
    virtual void wheelEvent( QGraphicsSceneWheelEvent * e );

    virtual void drawForeground ( QPainter * painter, const QRectF & rect );

private:
    int calculateDuration( QPointF pos1, QPointF pos2, qreal velocity ) const;

    KAbstractCardDeck * m_deck;
    QList<KCardPile*> m_piles;
    QSet<QGraphicsItem*> m_highlightedItems;

    QList<KCard*> m_cardsBeingDragged;
    QPointF m_startOfDrag;
    bool m_dragStarted;

    SceneAlignment m_alignment;
    qreal m_layoutMargin;
    qreal m_layoutSpacing;
    QSizeF m_contentSize;

    bool m_sizeHasBeenSet;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( KCardScene::SceneAlignment )

#endif
