#include "fortyeight.h"
#include <klocale.h>
#include <kdebug.h>
#include "deck.h"
#include "pile.h"
#include <assert.h>
#include "cardmaps.h"

HorLeftPile::HorLeftPile( int _index, Dealer* parent)
    : Pile(_index, parent)
{
}

QSize HorLeftPile::cardOffset( bool _spread, bool, const Card *) const
{
    if (_spread)
        return QSize(-hspread(), 0);

    return QSize(0, 0);
}

Fortyeight::Fortyeight( KMainWindow* parent, const char* name)
        : Dealer(parent,name)
{
    deck = new Deck(0, this, 2);

    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;

    connect(deck, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    deck->move(10 + cardMap::CARDX() * 82 / 10, 10 + cardMap::CARDX() * 56 / 10);
    deck->setZ(20);

    pile = new HorLeftPile(13, this);
    pile->setAddFlags(Pile::addSpread | Pile::disallow);
    pile->move(10 + cardMap::CARDX() * 69 / 10, 10 + cardMap::CARDX() * 56 / 10 );

    for (int i = 0; i < 8; i++) {

        target[i] = new Pile(9 + i, this);
        target[i]->move(8+dist_x*i, 10);
        target[i]->setType(Pile::KlondikeTarget);

        stack[i] = new Pile(1 + i, this);
        stack[i]->move(8+dist_x*i, 10 + dist_y);
        stack[i]->setAddFlags(Pile::addSpread);
        stack[i]->setRemoveFlags(Pile::autoTurnTop);
        stack[i]->setCheckIndex(1);
        stack[i]->setSpread(stack[i]->spread() * 3 / 4);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

//-------------------------------------------------------------------------//

void Fortyeight::restart()
{
    lastdeal = false;
    deck->collectAndShuffle();
    deal();
}

void Fortyeight::deckClicked(Card *)
{
    if (deck->isEmpty()) {
        if (lastdeal)
            return;
        lastdeal = true;
        while (!pile->isEmpty())
            deck->add(pile->at(pile->cardsLeft()-1), true, false);
    }
    Card *c = deck->nextCard();
    pile->add(c, true, true);
    int x = int(c->x());
    int y = int(c->y());
    c->move(deck->x(), deck->y());
    c->flipTo(x, y, 8);
}

Card *Fortyeight::demoNewCards()
{
    if (deck->isEmpty() && lastdeal)
        return 0;
    deckClicked(0);
    return pile->top();
}

bool Fortyeight::checkAdd(int, const Pile *c1, const CardList &c2) const
{
    if (c1->isEmpty())
        return true;

    // ok if in sequence, same suit
    return (c1->top()->suit() == c2.first()->suit())
       && (c1->top()->value() == (c2.first()->value()+1));
}

void Fortyeight::deal()
{
    for (int r = 0; r < 4; r++)
    {
        for (int column = 0; column < 8; column++)
        {
            if (false) { // doesn't look
                stack[column]->add(deck->nextCard(), true, true);
                stack[column]->top()->turn(true);
            } else {
                stack[column]->add(deck->nextCard(), false, true);
            }
        }
    }
    pile->add(deck->nextCard(), false, false);
}

void Fortyeight::getGameState( QDataStream & stream )
{
    stream << (Q_INT8)lastdeal;
}

void Fortyeight::setGameState( QDataStream & stream )
{
    Q_INT8 i;
    stream >> i;
    lastdeal = i;
}

bool Fortyeight::isGameLost() const{
   kdDebug(11111) << "isGameLost ?" << endl;
   if(!lastdeal)
	return false;
   if(!deck->isEmpty())
	return false;

   Card *c;
   for(int i=0; i < 8; i++){
	if(stack[i]->isEmpty())
		return false;
	c=stack[i]->top();	

	if(c->value() == Card::Ace)
		return false;
	if(!pile->isEmpty())
		if(pile->top()->suit() == c->suit() &&
			pile->top()->value()-1 == c->value())
			return false;
	for(int j=0; j <8;j++){
		if(target[j]->isEmpty())
			continue;
		if(c->suit() == target[j]->top()->suit() &&
			c->value()-1 ==target[j]->top()->value())
			return false;
		}
	for(int j=1; j < 8; j++){
		int k=(i+j) % 8;
		if(c->suit() == stack[k]->top()->suit() &&
			c->value()+1 ==stack[k]->top()->value()){
			int indexi=stack[i]->indexOf(c);
			if(indexi==0)	
				return false;
			Card *c2=stack[i]->at(indexi-1);
			if(c2->value()!=stack[k]->top()->value() ||
				c2->suit()!=stack[k]->top()->suit())
				return false;
			}
		}
	}

   return true;
}

static class LocalDealerInfo8 : public DealerInfo
{
public:
    LocalDealerInfo8() : DealerInfo(I18N_NOOP("F&orty and Eight"), 8) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Fortyeight(parent); }
} ldi9;

//-------------------------------------------------------------------------//

#include "fortyeight.moc"

//-------------------------------------------------------------------------//

