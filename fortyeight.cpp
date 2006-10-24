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
#include "fortyeight.h"
#include <klocale.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "cardmaps.h"

HorLeftPile::HorLeftPile( int _index, DealerScene* parent)
    : Pile(_index, parent)
{
    // TODO: create a pile that moves the cards together when filling space
    setHSpread( 4 );
}

QSizeF HorLeftPile::cardOffset( bool _spread, bool, const Card *) const
{
    if (_spread)
        return QSizeF(-hspread(), 0);

    return QSizeF(0, 0);
}

void HorLeftPile::initSizes()
{
    Pile::initSizes();
    setHSpread( 4 );
}


Fortyeight::Fortyeight( )
    : DealerScene()
{
    Deck::create_deck(this, 2);

    const qreal dist_x = 11.1;

    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    Deck::deck()->setPilePos( -2, -3);
    Deck::deck()->setZValue(20);

    pile = new HorLeftPile(20, this);
    pile->setAddFlags(Pile::addSpread | Pile::disallow);
    pile->setPilePos( -13.1, -3 );

    for (int i = 0; i < 8; i++) {

        target[i] = new Pile(9 + i, this);
        target[i]->setPilePos(1 + dist_x*i, 1);
        target[i]->setType(Pile::KlondikeTarget);

        stack[i] = new Pile(1 + i, this);
        stack[i]->setPilePos(1 + dist_x*i, 12 );
        stack[i]->setAddFlags(Pile::addSpread);
        stack[i]->setRemoveFlags(Pile::autoTurnTop);
        stack[i]->setCheckIndex(1);
        stack[i]->setSpread(stack[i]->spread() * 3 / 4);
    }

    Dealer::instance()->setActions(Dealer::Hint | Dealer::Demo);
}

//-------------------------------------------------------------------------//

void Fortyeight::restart()
{
    lastdeal = false;
    Deck::deck()->collectAndShuffle();
    deal();
}

void Fortyeight::deckClicked(Card *)
{
    if (Deck::deck()->isEmpty()) {
        if (lastdeal)
            return;
        lastdeal = true;
        while (!pile->isEmpty()) {
            Card *c = pile->at(pile->cardsLeft()-1);
            c->stopAnimation();
            Deck::deck()->add(c, true);
        }
    }
    Card *c = Deck::deck()->nextCard();
    pile->add(c, true);
    qreal x = c->x();
    qreal y = c->y();
    c->stopAnimation();
    c->setPos(Deck::deck()->x(), Deck::deck()->y());
    c->flipTo(x, y);
}

Card *Fortyeight::demoNewCards()
{
    if (Deck::deck()->isEmpty() && lastdeal)
        return 0;
    deckClicked(0);
    return pile->top();
}

bool Fortyeight::checkAdd(int, const Pile *c1, const CardList &c2) const
{
    if (c1->isEmpty())
        return true;

    // ok if in sequence, same suit
    return (c1->top()->suit() == c2.first()->suit())
       && (c1->top()->rank() == (c2.first()->rank()+1));
}

void Fortyeight::deal()
{
    for (int r = 0; r < 4; r++)
    {
        for (int column = 0; column < 8; column++)
        {
            if (false) { // doesn't look
                stack[column]->add(Deck::deck()->nextCard(), true);
                stack[column]->top()->turn(true);
            } else {
                stack[column]->add(Deck::deck()->nextCard(), false);
            }
        }
    }
    pile->add(Deck::deck()->nextCard(), false);
}

QString Fortyeight::getGameState() const
{
    return QString::number(lastdeal);
}

void Fortyeight::setGameState( const QString &s )
{
    lastdeal = s.toInt();
}

bool Fortyeight::isGameLost() const
{
    kDebug(11111) << "isGameLost ?" << endl;
    if(!lastdeal)
	return false;
    if(!Deck::deck()->isEmpty())
	return false;

    Card *c;
    for(int i=0; i < 8; i++)
    {
        if(stack[i]->isEmpty())
            return false;

        c=stack[i]->top();

        if(c->rank() == Card::Ace)
            return false;

        if(!pile->isEmpty()) {
            if(pile->top()->suit() == c->suit() &&
               pile->top()->rank()+1 == c->rank())
                return false;

            if ( !target[i]->isEmpty() &&
                 pile->top()->suit() == target[i]->top()->suit() &&
                 pile->top()->rank() == target[i]->top()->rank()+1)
                return false;
        }
        for(int j=0; j <8;j++){
            if(target[j]->isEmpty())
                continue;
            if(c->suit() == target[j]->top()->suit() &&
               c->rank()-1 ==target[j]->top()->rank())
                return false;
        }
        for(int j=1; j < 8; j++) {
            int k=(i+j) % 8;
            if (stack[k]->isEmpty())
                continue;
            if(c->suit() == stack[k]->top()->suit() &&
               c->rank()+1 ==stack[k]->top()->rank()){
                int indexi=stack[i]->indexOf(c);
                if(indexi==0)
                    return false;
                Card *c2=stack[i]->at(indexi-1);
                if(c2->rank()!=stack[k]->top()->rank() ||
                   c2->suit()!=stack[k]->top()->suit())
                    return false;
            }
        }
    }

    return true;
}

static class LocalDealerInfo8 : public DealerInfo
{
public:
    LocalDealerInfo8() : DealerInfo(I18N_NOOP("Forty && &Eight"), 8) {}
    virtual DealerScene *createGame() { return new Fortyeight(); }
} ldi9;

//-------------------------------------------------------------------------//

#include "fortyeight.moc"

//-------------------------------------------------------------------------//

