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


#ifndef PATIENCE_CARD
#define PATIENCE_CARD

#include <QPixmap>
#include <QImage>
#include <QList>
#include <QGraphicsScene>
#include <QTimer>
#include <QGraphicsSvgItem>

// The following classes are defined in other headers:
class cardPos;
class Dealer;
class Pile;
class Card;
class QGraphicsItemAnimation;

// A list of cards.  Used in many places.
typedef QList<Card*> CardList;

// In kpat, a Card is an object that has at least two purposes:
//  - It has card properties (Suit, Rank, etc)
//  - It is a graphic entity on a QCanvas that can be moved around.
//

class Card: public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

public:
    enum Suit { Clubs = 1, Diamonds, Hearts, Spades };
    enum Rank { None = 0, Ace = 1, Two,  Three, Four, Five,  Six, Seven,
		          Eight,   Nine, Ten,   Jack, Queen, King };

    Card( Rank r, Suit s, QGraphicsScene *parent=0 );
    virtual ~Card();

    // Properties of the card.
    Suit          suit()  const  { return m_suit; }
    Rank          rank()  const  { return m_rank; }
    const QString name()  const  { return m_name; }

    // Some basic tests.
    bool       isRed()    const  { return m_suit==Diamonds || m_suit==Hearts; }
    bool       isFaceUp() const  { return m_faceup; }

    void       turn(bool faceup = true);

    Pile        *source() const     { return m_source; }
    void         setSource(Pile *p) { m_source = p; }

    virtual int  type() const       { return UserType + my_type; }

    void         moveTo( qreal x2, qreal y2, qreal z, int duration);
    void         flipTo( qreal x, qreal y, int duration = 300);
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

    void setElementId( const QString & element );
    virtual QRectF boundingRect () const;

    QSizeF spread() const;
    void  setSpread(const QSizeF& spread);

signals:
    void         stoped(Card *c);

public slots:
    void       rescale();
    void       flip();
    void       flipAnimationChanged( qreal );
    void       stopAnimation();
    void       zoomInAnimation();
    void       zoomOutAnimation();


private:
    void       zoomIn(int t);
    void       zoomOut(int t);
private:
    // The card values.
    Suit        m_suit;
    Rank        m_rank;
    QString     m_name;
    QString     m_elementId;

    // Grapics properties.
    bool        m_faceup;	// True if card lies with the face up.
    bool        m_destFace;
    Pile       *m_source;

    bool        tookDown;

    // Used for animation
    qreal         m_destX;	// Destination point.
    qreal         m_destY;
    qreal         m_destZ;

    QSizeF        m_spread;

    // The maximum Z ever used.
    static int  Hz;

    static const int my_type;
    QGraphicsItemAnimation *animation;

    QPointF   m_originalPosition;
    QTimer   *m_hoverTimer;
    bool      m_hovered;
    bool      m_highlighted;
    bool      m_moving;
};


#endif
