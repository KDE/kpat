#include "pile.h"
#include <kdebug.h>
#include "card.h"
#include "deck.h"
#include "dealer.h"
#include <time.h>
#include <assert.h>

const int NumberOfCards = 52;

Card* Deck::nextCard() {
    CardList::Iterator c;

    c = myCards.fromLast();  // Dealing from bottom of deck ....
    if ( c != myCards.end() ) {
        Card *card = *c;
        myCards.remove(c);
        card->setSource(0);
        return card;
    } else
        return 0;
}

Deck::Deck( int index, Dealer* parent, int m )
    : Pile( index, parent ), mult( m )
{
    deck = new Card * [mult*NumberOfCards];
    CHECK_PTR (deck);

    makedeck();
    addToDeck();
    shuffle();

    setAddFlags(Pile::disallow);
    setRemoveFlags(Pile::disallow);
}

void Deck::makedeck() {
    int i=0;

    show();
    for ( int m = 0; m < mult; m++)
    {
        for ( int v = Card::Ace; v <= Card::King; v++)
        {
            for ( int s = Card::Clubs; s <=  Card::Spades ; s++)
            {
                deck[i] = new Card(static_cast<Card::Values>(v),
                                   static_cast<Card::Suits>(s), dealer()->canvas());
                deck[i]->move(x(), y());
                i++;
            }
        }
    }
}

Deck::~Deck() {
    for (int i=0; i < mult*NumberOfCards; i++) {
        delete deck[i];
    }
    myCards.clear();
    delete [] deck;
}

void Deck::collectAndShuffle() {
    addToDeck();
    shuffle();
}

static long pseudoRandomSeed = 0;

static void pseudoRandom_srand(long seed) {
    pseudoRandomSeed=seed;
}

static long pseudoRandom_random() {
    pseudoRandomSeed = 214013*pseudoRandomSeed+2531011;
    return (pseudoRandomSeed >> 16) & 0x7fff;
}

// Shuffle deck, assuming all cards are in myCards
void Deck::shuffle() {

    assert(myCards.count() == uint(mult*NumberOfCards));

    assert(dealer()->gameNumber() >= 0);
    pseudoRandom_srand(dealer()->gameNumber());

    kdDebug() << "first card " << myCards[0]->name() << " " << dealer()->gameNumber() << endl;

    Card* t;
    long z;
    int left = mult*NumberOfCards;
    for (int i = 0; i < mult*NumberOfCards; i++) {
        z = pseudoRandom_random() % left;
        t = myCards[z];
        myCards[z] = myCards[left-1];
        myCards[left-1] = t;
        left--;
    }
}

// add cards in deck[] to Deck
void Deck::addToDeck() {
    clear();

    for (int i = 0; i < mult*NumberOfCards; i++)
        add( deck[i], true, false );
}


