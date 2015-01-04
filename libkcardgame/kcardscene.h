/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <coates@kde.org>
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
#include <QGraphicsScene>

class LIBKCARDGAME_EXPORT KCardScene : public QGraphicsScene
{
    Q_OBJECT

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

    void setSceneAlignment( SceneAlignment alignment );
    SceneAlignment sceneAlignment() const;
    void setLayoutMargin( qreal margin );
    qreal layoutMargin() const;
    void setLayoutSpacing( qreal spacing );
    qreal layoutSpacing() const;
    QRectF contentArea() const;

    virtual void resizeScene( const QSize & size );
    virtual void relayoutScene();
    virtual void recalculatePileLayouts();

    void addPile( KCardPile * pile );
    void removePile( KCardPile * pile );
    QList<KCardPile*> piles() const;

    void setHighlightedItems( QList<QGraphicsItem*> items );
    void clearHighlightedItems();
    QList<QGraphicsItem*> highlightedItems() const;

    void moveCardsToPile( const QList<KCard*> & cards, KCardPile * pile, int duration );
    void moveCardToPile( KCard * card, KCardPile * pile, int duration );
    void moveCardsToPileAtSpeed( const QList<KCard*> & cards, KCardPile * pile, qreal velocity );
    void moveCardToPileAtSpeed( KCard * card, KCardPile * pile, qreal velocity );
    void flipCardsToPile( const QList<KCard*> & cards, KCardPile * pile, int duration );
    void flipCardToPile( KCard * card, KCardPile * pile, int duration );
    void flipCardsToPileAtSpeed( const QList<KCard*> & cards, KCardPile * pile, qreal velocity );
    void flipCardToPileAtSpeed( KCard * card, KCardPile * pile, qreal velocity );
    void updatePileLayout( KCardPile * pile, int duration );

    bool isCardAnimationRunning() const;

public Q_SLOTS:
    void keyboardFocusLeft();
    void keyboardFocusRight();
    void keyboardFocusUp();
    void keyboardFocusDown();
    void keyboardFocusCancel();
    void keyboardFocusSelect();

Q_SIGNALS:
    void cardClicked( KCard * card );
    void cardDoubleClicked( KCard * card );
    void cardRightClicked( KCard * card );
    void pileClicked( KCardPile * pile );
    void pileDoubleClicked( KCardPile * pile );
    void pileRightClicked( KCardPile * pile );

    void cardAnimationDone();

protected:
    void setKeyboardFocus( QGraphicsItem * item );

    void setKeyboardModeActive( bool keyboardMode );
    bool isKeyboardModeActive() const;

    virtual bool allowedToAdd( const KCardPile * pile, const QList<KCard*> & cards ) const;
    virtual bool allowedToRemove( const KCardPile * pile, const KCard * card ) const;

    virtual void cardsDroppedOnPile( const QList<KCard*> & cards, KCardPile * pile );
    virtual void cardsMoved( const QList<KCard*> & cards, KCardPile * oldPile, KCardPile * newPile );

    virtual void mouseDoubleClickEvent( QGraphicsSceneMouseEvent * e );
    virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * e );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent * e );
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * e );
    virtual void wheelEvent( QGraphicsSceneWheelEvent * e );

    virtual void drawForeground( QPainter * painter, const QRectF & rect );

private:
    friend class KCardScenePrivate;
    class KCardScenePrivate * const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( KCardScene::SceneAlignment )

#endif
