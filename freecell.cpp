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
#include <kdebug.h>

//-------------------------------------------------------------------------//

Freecell::Freecell( KMainWindow* parent, const char* name)
        : Dealer(parent,name)
{
    deck = new Deck(0, this);
    deck->hide();

    for (int i = 0; i < 8; i++) {
        store[i] = new Pile(1 + i, this);
        store[i]->move(8+80*i, 113);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several);
        store[i]->setCheckIndex(0);
    }

    for (int i = 0; i < 4; i++)
    {
        freecell[i] = new Pile (9+i, this);
        freecell[i]->move(8+76*i, 8);
        freecell[i]->setType(Pile::FreeCell);

        target[i] = new Pile(13+i, this);
        target[i]->move(338+76*i, 8);
        target[i]->setType(Pile::KlondikeTarget);
    }

    setActions(Dealer::Demo | Dealer::Hint);
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
        if (store[i]->isEmpty())
            n++;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty())
            n++;

    return n;
}

bool Freecell::CanPutStore(const Pile *c1, const CardList &c2) const
{
    kdDebug() << "CanPutStack " << (void*)c1 << " " << c1->cardsLeft() << " " << c2.first()->name() << " " << (c1->top() ? c1->top()->name() : "<none>") << " " << c1->index() << endl;
    // ok if the target is empty
    if (c1->isEmpty())
        return true;

    Card *c = c2.first(); // we assume there are only valid sequences

    // ok if in sequence, alternate colors
    return ((c1->top()->value() == (c->value()+1))
            && (c1->top()->isRed() != c->isRed()));
}

bool Freecell::checkAdd(int, const Pile *c1, const CardList &c2) const
{
    return CanPutStore(c1, c2);
}

//-------------------------------------------------------------------------//

bool Freecell::checkRemove (int checkIndex, const Pile *p, const Card *c) const
{
    if (checkIndex != 0)
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

        if (!((c->value() == (before->value()-1))
              && (c->isRed() != before->isRed())))
        {
            kdDebug() << c->name() << " - " << before->name() << endl;
            return false;
        }
        if (c == p->top())
            return true;
        before = c;
    }

    return true;
}

//-------------------------------------------------------------------------//

void Freecell::deal()
{
    int column = 0;
    while (!deck->isEmpty())
    {
        store[column]->add (deck->nextCard(), false, true);
        column = (column + 1) % 8;
    }
}

static class LocalDealerInfo3 : public DealerInfo
{
public:
    LocalDealerInfo3() : DealerInfo(I18N_NOOP("&Freecell"), 3) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Freecell(parent); }
} ldi8;

//-------------------------------------------------------------------------//

#include"freecell.moc"

//-------------------------------------------------------------------------//

