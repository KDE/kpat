#include <kdebug.h>
#include "deck.h"
#include "dealer.h"
#include <time.h>
#include <assert.h>

const int NumberOfCards = 52;

Card* Deck::nextCard() {
    CardList::Iterator c;

    c = myCards.fromLast();  // Dealing from bottom of deck ....
    if ( c != myCards.end() ) {
        return *c;
    } else
        return 0;
}

Deck::Deck( Dealer* parent, int m )
    : Pile( 0, parent ), mult( m )
{
    _deck = new Card * [mult*NumberOfCards];
    Q_CHECK_PTR (_deck);

    makedeck();
    addToDeck();
    shuffle();

    setAddFlags(Pile::disallow);
    setRemoveFlags(Pile::disallow);
}

Deck *Deck::my_deck = 0;

Deck *Deck::new_deck( Dealer *parent, int m )
{
    my_deck = new Deck(parent, m);
    return my_deck;
}

void Deck::makedeck() {
    int i=0;

    show();
    for ( uint m = 0; m < mult; m++)
    {
        for ( int v = Card::Ace; v <= Card::King; v++)
        {
            for ( int s = Card::Clubs; s <=  Card::Spades ; s++)
            {
                _deck[i] = new Card(static_cast<Card::Values>(v),
                                   static_cast<Card::Suits>(s), dealer()->canvas());
                _deck[i]->move(x(), y());
                i++;
            }
        }
    }
}

Deck::~Deck() {
    for (uint i=0; i < mult*NumberOfCards; i++) {
        delete _deck[i];
    }
    myCards.clear();
    delete [] _deck;
}

void Deck::collectAndShuffle() {
    addToDeck();
    shuffle();
}

static long pseudoRandomSeed = 0;

static void pseudoRandom_srand(long seed) {
    pseudoRandomSeed=seed;
}

// documented as in http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q28150
static long pseudoRandom_random() {
    pseudoRandomSeed = 214013*pseudoRandomSeed+2531011;
    return (pseudoRandomSeed >> 16) & 0x7fff;
}

// Shuffle deck, assuming all cards are in myCards
void Deck::shuffle() {

    assert(myCards.count() == uint(mult*NumberOfCards));

    assert(dealer()->gameNumber() >= 0);
    pseudoRandom_srand(dealer()->gameNumber());

    kdDebug(11111) << "first card " << myCards[0]->name() << " " << dealer()->gameNumber() << endl;

    Card* t;
    long z;
    int left = mult*NumberOfCards;
    for (uint i = 0; i < mult*NumberOfCards; i++) {
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

    for (uint i = 0; i < mult*NumberOfCards; i++) {
        _deck[i]->setTakenDown(false);
        add( _deck[i], true, false );
    }
}


