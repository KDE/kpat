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
#include "patsolve/mod3.h"

#include <KDebug>
#include <KLocale>


//-------------------------------------------------------------------------//

// Special pile type used for the fourth row. Automatically grabs a new card
// from the deck when emptied.
class Mod3Pile : public Pile
{
public:
    Mod3Pile( int _index, DealerScene* parent = 0)
        : Pile( _index, parent ) {}
    virtual void relayoutCards() {
        Pile::relayoutCards();
        // Don't pull cards from the deck if the deck still contains all 104
        // cards. This prevents glitchy things from happening before the initial
        // deal has happened.
        if ( isEmpty() && Deck::deck()->cardsLeft() < 104 )
        {
            add( Deck::deck()->nextCard(), false );
        }
    }
};


Mod3::Mod3( )
    : DealerScene( )
{
    const qreal dist_x = 1.114;
    const qreal dist_y = 1.31;
    const qreal bottomRowY = 3 * dist_y + 0.2;
    const qreal rightColumX = 8 * dist_x + 0.8;

    // This patience uses 2 deck of cards.
    Deck::create_deck( this, 2);
    Deck::deck()->setPilePos(rightColumX, bottomRowY);

    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(newCards()));

    aces = new Pile(50, this);
    aces->setPilePos(rightColumX, 0.5);
    aces->setTarget(true);
    aces->setCheckIndex(2);
    aces->setAddFlags(Pile::addSpread | Pile::several);
    aces->setObjectName( "aces" );
    aces->setReservedSpace( QSizeF( 1.0, 2.0 ));

    for ( int r = 0; r < 4; r++ ) {
        for ( int c = 0; c < 8; c++ ) {
            int pileIndex = r * 10 + c  + 1;
            // The first 3 rows are the playing field, the fourth is the store.
            if ( r < 3 ) {
                stack[r][c] = new Pile ( pileIndex, this );
                stack[r][c]->setPilePos( dist_x * c, dist_y * r );
                stack[r][c]->setCheckIndex( 0 );
                stack[r][c]->setTarget(true);
                stack[r][c]->setAddFlags( Pile::addSpread );
                stack[r][c]->setSpread( 0.5 );
                stack[r][c]->setObjectName( QString( "stack%1_%2" ).arg( r ).arg( c ) );
                stack[r][c]->setReservedSpace( QSizeF( 1.0, 1.23 ) );
            } else {
                stack[r][c] = new Mod3Pile ( pileIndex, this );
                stack[r][c]->setPilePos( dist_x * c, bottomRowY );
                stack[r][c]->setReservedSpace( QSizeF( 1.0, 1.8 ) );
                stack[r][c]->setAddFlags( Pile::addSpread );
                stack[r][c]->setCheckIndex( 1 );
                stack[r][c]->setObjectName( QString( "stack3_%1" ).arg( c ) );
            }
        }
    }

    setActions(DealerScene::Hint | DealerScene::Demo  | DealerScene::Deal);
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
    emit newCardsPossible(true);
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


//-------------------------------------------------------------------------//


void Mod3::deal()
{
    unmarkAll();
/*  CardList list = Deck::deck()->cards();
    for (CardList::Iterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->rank() == Card::Ace) {
            aces->add(*it);
            (*it)->hide();
        }
    kDebug(11111) << "init" << aces->cardsLeft() << " " << Deck::deck()->cardsLeft();
*/
    for (int r = 0; r < 4; r++)
        dealRow(r);
}

Card *Mod3::newCards()
{
    if (Deck::deck()->isEmpty())
        return 0;

    unmarkAll();
    dealRow(3);
    takeState();
    considerGameStarted();
    if (Deck::deck()->isEmpty())
        emit newCardsPossible(false);

    return stack[3][0]->top();
}

void Mod3::setGameState(const QString &)
{
    emit newCardsPossible(!Deck::deck()->isEmpty());
}

static class LocalDealerInfo5 : public DealerInfo
{
public:
    LocalDealerInfo5() : DealerInfo(I18N_NOOP("Mod3"), 5) {}
    virtual DealerScene *createGame() const { return new Mod3(); }
} ldi5;

//-------------------------------------------------------------------------//

#include "mod3.moc"

//-------------------------------------------------------------------------//

