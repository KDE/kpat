#include <qdialog.h>
#include "kings.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <kaction.h>
#include <qlist.h>
#include <assert.h>
#include <freecell-solver/fcs_enums.h>

Kings::Kings( KMainWindow* parent, const char *name )
    : FreecellBase( 2, 8, 8, FCS_ES_FILLED_BY_KINGS_ONLY, true, parent, name )
{
    for (int i=0; i<8; i++) {
        target[i]->move((8 + i/4) * 80 + 40, 10 + (i % 4) * 130);
        store[i]->move(10+80*i, 130);
        store[i]->setSpread(13);
        freecell[i]->move(10 + 80 * i, 10);
    }
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
