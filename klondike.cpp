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
#include <kmessagebox.h>
#include <card.h>
#include <kmainwindow.h>
#include <deck.h>
#include <pile.h>
#include <kdebug.h>

Klondike::Klondike( bool easy, KMainWindow* _parent, const char* _name )
  : Dealer( _parent, _name )
{
    deck = new Deck(13, this);
    deck->move(10, 10);

    EasyRules = easy;

    pile = new Pile( 0, this);
    pile->move(100, 10);
    pile->setAddFlags(Pile::disallow);
    pile->setRemoveFlags(Pile::Default);

    for( int i = 0; i < 7; i++ ) {
        play[ i ] = new Pile( i + 5, this);
        play[i]->move(10 + 85 * i, 130);
        play[i]->setAddFlags(Pile::addSpread | Pile::several);
        play[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop | Pile::wholeColumn);
        play[i]->setAddFun(&altStep);
    }

    for( int i = 0; i < 4; i++ ) {
        target[ i ] = new Pile( i + 1, this );
        target[i]->move(265 + i * 85, 10);
        target[i]->setAddFlags(Pile::Default);
        target[i]->setRemoveFlags(Pile::disallow);
        target[i]->setAddFun(&step1);
    }

    deal();
}

void Klondike::changeDiffLevel( int l ) {
    if ( EasyRules == (l == 0) )
        return;

    int r = KMessageBox::warningContinueCancel(this,
                                               i18n("This will end the current game.\n"
                                                    "Are you sure you want to do this?"),
                                               QString::null,
                                               i18n("OK"));
    if(r == KMessageBox::Cancel) {
        // TODO cb->setCurrentItem(1-cb->currentItem());
        return;
    }

    EasyRules = (bool)(l == 0);
    restart();
}

void Klondike::show() {
    QWidget::show();

    deck->show();

    pile->show();

    for(int i = 0; i < 4; i++)
        target[i]->show();

    for(int i = 0; i < 7; i++)
        play[i]->show();
}

void Klondike::restart() {
    kdDebug() << "restart\n";
    deck->collectAndShuffle();
    deal();
}

Klondike::~Klondike() {

    delete pile;

    for(int i=0; i<4; i++)
        delete target[i];

    for(int i=0; i<7; i++)
        delete play[i];

    delete deck;
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
            kdDebug() << "deck empty!!!\n";
            return;
        }
        pile->add(item, true, false); // facedown, nospread
        // move back to flip
        item->move(deck->x(), deck->y());

        item->flipTo( pile->x(), pile->y(), 8 * (flipped + 1) );
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
        deck->add(*it, true, false); // facedown, nospread
    }

}

void Klondike::deal() {
    for(int round=0; round < 7; round++)
        for (int i = round; i < 7; i++ )
            play[i]->add(deck->nextCard(), i != round, true);
    canvas()->update();
}

bool Klondike::step1( const Pile* c1, const CardList& c2 ) {

    if (c2.isEmpty()) {
        return false;
    }
    Card *top = c1->top();

    Card *newone = c2.first();
    if (!top) {
        return (newone->value() == Card::Ace);
    }

    bool t = ((newone->value() == top->value() + 1)
               && (top->suit() == newone->suit()));
    return t;
}

bool Klondike::altStep(  const Pile* c1, const CardList& c2 )
{
    if (c2.isEmpty()) {
        return false;
    }
    Card *top = c1->top();

    Card *newone = c2.first();
    if (!top) {
        return (newone->value() == Card::King);
    }

    bool t = ((newone->value() == top->value() - 1)
               && (top->isRed() != newone->isRed()));
    return t;
}

void Klondike::cardClicked(Card *c) {

    Dealer::cardClicked(c);
    if (c->source() == deck) {
        pileClicked(deck);
        return;
    }

}
void Klondike::pileClicked(Pile *c) {
    Dealer::pileClicked(c);

    if (c == deck) {
        deal3();
    }
}

void Klondike::cardDblClicked(Card *c) {
    Dealer::cardDblClicked(c);

    if (c == c->source()->top() && c->isFaceUp()) {
        CardList empty;
        empty.append(c);

        for (int j = 0; j < 4; j++)
        {
            if (step1(target[j], empty)) {
                c->source()->moveCards(empty, target[j]);
                canvas()->update();
                break;
            }
        }
    }
}

bool Klondike::isGameWon() const
{
    if (!deck->isEmpty() || !pile->isEmpty())
        return false;
    for (int i = 0; i < 7; i++)
        if (!play[i]->isEmpty())
            return false;
    return true;
}

static class LocalDealerInfo0 : public DealerInfo
{
public:
    LocalDealerInfo0() : DealerInfo(I18N_NOOP("&Klondike"), 0) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Klondike(true, parent); }
} gfi;

#include "klondike.moc"
