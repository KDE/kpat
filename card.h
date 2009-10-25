/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
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

#ifndef CARD_H
#define CARD_H

class Pile;
class Card;

#include <QtCore/QList>
class QGraphicsItemAnimation;
#include <QtGui/QGraphicsPixmapItem>
class QGraphicsScene;
class QAbstractAnimation;


// A list of cards.  Used in many places.
typedef QList<Card*> CardList;


// In kpat, a Card is an object that has at least two purposes:
//  - It has card properties (Suit, Rank, etc)
//  - It is a graphic entity on a QCanvas that can be moved around.
//

class AbstractCard
{
public:
    enum Suit { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };
    enum Rank { None = 0, Ace = 1, Two,  Three, Four, Five,  Six, Seven,
                Eight,   Nine,  Ten, Jack, Queen, King = 13};

    AbstractCard( Rank r, Suit s );

    // Properties of the card.
    Suit       suit()  const  { return m_suit; }
    Rank       rank()  const  { return m_rank; }

    bool       isRed()    const  { return m_suit == Diamonds || m_suit == Hearts; }
    bool       isFaceUp() const  { return m_faceup; }

protected:
    // The card values.
    Suit        m_suit;
    Rank        m_rank;

    bool        m_faceup;
};

class Card: public QObject, public AbstractCard, public QGraphicsPixmapItem {
    Q_OBJECT
    Q_PROPERTY( QPointF pos READ pos WRITE setPos )
    Q_PROPERTY( qreal rotation READ rotation WRITE setRotation )
    Q_PROPERTY( qreal scale READ scale WRITE setScale )
    Q_PROPERTY( qreal flippedness READ flippedness WRITE setFlippedness )

public:
    Card( Rank r, Suit s );
    virtual ~Card();

    // Some basic tests.
    void       turn(bool faceup = true);

    Pile        *source() const     { return m_source; }
    void         setSource(Pile *p) { m_source = p; }

    enum { Type = UserType + 1 };
    virtual int type() const { return Type; }

    void         moveTo( qreal x2, qreal y2, qreal z, int duration);
    void         flipTo( qreal x, qreal y, int duration );
    void         setZValue(qreal z);

    qreal        realX() const;
    qreal        realY() const;
    qreal        realZ() const;
    bool         realFace() const;

    void         setTakenDown(bool td);
    bool         takenDown() const;

    bool         animated() const;
    void         setVelocity( int x, int y );

    void setHighlighted( bool flag );
    bool isHighlighted() const { return m_highlighted; }

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );

    virtual QRectF boundingRect () const;

    QSizeF spread() const;
    void  setSpread(const QSizeF& spread);

    void         setFlippedness( qreal flippedness );
    qreal        flippedness() const;

signals:
    void       stopped(Card *c);

public slots:
    void       updatePixmap();
    void       flip();
    void       stopAnimation();
    void       zoomInAnimation();
    void       zoomOutAnimation();

private:
    void       zoomIn( int duration );
    void       zoomOut( int duration );

    // Grapics properties.
    bool        m_destFace;
    qreal       m_flippedness;
    Pile       *m_source;

    bool        tookDown;

    // Used for animation
    qreal         m_destX;	// Destination point.
    qreal         m_destY;
    qreal         m_destZ;

    QSizeF        m_spread;

    // The maximum Z ever used.
    static qreal  Hz;

    QAbstractAnimation *animation;

    QRectF    m_boundingRect;
    QPointF   m_originalPosition;
    bool      m_highlighted;
    bool      m_isZoomed;
};

#include <sys/time.h>

extern QString gettime();


#endif
