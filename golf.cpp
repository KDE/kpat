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
#include "golf.h"
#include <klocale.h>
#include "deck.h"
#include <kdebug.h>
#include "cardmaps.h"

HorRightPile::HorRightPile( int _index, DealerScene* parent)
    : Pile(_index, parent)
{
}

QSizeF HorRightPile::cardOffset( const Card *) const
{
    return QSizeF(1.2, 0);
}

//-------------------------------------------------------------------------//

Golf::Golf( )
    : DealerScene( )
{
    const qreal dist_x = 11.1;

    Deck::create_deck( this);
    Deck::deck()->setPilePos(1, -1);
    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    waste=new HorRightPile(8,this);
    waste->setPilePos(12, -1);
    waste->setTarget(true);
    waste->setCheckIndex( 0 );
    waste->setAddFlags( Pile::addSpread);
    waste->setReservedSpace( QSizeF( 40, 10 ) );
    waste->setObjectName( "waste" );

    for( int r = 0; r < 7; r++ ) {
        stack[r]=new Pile(1+r, this);
        stack[r]->setPilePos(1+r*dist_x,1);
        stack[r]->setAddFlags( Pile::addSpread | Pile::disallow);
        stack[r]->setCheckIndex( 1 );
        stack[r]->setReservedSpace( QSizeF( 10, 20 ) );
        stack[r]->setObjectName( QString( "stack%1" ).arg( r ) );
    }

    Dealer::instance()->setActions(Dealer::Hint | Dealer::Demo);
}

//-------------------------------------------------------------------------//

bool Golf::checkAdd( int checkIndex, const Pile *c1, const CardList& cl) const
{
    if (checkIndex == 1)
        return false;

    Card *c2 = cl.first();

    kDebug(11111)<<"check add "<< c1->objectName()<<" " << c2->objectName() <<" "<<endl;

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
    Deck::deck()->collectAndShuffle();
    deal();
}

void Golf::deckClicked(Card *)
{
    if (Deck::deck()->isEmpty()) {
         return;

    }
    Card *c = Deck::deck()->nextCard();
    waste->add(c, true );
    c->stopAnimation();
    qreal x = c->x();
    qreal y = c->y();
    c->setPos(Deck::deck()->x(), Deck::deck()->y());
    c->flipTo(x, y);
}

//-------------------------------------------------------------------------//

void Golf::deal()
{
    for(int r=0;r<7;r++)
    {
        for(int i=0;i<5;i++)
        {
            stack[r]->add(Deck::deck()->nextCard(),false);
        }
    }
    waste->add(Deck::deck()->nextCard(),false);

}

Card *Golf::demoNewCards()
{
    deckClicked(0);
    return waste->top();
}

bool Golf::cardClicked(Card *c)
{
    if (c->source()->checkIndex() !=1) {
        return DealerScene::cardClicked(c);
    }

    if (c != c->source()->top())
        return false;

    Pile*p=findTarget(c);
    if (p)
    {
        CardList empty;
        empty.append(c);
        c->source()->moveCards(empty, p);
        return true;
    }
    return false;
}

bool Golf::isGameLost() const
{
    if( !Deck::deck()->isEmpty())
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
    virtual DealerScene *createGame() { return new Golf(); }
} ldi13;

//-------------------------------------------------------------------------//

#include "golf.moc"

//-------------------------------------------------------------------------//

