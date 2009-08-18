/***********************-*-C++-*-********

  grandf.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete <paul@troll.no>
               (C) 2000  Stephan Kulow <coolo@kde.org>
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

#include "deck.h"
#include "speeds.h"
#include "patsolve/grandf.h"

#include <KDebug>
#include <KLocale>


Grandf::Grandf( )
    : DealerScene(  )
{
    Deck::createDeck(this);
    Deck::deck()->hide();

    const qreal distx = 1.4;
    const qreal targetOffset = 1.5 * distx;

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPilePos(targetOffset+i*distx, 0);
        target[i]->setType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) ) ;
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->setPilePos(distx*i, 1.2);
        store[i]->setAddFlags(Pile::addSpread | Pile::several);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
        store[i]->setObjectName( QString( "store%1" ).arg( i ) ) ;
        store[i]->setCheckIndex(1);
        store[i]->setReservedSpace( QSizeF( 1.0, 5.0 ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Redeal);
    setSolver( new GrandfSolver( this ) );
}

void Grandf::restart() {
    Deck::deck()->collectAndShuffle();
    deal();
    numberOfDeals = 1;
    emit newCardsPossible( true );
}

Card *Grandf::newCards()
{
//     if ( waiting() || demoActive() || isGameWon()  )
//         return;

    if (numberOfDeals >= 3)
        return 0;

    unmarkAll();
    collect();
    deal();
    numberOfDeals++;

    Card *t = store[3]->top();
    Q_ASSERT(t);
    if (t)
    {
       t->turn(false);
       t->flipTo( t->pos().x(), t->pos().y(), DURATION_FLIP );
    }

    takeState();
    considerGameStarted();
    if (numberOfDeals == 3)
        emit newCardsPossible(false);

    return t;
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
    kDebug(11111) << checkIndex << c1->isEmpty();
    Q_ASSERT (checkIndex == 1);
    if (c1->isEmpty())
        return c2.first()->rank() == Card::King;
    else
        return (c2.first()->rank() == c1->top()->rank() - 1)
               && c2.first()->suit() == c1->top()->suit();
}

QString Grandf::getGameState()
{
    return QString::number(numberOfDeals);
}

void Grandf::setGameState( const QString &s)
{
    numberOfDeals = s.toInt();
    emit newCardsPossible(numberOfDeals < 3);
}

static class LocalDealerInfo1 : public DealerInfo
{
public:
    LocalDealerInfo1() : DealerInfo(I18N_NOOP("Grandfather"), 1) {}
    virtual DealerScene *createGame() const { return new Grandf(); }
} gfdi;

#include "grandf.moc"
