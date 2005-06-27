#include "gypsy.h"
#include <klocale.h>
#include "deck.h"
#include "cardmaps.h"

Gypsy::Gypsy( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;

    deck = Deck::new_deck(this, 2);
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

bool Gypsy::isGameLost() const {
	if(!deck->isEmpty())
		return false;

	for(int i=0; i < 8; i++){
		if(store[i]->isEmpty())
			return false;

		if(store[i]->top()->rank() == Card::Ace)
			return false;

		for(int j=0; j <8; j++){
			if(!target[j]->isEmpty() &&
				(store[i]->top()->suit()==target[j]->top()->suit()) &&
				(store[i]->top()->rank()==(target[j]->top()->rank()+1)))
				return false;
		}
	}

	for(int i=0; i < 8; i++) {
            Card *cnext=store[i]->top();
            int indexi=store[i]->indexOf(cnext);

            Card *cardi= 0;
            do{
                cardi=cnext;
                if (indexi>0)
                    cnext=store[i]->at( --indexi );

                for(int k=0; k <8; k++) {
                    if (i == k)
                        continue;

                    if((cardi->rank()+1 == store[k]->top()->rank()) &&
                       cardi->isRed() != store[k]->top()->isRed()){

                        // this test doesn't apply if indexi==0, but fails gracefully.
                        if(cnext->rank() == store[k]->top()->rank() &&
                           cnext->suit() == store[k]->top()->suit())
                            break; //nothing gained; keep looking.

                        return false;// TODO: look deeper, move may not be helpful.
                    }
                }

            } while((indexi>=0) && (cardi->rank()+1 == cnext->rank()) &&
                    (cardi->isRed() != cnext->isRed()));
	}

	return true;
}

static class LocalDealerInfo7 : public DealerInfo
{
public:
    LocalDealerInfo7() : DealerInfo(I18N_NOOP("Gy&psy"), 7) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Gypsy(parent); }
} gyfdi;

#include "gypsy.moc"
