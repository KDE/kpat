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

#include <qpoint.h>
#include <qcanvas.h>

// The following classes are defined in other headers:
class cardPos;
class Deck;
class dealer;
class Pile;
class Card;

typedef QValueList<Card*> CardList;

class Card: public QCanvasRectangle {

public:
    enum Suits { Clubs = 1, Diamonds, Hearts, Spades };
    enum Values { Ace = 1, Two, Three, Four, Five, Six, Seven, Eight,
                  Nine, Ten, Jack, Queen, King };

    QPixmap pixmap() const;

    Card( Values v, Suits s,  QCanvas *parent=0);
    virtual ~Card();

    Suits suit() const { return _suit; }
    Values value() const { return _value; }

    bool isRed() const { return _suit==Diamonds || _suit==Hearts; }

    bool isFaceUp() const { return faceup; }

    void flipTo(int x, int y, int steps);

    void turn(bool faceup = true);
    static void enlargeCanvas(QCanvasRectangle *c);

    static const int RTTI = 1001;

    Pile *source() const { return _source; }
    void setSource(Pile *p) { _source = p; }
    const char *name() const { return _name; }
    virtual int rtti() const { return RTTI; }
    virtual void moveBy(double dx, double dy);

protected:
    void draw( QPainter &p );
    void advance(int stage);

private:
    Pile *_source;
    Suits _suit;
    Values _value;
    bool faceup;
    char *_name;

    // used for animations
    int destX, destY;
    int animSteps;
    int flipSteps;
    bool flipping;
    int savedX, savedY;
    double scaleX, scaleY;
    int xOff, yOff;
};

#endif
