/*---------------------------------------------------------------------------

  mod3.cpp  implements a patience card game

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

#include "mod3.h"
#include <klocale.h>
#include "deck.h"
#include "pile.h"
#include <kdebug.h>

//-------------------------------------------------------------------------//

Mod3::Mod3( KMainWindow* parent, const char* _name)
        : Dealer( parent, _name )
{
    deck = new Deck( 0, this, 2);
    deck->move(8 + 80 * 8 + 20, 8 + 105 * 3 + 32);
    connect(deck, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    aces = new Pile(40, this);
    aces->hide();
    aces->setTarget(true);

    for( int r = 0; r < 4; r++ ) {
        for( int c = 0; c < 8; c++ ) {
            stack[ r ][ c ] = new Pile ( r + 1, this );
            stack[r][c]->move( 8 + 80 * c, 8 + 105 * r + 32 * ( r == 3 ));
            if( r < 3 ) {
                stack[r][c]->setCheckIndex( 0 );
                stack[r][c]->setTarget(true);
            } else {
                stack[r][c]->setAddFlags( Pile::addSpread );
                stack[r][c]->setCheckIndex( 1 );
            }
        }
    }

    setTakeTargetForHints(true);
    setActions(Dealer::Hint | Dealer::Demo );
}

//-------------------------------------------------------------------------//

bool Mod3::checkAdd( int checkIndex, const Pile *c1, const CardList& cl) const
{
    if (checkIndex == 0) {
        Card *c2 = cl.first();

        if (c1->isEmpty())
            return (c2->value() == (c1->index()+1));

        if (c1->top()->suit() != c2->suit())
            return false;

        if (c2->value() != (c1->top()->value()+3))
            return false;

        if (c1->cardsLeft() == 1)
            return (c1->top()->value() == (c1->index()+1));

        return true;
    } else if (checkIndex == 1) {
        return c1->isEmpty();
    } else return false;
}

bool Mod3::checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const
{
    return (checkIndex == 0 && c1->isEmpty() && c2.first()->value() == (c1->index()+1));
}

//-------------------------------------------------------------------------//

void Mod3::restart()
{
    deck->collectAndShuffle();
    deal();
}

//-------------------------------------------------------------------------//

void Mod3::dealRow(int row)
{
    if (deck->isEmpty())
        return;

    for (int c = 0; c < 8; c++)
    {
        Card *card;

        card = deck->nextCard();
        stack[row][c]->add (card, false, true);
    }
}

void Mod3::deckClicked(Card*)
{
    kdDebug(11111) << "deck clicked " << deck->cardsLeft() << endl;
    if (deck->isEmpty())
        return;

    unmarkAll();
    dealRow(3);
    takeState();
}

//-------------------------------------------------------------------------//

void Mod3::deal()
{
    unmarkAll();
    CardList list = deck->cards();
    for (CardList::Iterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->value() == Card::Ace) {
            aces->add(*it);
            (*it)->hide();
        }
    kdDebug(11111) << "init " << aces->cardsLeft() << " " << deck->cardsLeft() << endl;

    for (int r = 0; r < 4; r++)
        dealRow(r);
}

Card *Mod3::demoNewCards()
{
   if (deck->isEmpty())
       return 0;
   deckClicked(0);
   return stack[3][0]->top();
}

static class LocalDealerInfo5 : public DealerInfo
{
public:
    LocalDealerInfo5() : DealerInfo(I18N_NOOP("M&od3"), 5) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Mod3(parent); }
} ldi5;

//-------------------------------------------------------------------------//

#include"mod3.moc"

//-------------------------------------------------------------------------//

