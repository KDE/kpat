/*---------------------------------------------------------------------------

  mod3.cpp  implements a patience card game

     Copyright 1997 Rodolfo Borges <barrett@labma.ufrj.br>

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
#include "deck.h"
#include "hint.h"

#include <klocale.h>
#include <kdebug.h>

#include "patsolve/mod3.h"

//-------------------------------------------------------------------------//

class Mod3Pile : public Pile
{
public:
    Mod3Pile( int _index, DealerScene* parent = 0)
        : Pile( _index, parent ) {}
    virtual void relayoutCards() {
        Pile::relayoutCards();
        if ( isEmpty() && objectName().startsWith( "stack3" ) )
        {
            add( Deck::deck()->nextCard(), false );
        }
    }
};


Mod3::Mod3( )
    : DealerScene( )
{
    const double dist_x = 11.14;
    const double dist_y = 13.1;
    const double margin = 2;

    // This patience uses 2 deck of cards.
    Deck::create_deck( this, 2);
    Deck::deck()->setPilePos(3 + dist_x * 8 + 5, 2 + dist_y * 3 + margin);

    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    aces = new Pile(50, this);
    aces->setPilePos(10 + dist_x * 8, dist_y / 2);
    aces->setTarget(true);
    aces->setCheckIndex(2);
    aces->setAddFlags(Pile::addSpread | Pile::several);
    aces->setObjectName( "aces" );
    aces->setReservedSpace( QSizeF( 10, 20 ));

    for ( int r = 0; r < 4; r++ ) {
        for ( int c = 0; c < 8; c++ ) {
            stack[r][c] = new Mod3Pile ( r * 10 + c  + 1, this );
            stack[r][c]->setPilePos( 2 + dist_x * c,
                                     2 + dist_y * r + margin * ( r == 3 ));

	    // The first 3 rows are the playing field, the fourth is the store.
            if ( r < 3 ) {
                stack[r][c]->setCheckIndex( 0 );
                stack[r][c]->setTarget(true);
                stack[r][c]->setAddFlags( Pile::addSpread );
                stack[r][c]->setSpread( 0.5 );
                stack[r][c]->setObjectName( QString( "stack%1_%2" ).arg( r ).arg( c ) );
                stack[r][c]->setReservedSpace( QSizeF( 10, 12.3 ) );
            } else {
                stack[r][c]->setReservedSpace( QSizeF( 10, 18 ) );
                stack[r][c]->setAddFlags( Pile::addSpread );
                stack[r][c]->setCheckIndex( 1 );
                stack[r][c]->setObjectName( QString( "stack3_%1" ).arg( c ) );
            }
        }
     }

    setActions(DealerScene::Hint | DealerScene::Demo );
    setSolver( new Mod3Solver( this ) );
}

bool Mod3::checkAdd( int checkIndex, const Pile *c1, const CardList& cl) const
{
    // kDebug(11111) << "checkAdd" << checkIndex << " " << ( c1->top() ? c1->top()->name() : "none" ) << " " << c1->index() << " " << c1->index() / 10;

    if (checkIndex == 0) {
        Card *c2 = cl.first();

        if (c1->isEmpty())
            return (c2->rank() == ( ( c1->index() / 10 ) + 2 ) );

        //kDebug(11111) << "not empty\n";

        if (c1->top()->suit() != c2->suit())
            return false;

        //kDebug(11111) << "same suit\n";
        if (c2->rank() != (c1->top()->rank()+3))
            return false;

        //kDebug(11111) << "+3" << c1->cardsLeft() << " " << c1->top()->rank() << " " << c1->index()+1;
        if (c1->cardsLeft() == 1)
            return (c1->top()->rank() == ((c1->index() / 10) + 2));

        //kDebug(11111) << "+1\n";

        return true;

    } else if (checkIndex == 1) {
        return c1->isEmpty();

    } else if (checkIndex == 2) {
        return cl.first()->rank() == Card::Ace;

    } else
	return false;
}


bool Mod3::checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const
{
    return (checkIndex == 0 && c1->isEmpty()
	    && c2.first()->rank() == (c1->index()+1));
}


//-------------------------------------------------------------------------//


void Mod3::restart()
{
    Deck::deck()->collectAndShuffle();
    deal();
}


//-------------------------------------------------------------------------//


void Mod3::dealRow(int row)
{
    if (Deck::deck()->isEmpty())
        return;

    for (int c = 0; c < 8; c++) {
        Card *card;

        card = Deck::deck()->nextCard();
        stack[row][c]->add (card, false);
    }
}


void Mod3::deckClicked(Card*)
{
    kDebug(11111) << "deck clicked" << Deck::deck()->cardsLeft();
    if (Deck::deck()->isEmpty())
        return;

    unmarkAll();
    dealRow(3);
    takeState();
}


//-------------------------------------------------------------------------//


void Mod3::deal()
{
    unmarkAll();
    CardList list = Deck::deck()->cards();
/*    for (CardList::Iterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->rank() == Card::Ace) {
            aces->add(*it);
            (*it)->hide();
        }
*/
    //kDebug(11111) << "init" << aces->cardsLeft() << " " << Deck::deck()->cardsLeft();

    for (int r = 0; r < 4; r++)
        dealRow(r);
}

Card *Mod3::demoNewCards()
{
   if (Deck::deck()->isEmpty())
       return 0;
   deckClicked(0);
   return stack[3][0]->top();
}

static class LocalDealerInfo5 : public DealerInfo
{
public:
    LocalDealerInfo5() : DealerInfo(I18N_NOOP("Mod3"), 5) {}
    virtual DealerScene *createGame() { return new Mod3(); }
} ldi5;

//-------------------------------------------------------------------------//

#include "mod3.moc"

//-------------------------------------------------------------------------//

