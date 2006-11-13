/*
  napoleon.cpp  implements a patience card game

      Copyright (C) 1995  Paul Olav Tvete

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

#include "napoleon.h"
#include <klocale.h>
#include "deck.h"
#include "cardmaps.h"

Napoleon::Napoleon( )
  : DealerScene( )
{
    Deck::create_deck( this );
    connect(Deck::deck(), SIGNAL(clicked(Card *)), SLOT(deal1(Card*)));

    pile = new Pile( 1, this );
    pile->setAddFlags( Pile::disallow );
    pile->setObjectName( "pile" );

    for (int i = 0; i < 4; i++)
    {
        store[i] = new Pile( 2 + i, this );
        store[i]->setCheckIndex( 0 );
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
        target[i] = new Pile( 6 + i, this);
        target[i]->setRemoveFlags( Pile::disallow );
        target[i]->setCheckIndex(2);
        target[i]->setTarget(true);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    const qreal dist_store = 5.5;
    const qreal dist_target = dist_store / 2;
    const qreal centre_x = 1 + 10 + dist_store;
    const qreal centre_y = 1 + 10 + dist_store;

    Deck::deck()->setPilePos( centre_x + 10 * 47 / 10, centre_y + 10 + dist_store);
    pile->setPilePos( centre_x + 10 * 33 / 10, centre_y + 10 + dist_store);

    centre = new Pile( 10, this );
    centre->setRemoveFlags( Pile::disallow );
    centre->setCheckIndex(1);
    centre->setTarget(true);
    centre->setObjectName( "centre" );

    store[0]->setPilePos( centre_x, centre_y - 10 - dist_store );
    store[1]->setPilePos( centre_x + 10 + dist_store, centre_y);
    store[2]->setPilePos( centre_x, centre_y + 10 + dist_store );
    store[3]->setPilePos( centre_x - 10 - dist_store, centre_y);
    target[0]->setPilePos( centre_x - 10 - dist_target, centre_y - 10 - dist_target );
    target[1]->setPilePos( centre_x + 10 + dist_target, centre_y - 10 - dist_target);
    target[2]->setPilePos( centre_x + 10 + dist_target, centre_y + 10 + dist_target);
    target[3]->setPilePos( centre_x - 10 - dist_target, centre_y + 10 + dist_target);
    centre->setPilePos(centre_x, centre_y);

    setActions(DealerScene::Hint | DealerScene::Demo);
}

void Napoleon::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
}

bool Napoleon::CanPutTarget( const Pile* c1, const CardList& cl) const {
    Card *c2 = cl.first();

    if (c1->isEmpty())
        return c2->rank() == Card::Seven;
    else
        return (c2->rank() == c1->top()->rank() + 1);
}

bool Napoleon::CanPutCentre( const Pile* c1, const CardList& cl) const {
    Card *c2 = cl.first();

    if (c1->isEmpty())
        return c2->rank() == Card::Six;

    if (c1->top()->rank() == Card::Ace)
        return (c2->rank() == Card::Six);
    else
        return (c2->rank() == c1->top()->rank() - 1);
}

bool Napoleon::checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const
{
    switch (checkIndex) {
        case 0:
            return c1->isEmpty();
        case 1:
            return CanPutCentre(c1, c2);
        case 2:
            return CanPutTarget(c1, c2);
        default:
            return false;
    }
}

void Napoleon::deal() {
    if (Deck::deck()->isEmpty())
        return;

    for (int i=0; i<4; i++)
        store[i]->add(Deck::deck()->nextCard(), false);
}

void Napoleon::deal1(Card *) {
    Card *c = Deck::deck()->nextCard();
    if (!c)
        return;
    pile->add(c, true);
    c->stopAnimation();
    c->setPos( Deck::deck()->pos() );
    c->flipTo(pile->x(), pile->y());
}

Card *Napoleon::demoNewCards()
{
    if (Deck::deck()->isEmpty())
        return 0;
    deal1(0);
    return pile->top();
}

void Napoleon::getHints() {
    CardList cards;
    for (int i = 0; i < 4; i++)
    {
        if (!store[i]->isEmpty())
            cards.append(store[i]->top());
    }
    if (pile->top())
        cards.append(pile->top());

    for (CardList::Iterator it = cards.begin(); it != cards.end(); ++it) {
        CardList empty;
        empty.append(*it);
        if (CanPutCentre(centre, empty)) {
            newHint(new MoveHint(*it, centre, 0));
            continue;
        }
        for (int i = 0; i < 4; i++) {
            if (CanPutTarget(target[i], empty)) {
                newHint(new MoveHint(*it, target[i], 10));
                break;
            }
        }
    }
    if (pile->isEmpty())
        return;

    for (int i = 0; i < 4; i++) {
        if (store[i]->isEmpty()) {
            newHint(new MoveHint(pile->top(), store[i], 0));
            return;
        }
    }
}

bool Napoleon::isGameLost() const
{
    CardList cards;
    for (int i = 0; i < 4; i++)
    {
        if (store[i]->isEmpty())
            return false;
        else
            cards.append(store[i]->top());
    }

    if (pile->top())
        cards.append(pile->top());

    for (CardList::Iterator it = cards.begin(); it != cards.end(); ++it) {
        CardList empty;
        empty.append(*it);
        if(CanPutCentre(centre,empty)) return false;
        for(int i=0; i<4; i++)
            if(CanPutTarget(target[i],empty)) return false;
    }

    return (Deck::deck()->isEmpty());
}



static class LocalDealerInfo4 : public DealerInfo
{
public:
    LocalDealerInfo4() : DealerInfo(I18N_NOOP("&Napoleon's Tomb"), 4) {}
    virtual DealerScene *createGame() { return new Napoleon(); }
} ldi3;

#include "napoleon.moc"
