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
#include "clock.h"
#include <klocale.h>
#include "deck.h"
#include "patsolve/clock.h"
#include <assert.h>

Clock::Clock( )
    : DealerScene( )
{
    const qreal dist_x = 11.1;
    const qreal dist_y = 11.1;

    Deck::create_deck(this);
    Deck::deck()->setPos(1, 1+dist_y*3);
    Deck::deck()->hide();

    for (int i=0; i<12; i++) {
        target[i] = new Pile(i+1, this);
        const double ys[12] = {   0./96,  15./96,  52./96, 158./96, 264./96, 301./96, 316./96, 301./96, 264./96, 158./96,  52./96,  15./96};
        const double xs[12] = { 200./72, 280./72, 360./72, 400./72, 360./72, 280./72, 200./72, 120./72, 40./72, 0./72, 40./72, 120./72};
        target[i]->setPilePos(1.2 + 10 * 24 / 5 + xs[i] * 10, 1 + ys[i] * 10);
        target[i]->setCheckIndex(1);
        target[i]->setTarget(true);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(14+i, this);
        store[i]->setPilePos(1.2+dist_x*(i%4), 1 + 10 * 5 / 2 * (i/4));
        store[i]->setAddFlags(Pile::addSpread);
        store[i]->setCheckIndex(0);
        store[i]->setReservedSpace( QSizeF( 10, 18 ) );
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new ClockSolver( this ) );
}

void Clock::restart()
{
    Deck::deck()->collectAndShuffle();
    deal();
}

bool Clock::checkAdd( int ci, const Pile *c1, const CardList& c2) const
{
    Card *newone = c2.first();
    if (ci == 0) {
        if (c1->isEmpty())
            return true;

        return (newone->rank() == c1->top()->rank() - 1);
    } else {
        if (c1->top()->suit() != newone->suit())
            return false;
        if (c1->top()->rank() == Card::King)
            return (newone->rank() == Card::Ace);
        return (newone->rank() == c1->top()->rank() + 1);
    }
}

void Clock::deal() {
    static const Card::Suit suits[12] = { Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs,
					  Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs,
					  Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs, };
    static const Card::Rank ranks[12] = { Card::Nine, Card::Ten, Card::Jack, Card::Queen,
					  Card::King, Card::Two, Card::Three, Card::Four,
					  Card::Five, Card::Six, Card::Seven, Card::Eight};

    int j = 0;
    while (!Deck::deck()->isEmpty()) {
        Card *c = Deck::deck()->nextCard();
        for (int i = 0; i < 12; i++)
            if (c->rank() == ranks[i] && c->suit() == suits[i]) {
                target[i]->add(c, false);
                c = 0;
                break;
            }
        if (c)
            store[j++]->add(c, false);
        if (j == 8)
            j = 0;
    }
}

static class LocalDealerInfo11 : public DealerInfo
{
public:
    LocalDealerInfo11() : DealerInfo(I18N_NOOP("G&randfather's Clock"), 11) {}
    virtual DealerScene *createGame() { return new Clock(); }
} gfi11;

#include "clock.moc"
