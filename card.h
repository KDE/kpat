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

#include <qcanvas.h>

// The following classes are defined in other headers:
class cardPos;
class Deck;
class Dealer;
class Pile;
class Card;


// A list of cards.  Used in many places.
typedef QValueList<Card*> CardList;


// In kpat, a Card is an object that has at least two purposes:
//  - It has card properties (Suit, Rank, etc)
//  - It is a graphic entity on a QCanvas that can be moved around.
//
class Card: public QObject, public QCanvasRectangle {
    Q_OBJECT

public:
    enum Suit { Clubs = 1, Diamonds, Hearts, Spades };
    enum Rank { None = 0, Ace = 1, Two,  Three, Four, Five,  Six, Seven, 
		          Eight,   Nine, Ten,   Jack, Queen, King };

    Card( Rank r, Suit s,  QCanvas *parent=0);
    virtual ~Card();

    // Properties of the card.
    Suit       suit()     const  { return m_suit; }
    Rank       rank()     const  { return m_rank; }

    // Some basic tests.
    bool       isRed()    const  { return m_suit==Diamonds || m_suit==Hearts; }
    bool       isFaceUp() const  { return faceup; }

    QPixmap    pixmap() const;

    void       flipTo(int x, int y, int steps);
    void       turn(bool faceup = true);

    static const int RTTI;

    Pile        *source() const     { return m_source; }
    void         setSource(Pile *p) { m_source = p; }

    const char  *name() const       { return m_name; }
    virtual int  rtti() const       { return RTTI; }

    virtual void moveBy(double dx, double dy);
    void         animatedMove(int x2, int y2, int z, int steps);
    virtual void setAnimated(bool anim);
    void         setZ(double z);
    void         getUp(int steps = 12);

    int          realX() const;
    int          realY() const;
    int          realZ() const;
    bool         realFace() const;

    void         setTakenDown(bool td);
    bool         takenDown() const;

signals:
    void         stoped(Card *c);

protected:
    void         draw( QPainter &p );	// Redraw the card.
    void         advance(int stage);

private:
    // The card.
    Suit        m_suit;
    Rank        m_rank;
    char       *m_name;

    bool        faceup;
    Pile       *m_source;

    double      scaleX;
    double      scaleY;

    bool        tookDown;

    // Used for animation
    int         destX;	// Destination point.
    int         destY;
    int         destZ;
    int         animSteps;

    // The maximum Z ever used.
    static int  Hz;

    // Used if flipping during an animated move.
    bool        flipping;
    int         flipSteps;
};


#endif
