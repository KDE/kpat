#ifndef _DECK_H_
#define _DECK_H_

#include "pile.h"
class dealer;

/***************************************

  Deck (Pile with id 0) -- create and shuffle 52 cards

**************************************/
class Deck: public Pile
{

private:
    Deck( Dealer* parent = 0, int m = 1 );
    virtual ~Deck();

public:
    static Deck *new_deck( Dealer *parent = 0, int m = 1 );
    static Deck *deck() { return my_deck; }

    static const long n;

    void collectAndShuffle();

    Card* nextCard();

    uint decksNum() const { return mult; }

private: // functions

    void makedeck();
    void addToDeck();
    void shuffle();

private:

    uint mult;
    Card** _deck;
    static Deck *my_deck;
};

#endif
