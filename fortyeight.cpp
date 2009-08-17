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

#include "deck.h"
#include "speeds.h"
#include "patsolve/fortyeight.h"

#include <KDebug>
#include <KLocale>


HorLeftPile::HorLeftPile( int _index, DealerScene* parent)
    : Pile(_index, parent)
{
}

QSizeF HorLeftPile::cardOffset( const Card *) const
{
    return QSizeF(-0.21, 0);
}

void HorLeftPile::relayoutCards()
{
    // this is just too heavy to animate, so we stop it
    Pile::relayoutCards();
    foreach ( Card *c, m_cards )
    {
        if ( c != top() )
            c->stopAnimation();
    }
}


Fortyeight::Fortyeight( )
    : DealerScene()
{
    const qreal dist_x = 1.11;
    const qreal smallNeg = -1e-6;

    Deck::createDeck(this, 2);
    connect(Deck::deck(), SIGNAL(pressed(Card*)), SLOT(newCards()));
    connect(Deck::deck(), SIGNAL(clicked(Card*)), SLOT(deckClicked(Card*)));
    Deck::deck()->setPilePos(smallNeg, smallNeg);
    Deck::deck()->setZValue(20);

    pile = new HorLeftPile(20, this);
    pile->setAddFlags(Pile::addSpread | Pile::disallow);
    pile->setPilePos(-dist_x, smallNeg);
    pile->setObjectName( "pile" );
    pile->setReservedSpace( QSizeF( -(1 + 6 * dist_x), 1 ) );

    for (int i = 0; i < 8; i++) {

        target[i] = new Pile(9 + i, this);
        target[i]->setPilePos(dist_x*i, 0);
        target[i]->setType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );

        stack[i] = new Pile(1 + i, this);
        stack[i]->setPilePos(dist_x*i, 1.1 );
        stack[i]->setAddFlags(Pile::addSpread);
        stack[i]->setRemoveFlags(Pile::autoTurnTop);
        stack[i]->setCheckIndex(1);
        stack[i]->setSpread(stack[i]->spread() * 3 / 4);
        stack[i]->setObjectName( QString( "stack%1" ).arg( i ) );
        stack[i]->setReservedSpace( QSizeF( 1.0, 4.0 ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new FortyeightSolver( this ) );
}

//-------------------------------------------------------------------------//

void Fortyeight::restart()
{
    lastdeal = false;
    Deck::deck()->collectAndShuffle();
    deal();
    emit newCardsPossible( true );
}

void Fortyeight::deckClicked( Card * )
{
    //kDebug(11111) << "deckClicked" << c->name() << " " << pile->top()->name() << " " << pile->top()->animated();
    if ( pile->top() && pile->top()->animated() )
        return;
    if ( Deck::deck()->isEmpty())
        newCards();
}

Card *Fortyeight::newCards()
{
    if (Deck::deck()->isEmpty() && lastdeal)
        return 0;

    if (pile->top() && pile->top()->animated())
        return pile->top();

    if (Deck::deck()->isEmpty())
    {
        lastdeal = true;
        while (!pile->isEmpty())
        {
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
    c->flipTo(x, y, DURATION_FLIP );

    takeState();
    considerGameStarted();
    if ( Deck::deck()->isEmpty() && lastdeal )
        emit newCardsPossible( false );

    return c;
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

QString Fortyeight::getGameState()
{
    return QString::number(lastdeal);
}

void Fortyeight::setGameState( const QString &s )
{
    lastdeal = s.toInt();
    emit newCardsPossible( !lastdeal || !Deck::deck()->isEmpty() );
}

static class LocalDealerInfo8 : public DealerInfo
{
public:
    LocalDealerInfo8() : DealerInfo(I18N_NOOP("Forty & Eight"), 8) {}
    virtual DealerScene *createGame() const { return new Fortyeight(); }
} ldi9;

//-------------------------------------------------------------------------//

#include "fortyeight.moc"

//-------------------------------------------------------------------------//

