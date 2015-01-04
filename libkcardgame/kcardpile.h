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

#ifndef KCARDPILE_H
#define KCARDPILE_H

#include "kcard.h"
class KCardScene;
#include "libkcardgame_export.h"

class QPropertyAnimation;
#include <QGraphicsPixmapItem>


class LIBKCARDGAME_EXPORT KCardPile : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit KCardPile( KCardScene * cardScene );
    virtual ~KCardPile();

    enum { Type = QGraphicsItem::UserType + 2 };
    virtual int type() const;

    virtual QRectF boundingRect() const;
    virtual void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    QList<KCard*> cards() const;
    int count() const;
    bool isEmpty() const;
    int indexOf( const KCard * card ) const;
    KCard * at( int index ) const;
    KCard * topCard() const;
    QList<KCard*> topCards( int depth ) const;
    QList<KCard*> topCardsDownTo( const KCard * card ) const;

    void setLayoutPos( QPointF pos );
    void setLayoutPos( qreal x, qreal y );
    QPointF layoutPos() const;

    enum WidthPolicy
    {
        FixedWidth,
        GrowLeft,
        GrowRight
    };
    void setWidthPolicy( WidthPolicy policy );
    WidthPolicy widthPolicy() const;

    enum HeightPolicy
    {
        FixedHeight,
        GrowUp,
        GrowDown
    };
    void setHeightPolicy( HeightPolicy policy );
    HeightPolicy heightPolicy() const;

    void setPadding( qreal topPadding, qreal rightPadding, qreal bottomPadding, qreal leftPadding );
    void setTopPadding( qreal padding );
    qreal topPadding() const;
    void setRightPadding( qreal padding );
    qreal rightPadding() const;
    void setBottomPadding( qreal padding );
    qreal bottomPadding() const;
    void setLeftPadding( qreal padding );
    qreal leftPadding() const;

    void setSpread( QPointF spread );
    void setSpread( qreal width, qreal height );
    QPointF spread() const;

    void setAutoTurnTop( bool autoTurnTop );
    bool autoTurnTop() const;

    enum KeyboardFocusHint
    {
        FreeFocus,
        AutoFocusTop,
        AutoFocusDeepestRemovable,
        AutoFocusDeepestFaceUp,
        AutoFocusBottom,
        ForceFocusTop,
        NeverFocus
    };

    void setKeyboardSelectHint( KeyboardFocusHint hint );
    KeyboardFocusHint keyboardSelectHint() const;
    void setKeyboardDropHint( KeyboardFocusHint hint );
    KeyboardFocusHint keyboardDropHint() const;

    virtual void setVisible(bool vis);

    void setHighlighted( bool highlighted );
    bool isHighlighted() const;

    void add( KCard * card );
    virtual void insert( int index, KCard * card );
    virtual void remove( KCard * card );
    void clear();
    void swapCards( int index1, int index2 );

    virtual QList<QPointF> cardPositions() const;

Q_SIGNALS:
    void clicked( KCard * card );
    void doubleClicked( KCard * card );
    void rightClicked( KCard * card );

protected:
    virtual void paintGraphic( QPainter * painter, qreal highlightedness );

private:
    void setGraphicSize( QSize size );

    class KCardPilePrivate * const d;
    friend class KCardPilePrivate;
    friend class KCardScene;
};

#endif
