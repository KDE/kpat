#include "clock.h"
#include <klocale.h>
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

Clock::Clock( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;

    deck = Deck::new_deck(this);
    deck->move(10, 10+dist_y*3);
    deck->hide();

    for (int i=0; i<12; i++) {
        target[i] = new Pile(i+1, this);
        const double ys[12] = {   0./96,  15./96,  52./96, 158./96, 264./96, 301./96, 316./96, 301./96, 264./96, 158./96,  52./96,  15./96};
        const double xs[12] = { 200./72, 280./72, 360./72, 400./72, 360./72, 280./72, 200./72, 120./72, 40./72, 0./72, 40./72, 120./72};
        target[i]->move(15 + cardMap::CARDX() * 24 / 5 + xs[i] * cardMap::CARDX(), 10 + ys[i] * cardMap::CARDY());
        target[i]->setCheckIndex(1);
        target[i]->setTarget(true);
        target[i]->setRemoveFlags(Pile::disallow);
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(14+i, this);
        store[i]->move(15+dist_x*(i%4), 10 + cardMap::CARDY() * 5 / 2 * (i/4));
        store[i]->setAddFlags(Pile::addSpread);
        store[i]->setCheckIndex(0);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

void Clock::restart()
{
    deck->collectAndShuffle();
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
    while (!deck->isEmpty()) {
        Card *c = deck->nextCard();
        for (int i = 0; i < 12; i++)
            if (c->rank() == ranks[i] && c->suit() == suits[i]) {
                target[i]->add(c, false, true);
                c = 0;
                break;
            }
        if (c)
            store[j++]->add(c, false, true);
        if (j == 8)
            j = 0;
    }
}

static class LocalDealerInfo11 : public DealerInfo
{
public:
    LocalDealerInfo11() : DealerInfo(I18N_NOOP("G&randfather's Clock"), 11) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Clock(parent); }
} gfi11;

#include "clock.moc"
