/***********************-*-C++-*-********

  grandf.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete
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

****************************************/

#include <qdialog.h>
#include "grandf.h"
#include <klocale.h>
#include <kdebug.h>
#include "pile.h"
#include "deck.h"
#include <kmainwindow.h>
#include <kaction.h>
#include <qlist.h>

void Grandf::restart() {
    deck->collectAndShuffle();

    deal();
    numberOfDeals = 1;
}

void Grandf::hint() {

    unmarkAll();

    Card* t[7];
    for(int i=0; i<7;i++)
        t[i] = store[i]->top();

    for(int i=0; i<7; i++) {
        CardList list = store[i]->cards();

        for (CardList::ConstIterator it = list.begin(); it != list.end(); ++it)
        {
            if (!(*it)->isFaceUp())
                continue;

            CardList empty;
            empty.append(*it);

            for (int j = 0; j < 7; j++)
            {
                if (i == j)
                    continue;

                if (Sstep1(store[j], empty)) {
                    if (((*it)->value() != Card::King) || it != list.begin()) {
                        mark(*it);
                        break;
                    }
                }
            }
        }

        for (int j = 0; j < 4; j++)
        {
            if (!t[i])
                continue;

            CardList empty;
            empty.append(t[i]);

            if (step1(target[j], empty)) {
                mark(t[i]);
                break;
            }
        }
    }

    canvas()->update();

}

Grandf::Grandf( KMainWindow* parent, const char *name )
    : Dealer( parent, name ), aredeal(0)
{
    deck = new Deck(0, this);
    deck->hide();

    QValueList<int> list;
    for (int i=0; i < 7; i++) list.append(5+i);
    for (int i=0; i < 4; i++) list.append(1+i);

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(110+i*100, 10);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFun(&step1);
        target[i]->setLegalMove(list);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(10+100*i, 150);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setAddFun(&Sstep1);
        store[i]->setLegalMove(list);
    }

    QList<KAction> actions;
    ahint = new KAction( i18n("&Hint"), 0, this,
                         SLOT(hint()),
                         parent->actionCollection(), "game_hint");
    aredeal = new KAction (i18n("&Redeal"), 0, this,
                           SLOT(redeal()),
                           parent->actionCollection(), "game_redeal");
    actions.append(ahint);
    actions.append(aredeal);
    parent->guiFactory()->plugActionList( parent, QString::fromLatin1("game_actions"), actions);

    deal();
    numberOfDeals = 1;
}

void Grandf::redeal() {
    unmarkAll();

    if (numberOfDeals < 3) {
        collect();
        deal();
        numberOfDeals++;
    }
    if (numberOfDeals == 3) {
        aredeal->setEnabled(false);
    }
}

void Grandf::deal() {
    int start = 0;
    int stop = 7-1;
    int dir = 1;

    for (int round=0; round < 7; round++)
    {
        int i = start;
        do
        {
            Card *next = deck->nextCard();
            if (next)
                store[i]->add(next, i != start, true);
            i += dir;
        } while ( i != stop + dir);
        int t = start;
        start = stop;
        stop = t+dir;
        dir = -dir;
    }

    int i = 0;
    Card *next = deck->nextCard();
    while (next)
    {
        store[i+1]->add(next, false , true);
        next = deck->nextCard();
        i = (i+1)%6;
    }

    for (int round=0; round < 7; round++)
    {
        Card *c = store[i]->top();
        if (c && !c->isFaceUp()) {
            c->turn(true);
        }
    }
    aredeal->setEnabled(true);
    canvas()->update();
}

/*****************************

  Does the collecting step of the game

  NOTE: this is not quite correct -- the piles should be turned
  facedown (ie partially reversed) during collection.

******************************/
void Grandf::collect() {
    unmarkAll();

    for (int pos = 6; pos >= 0; pos--) {
        CardList p = store[pos]->cards();
        for (CardList::ConstIterator it = p.begin(); it != p.end(); ++it)
            deck->add(*it, true, false);
    }
}

/**
 * final location
 **/
bool Grandf::step1( const Pile* c1, const CardList& c2 ) {

    if (c2.isEmpty()) {
        return false;
    }
    Card *top = c1->top();

    Card *newone = c2.first();
    if (!top) {
        return (newone->value() == Card::Ace);
    }

    bool t = ((newone->value() == top->value() + 1)
               && (top->suit() == newone->suit()));
    return t;
}

bool Grandf::Sstep1( const Pile* c1, const CardList& c2)
{
    if (c1->isEmpty())
        return c2.first()->value() == Card::King;
    else {
        return (c2.first()->value() == c1->top()->value() - 1)
                     && c1->top()->suit() == c2.first()->suit();
    }
}

void Grandf::cardDblClicked(Card *c)
{
    if (c == c->source()->top() && c->isFaceUp()) {
        CardList empty;
        empty.append(c);

        for (int j = 0; j < 4; j++)
        {
            if (step1(target[j], empty)) {
                c->source()->moveCards(empty, target[j]);
                canvas()->update();
                break;
            }
        }
    }
}

bool Grandf::isGameWon() const
{
    if (!deck->isEmpty())
        return false;

    for (int i = 0; i < 7; i++)
        if (!store[i]->isEmpty())
            return false;

    return true;
}

static class GrandfDealerInfo : public DealerInfo
{
public:
    GrandfDealerInfo() : DealerInfo(I18N_NOOP("&Grandfather"), 1) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Grandf(parent); }
} gfi;

#include "grandf.moc"
