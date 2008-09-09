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
#include <patsolve/fortyeight.h>

HorLeftPile::HorLeftPile( int _index, DealerScene* parent)
    : Pile(_index, parent)
{
}

QSizeF HorLeftPile::cardOffset( const Card *) const
{
    return QSizeF(-2.1, 0);
}

void HorLeftPile::relayoutCards()
{
    // this is just too heavy to animate, so we stop it
    Pile::relayoutCards();
    for ( CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it )
    {
        if ( *it != top() )
            ( *it )->stopAnimation();
    }
}


Fortyeight::Fortyeight( )
    : DealerScene()
{
    Deck::create_deck(this, 2);

    const qreal dist_x = 11.1;

    connect(Deck::deck(), SIGNAL(pressed(Card*)), SLOT(deckPressed(Card*)));
    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    Deck::deck()->setPilePos( -2, -1);
    Deck::deck()->setZValue(20);

    pile = new HorLeftPile(20, this);
    pile->setAddFlags(Pile::addSpread | Pile::disallow);
    pile->setPilePos( -13.1, -1 );
    pile->setObjectName( "pile" );
    pile->setReservedSpace( QSizeF( -50, 10 ) );

    for (int i = 0; i < 8; i++) {

        target[i] = new Pile(9 + i, this);
        target[i]->setPilePos(1 + dist_x*i, 1);
        target[i]->setType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );

        stack[i] = new Pile(1 + i, this);
        stack[i]->setPilePos(1 + dist_x*i, 12 );
        stack[i]->setAddFlags(Pile::addSpread);
        stack[i]->setRemoveFlags(Pile::autoTurnTop);
        stack[i]->setCheckIndex(1);
        stack[i]->setSpread(stack[i]->spread() * 3 / 4);
        stack[i]->setObjectName( QString( "stack%1" ).arg( i ) );
        stack[i]->setReservedSpace( QSizeF( 10, 40 ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new FortyeightSolver( this ) );
}

//-------------------------------------------------------------------------//

void Fortyeight::restart()
{
    lastdeal = false;
    Deck::deck()->collectAndShuffle();
    deal();
}

void Fortyeight::deckClicked( Card * )
{
    //kDebug() << "deckClicked" << c->name() << " " << pile->top()->name() << " " << pile->top()->animated();
    if ( pile->top() && pile->top()->animated() )
        return;
    if ( Deck::deck()->isEmpty())
        deckPressed( 0 );
}

void Fortyeight::deckPressed(Card *c2)
{
    if( c2 )
        kDebug(11111) << gettime() << "deckPressed" << c2->rank() << " " << c2->suit();
    else
        kDebug(11111) << gettime() << "deckPressed" << "(nil)";
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
    c->stopAnimation();
    qreal x = c->realX();
    qreal y = c->realY();
    c->setPos( Deck::deck()->pos() );
    c->flipTo(x, y);
    takeState();
}

Card *Fortyeight::demoNewCards()
{
    if (Deck::deck()->isEmpty() && lastdeal)
        return 0;
    deckPressed(0);
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

static class LocalDealerInfo8 : public DealerInfo
{
public:
    LocalDealerInfo8() : DealerInfo(I18N_NOOP("Forty && Eight"), 8) {}
    virtual DealerScene *createGame() { return new Fortyeight(); }
} ldi9;

//-------------------------------------------------------------------------//

#include "fortyeight.moc"

//-------------------------------------------------------------------------//

