#include "yukon.h"
#include <klocale.h>
#include "pile.h"
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

Yukon::Yukon( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;

    deck = new Deck(0, this);
    deck->move(10, 10+dist_y*3);
    deck->hide();

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(20+7*dist_x, 10+dist_y *i);
        target[i]->setType(Pile::KlondikeTarget);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(15+dist_x*i, 10);
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
