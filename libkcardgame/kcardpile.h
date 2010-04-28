/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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
#include <QtGui/QGraphicsPixmapItem>


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
    KCard * top() const;
    QList<KCard*> topCardsDownTo( const KCard * card ) const;

    void setLayoutPos( QPointF pos );
    void setLayoutPos( qreal x, qreal y );
    QPointF layoutPos() const;

    void setReservedSpace( QRectF space );
    void setReservedSpace( qreal x, qreal y, qreal width, qreal height );
    QRectF reservedSpace() const;

    void setSpread( QSizeF spread );
    void setSpread( qreal width, qreal height );
    QSizeF spread() const;

    void setAutoTurnTop( bool autoTurnTop );
    bool autoTurnTop() const;

    virtual void setVisible(bool vis);

    void setHighlighted( bool highlighted );
    bool isHighlighted() const;

    void setGraphicVisible( bool visible );
    bool isGraphicVisible();

    void add( KCard * card );
    virtual void insert( KCard * card, int index );
    virtual void remove( KCard * card );
    void clear();
    void swapCards( int index1, int index2 );

    virtual void layoutCards( int duration );

Q_SIGNALS:
    void clicked( KCard * card );
    void doubleClicked( KCard * card );
    void rightClicked( KCard * card );

protected:
    virtual void paintNormalGraphic( QPainter * painter );
    virtual void paintHighlightedGraphic( QPainter * painter );

    QRectF availableSpace() const;

    virtual QPointF cardOffset( const KCard * card ) const;

private:
    void setGraphicSize( QSize size );
    void setAvailableSpace( QRectF space );

    class KCardPilePrivate * const d;
    friend class KCardPilePrivate;
    friend class KCardScene;
};

#endif
