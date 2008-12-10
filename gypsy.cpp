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
#include "gypsy.h"
#include "deck.h"
#include "patsolve/gypsy.h"

#include <klocale.h>

Gypsy::Gypsy( )
    : DealerScene(  )
{
    const qreal dist_x = 11.1;
    const qreal dist_y = 11.1;

    Deck::create_deck(this, 2);
    Deck::deck()->setPilePos(1 + dist_x / 2 + 8*dist_x, 4 * dist_y + 2 );

    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(slotClicked(Card *)));

    for (int i=0; i<8; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos(1 +dist_x*(8+(i/4)), 1 + (i%4)*dist_y);
        target[i]->setAddType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    for (int i=0; i<8; i++) {
        store[i] = new Pile(9+i, this);
        store[i]->setPilePos(1+dist_x*i, 1);
        store[i]->setAddType(Pile::GypsyStore);
        store[i]->setRemoveType(Pile::FreecellStore);
        store[i]->setReservedSpace( QSizeF( 10, 20 ) );
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new GypsySolver( this ) );
}

void Gypsy::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
}

void Gypsy::dealRow(bool faceup) {
    for (int round=0; round < 8; round++)
        store[round]->add(Deck::deck()->nextCard(), !faceup);
}

void Gypsy::deal() {
    dealRow(false);
    dealRow(false);
    dealRow(true);
    takeState();
}

Card *Gypsy::demoNewCards()
{
    if (Deck::deck()->isEmpty())
        return 0;
    dealRow(true);
    return store[0]->top();
}

static class LocalDealerInfo7 : public DealerInfo
{
public:
    LocalDealerInfo7() : DealerInfo(I18N_NOOP("Gypsy"), 7) {}
    virtual DealerScene *createGame() { return new Gypsy(); }
} gyfdi;

#include "gypsy.moc"
