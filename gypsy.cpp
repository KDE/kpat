#include "gypsy.h"
#include <klocale.h>
#include "pile.h"
#include "deck.h"
#include "cardmaps.h"

Gypsy::Gypsy( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;

    deck = new Deck(0, this, 2);
    deck->move(10 + dist_x / 2 + 8*dist_x, 10 + 45 * cardMap::CARDY() / 10);

    connect(deck, SIGNAL(clicked(Card*)), SLOT(slotClicked(Card *)));

    for (int i=0; i<8; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(10+dist_x*(8+(i/4)), 10 + (i%4)*dist_y);
        target[i]->setAddType(Pile::KlondikeTarget);
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(9+i, this);
        store[i]->move(10+dist_x*i, 10);
        store[i]->setAddType(Pile::GypsyStore);
        store[i]->setRemoveType(Pile::FreecellStore);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

void Gypsy::restart() {
    deck->collectAndShuffle();
    deal();
}

void Gypsy::dealRow(bool faceup) {
    for (int round=0; round < 8; round++)
        store[round]->add(deck->nextCard(), !faceup, true);
}

void Gypsy::deal() {
    dealRow(false);
    dealRow(false);
    dealRow(true);
    takeState();
}

Card *Gypsy::demoNewCards()
{
    if (deck->isEmpty())
        return 0;
    dealRow(true);
    return store[0]->top();
}

static class LocalDealerInfo7 : public DealerInfo
{
public:
    LocalDealerInfo7() : DealerInfo(I18N_NOOP("Gy&psy"), 7) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Gypsy(parent); }
} gyfdi;

#include "gypsy.moc"
