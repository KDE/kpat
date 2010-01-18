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

#include "abstractcard.h"
class Card;
class CardDeck;
#include "libkcardgame_export.h"
class Pile;

#include <QtCore/QList>
class QGraphicsScene;
class QParallelAnimationGroup;
class QPropertyAnimation;


class LIBKCARDGAME_EXPORT Card: public QObject, public AbstractCard
{
    Q_OBJECT
    Q_PROPERTY( QPointF pos READ pos WRITE setPos )
    Q_PROPERTY( qreal rotation READ rotation WRITE setRotation )
    Q_PROPERTY( qreal scale READ scale WRITE setScale )
    Q_PROPERTY( qreal flippedness READ flippedness WRITE setFlippedness )
    Q_PROPERTY( qreal highlightedness READ highlightedness WRITE setHighlightedness )

    friend class CardDeck;

public:
    enum Suit
    {
        Clubs = 0,
        Diamonds = 1,
        Hearts = 2,
        Spades = 3
    };

    enum Rank
    {
        Ace = 1,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,
        Ten,
        Jack,
        Queen,
        King
    };

private:
    Card( Rank r, Suit s, CardDeck * deck );

public:
    virtual ~Card();

    Suit       suit()  const  { return Suit( m_data >> 4 ); }
    Rank       rank()  const  { return Rank( m_data & 0xf ); }

    bool       isRed()    const  { return suit() == Diamonds || suit() == Hearts; }

    void       raise();
    void       turn(bool faceup = true);

    Pile        *source() const     { return m_source; }
    void         setSource(Pile *p) { m_source = p; }

    enum { Type = UserType + 1 };
    virtual int type() const { return Type; }

    void         moveTo( QPointF pos2, qreal z, int duration);
    void         flipTo( QPointF pos2, int duration );
    void         flipToPile( Pile * destPile, int duration );
    void         animate( QPointF pos2, qreal z2, qreal scale2, qreal rotation2, bool faceup2, bool raised, int duration );

    QPointF      realPos() const;
    qreal        realZ() const;
    bool         realFace() const;

    void         setTakenDown(bool td);
    bool         takenDown() const;

    bool         animated() const;

    void setHighlighted( bool flag );
    bool isHighlighted() const;

signals:
    void       animationStarted(Card *c);
    void       animationStopped(Card *c);

public slots:
    void       flip();
    void       completeAnimation();
    void       stopAnimation();

private:
    void         setHighlightedness( qreal highlightedness );
    qreal        highlightedness() const;

    void         setFlippedness( qreal flippedness );
    qreal        flippedness() const;

    void       updatePixmap();

    CardDeck   *m_deck;
    Pile       *m_source;
    QParallelAnimationGroup *m_animation;
    QPropertyAnimation *m_fadeAnimation;

    qreal         m_flippedness;
    qreal         m_highlightedness;

    qreal         m_destX;	// Destination point.
    qreal         m_destY;
    qreal         m_destZ;
    bool          m_destFace;

    bool      m_takenDown;
    bool      m_highlighted;
};

#endif
