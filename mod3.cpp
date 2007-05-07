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
                stack[r][c]->setReservedSpace( QSizeF( 10, 15 ) );
                stack[r][c]->setAddFlags( Pile::addSpread );
                stack[r][c]->setCheckIndex( 1 );
                stack[r][c]->setObjectName( QString( "stack3_%1" ).arg( c ) );
            }
        }
     }

    setActions(DealerScene::Hint | DealerScene::Demo );
}


//-------------------------------------------------------------------------//

void Mod3::getHints()
{
    bool foundone = false;

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *store = *it;
        Card *top = store->top();

        if ( !top || store == aces || store == Deck::deck() )
            continue;

        if ( store->top()->rank() == Card::Ace )
        {
            newHint(new MoveHint(store->top(), aces, 127));
            foundone = true;
            continue;
        }

        if ( store->cardsLeft() > 1 && store->target() )
            continue;

        // kDebug(11111) << "trying " << top->name() << " " << store->legalRemove(top) << endl;

        // Q_ASSERT( store->legalRemove(top) );
        if (store->legalRemove(top)) {
//                kDebug(11111) << "could remove " << top->name() << endl;
            for (PileList::Iterator pit = piles.begin(); pit != piles.end(); ++pit)
            {
                Pile *dest = *pit;
                if (dest == store)
                    continue;
                if (dest->isEmpty() && !dest->target()) // later
                    continue;
                CardList cards;
                cards.append( top );
                if (!dest->legalAdd(cards))
                    continue;

                // kDebug() << "could still be it " << dest->objectName() << " " << store->y() << " " << dest->y() << " " << store->objectName() << endl;
                if ( store->y() == dest->y() )
                {
                    if ( top->rank() == Card::Two || top->rank() == Card::Three || top->rank() == Card::Four )
                        continue;
                }
                newHint(new MoveHint(top, dest, 50));
                foundone = true;
            }
        }
    }

    if ( foundone )
        return;

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *store = *it;
        Card *top = store->top();

        if ( !top || store == aces || store == Deck::deck() )
            continue;

        if ( store->cardsLeft() > 1 && store->target() )
            continue;

        if ( store->cardsLeft() == 1 && !store->target() )
            continue;

        if ( store->cardsLeft() == 1 )
        {
            // HACK: cards that are already on the right target should not be moved away
            if (top->rank() == Card::Four && store->y() == stack[2][0]->y())
                continue;
            if (top->rank() == Card::Three && store->y() == stack[1][0]->y())
                continue;
            if (top->rank() == Card::Two && store->y() == stack[0][0]->y())
                continue;
        }

        if (store->legalRemove(top)) {
//                kDebug(11111) << "could remove " << top->name() << endl;
            for (PileList::Iterator pit = piles.begin(); pit != piles.end(); ++pit)
            {
                Pile *dest = *pit;
                if (dest == store || dest == Deck::deck() )
                    continue;
                if (dest->isEmpty() && !dest->target())
                {
                    newHint(new MoveHint(top, dest, 0));
                    continue;
                }
            }
        }
    }

}

bool Mod3::checkAdd( int checkIndex, const Pile *c1, const CardList& cl) const
{
    // kDebug(11111) << "checkAdd " << checkIndex << " " << ( c1->top() ? c1->top()->name() : "none" ) << " " << c1->index() << " " << c1->index() / 10 << endl;

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

        //kDebug(11111) << "+3 " << c1->cardsLeft() << " " << c1->top()->rank() << " " << c1->index()+1 << endl;
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
    kDebug(11111) << "deck clicked " << Deck::deck()->cardsLeft() << endl;
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
    kDebug(11111) << "init " << aces->cardsLeft() << " " << Deck::deck()->cardsLeft() << endl;

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

bool Mod3::isGameLost() const
{
    int n,row,col;
    //kDebug(11111) << "isGameLost ?"<< endl;

    bool nextTest=false;

    // If there is an empty stack or an ace below, the game is not lost.
    for (col=0; col < 8; col++){
        if (stack[3][col]->isEmpty()
	    || stack[3][col]->at(0)->rank() == Card::Ace)
            return false;
    }

    // Ok, so no empty stack below.
    // If there is neither an empty stack on the board (an ace counts
    // as this) nor a card placed in the correct row, all is lost.
    // Otherwise we have to do more tests.
    for (n = 0; n < 24; n++) {
        row = n / 8;
        col = n % 8;

	// If there is a stack on the board that is either empty or
	// contains an ace, the game is not finished.
        if (stack[row][col]->isEmpty()
	    || stack[row][col]->at(0)->rank() == Card::Ace) {
            nextTest = true;
            break;
        }

	// If there is a card that is correctly placed, the game is
	// not lost.
        if (stack[row][col]->at(0)->rank() == Card::Two + row) {
            nextTest = true;
            break;
        }
    }
    if (!nextTest)
        return true;

    // If there are more cards in the deck, the game is not lost.
    if (!Deck::deck()->isEmpty())
        return false;

    int    n2, row2, col2, col3;
    Card  *ctop;
    Card  *card;

    // For all stacks on the board, check if:
    //
    for (n = 0; n < 24; n++){
        row = n / 8;
        col = n % 8;

	// Empty stack: Can we move a card there?
        if (stack[row][col]->isEmpty()) {
	    // Can we move a card from below?
            for (col3=0; col3 < 8; col3++) {
                if (stack[3][col3]->top()->rank() == (Card::Two+row))
                    return false;
            }

	    // Can we move a card from another row?
            for (n2 = 0; n2 < 16; n2++) {
                row2 = (row + 1 + (n2 / 8)) % 3;
                col2 = n2 % 8;

                if (stack[row2][col2]->isEmpty())
                    continue;
                if (stack[row2][col2]->top()->rank() == (Card::Two + row))
                    return false;
            }
        }
        else {
	    // Non-empty stack.
            ctop = stack[row][col]->top();
            //kDebug(11111) << "considering ["<<row<<"]["<<col<<"] " << ctop->name() << flush;

	    // Card not in its final position?  Then we can't build on it.
            if (stack[row][col]->at(0)->rank() != Card::Two + row)
                continue;

	    // Can we move a card from below here?
            for (col3 = 0; col3 < 8; col3++) {
                card = stack[3][col3]->top();
                if (card->suit() == ctop->suit()
		    && card->rank() == ctop->rank() + 3)
                    return false;
            }
            //kDebug(11111) <<" Can't stack from bottom row" << flush;

	    // Can we move a card from another stack here?
            for (int n_2 = 1; n_2 < 24; n_2++) {
                n2 = (n + n_2) % 24;
                row2 = n2 / 8;
                col2 = n2 % 8;

                if (stack[row2][col2]->isEmpty())
                    continue;

                card = stack[row2][col2]->top();

		// Only consider cards that are not on top of other cards.
                if (stack[row2][col2]->indexOf(card) != 0)
                    continue;

                if (card->suit() == ctop->suit()
		    && card->rank() == ctop->rank() + 3)
                    return false;
            }
        }
    }

    return true;
}


static class LocalDealerInfo5 : public DealerInfo
{
public:
    LocalDealerInfo5() : DealerInfo(I18N_NOOP("M&od3"), 5) {}
    virtual DealerScene *createGame() { return new Mod3(); }
} ldi5;

//-------------------------------------------------------------------------//

#include "mod3.moc"

//-------------------------------------------------------------------------//

