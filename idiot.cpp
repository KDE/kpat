/*
   idiot.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation.

   This file is provided AS IS with no warranties of any kind.  The author
   shall have no liability with respect to the infringement of copyrights,
   trade secrets or any patents by this file or any part thereof.  In no
   event will the author be liable for any lost revenue or profits or
   other special, indirect and consequential damages.

   4 positions, remove lowest card(s) of suit
*/

#include <qapplication.h>

#include "idiot.h"
#include "dealer.h"
#include <klocale.h>
#include <deck.h>
#include <pile.h>
#include <kmainwindow.h>

Idiot::Idiot( KMainWindow* parent, const char* _name)
  : Dealer( parent, _name )
{
    deck = new Deck( 0, this );
    deck->move(210, 10);

    away = new Pile( 5, this );
    away->hide();
    away->setTarget(true);

    for( int i = 0; i < 4; i++ )
    {
        play[ i ] = new Pile( i + 1, this);
        play[i]->move(10 + 100 * i, 150);
        play[i]->setAddFlags( Pile::addSpread );
        play[i]->setRemoveFlags( Pile::disallow );
    }
    deal();
}

void Idiot::restart()
{
    deck->collectAndShuffle();
    deal();
}

inline bool higher( const Card* c1, const Card* c2)
{
    if (!c1 || !c2 || c1 == c2)
        return false;
    if (c1->suit() != c2->suit())
        return false;
    if (c2->value() == Card::Ace) // special case
        return true;
    if (c1->value() == Card::Ace)
        return false;
    return (c1->value() < c2->value());
}

void Idiot::cardClicked(Card *c)
{
    if (c->source() == deck) {
        deal();
        return;
    }

    if( higher( c, play[ 0 ]->top() ) ||
        higher( c, play[ 1 ]->top() ) ||
        higher( c, play[ 2 ]->top() ) ||
        higher( c, play[ 3 ]->top() ) )
        away->add(c, false, true);
    else if( play[ 0 ]->isEmpty() )
        play[0]->add(c, false, true);
    else if( play[ 1 ]->isEmpty() )
        play[1]->add(c, false, true);
    else if( play[ 2 ]->isEmpty() )
        play[2]->add( c, false, true);
    else if( play[ 3 ]->isEmpty() )
        play[3]->add(c, false, true);

}

void Idiot::deal()
{
    if( !deck->isEmpty() )
        for( int i = 0; i < 4; i++ )
            play[ i ]->add( deck->nextCard(), false, true );
}

static class LocalDealerInfo4 : public DealerInfo
{
public:
    LocalDealerInfo4() : DealerInfo(I18N_NOOP("The &Idiot"), 2) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Idiot(parent); }
} ldi4;


#include "idiot.moc"
