#include "simon.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <assert.h>

Simon::Simon( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = new Deck(0, this);
    deck->move(10, 10);
    deck->hide();

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(250+i*80, 10);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFlags(Pile::several);
        target[i]->setCheckIndex(0);
        target[i]->setTarget(true);
    }

    for (int i=0; i<10; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(15+80*i, 150);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several);
        store[i]->setCheckIndex(1);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

void Simon::restart() {
    deck->collectAndShuffle();
    deal();
}

Card *Simon::demoNewCards()
{
    return 0;
}

void Simon::deal() {
    int piles = 3;

    for (int round = 0; round < 8; round++)
    {
        for (int j = 0; j < piles; j++)
        {
            store[j]->add(deck->nextCard(), false, true);
        }
        piles++;
    }
    assert(deck->isEmpty());
}

bool Simon::checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const
{
    if (checkIndex == 1) {
        if (c1->isEmpty())
            return true;

        return (c1->top()->value() == c2.first()->value() + 1);
    } else {
        if (!c1->isEmpty())
            return false;
        kdDebug() << "checkAdd " << c2.first()->name() << " " << c2.last()->name() << endl;
        return (c2.first()->value() == Card::King && c2.last()->value() == Card::Ace);
    }
}

bool Simon::checkRemove(int checkIndex, const Pile *p, const Card *c) const
{
    if (checkIndex != 1)
        return false;

    // ok if just one card
    if (c == p->top())
        return true;

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    int index = p->indexOf(c) + 1;
    const Card *before = c;
    while (true)
    {
        c = p->at(index++);

        if (!((c->value() == (before->value()-1))
              && (c->suit() == before->suit())))
        {
            kdDebug() << c->name() << " - " << before->name() << endl;
            return false;
        }
        if (c == p->top())
            return true;
        before = c;
    }

    return true;
}

static class LocalDealerInfo9 : public DealerInfo
{
public:
    LocalDealerInfo9() : DealerInfo(I18N_NOOP("&Simple Simon"), 9) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Simon(parent); }
} gfi9;

#include "simon.moc"
