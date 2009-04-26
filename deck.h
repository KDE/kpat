/*
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
*/
#ifndef DECK_H
#define DECK_H

#include "pile.h"

/***************************************

  Deck (Pile with id 0) -- create and shuffle 52 cards

**************************************/
class Deck: public Pile
{

private:
    explicit Deck( DealerScene* parent = 0, int m = 1, int s = 4 );
    virtual ~Deck();

public:
    static void create_deck( DealerScene *parent = 0, uint m = 1, uint s = 4 );
    static void destroy_deck();
    static Deck *deck() { return my_deck; }

    static const long n;

    void collectAndShuffle();

    Card* nextCard();

    uint decksNum() const { return mult; }
    uint suitsNum() const { return suits; }
    void update();

private: // functions

    void makedeck();
    void addToDeck();
    void shuffle();

private:

    uint mult;
    uint suits;
    Card** _deck;

    static Deck *my_deck;
};

#endif
