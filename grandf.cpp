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
#include "pile.h"
#include "deck.h"
#include <kaction.h>
#include <assert.h>
#include "cardmaps.h"

Grandf::Grandf( KMainWindow* parent, const char *name )
    : Dealer( parent, name )
{
    deck = new Deck(0, this);
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
        return c2.first()->value() == Card::King;
    else
        return (c2.first()->value() == c1->top()->value() - 1)
               && c2.first()->suit() == c1->top()->suit();
}

void Grandf::getGameState( QDataStream & stream )
{
    stream << (Q_INT8)numberOfDeals;
}

void Grandf::setGameState( QDataStream & stream )
{
    Q_INT8 i;
    stream >> i;
    numberOfDeals = i;
    aredeal->setEnabled(numberOfDeals < 3);
}

bool Grandf::isGameLost() const
{
    int i,i2,j;

    if(numberOfDeals <3)
        return false;

    for(i=0; i < 7; i++) { // check each store

        if(store[i]->isEmpty()){ //look for a face up king
            for(i2=1; i2 < 7; i2++) { // check the other stores.
                j=(i+i2) % 7;
                CardList p = store[j]->cards();
                for (CardList::ConstIterator it = p.begin(); it != p.end(); ++it){
                    Card *c= *it;
                    if( it != p.begin() && c->isFaceUp() && c->value() == Card::King)
                        return false;
                }
            }
        }
        else{
            //can we start a target pile ?
            if(store[i]->top()->value() == Card::Ace)
                return false;

            // can we add to a target ?
            for(j=0; j <4; j++)
                if( !target[j]->isEmpty())
                    if(store[i]->top()->suit() == target[j]->top()->suit())
                        if( store[i]->top()->value() == target[j]->top()->value() +1)
                            return false;

            for(i2=1; i2 < 7; i2++) { // check the other stores.
                j=(i+i2) % 7;
                CardList p = store[j]->cards();
                for (CardList::ConstIterator it = p.begin(); it != p.end(); ++it){
                    Card *c= *it;
                    if( c->isFaceUp() &&
                        c->value() == (store[i]->top()->value()-1) &&
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
