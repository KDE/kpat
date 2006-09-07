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
#include "cardmaps.h"
#include <klocale.h>
#include "deck.h"
#include <kdebug.h>

//-------------------------------------------------------------------------//

Mod3::Mod3( KMainWindow* parent, const char* _name)
        : Dealer( parent, _name )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;
    const int margin = cardMap::CARDY() / 3;

    // This patience uses 2 deck of cards.
    deck = Deck::new_deck( this, 2);
    deck->move(8 + dist_x * 8 + 20, 8 + dist_y * 3 + margin);

    connect(deck, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    aces = new Pile(50, this);
    aces->move(16 + dist_x * 8, 8 + dist_y / 2);
    aces->setTarget(true);
    aces->setCheckIndex(2);
    aces->setAddFlags(Pile::addSpread | Pile::several);

    for ( int r = 0; r < 4; r++ ) {
        for ( int c = 0; c < 8; c++ ) {
            stack[r][c] = new Pile ( r * 10 + c  + 1, this );
            stack[r][c]->move( 8 + dist_x * c,
			       8 + dist_y * r + margin * ( r == 3 ));

	    // The first 3 rows are the playing field, the fourth is the store.
            if ( r < 3 ) {
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
    // kdDebug(11111) << "checkAdd " << checkIndex << " " << c1->top()->name() << " " << c1->index() << " " << c1->index() / 10 << endl;
    if (checkIndex == 0) {
        Card *c2 = cl.first();

        if (c1->isEmpty())
            return (c2->rank() == ( ( c1->index() / 10 ) + 2 ) );

        kdDebug(11111) << "not empty\n";

        if (c1->top()->suit() != c2->suit())
            return false;

        kdDebug(11111) << "same suit\n";
        if (c2->rank() != (c1->top()->rank()+3))
            return false;

        kdDebug(11111) << "+3 " << c1->cardsLeft() << " " << c1->top()->rank() << " " << c1->index()+1 << endl;
        if (c1->cardsLeft() == 1)
            return (c1->top()->rank() == ((c1->index() / 10) + 2));

        kdDebug(11111) << "+1\n";

        return true;
    } else if (checkIndex == 1) {
        return c1->isEmpty();
    } else if (checkIndex == 2) {
        return cl.first()->rank() == Card::Ace;
    } else return false;
}


bool Mod3::checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const
{
    return (checkIndex == 0 && c1->isEmpty() 
	    && c2.first()->rank() == (c1->index()+1));
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

    for (int c = 0; c < 8; c++) {
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
/*    for (CardList::Iterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->rank() == Card::Ace) {
            aces->add(*it);
            (*it)->hide();
        }
*/
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

bool Mod3::startAutoDrop() {
    return false;
}

bool Mod3::isGameLost() const
{
    int n,row,col;
    kdDebug(11111) << "isGameLost ?"<< endl;

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
    if (!deck->isEmpty())
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
            kdDebug(11111) << "considering ["<<row<<"]["<<col<<"] " << ctop->name() << flush;

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
            kdDebug(11111) <<" Can't stack from bottom row" << flush;

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
    virtual Dealer *createGame(KMainWindow *parent) { return new Mod3(parent); }
} ldi5;

//-------------------------------------------------------------------------//

#include"mod3.moc"

//-------------------------------------------------------------------------//

