/*
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
*/

#include "yukon.h"
#include <klocale.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

Yukon::Yukon( KMainWindow* parent )
    : Dealer( parent )
{
    const int dist_x = cardMap::CARDX() * 11 / 10 + 1;
    const int dist_y = cardMap::CARDY() * 11 / 10 + 1;

    deck = Deck::new_deck(this);
    deck->setPos(10, 10+dist_y*3);
    deck->hide();

    for (int i=0; i<4; i++) {
        target[i] = new Pile(i+1, this);
        target[i]->setPos(20+7*dist_x, 10+dist_y *i);
        target[i]->setType(Pile::KlondikeTarget);
    }

    for (int i=0; i<7; i++) {
        store[i] = new Pile(5+i, this);
        store[i]->setPos(15+dist_x*i, 10);
        store[i]->setAddType(Pile::KlondikeStore);
        store[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

void Yukon::restart() {
    deck->collectAndShuffle();
    deal();
}

void Yukon::deal() {
    for (int round = 0; round < 11; round++)
    {
        for (int j = 0; j < 7; j++)
        {
            bool doit = false;
            switch (j) {
            case 0:
                doit = (round == 0);
                break;
            default:
                doit = (round < j + 5);
            }
            if (doit)
                store[j]->add(deck->nextCard(), round < j && j != 0, true);
        }
    }
}

bool Yukon::isGameLost() const {
   int i,j,k,l,indexi,freeStore=0;
   Card *c, *cNewTop;

 kDebug(11111) <<"isGameLost" << endl;

   for(i=0; i < 7; i++){
      if( store[i]->isEmpty() ){
         freeStore++;
         continue;
         }

      if(store[i]->top()->rank() == Card::Ace ||
         ! store[i]->top()->isFaceUp())
         return false;

      for(indexi=store[i]->indexOf(store[i]->top()); indexi >=0; indexi--){

         c=store[i]->at(indexi);
         if( !c->isFaceUp() )
            break;

         if(freeStore > 0 && indexi > 0 && c->rank() == Card::King)
            return false;

         for(j=0; j < 4;j++){
            if(!target[j]->isEmpty() &&
               c->rank()-1 == target[j]->top()->rank() &&
               c->suit() == target[j]->top()->suit())
               return false;
         }

         for(j=1; j < 7; j++){
            k=(i+j) % 7;
            if( !store[k]->isEmpty() ) {
               if(c->rank()+1 == store[k]->top()->rank() &&
                  (c->isRed() != store[k]->top()->isRed())){

                  if(indexi ==  0)
                     return false;
                  else{
                     cNewTop=store[i]->at(indexi-1);
                     if(!cNewTop->isFaceUp())
                        return false;
                     if(cNewTop->rank() == Card::Ace)
                        return false;
                     if(cNewTop->rank() != store[k]->top()->rank() ||
                        cNewTop->isRed() != store[k]->top()->isRed())
                        return false;

                     for(l=0; l < 4;l++){
                        if(!target[l]->isEmpty() &&
                           cNewTop->rank()-1 == target[l]->top()->rank() &&
                           cNewTop->suit() == target[l]->top()->suit())
                           return false;
                     }
                  }
               }
            }
         }
      }
   }
   return (freeStore!=7);
}

static class LocalDealerInfo10 : public DealerInfo
{
public:
    LocalDealerInfo10() : DealerInfo(I18N_NOOP("&Yukon"), 10) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Yukon(parent); }
} gfi10;

#include "yukon.moc"
