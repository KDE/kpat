
#include "gypsy.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <kaction.h>
#include <qlist.h>

void Gypsy::restart() {
    deck->collectAndShuffle();
    deal();
}

Gypsy::Gypsy( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = new Deck(0, this, 2);
    deck->move(54 + 8*80, 440);

    connect(deck, SIGNAL(clicked(Card*)), SLOT(slotClicked(Card *)));

    for (int i=0; i<8; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(10+80*(8+(i/4)), 10 + (i%4)*105);
//        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setTarget(true);
        target[i]->setCheckIndex(1);
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(9+i, this);
        store[i]->move(10+80*i, 10);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setCheckIndex(0);
    }

    deal();
}

void Gypsy::dealRow(bool faceup) {
    unmarkAll();
    for (int round=0; round < 8; round++)
    {
        Card *c = deck->nextCard();
        if (!c)
            return;
        store[round]->add(c, !faceup, true);
    }
}

void Gypsy::deal() {
    dealRow(false);
    dealRow(false);
    dealRow(true);

    canvas()->update();
}

/**
 * final location
 **/
bool Gypsy::CanPutTarget( const Pile* c1, const CardList& c2 ) const
{
    Card *top = c1->top();

    Card *newone = c2.first();
    if (!top)
        return (newone->value() == Card::Ace);

    bool t = ((newone->value() == top->value() + 1)
               && (top->suit() == newone->suit()));
    return t;
}

bool Gypsy::CanPutStore( const Pile* c1, const CardList& c2) const
{
    if (c1->isEmpty())
        return true;
    else {
        return (c2.first()->value() == c1->top()->value() - 1)
             && c1->top()->isRed() != c2.first()->isRed();
    }
}

bool Gypsy::checkAdd( int index, const Pile* c1, const CardList& c2) const
{
    if (index == 0)
        return CanPutStore(c1, c2);
    else
        return CanPutTarget(c1, c2);
}

/// freecell
bool Gypsy::checkRemove( int checkIndex, const Pile* p, const Card *c) const
{
    if (checkIndex != 0)
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
              && (c->isRed() != before->isRed())))
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

static class GypsyDealerInfo : public DealerInfo
{
public:
    GypsyDealerInfo() : DealerInfo(I18N_NOOP("G&ypsy"), 7) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Gypsy(parent); }
} gyfdi;

#include "gypsy.moc"
