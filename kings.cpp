#include "kings.h"
#include <klocale.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "freecell-solver/fcs_enums.h"
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
        while (it != cards.end() && (*it)->rank() != Card::King) {
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

bool Kings::isGameLost() const {
	int i,indexi;
	Card *c,*cnext,*ctarget;
	CardList targets,ctops;

	for(i=0; i < 8; i++){
		if(freecell[i]->isEmpty())
			return false;
		if(store[i]->isEmpty())
			return false;
		if(store[i]->top()->rank() == Card::Ace)
			return false;
		}

	for(i=0; i < 8; i++){
		if(!target[i]->isEmpty())
			targets.append(target[i]->top());

		if(!store[i]->isEmpty())
			ctops.append(store[i]->top());
		} 

	for(i=0; i < 8; i++){
		if(store[i]->isEmpty())
			continue;

		c=store[i]->top();
		for (CardList::Iterator it = targets.begin(); it != targets.end(); ++it) {
			ctarget=*it;
			if(c->rank()-1 == ctarget->rank() &&
				c->suit() == ctarget->suit()){
				kdDebug(11111)<< "test 1" << endl;
				return false;
			}
		}
		
		for(indexi=store[i]->indexOf(store[i]->top()); indexi>=0;indexi--){
			c=store[i]->at(indexi);
			if(indexi > 0)
				cnext=store[i]->at(indexi-1);

			for (CardList::Iterator it = ctops.begin(); it != ctops.end(); ++it) {
				ctarget=*it;
				if(c->rank()+1 == ctarget->rank() &&
					c->isRed() != ctarget->isRed()){

					if(indexi == 0)
						return false;

					if(cnext->rank() != ctarget->rank()
					   || cnext->suit() != ctarget->suit())
						return false;
				}
			}
			if(cnext->rank() != c->rank()+1 && 
				cnext->isRed() != c->isRed())
				break;
		}
	}

	return true;
}

#if 0
NOTE: When this is reenabled, renumber the following patiences back again:
Golf
Klondike, draw 3
Spider Easy
Spider Medium
Spider Hard

static class LocalDealerInfo12 : public DealerInfo
{
public:
    LocalDealerInfo12() : DealerInfo(I18N_NOOP("&The Kings"), 12) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Kings(parent); }
} gfdi12;
#endif

#include "kings.moc"
