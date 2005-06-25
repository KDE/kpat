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

#include "grandf.h"
#include <klocale.h>
#include "deck.h"
#include <kaction.h>
#include <assert.h>
#include "cardmaps.h"

Grandf::Grandf( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = Deck::new_deck(this);
    deck->hide();

    const int distx = cardMap::CARDX() * 14 / 10;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->move(10+(i+1)*distx, 10);
        target[i]->setType(Pile::KlondikeTarget);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->move(10+distx*i, 10 + cardMap::CARDY() * 15 / 10);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setCheckIndex(1);
    }

    setActions(Dealer::Hint | Dealer::Demo | Dealer::Redeal);
}

void Grandf::restart() {
    deck->collectAndShuffle();
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
    takeState();
}

Card *Grandf::demoNewCards()
{
    if (numberOfDeals < 3) {
        redeal();
        return store[3]->top();
    } else
        return 0;
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
        Card *c = store[round]->top();
        if (c)
            c->turn(true);
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

bool Grandf::checkAdd( int checkIndex, const Pile *c1, const CardList& c2) const {
    assert (checkIndex == 1);
    if (c1->isEmpty())
        return c2.first()->rank() == Card::King;
    else
        return (c2.first()->rank() == c1->top()->rank() - 1)
               && c2.first()->suit() == c1->top()->suit();
}

QString Grandf::getGameState() const
{
    return QString::number(numberOfDeals);
}

void Grandf::setGameState( const QString &s)
{
    numberOfDeals = s.toInt();
    aredeal->setEnabled(numberOfDeals < 3);
}

bool Grandf::isGameLost() const
{
    // If we can redeal, then nothing's lost yet.
    if (numberOfDeals <3)
        return false;

    // Work through the stores, look for killer criteria.
    for(int i=0; i < 7; i++) {

        /* If this store is empty, then iterate through the other stores and
         * check if there is a (visible) King card. If so, then we could move
         * that to the free store (which means a turn is possible, so the
         * game is not lost yet).
         */
        if(store[i]->isEmpty()){
            for(int i2=1; i2 < 7; i2++) {
                int j=(i+i2) % 7;
                CardList p = store[j]->cards();
                for (CardList::ConstIterator it = p.begin(); it != p.end(); ++it){
                    Card *c= *it;
                    if( it != p.begin() && c->realFace() && c->rank() == Card::King)
                        return false;
                }
            }
        }
        else{
            /* If this store has an Ace as it's top card, then we can start a
             * new target pile!
             */
            if(store[i]->top()->rank() == Card::Ace)
                return false;

            /* Check whether the top card of this store could be added to
             * any of the target piles.
             */
            for(int j=0; j <4; j++)
                if( !target[j]->isEmpty())
                    if(store[i]->top()->suit() == target[j]->top()->suit())
                        if( store[i]->top()->rank() == target[j]->top()->rank() +1)
                            return false;

            /* Check whether any (group of) cards from another store could
             * be put onto this store's top card.
             */
            for(int i2=1; i2 < 7; i2++) {
                int j=(i+i2) % 7;
                CardList p = store[j]->cards();
                for (CardList::ConstIterator it = p.begin(); it != p.end(); ++it){
                    Card *c= *it;
                    if( c->realFace() &&
                        c->rank() == (store[i]->top()->rank()-1) &&
                        c->suit() == store[i]->top()->suit() )
                        return false;
                }
            }
        }
    }
    return true; // can't move.
}

static class LocalDealerInfo1 : public DealerInfo
{
public:
    LocalDealerInfo1() : DealerInfo(I18N_NOOP("&Grandfather"), 1) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Grandf(parent); }
} gfdi;

#include "grandf.moc"
