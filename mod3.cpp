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

    deck = Deck::new_deck( this, 2);

    deck->move(8 + dist_x * 8 + 20, 8 + dist_y * 3 + margin);
    connect(deck, SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));

    aces = new Pile(50, this);
    aces->move(16 + dist_x * 8, 8 + dist_y / 2);
    // aces->hide();
    aces->setTarget(true);
    aces->setCheckIndex(2);
    aces->setAddFlags(Pile::addSpread | Pile::several);

    for( int r = 0; r < 4; r++ ) {
        for( int c = 0; c < 8; c++ ) {
            stack[ r ][ c ] = new Pile ( r * 10 + c  + 1, this );
            stack[r][c]->move( 8 + dist_x * c, 8 + dist_y * r + margin * ( r == 3 ));
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
    // kdDebug(11111) << "checkAdd " << checkIndex << " " << c1->top()->name() << " " << c1->index() << " " << c1->index() / 10 << endl;
    if (checkIndex == 0) {
        Card *c2 = cl.first();

        if (c1->isEmpty())
            return (c2->value() == ( ( c1->index() / 10 ) + 2 ) );

        kdDebug(11111) << "not empty\n";

        if (c1->top()->suit() != c2->suit())
            return false;

        kdDebug(11111) << "same suit\n";
        if (c2->value() != (c1->top()->value()+3))
            return false;

        kdDebug(11111) << "+3 " << c1->cardsLeft() << " " << c1->top()->value() << " " << c1->index()+1 << endl;
        if (c1->cardsLeft() == 1)
            return (c1->top()->value() == ((c1->index() / 10) + 2));

        kdDebug(11111) << "+1\n";

        return true;
    } else if (checkIndex == 1) {
        return c1->isEmpty();
    } else if (checkIndex == 2) {
        return cl.first()->value() == Card::Ace;
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
/*    for (CardList::Iterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->value() == Card::Ace) {
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

bool Mod3::isGameLost() const {
    int n,r,c;
    kdDebug(11111) << "isGameLost ?"<< endl;

    bool nextTest=false;
    for(n=0; n <24; n++){
        r=n/8;
        c= n %8;
        if(stack[r][c]->isEmpty()){
            nextTest=true;
            break;
        }
        if(stack[r][c]->at(0)->value() == (Card::Two +r) || stack[r][c]->at(0)->value() == Card::Ace) {
            nextTest=true;
            break;
        }
    }
    if(!nextTest)
        return true;

    if(!deck->isEmpty())
        return false;

    for(c=0; c < 8; c++){
        if(stack[3][c]->isEmpty())
            return false;
    }

    int n2,r2,c2,c3;
    Card *ctop, *card;

    for(n=0; n < 24; n++){
        r=n / 8;
        c=n % 8;
        if(stack[r][c]->isEmpty()){
            for(c3=0; c3 < 8; c3++){
                if(stack[3][c3]->top()->value()==(Card::Two+r))
                    return false;
            }
            for(n2=0; n2 < 16;n2++){
                r2=(r+1+(n2 / 8)) % 3;
                c2=n2 % 8;

                if(stack[r2][c2]->isEmpty())
                    continue;
                if(stack[r2][c2]->top()->value()==(Card::Two+r))
                    return false;
            }
        }
        else{
            ctop=stack[r][c]->top();
            kdDebug(11111) << "considering ["<<r<<"]["<<c<<"] " << ctop->name() << flush;

            if(stack[r][c]->at(0)->value() !=(Card::Two+r))
                continue;


            for(c3=0; c3 < 8; c3++){
                card=stack[3][c3]->top();
                if(card->suit() == ctop->suit() && card->value() == ctop->value()+3)
                    return false;
            }
            kdDebug(11111) <<" cant stack from bottom row" << flush;

            for(int n_2=1;n_2 < 24; n_2++){
                n2=(n+n_2) % 24;
                r2= n2 / 8;
                c2= n2 % 8;

                if(stack[r2][c2]->isEmpty())
                    continue;

                card=stack[r2][c2]->top();

                if(stack[r2][c2]->indexOf(card) != 0)
                    continue;

                if(card->suit() == ctop->suit() && card->value() == ctop->value()+3)
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

