#include "golf.h"
#include <klocale.h>
#include "deck.h"
#include "pile.h"
#include <kdebug.h>

HorRightPile::HorRightPile( int _index, Dealer* parent)
    : Pile(_index, parent)
{
}

QSize HorRightPile::cardOffset( bool _spread, bool, const Card *) const
{
    if (_spread)
        return QSize(+hspread(), 0);

    return QSize(0, 0);
}

//-------------------------------------------------------------------------//

Golf::Golf( KMainWindow* parent, const char* _name)
        : Dealer( parent, _name )
{
    deck = new Deck( 0, this);
    deck->move(30, 300);
    connect(deck, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    for( int r = 0; r < 7; r++ ) {
        stack[r]=new Pile(1+r, this);
        stack[r]->move(30+r*90,10);
        stack[r]->setAddFlags( Pile::addSpread | Pile::disallow);
        stack[r]->setCheckIndex( 1 );
    }

    waste=new HorRightPile(8,this);
    waste->move(120,300);
    waste->setTarget(true);
    waste->setCheckIndex( 0 );
    waste->setAddFlags( Pile::addSpread);

    setActions(Dealer::Hint | Dealer::Demo);
}

//-------------------------------------------------------------------------//

bool Golf::checkAdd( int checkIndex, const Pile *c1, const CardList& cl) const
{
    if (checkIndex == 1)
        return false;

    Card *c2 = cl.first();

    kdDebug(11111)<<"check add "<< c1->name()<<" " << c2->name() <<" "<<endl;

    if ((c2->value() != (c1->top()->value()+1)) && (c2->value() != (c1->top()->value()-1)))
        return false;


    return true;

}

bool Golf::isGameLost() const
{
	if( !deck->isEmpty())
		return false;

	for( int r = 0; r < 7; r++ ) {
		if( !stack[r]->isEmpty()){
			CardList stackTops;
			stackTops.append(stack[r]->top());
			if(this->checkAdd(0,waste,stackTops))
				return false;
			}
		}
			
    return true;
}

bool Golf::checkRemove( int checkIndex, const Pile *, const Card *c2) const
{
    if (checkIndex == 0)
        return false;
    return (c2 ==  c2->source()->top());
}

//-------------------------------------------------------------------------//

void Golf::restart()
{
    deck->collectAndShuffle();
    deal();
}

void Golf::deckClicked(Card *)
{
    if (deck->isEmpty()) {
         return;

    }
    Card *c = deck->nextCard();
    waste->add(c, true, true);
    int x = int(c->x());
    int y = int(c->y());
    c->move(deck->x(), deck->y());
    c->flipTo(x, y, 8);
}

//-------------------------------------------------------------------------//

void Golf::deal()
{
    for(int r=0;r<7;r++)
    {
        for(int i=0;i<5;i++)
        {
            stack[r]->add(deck->nextCard(),false,true);
        }
    }
    waste->add(deck->nextCard(),false,false);

}

Card *Golf::demoNewCards()
{
    deckClicked(0);
    return waste->top();
}

void Golf::cardClicked(Card *c)
{
    if (c->source()->checkIndex() !=1) {
        Dealer::cardClicked(c);
        return;
    }

    if (c != c->source()->top())
        return;

    Pile*p=findTarget(c);
    if(p)
    {
        CardList empty;
        empty.append(c);
        c->source()->moveCards(empty, p);
        canvas()->update();
    }
}


static class LocalDealerInfo13 : public DealerInfo
{
public:
    LocalDealerInfo13() : DealerInfo(I18N_NOOP("G&olf"), 13) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Golf(parent); }
} ldi13;

//-------------------------------------------------------------------------//

#include"golf.moc"

//-------------------------------------------------------------------------//

