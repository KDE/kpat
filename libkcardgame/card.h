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

#ifndef ABSTRACTCARD_H
#define ABSTRACTCARD_H

class KAbstractCardDeck;
class Pile;
#include "libkcardgame_export.h"

#include <QtGui/QGraphicsItem>
class QGraphicsScene;
class QParallelAnimationGroup;
class QPropertyAnimation;


class LIBKCARDGAME_EXPORT Card : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    Q_PROPERTY( QPointF pos READ pos WRITE setPos )
    Q_PROPERTY( qreal rotation READ rotation WRITE setRotation )
    Q_PROPERTY( qreal scale READ scale WRITE setScale )
    Q_PROPERTY( qreal flippedness READ flippedness WRITE setFlippedness )
    Q_PROPERTY( qreal highlightedness READ highlightedness WRITE setHighlightedness )

    friend class KAbstractCardDeck;

public:
    Card( quint32 data, KAbstractCardDeck * deck );
    virtual ~Card();

    bool isFaceUp() const;
    quint32 data() const;

    void raise();

    void turn( bool faceUp );
    void flip();

    void setSource( Pile * pile ) { m_source = pile; }
    Pile * source() const { return m_source; }

    void animate( QPointF pos2, qreal z2, qreal scale2, qreal rotation2, bool faceup2, bool raised, int duration );
    void moveTo( QPointF pos2, qreal z2, int duration );
    bool animated() const;

    QPointF realPos() const;
    qreal realZ() const;
    bool realFace() const;

    void setHighlighted( bool highlighted );
    bool isHighlighted() const;

signals:
    void       animationStarted( Card * card );
    void       animationStopped( Card * card );

public slots:
    void       completeAnimation();
    void       stopAnimation();

protected:


//private:
    void updatePixmap();

    void setHighlightedness( qreal highlightedness );
    qreal highlightedness() const;

    void setFlippedness( qreal flippedness );
    qreal flippedness() const;

    bool m_faceup;
    bool m_highlighted;
    const quint32 m_data;

    bool m_destFace;
    qreal m_destX;
    qreal m_destY;
    qreal m_destZ;

    qreal m_flippedness;
    qreal m_highlightedness;

    KAbstractCardDeck * m_deck;
    Pile * m_source;

    QParallelAnimationGroup * m_animation;
    QPropertyAnimation * m_fadeAnimation;
};

#endif
