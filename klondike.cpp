/***********************-*-C++-*-********

  klondike.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete

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

//
// 7 positions, alternating red and black
//

****************************************/

#include "klondike.h"
#include <klocale.h>
#include <card.h>
#include <deck.h>
#include <pile.h>
#include <kdebug.h>
#include <assert.h>
#include "cardmaps.h"

Klondike::Klondike( bool easy, KMainWindow* parent, const char* _name )
  : Dealer( parent, _name )
{
    // The units of the follwoing constants are pixels
    const int margin = 10; // between card piles and board edge
    const int hspacing = cardMap::CARDX() / 6 + 1; // horizontal spacing between card piles
    const int vspacing = cardMap::CARDY() / 4; // vertical spacing between card piles

    deck = new Deck(13, this);
    deck->move(margin, margin);

    EasyRules = easy;

    pile = new Pile( 0, this);
    pile->move(margin + cardMap::CARDX() + cardMap::CARDX() / 4, margin);
    // Move the visual representation of the pile to the intended position
    // on the game board.

    pile->setAddFlags(Pile::disallow);
    pile->setRemoveFlags(Pile::Default);

    for( int i = 0; i < 7; i++ ) {
        play[ i ] = new Pile( i + 5, this);
        play[i]->move(margin + (cardMap::CARDX() + hspacing) * i, margin + cardMap::CARDY() + vspacing);
        play[i]->setAddType(Pile::KlondikeStore);
        play[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop | Pile::wholeColumn);
    }

    for( int i = 0; i < 4; i++ ) {
        target[ i ] = new Pile( i + 1, this );
        target[i]->move(margin + (3 + i) * (cardMap::CARDX()+ hspacing), margin);
        target[i]->setAddType(Pile::KlondikeTarget);
        if (EasyRules) // change default
            target[i]->setRemoveFlags(Pile::Default);
        else
            target[i]->setRemoveType(Pile::KlondikeTarget);
    }

    setActions(Dealer::Hint | Dealer::Demo);
}

bool Klondike::tryToDrop(Card *t)
{
    if (!t || !t->realFace() || t->takenDown())
        return false;

//    kdDebug(11111) << "tryToDrop " << t->name() << endl;

    bool shoulddrop = (t->value() <= Card::Two || t->value() <= lowest_card[t->isRed() ? 1 : 0] + 1);
    Pile *tgt = findTarget(t);
    if (tgt) {
        newHint(new MoveHint(t, tgt, shoulddrop));
        return true;
    }
    return false;
}

void Klondike::getHints() {

    int tops[4] = {0, 0, 0, 0};

    for( int i = 0; i < 4; i++ )
    {
        Card *c = target[i]->top();
        if (!c) continue;
        tops[c->suit() - 1] = c->value();
    }

    lowest_card[0] = static_cast<Card::Values>(QMIN(tops[1], tops[2])); // red
    lowest_card[1] = static_cast<Card::Values>(QMIN(tops[0], tops[3])); // black

//    kdDebug(11111) << "startAutoDrop red:" << lowest_card[0] << " black:" << lowest_card[1] << endl;

    Card* t[7];
    for(int i=0; i<7;i++)
        t[i] = play[i]->top();

    for(int i=0; i<7; i++)
    {
        CardList list = play[i]->cards();

        for (CardList::ConstIterator it = list.begin(); it != list.end(); ++it)
        {
            if (!(*it)->isFaceUp())
                continue;

            CardList empty;
            empty.append(*it);

            for (int j = 0; j < 7; j++)
            {
                if (i == j)
                    continue;

                if (play[j]->legalAdd(empty)) {
                    if (((*it)->value() != Card::King) || it != list.begin()) {
                        newHint(new MoveHint(*it, play[j]));
                        break;
                    }
                }
            }
            break; // the first face up
        }

        tryToDrop(play[i]->top());
    }
    if (!pile->isEmpty())
    {
        Card *t = pile->top();
        if (!tryToDrop(t))
        {
            for (int j = 0; j < 7; j++)
            {
                CardList empty;
                empty.append(t);
                if (play[j]->legalAdd(empty)) {
                    newHint(new MoveHint(t, play[j]));
                    break;
                }
            }
        }
    }
}

Card *Klondike::demoNewCards() {
    deal3();
    if (!deck->isEmpty() && pile->isEmpty())
        deal3(); // again
    return pile->top();
}

void Klondike::restart() {
    kdDebug(11111) << "restart\n";
    deck->collectAndShuffle();
    deal();
}

void Klondike::deal3()
{
    int draw;

    if ( EasyRules) {
        draw = 1;
    } else {
        draw = 3;
    }

    if (deck->isEmpty())
    {
        redeal();
        return;
    }
    for (int flipped = 0; flipped < draw ; ++flipped) {

        Card *item = deck->nextCard();
        if (!item) {
            kdDebug(11111) << "deck empty!!!\n";
            return;
        }
        pile->add(item, true, false); // facedown, nospread
        // move back to flip
        item->move(deck->x(), deck->y());

        item->flipTo( int(pile->x()), int(pile->y()), 8 * (flipped + 1) );
    }

}

//  Add cards from  pile to deck, in reverse direction
void Klondike::redeal() {

    CardList pilecards = pile->cards();
    if (EasyRules)
        // the remaining cards in deck should be on top
        // of the new deck
        pilecards += deck->cards();

    for (CardList::Iterator it = pilecards.fromLast(); it != pilecards.end(); --it)
    {
        (*it)->setAnimated(false);
        deck->add(*it, true, false); // facedown, nospread
    }
}

void Klondike::deal() {
    for(int round=0; round < 7; round++)
        for (int i = round; i < 7; i++ )
            play[i]->add(deck->nextCard(), i != round, true);
}

void Klondike::cardClicked(Card *c) {
    kdDebug(11111) << "card clicked " << c->name() << endl;

    Dealer::cardClicked(c);
    if (c->source() == deck) {
        pileClicked(deck);
        return;
    }
}

void Klondike::pileClicked(Pile *c) {
    kdDebug(11111) << "pile clicked " << endl;
    Dealer::pileClicked(c);

    if (c == deck) {
        deal3();
    }
}

bool Klondike::startAutoDrop()
{
    bool pileempty = pile->isEmpty();
    if (!Dealer::startAutoDrop())
        return false;
    if (pile->isEmpty() && !pileempty)
        deal3();
    return true;
}

static class LocalDealerInfo0 : public DealerInfo
{
public:
    LocalDealerInfo0() : DealerInfo(I18N_NOOP("&Klondike"), 0) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Klondike(true, parent); }
} ldi0;

#include "klondike.moc"
