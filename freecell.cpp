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

void Freecell::restart()
{
    deck->collectAndShuffle();
    deal();
}

//-------------------------------------------------------------------------//

void Freecell::show()
{
    QWidget::show();

    for (int i = 0; i < 8; i++)
        stack[i]->show();

    for (int i = 0; i < 4; i++)
    {
        freecell[i]->show();
        store[i]->show();
    }
}

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
        }
    }

    deal();
}

//-------------------------------------------------------------------------//

Freecell::~Freecell()
{
    for (int i = 0; i < 8; i++)
        delete stack[i];

    for (int i = 0; i < 4; i++)
    {
        delete freecell[i];
        delete store[i];
    }

    delete deck;
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

bool Freecell::CanRemove (const Pile *p, const Card *c)
{
    // ok if just one card
    if (c == p->top())
        return true;

    return false;
    /* TODO

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    for (const Card *t = c; t->next(); t = t->next())
    {
        if (!((t->Value() == (t->next()->Value()+1))
              && (t->Red() != t->next()->Red())))
        {
            return 0;
        }
    }

    // Now, let's see if there are enough free cells available.
    int numFreeCells = CountFreeCells();
    int numCards = freecell_game->CountCards (c);

    // If the the destination will be a free stack, the number of
    // free cells needs to be greater. (We couldn't count the
    // destination free stack.)
    if (numFreeCells == numCards)
        dont_put_on_free_stack = 1;

    return (numCards <= numFreeCells);
*/
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

bool Freecell::isGameWon() const
{
    for (int c = 0; c < 4; c++)
        if (!freecell[c]->isEmpty())
            return false;

    for (int c = 0; c < 8; c++)
        if (!stack[c]->isEmpty())
            return false;

    return true;
}


static class LocalDealerInfo8 : public DealerInfo
{
public:
    LocalDealerInfo8() : DealerInfo(I18N_NOOP("&Freecell"), 3) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Freecell(parent); }
} gfi;

//-------------------------------------------------------------------------//

#include"freecell.moc"

//-------------------------------------------------------------------------//

