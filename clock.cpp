#include "clock.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <assert.h>

Clock::Clock( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = new Deck(0, this);
    deck->move(10, 10+105*3);
    deck->hide();

    for (int i=0; i<12; i++) {
        target[i] = new Pile(i+1, this);
        const int ys[12] = {   9,  24,  61, 167, 273, 310, 325, 310, 273, 167,  61,  24};
        const int xs[12] = { 540, 620, 700, 740, 700, 620, 540, 460, 380, 340, 380, 460};
        target[i]->move(xs[i], ys[i]);
        target[i]->setCheckIndex(1);
        target[i]->setTarget(true);
        target[i]->setRemoveFlags(Pile::disallow);
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(15+80*(i%4), 10 + 240 * (i/4));
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

        return (newone->value() == c1->top()->value() - 1);
    } else {
        if (c1->top()->suit() != newone->suit())
            return false;
        if (c1->top()->value() == Card::King)
            return (newone->value() == Card::Ace);
        return (newone->value() == c1->top()->value() + 1);
    }
}

void Clock::deal() {
    static const Card::Suits suits[12] = { Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs,
                                           Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs,
                                           Card::Diamonds, Card::Spades, Card::Hearts, Card::Clubs, };
    static const Card::Values values[12] = { Card::Nine, Card::Ten, Card::Jack, Card::Queen,
                                             Card::King, Card::Two, Card::Three, Card::Four,
                                             Card::Five, Card::Six, Card::Seven, Card::Eight};

    int j = 0;
    while (!deck->isEmpty()) {
        Card *c = deck->nextCard();
        for (int i = 0; i < 12; i++)
            if (c->value() == values[i] && c->suit() == suits[i]) {
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
