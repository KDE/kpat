#include "kings.h"
#include <klocale.h>
#include "pile.h"
#include "deck.h"
#include <assert.h>
#include <freecell-solver/fcs_enums.h>
#include "cardmaps.h"

Kings::Kings( KMainWindow* parent, const char *name )
    : FreecellBase( 2, 8, 8, FCS_ES_FILLED_BY_KINGS_ONLY, true, parent, name )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;

    for (int i=0; i<8; i++) {
        target[i]->move((8 + i/4) * dist_x + 10 + cardMap::CARDX() * 4 / 10, 10 + (i % 4) * cardMap::CARDY() * 14 / 10 );
        store[i]->move(10+dist_x*i, 10 + cardMap::CARDY() * 5 / 4);
        store[i]->setSpread(13);
        freecell[i]->move(10 + dist_x * i, 10);
    }
}

void Kings::demo()
{
    Dealer::demo();
}

void Kings::deal() {
    CardList cards = deck->cards();
    CardList::Iterator it = cards.begin();
    int cn = 0;
    for (int stack = -1; stack < 8; )
    {
        while (it != cards.end() && (*it)->value() != Card::King) {
            if (stack >= 0) {
                store[stack]->add(*it, false, true);
                cn++;
            }
            ++it;
        }
        if (it == cards.end())
            break;
        cn++;
        store[++stack]->add(*it, false, true);
        if (stack == 0) {
            cards = deck->cards(); // reset to start
            it = cards.begin();
        } else
            ++it;
    }
    assert(cn == 104);
}

static class LocalDealerInfo12 : public DealerInfo
{
public:
    LocalDealerInfo12() : DealerInfo(I18N_NOOP("&The Kings"), 12) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Kings(parent); }
} gfdi12;

#include "kings.moc"
