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
#include <kdebug.h>
#include "deck.h"
#include "dealer.h"
#include <time.h>
#include <assert.h>

const unsigned int NumberOfCards = 52;


Deck *Deck::my_deck = 0;


Deck::Deck( DealerScene* parent, int m, int s )
    : Pile( 0, parent ), mult( m )
{
    setObjectName( "deck" );
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

void Deck::create_deck( DealerScene *parent, uint m, uint s )
{
    if ( my_deck && ( m == my_deck->mult && s == my_deck->suits ) )
        return;
    delete my_deck;
    my_deck = new Deck(parent, m, s);
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
                                    static_cast<Card::Suit>(Card::Spades - (s % suits)), dscene() );
                _deck[i]->setPos(0, 0);
                i++;
            }
        }
    }
}

void Deck::update()
{
    for ( uint i = 0; i < mult*NumberOfCards; ++i )
        _deck[i]->setPixmap();
}

void Deck::collectAndShuffle()
{
    addToDeck();
    shuffle();
}


Card* Deck::nextCard()
{
    if (m_cards.isEmpty())
        return 0;

    return m_cards.last();
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
    assert((uint)m_cards.count() == mult*NumberOfCards);

    assert(dscene()->gameNumber() >= 0);
    pseudoRandom_srand(dscene()->gameNumber());

    kDebug(11111) << "first card " << m_cards[0]->name() << " " << dscene()->gameNumber() << endl;

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
        add( _deck[i], true );
        if ( isVisible() )
            _deck[i]->setPos( x(), y() );
        else
            _deck[i]->setPos( 2000, 2000 );
    }
}
