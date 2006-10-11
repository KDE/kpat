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
#include "deck.h"
#include <kdebug.h>
#include <assert.h>
#include "cardmaps.h"

class KlondikePile : public Pile
{
public:
    KlondikePile( int _index, Dealer* parent)
        : Pile(_index, parent) {}

    void clearSpread() { cardlist.clear(); }

    void addSpread(Card *c) {
        cardlist.append(c);
    }
    virtual QSize cardOffset( bool _spread, bool, const Card *c) const {
        kdDebug(11111) << "cardOffset " << _spread << " " << (c? c->name() : "(null)") << endl;
        if (cardlist.contains(const_cast<Card * const>(c)))
            return QSize(+dspread(), 0);
        return QSize(0, 0);
    }
private:
    CardList cardlist;
};

Klondike::Klondike( bool easy, KMainWindow* parent, const char* _name )
  : Dealer( parent, _name )
{
    // The units of the follwoing constants are pixels
    const int margin = 10; // between card piles and board edge
    const int hspacing = cardMap::CARDX() / 6 + 1; // horizontal spacing between card piles
    const int vspacing = cardMap::CARDY() / 4; // vertical spacing between card piles

    deck = Deck::new_deck(this);
    deck->move(margin, margin);

    EasyRules = easy;

    pile = new KlondikePile( 13, this);

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

    redealt = false;
}

//  This function returns true when it is certain that the card t is no longer
//  needed on any of the play piles. This function is recursive but the
//  recursion will not get deep.
//
//  To determine wether a card is no longer needed on any of the play piles we
//  obviously must know what a card can be used for there. According to the
//  rules a card can be used to store another card with 1 less unit of value
//  and opposite color. This is the only thing that a card can be used for
//  there. Therefore the cards with lowest value (1) are useless there (base
//  case). The other cards each have 2 cards that can be stored on them, let us
//  call those 2 cards *depending cards*.
//
//  The object of the game is to put all cards on the target piles. Therefore
//  cards that are no longer needed on any of the play piles should be put on
//  the target piles if possible. Cards on the target piles can not be moved
//  and they can not store any of its depending cards. Let us call this that
//  the cards on the target piles are *out of play*.
//
//  The simple and obvious rule is:
//    A card is no longer needed when both of its depending cards are out of
//    play.
//
//  But using only the simplest rule to determine if a card is no longer
//  needed on any of the play piles is not ambitios enough. Therefore, if a
//  depending card is not out of play, we test if it could become out of play.
//  The requirement for getting a card out of play is that it can be placed on
//  a target pile and that it is no longer needed on any of the play piles
//  (this is why this function is recursive). This more ambitious rule lets
//  us extend the base case with the second lowest value (2).
bool Klondike::noLongerNeeded(Card::Rank r, Card::Suit s) {

    if (r <= Card::Two) return true; //  Base case.

    //  Find the 2 suits of opposite color. "- 1" is used here because the
    //  siuts are ranged 1 .. 4 but target_tops is indexed 0 .. 3. (Of course
    //  the subtraction of 1 does not affect performance because it is a
    //  constant expression that is calculated at compile time).
    unsigned char a = Card::Clubs - 1, b = Card::Spades - 1;
    if (s == Card::Clubs || s == Card::Spades)
        a = Card::Diamonds - 1, b = Card::Hearts - 1;

    const Card::Rank depending_rank = static_cast<Card::Rank>(r - 1);
    return
      (((target_tops[a] >= depending_rank)
        ||
        ((target_tops[a] >= depending_rank - 1)
         &&
         (noLongerNeeded
              (depending_rank, static_cast<Card::Suit>(a + 1)))))
       &&
       ((target_tops[b] >= depending_rank)
        ||
        ((target_tops[b] >= depending_rank - 1)
         &&
         (noLongerNeeded
              (depending_rank, static_cast<Card::Suit>(b + 1))))));
}

bool Klondike::tryToDrop(Card *t)
{
    if (!t || !t->realFace() || t->takenDown())
        return false;

//    kdDebug(11111) << "tryToDrop " << t->name() << endl;

    Pile *tgt = findTarget(t);
    if (tgt) {
        newHint
            (new MoveHint(t, tgt, noLongerNeeded(t->rank(), t->suit())));
        return true;
    }
    return false;
}

void Klondike::getHints() {

    target_tops[0] = target_tops[1] = target_tops[2] = target_tops[3]
        = Card::None;

    for( int i = 0; i < 4; i++ )
    {
        Card *c = target[i]->top();
        if (!c) continue;
        target_tops[c->suit() - 1] = c->rank();
    }


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
                    if (((*it)->rank() != Card::King) || it != list.begin()) {
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
    redealt = false;
    deal();
}

void Klondike::deal3()
{
    int draw;

    if ( EasyRules ) {
        draw = 1;
    } else {
        draw = 3;
    }

    pile->clearSpread();

    if (deck->isEmpty())
    {
        redeal();
        return;
    }

    // move the cards back on the deck, so we can have three new
    for (int i = 0; i < pile->cardsLeft(); ++i) {
        pile->at(i)->move(pile->x(), pile->y());
    }

    for (int flipped = 0; flipped < draw ; ++flipped) {

        Card *item = deck->nextCard();
        if (!item) {
            kdDebug(11111) << "deck empty!!!\n";
            return;
        }
        pile->add(item, true, true); // facedown, nospread
        if (flipped < draw - 1)
            pile->addSpread(item);
        // move back to flip
        item->move(deck->x(), deck->y());

        item->flipTo( int(pile->x()) + pile->dspread() * (flipped), int(pile->y()), 8 * (flipped + 1) );
    }

}

//  Add cards from  pile to deck, in reverse direction
void Klondike::redeal() {

    CardList pilecards = pile->cards();
    if (EasyRules)
        // the remaining cards in deck should be on top
        // of the new deck
        pilecards += deck->cards();

    for (int count = pilecards.count() - 1; count >= 0; --count)
    {
        Card *card = pilecards[count];
        card->setAnimated(false);
        deck->add(card, true, false); // facedown, nospread
    }

    redealt = true;
}

void Klondike::deal() {
    for(int round=0; round < 7; round++)
        for (int i = round; i < 7; i++ )
            play[i]->add(deck->nextCard(), i != round, true);
}

bool Klondike::cardClicked(Card *c) {
    kdDebug(11111) << "card clicked " << c->name() << endl;

    if (Dealer::cardClicked(c))
        return true;

    if (c->source() == deck) {
        pileClicked(deck);
        return true;
    }

    return false;
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


bool Klondike::isGameLost() const
{
    kdDebug( 11111 ) << "Is the game lost?" << endl;

    if (!deck->isEmpty()) {
        kdDebug( 11111 ) << "We should only check this when the deck is exhausted." << endl;
        return false;
    }

    // Check whether top of the pile can be added to any of the target piles.
    if ( !pile->isEmpty() ) {
        for ( int i = 0; i < 4; ++i ) {
            if ( target[ i ]->isEmpty() ) {
                continue;
            }
            if ( pile->top()->suit() == target[ i ]->top()->suit() &&
                 pile->top()->rank() - 1 == target[ i ]->top()->rank() ) {
                kdDebug( 11111 ) << "No, the source pile's top card could be added to target pile " << i << endl;
                return false;
            }
        }
    }

    // Create a card list - srcPileCards - that contains all accessible
    // cards in the pile and the deck.
    CardList srcPileCards;
    if ( EasyRules ) {
        srcPileCards = pile->cards();
    } else {
        /* In the draw3 mode, not every card in the source pile is
         * accessible, but only every third one.
         */
        for ( unsigned int i = 2; i < pile->cards().count(); i += 3 ) {
	    kdDebug( 11111 ) << "Found card "<< pile->cards()[i]->name()<< endl;
            srcPileCards += pile->cards()[ i ];
        }
        if ( !pile->cards().isEmpty() && pile->cards().count() % 3 != 0 ) {
	    kdDebug( 11111 ) << "Found last card "<< pile->cards()[pile->cards().count() - 1]->name()<< endl;
            srcPileCards += pile->cards()[ pile->cards().count() - 1 ];
        }
    }

    //  Check all seven stores
    for ( int i = 0; i < 7; ++i ) {

        // If this store is empty...
        if ( play[ i ]->isEmpty() ) {
            // ...check whether the pile contains a king we could move here.
            CardList::ConstIterator it  = srcPileCards.begin();
            CardList::ConstIterator end = srcPileCards.end();
            for ( ; it != end; ++it ) {
                if ( ( *it )->rank() == Card::King ) {
                    kdDebug( 11111 ) << "No, the pile contains a king which we could move onto store " << i << endl;
                    return false;
                }
            }

            // ...check whether any of the other stores contains a (visible)
            // king we could move here.
            for ( int j = 0; j < 7; ++j ) {
                if ( j == i || play[ j ]->isEmpty() ) {
                    continue;
                }
                const CardList cards = play[ j ]->cards();
                CardList::ConstIterator it = ++cards.begin();
                CardList::ConstIterator end = cards.end();
                for ( ; it != end; ++it ) {
                    if ( ( *it )->realFace() && ( *it )->rank() == Card::King ) {
                        kdDebug( 11111 ) << "No, store " << j << " contains a visible king which we could move onto store " << i << endl;
                        return false;
                    }
                }
            }
        } else { // This store is not empty...
            Card *topCard = play[ i ]->top();

            // ...check whether the top card is an Ace (we can start a target)
            if ( topCard->rank() == Card::Ace ) {
                kdDebug( 11111 ) << "No, store " << i << " has an Ace, we could start a target pile." << endl;
                return false;
            }

            // ...check whether the top card can be added to any target pile
            for ( int targetIdx = 0; targetIdx < 4; ++targetIdx ) {
                if ( target[ targetIdx ]->isEmpty() ) {
                    continue;
                }
                if ( target[ targetIdx ]->top()->suit() == topCard->suit() &&
                     target[ targetIdx ]->top()->rank() == topCard->rank() - 1 ) {
                    kdDebug( 11111 ) << "No, store " << i << "'s top card could be added to target pile " << targetIdx << endl;
                    return false;
                }
            }

            // ...check whether the source pile contains a card which can be
            // put onto this store.
            CardList::ConstIterator it = srcPileCards.begin();
            CardList::ConstIterator end = srcPileCards.end();
            for ( ; it != end; ++it ) {
                if ( ( *it )->isRed() != topCard->isRed() &&
                     ( *it )->rank() == topCard->rank() - 1 ) {
                    kdDebug( 11111 ) << "No, the pile contains a card which we could add to store " << i << endl;
                    return false;
                }
            }

            // ...check whether any of the other stores contains a visible card
            // which can be put onto this store, and which is on top of an
            // uncovered card.
            for ( int j = 0; j < 7; ++j ) {
                if ( j == i ) {
                    continue;
                }
                const CardList cards = play[ j ]->cards();
                CardList::ConstIterator it = cards.begin();
                CardList::ConstIterator end = cards.end();
                for ( ; it != end; ++it ) {
                    if ( ( *it )->realFace() &&
                         ( *it )->isRed() != topCard->isRed() &&
                         ( *it )->rank() == topCard->rank() - 1 ) {
                        kdDebug( 11111 ) << "No, store " << j << " contains a card which we could add to store " << i << endl;
                        return false;
                    }
                }
            }
        }
    }
    kdDebug( 11111 ) << "Yep, all hope is lost." << endl;
    return true;
}

static class LocalDealerInfo0 : public DealerInfo
{
public:
    LocalDealerInfo0() : DealerInfo(I18N_NOOP("&Klondike"), 0) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Klondike(true, parent); }
} ldi0;

static class LocalDealerInfo14 : public DealerInfo
{
public:
    LocalDealerInfo14() : DealerInfo(I18N_NOOP("Klondike (&draw 3)"), 13) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Klondike(false, parent); }
} ldi14;


#include "klondike.moc"
