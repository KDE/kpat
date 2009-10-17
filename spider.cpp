/*
 * Copyright (C) 2003 Josh Metzler <joshdeb@metzlers.org>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of 
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "spider.h"

#include "carddeck.h"
#include "dealerinfo.h"
#include "speeds.h"
#include "version.h"
#include "patsolve/spidersolver.h"

#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KRandom>
#include <KSelectAction>


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

Spider::Spider()
    : DealerScene(), m_leg(0), m_redeal(0)
{
    const qreal dist_x = 1.12;
    const qreal smallNeg = -1e-6;

    KConfigGroup cg(KGlobal::config(), settings_group );
    m_suits = cg.readEntry( "SpiderSuits", 2);

    setupDeck();

    // Dealing the cards out into 5 piles so the user can see how many
    // sets of 10 cards are left to be dealt out
    for( int column = 0; column < 5; column++ ) {
        redeals[column] = new Pile(column + 1, QString( "redeals%1" ).arg( column ));
        redeals[column]->setPilePos(smallNeg - dist_x / 3 * ( 4 - column ), smallNeg);
        redeals[column]->setZValue(12 * ( 5-column ));
        redeals[column]->setCheckIndex(0);
        redeals[column]->setAddFlags(Pile::disallow);
        redeals[column]->setRemoveFlags(Pile::disallow);
        redeals[column]->setGraphicVisible( false );
        connect(redeals[column], SIGNAL(clicked(Card*)), SLOT(newCards()));
        addPile(redeals[column]);
    }

    // The 10 playing piles
    for( int column = 0; column < 10; column++ ) {
        stack[column] = new SpiderPile(column + 6, QString( "stack%1" ).arg( column ));
        stack[column]->setPilePos(dist_x * column, 0);
        stack[column]->setZValue(20);
        stack[column]->setCheckIndex(1);
        stack[column]->setAddFlags(Pile::addSpread | Pile::several);
        stack[column]->setRemoveFlags(Pile::several |
                                      Pile::autoTurnTop | Pile::wholeColumn);
        stack[column]->setReservedSpace( QSizeF( 1.0, 3.5 ) );
        addPile(stack[column]);
    }

    // The 8 'legs' so named by me because spiders have 8 legs - why
    // else the name Spider?
    for( int column = 0; column < 8; column++ ) {
        legs[column] = new Pile(column + 16, QString( "legs%1" ).arg( column ));
        legs[column]->setPilePos(dist_x / 3 * column, smallNeg);
        legs[column]->setZValue(column+1);
        legs[column]->setCheckIndex(0);
        legs[column]->setAddFlags(Pile::disallow);
        legs[column]->setRemoveFlags(Pile::disallow);
        legs[column]->setTarget(true);
        legs[column]->setGraphicVisible( false );
        legs[column]->setZValue(14 * column);
        addPile(legs[column]);
    }

    // Moving an A-K run to a leg is not really an autoDrop - the
    // user should have no choice.  Also, it must be moved A first, ...
    // up to K so the King will be on top.
    setAutoDropEnabled(false);
    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new SpiderSolver( this ) );

    options = new KSelectAction(i18n("Spider &Options"), this );
    options->addAction( i18n("1 Suit (Easy)") );
    options->addAction( i18n("2 Suits (Medium)") );
    options->addAction( i18n("4 Suits (Hard)") );
    if ( m_suits == 1 )
        options->setCurrentItem( 0 );
    else if ( m_suits == 2 )
        options->setCurrentItem( 1 );
    else
        options->setCurrentItem( 2 );
    connect( options, SIGNAL(triggered(int)), SLOT(gameTypeChanged()) );
}

QList<QAction*> Spider::configActions() const
{
    return QList<QAction*>() << options;
}

void Spider::gameTypeChanged()
{
    stopDemo();

    if ( allowedToStartNewGame() )
    {
        if ( options->currentItem() == 0 )
            setSuits( 1 );
        else if ( options->currentItem() == 1 )
            setSuits( 2 );
        else
            setSuits( 4 );
        startNew( gameNumber() );
    }
    else
    {
        // If we're not allowed, reset the option to
        // the current number of suits.
        if ( m_suits == 1 )
            options->setCurrentItem( 0 );
        else if ( m_suits == 2 )
            options->setCurrentItem( 1 );
        else
            options->setCurrentItem( 2 );
    }
}

void Spider::setSuits(int suits)
{
    if ( suits != m_suits )
    {
        m_suits = suits;

        stopDemo();
        unmarkAll();

        setupDeck();

        KConfigGroup cg(KGlobal::config(), settings_group );
        cg.writeEntry( "SpiderSuits", m_suits);
        cg.sync();

        if ( m_suits == 1 )
            options->setCurrentItem( 0 );
        else if ( m_suits == 2 )
            options->setCurrentItem( 1 );
        else
            options->setCurrentItem( 2 );
    }
}


void Spider::setupDeck()
{
    delete deck;
    deck = 0;

    // These look a bit weird, but are needed to keep the game numbering
    // from breaking. The original logic always created groupings of 4
    // suits, while the new logic is more flexible. We maintain the card
    // ordering by always passing a list of 4 suits even if we really only
    // have one or two.
    if ( m_suits == 1 )
        deck = new CardDeck( 2, QList<Card::Suit>() << Card::Spades
                                                    << Card::Spades
                                                    << Card::Spades
                                                    << Card::Spades );
    else if ( m_suits == 2 )
        deck = new CardDeck( 2, QList<Card::Suit>() << Card::Hearts
                                                    << Card::Spades
                                                    << Card::Hearts
                                                    << Card::Spades );
    else
        deck = new CardDeck( 2 );
}


void Spider::cardStopped(Card * t)
{
    for( int column = 0; column < 10; column++ ) {
        Card *t = stack[column]->top();
        if (t && !t->isFaceUp())
           t->flipTo(t->x(), t->y(), DURATION_FLIP );
    }
    t->disconnect(this, SLOT( cardStopped( Card* ) ) );
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

//-------------------------------------------------------------------------//

QString Spider::getGameState()
{
    return QString::number(m_leg*10 + m_redeal);
}

void Spider::setGameState(const QString &stream)
{
    int state = stream.toInt();
    int numLegs = state / 10;
    int numRedeals = state % 10;

    if ( numRedeals != m_redeal || numLegs != m_leg )
    {
        m_redeal = numRedeals;
        for ( int i = 0; i < 5; ++i )
            redeals[i]->setVisible( i >= m_redeal );

        m_leg = numLegs;
        for ( int i = 0; i < 8; ++i )
            legs[i]->setVisible( i < m_leg );

        relayoutPiles();

        emit newCardsPossible(m_redeal <= 4);
    }
}

QString Spider::getGameOptions() const
{
    return QString::number(m_suits);
}

void Spider::setGameOptions(const QString& options)
{
    setSuits(options.toInt());
}

//-------------------------------------------------------------------------//

void Spider::restart()
{
    deck->returnAllCards();
    deck->shuffle( gameNumber() );
    deal();
    emit newCardsPossible(true);
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

bool Spider::checkPileDeck(Pile *check, bool checkForDemo)
{
    if ( checkForDemo && demoActive() )
        return false;

    //kDebug(11111) << "check for run";
    if (check->isEmpty())
        return false;

    if (check->top()->rank() == Card::Ace) {
        // just using the CardList to see if this goes to King
        CardList run = getRun(check->top());
        if (run.first()->rank() == Card::King) {
            legs[m_leg]->setVisible(true);
            relayoutPiles();

            CardList::iterator it = run.end();
            qreal z = 1;
            while (it != run.begin()) {
                --it;
                legs[m_leg]->add( *it );
                // ( *it )->setSpread( QSize( 0, 0 ) );
                ( *it )->moveTo( legs[m_leg]->x(), legs[m_leg]->y(), legs[m_leg]->zValue() + z, 300 + int( z ) * 30 );
                z += 1;
            }
            connect(run.first(), SIGNAL(stopped(Card*)), SLOT(cardStopped(Card*)));
            setWaiting( true );
            /*if ( demoActive() )
              newDemoMove( run.first() );*/
            m_leg++;

            return true;
        }
    }
    return false;
}

//-------------------------------------------------------------------------//


QPointF Spider::randomPos()
{
    QRectF rect = sceneRect();
    qreal x = rect.left() + qreal(KRandom::random()) / RAND_MAX * (rect.width() - deck->cardWidth());
    qreal y = rect.top() + qreal(KRandom::random()) / RAND_MAX * (rect.height() - deck->cardHeight());
    return QPointF( x, y );
}


void Spider::deal()
{
    unmarkAll();

    m_leg = 0;
    m_redeal = 0;

    int column = 0;
    // deal face down cards (5 to first 4 piles, 4 to last 6)
    for (int i = 0; i < 44; i++ ) {
        stack[column]->add(deck->takeCard(), true, randomPos());
        column = (column + 1) % 10;
    }
    // deal face up cards, one to each pile
    for (int i = 0; i < 10; i++ ) {
        stack[column]->add(deck->takeCard(), false, randomPos());
        column = (column + 1) % 10;
    }
    // deal the remaining cards into 5 'redeal' piles
    for (int column = 0; column < 5; column++ )
        for (int i = 0; i < 10; i++ )
            redeals[column]->add(deck->takeCard(), true, randomPos());

    // make the redeal piles visible
    for (int i = 0; i < 5; i++ )
        redeals[i]->setVisible(true);

    // make the leg piles invisible
    for (int i = 0; i < 8; i++ )
        legs[i]->setVisible(false);

    relayoutPiles();
}

Card *Spider::newCards()
{
    if ( demoActive() )
    {
        for ( int i = 0; i < 10; i++ )
        {
            if ( checkPileDeck( stack[i], false ) )
                return legs[m_leg-1]->top();
        }
    }

    if (m_redeal > 4)
        return 0;

    unmarkAll();

    for (int column = 0; column < 10; column++) {
        stack[column]->add(redeals[m_redeal]->top(), false);

        // I may put an Ace on a K->2 pile so it could need cleared.
        if (stack[column]->top()->rank() == Card::Ace)
            checkPileDeck(stack[column]);
    }
    redeals[m_redeal++]->setVisible(false);
    relayoutPiles();

    takeState();
    considerGameStarted();
    if (m_redeal > 4)
        emit newCardsPossible(false);

    return stack[0]->top();
}

void Spider::mapOldId(int id)
{
   switch (id) {
   case 14:
       setSuits(1);
       break;
   case 15:
       setSuits(2);
       break;
   case 16:
       setSuits(4);
       break;
   }
}



static class SpideDealerInfo : public DealerInfo
{
public:
    SpideDealerInfo()
      : DealerInfo(I18N_NOOP("Spider"), 17)
    {
        addId(14);
        addId(15);
        addId(16);
    }

    virtual DealerScene *createGame() const
    {
        return new Spider();
    }
} spideDealerInfo;


#include "spider.moc"
