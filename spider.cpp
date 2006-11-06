/*---------------------------------------------------------------------------

  spider.cpp  implements a patience card game

     Copyright (C) 2003  Josh Metzler

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

#include "spider.h"
#include "cardmaps.h"
#include <klocale.h>
#include "deck.h"
#include <kdebug.h>
#include "patsolve/spider.h"

void SpiderPile::moveCards(CardList &c, Pile *to)
{
    Pile::moveCards(c, to);

    // if this is a leg pile, don't do anything special
    if ( to->checkIndex() == 0 )
        return;

    // if the top card of the list I just moved is an Ace,
    // the run I just moved is the same suit as the pile,
    // and the destination pile now has more than 12 cards,
    // then it could have a full deck that needs removed.
    if (c.last()->rank() == Card::Ace &&
        c.first()->suit() == to->top()->suit() &&
        to->cardsLeft() > 12) {
            Spider *b = dynamic_cast<Spider*>(dscene());
            if (b) {
                b->checkPileDeck(to);
            }
        }
}

//-------------------------------------------------------------------------//

Spider::Spider(int suits)
    : DealerScene(), m_leg(0), m_redeal(0)
{
    const qreal dist_x = 11.2;

    Deck::create_deck(this, 2, suits);

    // I deal the cards into 'redeal' piles, so hide the deck
    Deck::deck()->setVisible(false);

    // Dealing the cards out into 5 piles so the user can see how many
    // sets of 10 cards are left to be dealt out
    for( int column = 0; column < 5; column++ ) {
        redeals[column] = new Pile(column + 1, this);
        redeals[column]->setPilePos( 0 - dist_x / 3 * ( 5 - column ), -1);
        redeals[column]->setZValue(12 * ( 5-column ));
        redeals[column]->setCheckIndex(0);
        redeals[column]->setAddFlags(Pile::disallow);
        redeals[column]->setRemoveFlags(Pile::disallow);
        redeals[column]->setObjectName( QString( "redeals%1" ).arg( column ) );
        connect(redeals[column], SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    }
    redeals[0]->setReservedSpace( QSizeF( -50, 10 ) );

    // The 10 playing piles
    for( int column = 0; column < 10; column++ ) {
        stack[column] = new SpiderPile(column + 6, this);
        stack[column]->setPilePos(1 + dist_x * column, 1);
        stack[column]->setZValue(20);
        stack[column]->setCheckIndex(1);
        stack[column]->setAddFlags(Pile::addSpread | Pile::several);
        stack[column]->setRemoveFlags(Pile::several |
                                      Pile::autoTurnTop | Pile::wholeColumn);
        stack[column]->setObjectName( QString( "stack%1" ).arg( column ) );
        stack[column]->setReservedSpace( QSizeF( 10, 20 ) );
    }

    // The 8 'legs' so named by me because spiders have 8 legs - why
    // else the name Spider?
    for( int column = 0; column < 8; column++ ) {
        legs[column] = new Pile(column + 16, this);
        legs[column]->setPilePos(1 + dist_x / 3 * column, -1);
        legs[column]->setZValue(column+1);
        legs[column]->setCheckIndex(0);
        legs[column]->setAddFlags(Pile::disallow);
        legs[column]->setRemoveFlags(Pile::disallow);
        legs[column]->setTarget(true);
        legs[column]->setVisible( false );
        legs[column]->setZValue(14 * column);
        legs[column]->setObjectName( QString( "legs%1" ).arg( column ) );
    }

    // Moving an A-K run to a leg is not really an autoDrop - the
    // user should have no choice.  Also, it must be moved A first, ...
    // up to K so the King will be on top.
    setAutoDropEnabled(false);
    setActions(DealerScene::Hint | DealerScene::Demo );
    setSolver( new SpiderSolver( this ) );
}

void Spider::cardStoped(Card * t)
{
    for( int column = 0; column < 10; column++ ) {
        Card *t = stack[column]->top();
        if (t && !t->isFaceUp())
           t->flipTo(t->x(), t->y());
    }
    t->disconnect(this, SLOT( cardStoped( Card* ) ) );
    setWaiting( false );
}

//-------------------------------------------------------------------------//

bool Spider::checkAdd(int /*checkIndex*/, const Pile *c1, const CardList& c2) const
{
    // assuming the cardlist is a valid unit, since I allowed
    // it to be removed - can drop any card on empty pile or
    // on any suit card of one higher rank
    if (c1->isEmpty() || c1->top()->rank() == c2.first()->rank()+1)
        return true;

    return false;
}

bool Spider::checkRemove(int /*checkIndex*/, const Pile *p, const Card *c) const
{
    // if the pile from c up is decreasing by 1 and all the same suit, ok
    // note that this is true if c is the top card
    const Card *before;
    int index = p->indexOf(c);
    while (c != p->top()) {
        before = c;
        c = p->at(++index);
        if (before->suit() != c->suit() || before->rank() != c->rank()+1)
            return false;
    }
    return true;
}

void Spider::getHints()
{
    // first, get runs from each stack
    CardList cl[10];

    Pile* empty = NULL;
    for (int column = 0; column < 10; column++) {
        if (stack[column]->isEmpty())
           empty = stack[column];
        else
            cl[column] = getRun(stack[column]->top());
    }

    // if I can build a run from Ace->King in one suit then
    // hint those moves
    HintList hl;
    for (int s = Card::Clubs; s <= Card::Spades; s++) {
        bool bGrowing = true;
        int vTopNew = 0;
        int colNew = -1;
        while (bGrowing && vTopNew < 13) {
            bGrowing = false;
            int col = colNew;
            int vTop = vTopNew;
            for (int column = 0; column < 10; column++) {
                if (cl[column].isEmpty() || col == column)
                    continue;
                if (cl[column].last()->suit() == s &&
                    cl[column].last()->rank() <= vTop+1 &&
                    cl[column].first()->rank() > vTop)
                {
                    bGrowing = true;
                    if (cl[column].first()->rank() > vTopNew) {
                        colNew = column;
                        vTopNew = cl[column].first()->rank();
                    }
                }
            }
            if (bGrowing && vTop)
                hl.append(new MoveHint(cl[col][vTop-
                   cl[colNew].last()->rank()+1], stack[colNew]));
        }
        if (vTopNew == 13)
            hints += hl;
        else
            for (HintList::Iterator it = hl.begin(); it != hl.end(); ++it)
                delete *it;
        hl.clear();
    }

    // now check to see if a run from one column can go on the end
    // of a run from another stack
    for (int column = 0; column < 10; column++) {
        if (cl[column].isEmpty())
            continue;

        // if there is an empty column and this stack is on
        // another card, hint
        if (empty && cl[column].count() < stack[column]->cardsLeft()) {
            newHint(new MoveHint(cl[column].first(), empty));
            continue;
        }

        // now see if I can move this stack to any other column
        for (int c2 = 0; c2 < 10; c2++) {
            if (c2 == column || cl[c2].isEmpty())
                continue;

            if (cl[c2].last()->rank() == cl[column].first()->rank()+1)
            {
                // I can hint this move - should I?
                int index = stack[column]->indexOf(cl[column].first());

                // if target pile is the same suit as this card,
                // or if there are no cards under this one,
                // or if it couldn't move to where it is now,
                // or if the card under this one is face down, hint
                if (cl[c2].last()->suit() == cl[column].first()->suit() ||
                    index == 0 || stack[column]->at(index-1)->rank() !=
                    cl[column].first()->rank()+1 ||
                    !(stack[column]->at(index-1)->realFace()))
                        newHint(new MoveHint(cl[column].first(), stack[c2]));
            }
        }
    }
}

MoveHint *Spider::chooseHint()
{
    kDebug(11111) << "choose 1 of " << hints.count() << " hints" << endl;
    if (hints.isEmpty())
        return 0;

    // first, choose a card that is moving to the same suit
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it)
    {
        if (!(*it)->pile()->isEmpty() &&
            (*it)->pile()->top()->suit() == (*it)->card()->suit())
            return *it;
    }

    // second, choose a card that is moving from the base
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it)
    {
        if ((*it)->card()->source() &&
            (*it)->card()->source()->at(0) == (*it)->card())
            return *it;
    }

    // otherwise, go with a random hint
    return hints[randseq.getLong(hints.count())];
}

//-------------------------------------------------------------------------//

QString Spider::getGameState() const
{
    return QString::number(m_leg*10 + m_redeal);
}

void Spider::setGameState(const QString &stream)
{
    int i = stream.toInt();

    if (m_leg > i/10) {
        for (m_leg--; m_leg > i/10; m_leg--)
            legs[m_leg]->setVisible(false);
        legs[m_leg]->setVisible(false);
    } else
        for (; m_leg < i/10; m_leg++)
            legs[m_leg]->setVisible(true);

    if (m_redeal > i%10) {
        for (m_redeal--; m_redeal > i%10; m_redeal--)
            redeals[m_redeal]->setVisible(true);
        redeals[m_redeal]->setVisible(true);
    } else
        for (; m_redeal < i%10; m_redeal++)
            redeals[m_redeal]->setVisible(false);
}

//-------------------------------------------------------------------------//

void Spider::restart()
{
    Deck::deck()->collectAndShuffle();
    deal();
}

//-------------------------------------------------------------------------//

CardList Spider::getRun(Card *c) const
{
    CardList result;

    Pile *p = c->source();
    if (!p || p->isEmpty())
        return result;

    result.append(c);

    Card::Suit s = c->suit();
    int v = c->rank();

    int index = p->indexOf(c);
    c = p->at(--index);
    while (index >= 0 && c->realFace() &&
        c->suit() == s && c->rank() == ++v)
    {
        result.prepend(c);
        c = p->at(--index);
    }

    return result;
}

void Spider::checkPileDeck(Pile *check)
{
    kDebug(11111) << "check for run" << endl;
    if (check->isEmpty())
        return;

    if (check->top()->rank() == Card::Ace) {
        // just using the CardList to see if this goes to King
        CardList run = getRun(check->top());
        if (run.first()->rank() == Card::King) {
            legs[m_leg]->setVisible(true);

            CardList::iterator it = run.end();
            qreal z = 1;
            kDebug() << "z " << z << endl;
            while (it != run.begin()) {
                --it;
                kDebug() << "moving " << ( *it )->name() << endl;
                legs[m_leg]->add( *it );
                // ( *it )->setSpread( QSize( 0, 0 ) );
                ( *it )->moveTo( legs[m_leg]->x(), legs[m_leg]->y(), legs[m_leg]->zValue() + z, 300 + int( z ) * 30 );
                z += 1;
            }
            connect( run.first(), SIGNAL(stoped(Card*)), SLOT(cardStoped(Card*)));
            setWaiting( true );
            if ( demoActive() )
                newDemoMove( run.first() );

            m_leg++;
        }
    }
}

void Spider::dealRow()
{
    if (m_redeal > 4)
        return;

    for (int column = 0; column < 10; column++) {
        stack[column]->add(redeals[m_redeal]->top(), false);

        // I may put an Ace on a K->2 pile so it could need cleared.
        if (stack[column]->top()->rank() == Card::Ace)
            checkPileDeck(stack[column]);
    }

    redeals[m_redeal++]->setVisible(false);
}

//-------------------------------------------------------------------------//

void Spider::deal()
{
    unmarkAll();

    m_leg = 0;
    m_redeal = 0;

    int column = 0;
    // deal face down cards (5 to first 4 piles, 4 to last 6)
    for (int i = 0; i < 44; i++ ) {
        stack[column]->add(Deck::deck()->nextCard(), true);
        column = (column + 1) % 10;
    }
    // deal face up cards, one to each pile
    for (int i = 0; i < 10; i++ ) {
        stack[column]->add(Deck::deck()->nextCard(), false);
        column = (column + 1) % 10;
    }
    // deal the remaining cards into 5 'redeal' piles
    for (int column = 0; column < 5; column++ )
        for (int i = 0; i < 10; i++ )
            redeals[column]->add(Deck::deck()->nextCard(), true);

    // make the leg piles invisible
    for (int i = 0; i < 8; i++ )
        legs[i]->setVisible(false);
    // make the redeal piles visible
    for (int i = 0; i < 5; i++ )
        redeals[i]->setVisible(true);
}

Card *Spider::demoNewCards()
{
   if (m_leg > 4)
       return 0;
   deckClicked(0);
   return stack[0]->top();
}

void Spider::deckClicked(Card*)
{
    kDebug(11111) << "deck clicked " << m_redeal << endl;
    if (m_redeal > 4)
        return;

    unmarkAll();
    dealRow();
    takeState();
}

bool Spider::isGameLost() const
{
    kDebug(11111) << "isGameLost ?"<< endl;

    // if there are still cards to deal out, you have not lost
    if (m_redeal < 5)
        return false;

    // first, get runs from each stack - returning if empty
    CardList cl[10];

    for (int column = 0; column < 10; column++) {
        if (stack[column]->isEmpty())
           return false;
        cl[column] = getRun(stack[column]->top());
    }

    // from this point on, I know that none of the columns is empty
    // now check to see if a run from one column can go on the end
    // of a run from another stack
    for (int column = 0; column < 10; column++)
        for (int c2 = 0; c2 < 10; c2++) {
            if (c2 == column)
                continue;

            // if I can move this run to another pile, I'm not done
            if (cl[c2].last()->rank() == cl[column].first()->rank()+1)
                return false;
        }

    // if you can build a run from Ace->King in one suit then
    // you can clear it and keep playing
    for (int s = Card::Clubs; s <= Card::Spades; s++) {
        bool bGrowing = true;
        int vTop = 0;
        while (bGrowing && vTop < 13) {
            bGrowing = false;
            int column = 0;
            while (column < 10 && !bGrowing) {
                if (cl[column].last()->suit() == s &&
                    cl[column].last()->rank() <= vTop+1 &&
                    cl[column].first()->rank() > vTop)
                {
                    bGrowing = true;
                    vTop = cl[column].first()->rank();
                }
                column++;
            }
        }
        // if you can build such a pile, you can continue
        if (vTop == 13)
           return false;
    }

    kDebug() << "I think the game is lost! " << solver()->patsolve() << endl;
    return false;
    return true;
}

static class LocalDealerInfo15 : public DealerInfo
{
public:
    LocalDealerInfo15() : DealerInfo(I18N_NOOP("S&pider (Easy)"), 14) {}
    virtual DealerScene *createGame() { return new Spider(1); }
} ldi15;

static class LocalDealerInfo16 : public DealerInfo
{
public:
    LocalDealerInfo16() : DealerInfo(I18N_NOOP("Spider (&Medium)"), 15) {}
    virtual DealerScene *createGame() { return new Spider(2); }
} ldi16;

static class LocalDealerInfo17 : public DealerInfo
{
public:
    LocalDealerInfo17() : DealerInfo(I18N_NOOP("Spider (&Hard)"), 16) {}
    virtual DealerScene *createGame() { return new Spider(4); }
} ldi17;

//-------------------------------------------------------------------------//

#include "spider.moc"

//-------------------------------------------------------------------------//

