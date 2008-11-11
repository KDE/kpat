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
#include <kdebug.h>
#include "deck.h"
#include <kaction.h>
#include "patsolve/grandf.h"
#include <assert.h>

Grandf::Grandf( )
    : DealerScene(  )
{
    Deck::create_deck(this);
    Deck::deck()->hide();

    const int distx = 14;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos(10+(i+1)*distx, 1);
        target[i]->setType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) ) ;
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->setPilePos(1+distx*i, 16 );
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setObjectName( QString( "store%1" ).arg( i ) ) ;
        store[i]->setCheckIndex(1);
        store[i]->setReservedSpace( QSizeF( 10, 45 ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Redeal);
    setSolver( new GrandfSolver( this ) );
}

void Grandf::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
    numberOfDeals = 1;
}

void Grandf::redeal()
{
    if ( waiting() || demoActive()  )
        return;

    kDebug(11111) << "redeal\n";
    unmarkAll();

    if (numberOfDeals < 3) {
        collect();
        deal();
        numberOfDeals++;
    }
    if (numberOfDeals == 3) {
        emit redealPossible(false);
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
            Card *next = Deck::deck()->nextCard();
            if (next)
                store[i]->add(next, i != start);
            i += dir;
        } while ( i != stop + dir);
        int t = start;
        start = stop;
        stop = t+dir;
        dir = -dir;
    }

    int i = 0;
    Card *next = Deck::deck()->nextCard();
    while (next)
    {
        store[i+1]->add(next, false);
        next = Deck::deck()->nextCard();
        i = (i+1)%6;
    }

    for (int round=0; round < 7; round++)
    {
        Card *c = store[round]->top();
        if (c)
            c->turn(true);
    }
    emit redealPossible( true );
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
        for (CardList::ConstIterator it = p.constBegin(); it != p.constEnd(); ++it)
            Deck::deck()->add(*it, true);
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
    emit redealPossible(numberOfDeals < 3);
}

static class LocalDealerInfo1 : public DealerInfo
{
public:
    LocalDealerInfo1() : DealerInfo(I18N_NOOP("Grandfather"), 1) {}
    virtual DealerScene *createGame() { return new Grandf(); }
} gfdi;

#include "grandf.moc"
