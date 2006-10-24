/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
*/
#include "simon.h"
#include <klocale.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

Simon::Simon( )
    : DealerScene( )
{
    Deck::create_deck(this);
    Deck::deck()->setPilePos(1, 1);
    Deck::deck()->hide();

    const qreal dist_x = 11.1;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos(1+(i+3)*dist_x, 1);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFlags(Pile::several);
        target[i]->setCheckIndex(0);
        target[i]->setTarget(true);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    for (int i=0; i<10; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->setPilePos(1+dist_x*i, 13.7);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several);
        store[i]->setReservedSpace( QSizeF( 10, 30 ) );
        store[i]->setCheckIndex(1);
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
    }

    Dealer::instance()->setActions(Dealer::Hint | Dealer::Demo);
}

void Simon::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
}

void Simon::deal() {
    int piles = 3;

    for (int round = 0; round < 8; round++)
    {
        for (int j = 0; j < piles; j++)
        {
            Card *c = Deck::deck()->nextCard();
            store[j]->add(c, false);
        }
        piles++;
    }
    assert(Deck::deck()->isEmpty());
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
	kDebug(11111) <<"isGameLost" << endl;
    for (int i=0; i<10; i++) {
		if(store[i]->isEmpty())
			return false;
	kDebug(11111) <<"store["<<i<<"]" << endl;

		Card *c;
		Card *top=store[i]->top();
		int indexi=store[i]->indexOf(top);
		while(--indexi >=0){
		kDebug(11111) <<top->name() << endl;
			c=store[i]->at(indexi);
			if(c->suit() == top->suit() &&
				(top->rank()+1) == c->rank())
				top=c;
			else
				break;
			}

		kDebug(11111) <<"selected: " << top->name() << endl;
		for(int j=1; j <10; j++){
			int k=(i+j) % 10;

			if(store[k]->isEmpty())
				return false;

			kDebug(11111) <<"vs "<<store[k]->top()->name() << endl;
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
    virtual DealerScene *createGame() { return new Simon(); }
} gfi9;

#include "simon.moc"
