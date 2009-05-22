/*****************-*-C++-*-****************



  Card.h -- movable  and stackable cards
            with check for legal  moves



     Copyright (C) 1995  Paul Olav Tvete

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

 ****************************************************/


#ifndef CARD_H
#define CARD_H

class Pile;
class Card;

#include <QtCore/QList>
#include <QtCore/QTimer>
class QGraphicsItemAnimation;
#include <QtGui/QGraphicsPixmapItem>
class QGraphicsScene;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;


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

public:
    Card( Rank r, Suit s, QGraphicsScene *parent=0 );
    virtual ~Card();

    // Some basic tests.
    void       turn(bool faceup = true);

    Pile        *source() const     { return m_source; }
    void         setSource(Pile *p) { m_source = p; }

    enum { Type = UserType + 1 };
    virtual int type() const { return Type; }

    void         moveTo( qreal x2, qreal y2, qreal z, int duration);
    void         flipTo( qreal x, qreal y, int duration );
    void         setZValue(double z);
    void         getUp();

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

    virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
    virtual void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget = 0);

    virtual QRectF boundingRect () const;

    QSizeF spread() const;
    void  setSpread(const QSizeF& spread);

    bool isHovered() const  { return m_hovered; }
    // overload to move shadow
    void setPos(const QPointF &pos);
    virtual bool collidesWithItem ( const QGraphicsItem * other,
                                    Qt::ItemSelectionMode mode ) const;

    enum VisibleState { CardVisible, CardHidden, Unknown };
    void setIsSeen( VisibleState is );

signals:
    void         stoped(Card *c);

public slots:
    void       setPixmap();
    void       flip();
    void       flipAnimationChanged( qreal );
    void       stopAnimation();
    void       zoomInAnimation();
    void       zoomOutAnimation();

private:
    void       zoomIn(int t);
    void       zoomOut(int t);
    void       testVisibility();

    // Grapics properties.
    bool        m_destFace;
    Pile       *m_source;

    bool        tookDown;

    // Used for animation
    qreal         m_destX;	// Destination point.
    qreal         m_destY;
    qreal         m_destZ;

    QSizeF        m_spread;

    // The maximum Z ever used.
    static qreal  Hz;

    QGraphicsItemAnimation *animation;
    QGraphicsPixmapItem *m_shadow;

    QRectF    m_boundingRect;
    QPointF   m_originalPosition;
    QTimer   *m_hoverTimer;
    bool      m_hovered;
    bool      m_highlighted;
    bool      m_moving;
    bool      m_isZoomed;
    VisibleState  m_isSeen;

    // do not use
    void setPos( qreal, qreal );
    void moveBy( qreal, qreal );
    QList<Card*> m_hiddenCards;
};

#include <sys/time.h>

extern QString gettime();


#endif
