#include "yukon.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <assert.h>

Yukon::Yukon( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = new Deck(0, this);
    deck->move(10, 10+105*3);
    deck->hide();

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(20+7*80, 10+105 *i);
        target[i]->setType(Pile::KlondikeTarget);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(15+80*i, 10);
        store[i]->setAddType(Pile::KlondikeStore);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

void Yukon::restart() {
    deck->collectAndShuffle();
    deal();
}

void Yukon::deal() {
    for (int round = 0; round < 11; round++)
    {
        for (int j = 0; j < 7; j++)
        {
            bool doit = false;
            switch (j) {
            case 0:
                doit = (round == 0);
                break;
            default:
                doit = (round < j + 5);
            }
            if (doit)
                store[j]->add(deck->nextCard(), round < j && j != 0, true);
        }
    }
}

static class LocalDealerInfo10 : public DealerInfo
{
public:
    LocalDealerInfo10() : DealerInfo(I18N_NOOP("&Yukon"), 10) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Yukon(parent); }
} gfi10;

#include "yukon.moc"
