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

#ifndef KCARD_P_H
#define KCARD_P_H

#include "kcard.h"

class KAbstractCardDeck;
class KCardPile;

class QGraphicsScene;
class QParallelAnimationGroup;
class QPropertyAnimation;


class KCardPrivate : public QObject
{
    Q_OBJECT

    Q_PROPERTY( QPointF pos READ pos WRITE setPos )
    Q_PROPERTY( qreal rotation READ rotation WRITE setRotation )
    Q_PROPERTY( qreal scale READ scale WRITE setScale )
    Q_PROPERTY( qreal flippedness READ flippedness WRITE setFlippedness )
    Q_PROPERTY( qreal highlightedness READ highlightedness WRITE setHighlightedness )

public:
    KCardPrivate( KCard * card );
    void updatePixmap();

    void setPos( QPointF pos );
    QPointF pos() const;

    void setZValue( qreal z );
    qreal zValue() const;

    void setRotation( qreal rotation );
    qreal rotation() const;

    void setScale( qreal scale );
    qreal scale() const;

    void setFlippedness( qreal flippedness );
    qreal flippedness() const;

    void setHighlightedness( qreal highlightedness );
    qreal highlightedness() const;

    bool faceUp;
    bool highlighted;
    quint32 id;

    qreal destZ;
    qreal flipValue;
    qreal highlightValue;

    KCard * q;
    KAbstractCardDeck * deck;
    KCardPile * source;

    QParallelAnimationGroup * animation;
    QPropertyAnimation * fadeAnimation;
};


#endif
