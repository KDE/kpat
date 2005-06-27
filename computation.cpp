/***********************-*-C++-*-********

  computation.h  implements a patience card game

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

//
// This one was discussed on the newsgroup rec.games.abstract
//
****************************************/

#include "computation.h"
#include <klocale.h>
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

Computation::Computation( KMainWindow *parent, const char *name )
    :Dealer( parent, name)
{
    deck = Deck::new_deck(this);
    deck->hide();

    for (int i = 0; i < 4; i++) {
        play[i] = new Pile(1 + i, this);
        play[i]->move(10 + (i+1) * cardMap::CARDX() * 14 / 10, 10 + cardMap::CARDY() * 15 / 10);
        play[i]->setAddFlags(Pile::addSpread);
        play[i]->setCheckIndex(1);

        target[i] = new Pile(5 + i, this);
        target[i]->move(10 + (i+1) * cardMap::CARDX() * 14 / 10, 10);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setCheckIndex(0);
        target[i]->setTarget(true);
    }

    pile = new Pile(13, this);
    pile->setAddFlags(Pile::disallow);
    pile->setRemoveFlags(Pile::autoTurnTop);
    pile->move(10, 10);

    setActions(Dealer::Demo | Dealer::Hint);
}

void Computation::restart() {
    deck->collectAndShuffle();
    deal();
}

void Computation::deal() {
    while (!deck->isEmpty()) {
        Card *c = deck->nextCard();
        pile->add(c, true, false);
    }
    // no animation
    pile->top()->turn(true);
}

inline bool matches(const CardList &cl, Card *start, int offset)
{
    Card *before = start; // maybe 0 for ignore first card
    for (CardList::ConstIterator it = cl.begin(); it != cl.end(); ++it)
    {
        if (before && (*it)->rank() % 13 != (before->rank() + offset) % 13)
            return false;
        before = *it;
    }
    return true;
}

bool Computation::checkStore( const Pile*, const CardList& cl) const
{
    if (cl.count() != 1)
        return false;
    return (cl.first()->source()->index() == 13);
}

bool Computation::checkAdd( int index, const Pile* c1, const CardList& cl) const
{
    if (index == 1)
        return checkStore(c1, cl);

    assert(c1->index() >= 5 && c1->index() <= 8);

    if ( c1->top() && c1->top()->rank() == Card::King) // finished
        return false;

    if ( c1->cardsLeft() == 13 )
      return false;

    int offset = c1->index() - 4;

    if (c1->isEmpty()) {
        Card::Rank start = static_cast<Card::Rank>(Card::Ace + (offset - 1));
        return cl.first()->rank() == start && matches(cl, 0, offset);
    }

    return matches(cl, c1->top(), offset);
}

static class LocalDealerInfo6 : public DealerInfo
{
public:
    LocalDealerInfo6() : DealerInfo(I18N_NOOP("&Calculation"), 6) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Computation(parent); }
} ldi6;

#include "computation.moc"
