/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright (C) 1997  Rodolfo Borges

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

---------------------------------------------------------------------------*/

#include <qdialog.h>
#include "freecell.h"
#include <klocale.h>
#include "deck.h"
#include "pile.h"
#include <assert.h>

//-------------------------------------------------------------------------//

Freecell::Freecell( KMainWindow* parent, const char* name)
        : Dealer(parent,name)
{
    deck = new Deck(0, this);
    deck->hide();

    for (int r = 0; r < 4; r++)
    {
        for (int i = 0; i < 8; i++) {
            stack[i] = new Pile(1 + i, this);
            stack[i]->move(8+80*i, 113);
            stack[i]->setAddFlags(Pile::addSpread | Pile::several);
            stack[i]->setRemoveFlags(Pile::several);
            stack[i]->setAddFun(&CanPutStack);
            stack[i]->setRemoveFun(&CanRemove);
        }

        for (int i = 0; i < 4; i++)
        {
            freecell[i] = new Pile (9+i, this);
            freecell[i]->move(8+76*i, 8);
            freecell[i]->setAddFun (&CanPutFreeCell);

            store[i] = new Pile(13+i, this);
            store[i]->move(338+76*i, 8);
            store[i]->setRemoveFlags(Pile::disallow);
            store[i]->setAddFun(&CanPutStore);
            store[i]->setTarget(true);
        }
    }

    deal();
}

//-------------------------------------------------------------------------//

void Freecell::restart()
{
    deck->collectAndShuffle();
    deal();
}

//-------------------------------------------------------------------------//

int Freecell::CountFreeCells()
{
    int n = 0;

    for (int i = 0; i < 8; i++)
        if (stack[i]->isEmpty())
            n++;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty())
            n++;

    return n;
}

//-------------------------------------------------------------------------//

bool Freecell::CanPutStore(const Pile *c1, const CardList &c2)
{
    assert(c2.count() == 1);
    Card *c = c2.first();

    // only aces in empty spaces
    if (c1->isEmpty())
        return (c->value() == Card::Ace);

    // ok if in sequence, same suit
    return (c1->top()->suit() == c->suit())
          && ((c1->top()->value()+1) == c->value());
}

bool Freecell::CanPutFreeCell(const Pile *c1, const CardList &)
{
    return (c1->isEmpty());
}

bool Freecell::CanPutStack(const Pile *c1, const CardList &c2)
{
    // ok if the target is empty
    if (c1->isEmpty())
        return true;

    Card *c = c2.first(); // we assume there are only valid sequences

    // ok if in sequence, alternate colors
    return ((c1->top()->value() == (c->value()+1))
            && (c1->top()->isRed() != c->isRed()));
}

//-------------------------------------------------------------------------//

bool Freecell::CanRemove (const Pile *, const Card *)
{
// see gypsy
    return true;
}

//-------------------------------------------------------------------------//

void Freecell::deal()
{
    int column = 0;
    while (!deck->isEmpty())
    {
        stack[column]->add (deck->nextCard(), false, true);
        column = (column + 1) % 8;
    }
}

static class LocalDealerInfo8 : public DealerInfo
{
public:
    LocalDealerInfo8() : DealerInfo(I18N_NOOP("&Freecell"), 3) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Freecell(parent); }
} ldi8;

//-------------------------------------------------------------------------//

#include"freecell.moc"

//-------------------------------------------------------------------------//

