#include "gypsy.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <kaction.h>
#include <qlist.h>

Gypsy::Gypsy( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = new Deck(0, this, 2);
    deck->move(54 + 8*80, 440);

    connect(deck, SIGNAL(clicked(Card*)), SLOT(slotClicked(Card *)));

    for (int i=0; i<8; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(10+80*(8+(i/4)), 10 + (i%4)*105);
        target[i]->setAddType(Pile::KlondikeTarget);
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(9+i, this);
        store[i]->move(10+80*i, 10);
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
