#include "golf.h"
#include <klocale.h>
#include "deck.h"
#include <kdebug.h>
#include "cardmaps.h"

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
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int pile_dist = 10 + 3 * cardMap::CARDY();

    deck = Deck::new_deck( this);
    deck->move(10, pile_dist);
    connect(deck, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    for( int r = 0; r < 7; r++ ) {
        stack[r]=new Pile(1+r, this);
        stack[r]->move(10+r*dist_x,10);
        stack[r]->setAddFlags( Pile::addSpread | Pile::disallow);
        stack[r]->setCheckIndex( 1 );
    }

    waste=new HorRightPile(8,this);
    waste->move(10 + cardMap::CARDX() * 5 / 4, pile_dist);
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

    if ((c2->rank() != (c1->top()->rank()+1)) && (c2->rank() != (c1->top()->rank()-1)))
        return false;

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

bool Golf::cardClicked(Card *c)
{
    if (c->source()->checkIndex() !=1) {
        return Dealer::cardClicked(c);
    }

    if (c != c->source()->top())
        return false;

    Pile*p=findTarget(c);
    if (p)
    {
        CardList empty;
        empty.append(c);
        c->source()->moveCards(empty, p);
        canvas()->update();
        return true;
    }
    return false;
}

bool Golf::isGameLost() const
{
    if( !deck->isEmpty())
        return false;

    bool onecard = false;

    for( int r = 0; r < 7; r++ ) {
        if( !stack[r]->isEmpty()){
            onecard = true;
            CardList stackTops;
            stackTops.append(stack[r]->top());
            if(this->checkAdd(0,waste,stackTops))
                return false;
        }
    }

    return onecard;
}


static class LocalDealerInfo13 : public DealerInfo
{
public:
    LocalDealerInfo13() : DealerInfo(I18N_NOOP("Go&lf"), 12) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Golf(parent); }
} ldi13;

//-------------------------------------------------------------------------//

#include"golf.moc"

//-------------------------------------------------------------------------//

