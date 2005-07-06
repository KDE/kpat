#include <kdebug.h>
#include "deck.h"
#include "dealer.h"
#include <time.h>
#include <assert.h>

const int NumberOfCards = 52;


Deck *Deck::my_deck = 0;


Deck::Deck( Dealer* parent, int m, int s )
    : Pile( 0, parent ), mult( m )
{
    _deck = new Card * [mult*NumberOfCards];
    Q_CHECK_PTR (_deck);

    // only allow 1, 2, or 4 suits
    if ( s == 1 || s == 2 )
        suits = s;
    else
        suits = 4;

    makedeck();
    addToDeck();
    shuffle();

    setAddFlags(Pile::disallow);
    setRemoveFlags(Pile::disallow);
}


Deck::~Deck()
{
    for (uint i=0; i < mult*NumberOfCards; i++) {
        delete _deck[i];
    }
    m_cards.clear();
    delete [] _deck;
}


// ----------------------------------------------------------------


Deck *Deck::new_deck( Dealer *parent, int m, int s )
{
    my_deck = new Deck(parent, m, s);
    return my_deck;
}


void Deck::makedeck()
{
    int i=0;

    show();
    for ( uint m = 0; m < mult; m++)
    {
        for ( int r = Card::Ace; r <= Card::King; r++)
        {
            for ( int s = Card::Spades-1; s >=  Card::Clubs-1 ; s--)
            {
                _deck[i] = new Card(static_cast<Card::Rank>(r),
                                   static_cast<Card::Suit>(Card::Spades - (s % suits)),
                                   dealer()->canvas());
                _deck[i]->move(x(), y());
                i++;
            }
        }
    }
}


void Deck::collectAndShuffle()
{
    addToDeck();
    shuffle();
}


Card* Deck::nextCard()
{
    CardList::Iterator c;

    c = m_cards.fromLast();  // Dealing from bottom of deck ....
    if ( c != m_cards.end() ) {
        return *c;
    } else
        return 0;
}


// ----------------------------------------------------------------


static long pseudoRandomSeed = 0;

static void pseudoRandom_srand(long seed)
{
    pseudoRandomSeed=seed;
}


// Documented as in
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q28150
// 

static long pseudoRandom_random() {
    pseudoRandomSeed = 214013*pseudoRandomSeed+2531011;
    return (pseudoRandomSeed >> 16) & 0x7fff;
}


// Shuffle deck, assuming all cards are in m_cards

void Deck::shuffle()
{

    assert(m_cards.count() == uint(mult*NumberOfCards));

    assert(dealer()->gameNumber() >= 0);
    pseudoRandom_srand(dealer()->gameNumber());

    kdDebug(11111) << "first card " << m_cards[0]->name() << " " << dealer()->gameNumber() << endl;

    Card* t;
    long z;
    int left = mult*NumberOfCards;
    for (uint i = 0; i < mult*NumberOfCards; i++) {
        z = pseudoRandom_random() % left;
        t = m_cards[z];
        m_cards[z] = m_cards[left-1];
        m_cards[left-1] = t;
        left--;
    }
}


// add cards in deck[] to Deck
// FIXME: Rename to collectCards()

void Deck::addToDeck()
{
    clear();

    for (uint i = 0; i < mult*NumberOfCards; i++) {
        _deck[i]->setTakenDown(false);
        add( _deck[i], true, false );
    }
}


