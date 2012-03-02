/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include <QtCore/QAbstractAnimation>
class QPropertyAnimation;


class KCardAnimation : public QAbstractAnimation
{
public:
    KCardAnimation( KCardPrivate * d, int duration, QPointF pos, qreal rotation, bool faceUp );
    int duration() const;
    void updateCurrentTime( int msec );

private:
    KCardPrivate * d;

    int m_duration;

    qreal m_x0;
    qreal m_y0;
    qreal m_rotation0;
    qreal m_flippedness0;

    qreal m_xDelta;
    qreal m_yDelta;
    qreal m_rotationDelta;
    qreal m_flippednessDelta;

    qreal m_flipProgressFactor;
};


class KCardPrivate : public QObject
{
    Q_OBJECT

    Q_PROPERTY( qreal highlightedness READ highlightedness WRITE setHighlightedness )

public:
    KCardPrivate( KCard * card );

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
    KCardPile * pile;

    QPixmap frontPixmap;
    QPixmap backPixmap;

    KCardAnimation * animation;
    QPropertyAnimation * fadeAnimation;
};


#endif
