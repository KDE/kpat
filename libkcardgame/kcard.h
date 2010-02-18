/*
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KCARD_H
#define KCARD_H

class KAbstractCardDeck;
class KCardPile;
#include "libkcardgame_export.h"

#include <QtGui/QGraphicsItem>

class LIBKCARDGAME_EXPORT KCard : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES( QGraphicsItem )

private:
    KCard( quint32 id, KAbstractCardDeck * deck );
    virtual ~KCard();

public:
    enum { Type = QGraphicsItem::UserType + 1 };
    virtual int type() const;

    virtual QRectF boundingRect() const;

    quint32 id() const;

    void setSource( KCardPile * pile );
    KCardPile * source() const;

    void setFaceUp( bool faceUp );
    bool isFaceUp() const;

    void animate( QPointF pos, qreal z, qreal scale, qreal rotation, bool faceUp, bool raised, int duration );
    bool isAnimated() const;

    void raise();

    void setHighlighted( bool highlighted );
    bool isHighlighted() const;

signals:
    void animationStarted( KCard * card );
    void animationStopped( KCard * card );

public slots:
    void completeAnimation();
    void stopAnimation();

protected:
    virtual void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

private:
    void setFrontsidePixmap( const QPixmap & pix );

    class KCardPrivate * const d;

    friend class KCardPrivate;
    friend class KAbstractCardDeck;
    friend class KAbstractCardDeckPrivate;
};

#endif
