#include "simon.h"
#include <klocale.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

Simon::Simon( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = Deck::new_deck(this);
    deck->move(10, 10);
    deck->hide();

    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(10+(i+3)*dist_x, 10);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFlags(Pile::several);
        target[i]->setCheckIndex(0);
        target[i]->setTarget(true);
    }

    for (int i=0; i<10; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(15+dist_x*i, 10 + cardMap::CARDY() * 73 / 50);
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

bool Simon::checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const
{
    if (checkIndex == 1) {
        if (c1->isEmpty())
            return false;

        return (c1->top()->suit() == c2.first()->suit());
    } else return false; // it's just important to keep this unique
}

bool Simon::checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const
{
    if (checkIndex == 1) {
        if (c1->isEmpty())
            return true;

        return (c1->top()->rank() == c2.first()->rank() + 1);
    } else {
        if (!c1->isEmpty())
            return false;
        return (c2.first()->rank() == Card::King && c2.last()->rank() == Card::Ace);
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

        if (!((c->rank() == (before->rank()-1))
              && (c->suit() == before->suit())))
        {
            return false;
        }
        if (c == p->top())
            return true;
        before = c;
    }

    return true;
}

bool Simon::isGameLost() const
{
	kdDebug(11111) <<"isGameLost" << endl;
    for (int i=0; i<10; i++) {
		if(store[i]->isEmpty())
			return false;
	kdDebug(11111) <<"store["<<i<<"]" << endl;

		Card *c;
		Card *top=store[i]->top();
		int indexi=store[i]->indexOf(top);
		while(--indexi >=0){
		kdDebug(11111) <<top->name() << endl;
			c=store[i]->at(indexi);
			if(c->suit() == top->suit() &&
				(top->rank()+1) == c->rank())
				top=c;
			else
				break;
			}

		kdDebug(11111) <<"selected: " << top->name() << endl;
		for(int j=1; j <10; j++){
			int k=(i+j) % 10;

			if(store[k]->isEmpty())
				return false;

			kdDebug(11111) <<"vs "<<store[k]->top()->name() << endl;
			if((top->rank() +1) == store[k]->top()->rank())
				return false;
			}
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
