#include "pile.h"
#include <kdebug.h>
#include "card.h"
#include "krandomsequence.h"
#include "deck.h"
#include "dealer.h"

const int NumberOfCards = 52;

Card* Deck::nextCard() {
    CardList::Iterator c;

    c = myCards.fromLast();  // Dealing from bottom of deck ....
    if ( c != myCards.end() ) {
        Card *card = *c;
        myCards.remove(c);
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
    shuffle();
    addToDeck();

    setAddFlags(Pile::disallow);
    setRemoveFlags(Pile::disallow);
}

void Deck::makedeck() {
    int i=0;

    show();
    for ( int s = Card::Clubs; s <=  Card::Spades ; s++)
    {
        for ( int v = Card::Ace; v <= Card::King; v++)
        {
            for ( int m = 0; m < mult; m++)
            {
                deck[i] = new Card(static_cast<Card::Values>(v),
                                   static_cast<Card::Suits>(s), dealer()->canvas());
                deck[i]->setSource(this);
                deck[i]->move(x(), y());
                i++;
            }
        }
    }
}

Deck::~Deck() {
    for (int i=0; i < mult*NumberOfCards; i++) {
        deck[i]->setSource(0);
        delete deck[i];
    }
    myCards.clear();
    delete [] deck;
}

void Deck::collectAndShuffle() {
    shuffle();
    addToDeck();
}

// Shuffle deck, assuming all cards are in deck[]
void Deck::shuffle() {
    //  Something is rotten...
    KRandomSequence R(0);

    Card* t;
    long z;
    for (int i = mult*NumberOfCards-1; i >= 1; i--) {
        z = R.getLong(i);
        t = deck[z];
        deck[z] = deck[i];
        deck[i] = t;
    }
}

// add cards in deck[] to Deck
void Deck::addToDeck() {
    for (int i = 0; i < mult*NumberOfCards; i++)
        add( deck[i], true, false );
}

// #include "deck.moc"

