/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright (C) 1997  Rodolfo Borges
               (C) 2000  Stephan Kulow

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

class FreecellPile : public Pile
{
public:
    FreecellPile(int _index, Dealer* parent = 0) : Pile(_index, parent) {}
    virtual void moveCards(CardList &c, Pile *to);
};

void FreecellPile::moveCards(CardList &c, Pile *to)
{
    if (c.count() == 1) {
        Pile::moveCards(c, to);
        return;
    }
    dynamic_cast<Freecell*>(dealer())->moveCards(c, this, to);
}

//-------------------------------------------------------------------------//

Freecell::Freecell( KMainWindow* parent, const char* name)
        : Dealer(parent,name)
{
    deck = new Deck(0, this);
    deck->hide();

    for (int i = 0; i < 8; i++) {
        store[i] = new FreecellPile(1 + i, this);
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
        target[i]->setRemoveFlags(Pile::Default);
    }

    setActions(Dealer::Demo | Dealer::Hint);
}

//-------------------------------------------------------------------------//

void Freecell::restart()
{
    deck->collectAndShuffle();
    deal();
}

void Freecell::countFreeCells(int &free_cells, int &free_stores) const
{
    free_cells = 0;
    free_stores = 0;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty()) free_cells++;
    for (int i = 0; i < 8; i++)
        if (store[i]->isEmpty()) free_stores++;
}

void Freecell::moveCards(CardList &c, FreecellPile *from, Pile *to)
{
    assert(c.count() > 1);

    from->moveCardsBack(c);
    waitfor = c.first();
    connect(waitfor, SIGNAL(stoped(Card*)), SLOT(waitForMoving(Card*)));

    PileList fcs;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty()) fcs.append(freecell[i]);

    PileList fss;

    for (int i = 0; i < 8; i++)
        if (store[i]->isEmpty() && to != store[i]) fss.append(store[i]);

    uint free_stores_needed = (c.count() - 1) / (fcs.count() + 1);
    kdDebug() << "free stores needed " << free_stores_needed << endl;
    assert(free_stores_needed <= fss.count());

    while (moves.count()) { delete moves.first(); moves.remove(moves.begin()); }
    uint already = 0;
    if (free_stores_needed > 0) {
        for (uint i = 0; i < free_stores_needed; ++i) {
            already += movePileToPile(c, fss[i], fcs, already);
        }
        already += movePileToPile(c, to, fcs, already);
        kdDebug() << "already moved " << already << endl;
        assert(already == c.count());
        kdDebug() << "real main card " << c[0]->name() << endl;
//        moves.append(new MoveHint(c[0], to));
        already = 0;
        for (uint i = 0; i < free_stores_needed; ++i) {
            already = movePileToPile(c, to, fcs, already);
        }
    } else
        movePileToPile(c, to, fcs, 0);
}

int Freecell::movePileToPile(CardList &c, Pile *to, PileList &fcs, int start)
{
    kdDebug() << "movePileToPile " << c.count() <<  " " << fcs.count() << " " << start << endl;
    uint moving = QMIN(c.count() - start, fcs.count() + 1);

    for (uint i = 0; i < moving - 1; i++) {
        kdDebug() << "moving " << c[c.count() - i - 1 - start]->name() << endl;
        moves.append(new MoveHint(c[c.count() - i - 1 - start], fcs[i]));
    }
    kdDebug() << "main card " << c[c.count() - start - 1 - (moving - 1)]->name() << endl;
    moves.append(new MoveHint(c[c.count() - start - 1 - (moving - 1)], to));

    for (int i = moving - 2; i >= 0; --i) {
        kdDebug() << "moving back " << c[c.count() - i - 1 - start]->name() << endl;
        moves.append(new MoveHint(c[c.count() - i - 1 - start], to));
    }
    return moving;
}

void Freecell::startMoving()
{
    if (moves.isEmpty()) {
        takeState();
        return;
    }

    MoveHint *mh = moves.first();
    kdDebug() << "startMoving " << mh->card()->name() << " " << moves.count() << " left\n";
    moves.remove(moves.begin());
    CardList empty;
    empty.append(mh->card());
    mh->pile()->add(mh->card());
    mh->pile()->moveCardsBack(empty, true);
    waitfor = mh->card();
    connect(mh->card(), SIGNAL(stoped(Card*)), SLOT(waitForMoving(Card*)));
    delete mh;
}

void Freecell::waitForMoving(Card *c)
{
    if (waitfor != c)
        return;
    startMoving();
}

bool Freecell::CanPutStore(const Pile *c1, const CardList &c2) const
{
    int n, m;
    countFreeCells(n, m);

    if (c1->isEmpty()) // destination is empty
        m--;

    if (int(c2.count()) > (n+1) * (m+1))
        return false;

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

bool Freecell::checkRemove(int checkIndex, const Pile *p, const Card *c) const
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

