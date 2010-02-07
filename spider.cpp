/*
 * Copyright (C) 2003 Josh Metzler <joshdeb@metzlers.org>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include "dealerinfo.h"
#include "pileutils.h"
#include "settings.h"
#include "speeds.h"
#include "patsolve/spidersolver.h"

#include <KDebug>
#include <KLocale>
#include <KRandom>
#include <KSelectAction>


void SpiderPile::moveCards(QList<KCard*> & c, KCardPile *to)
{
    KCardPile::moveCards(c, to);

    // if this is a leg pile, don't do anything special
    PatPile * p = dynamic_cast<PatPile*>( to );
    if ( !p || p->pileRole() == Foundation )
        return;

    // if the top card of the list I just moved is an Ace,
    // the run I just moved is the same suit as the pile,
    // and the destination pile now has more than 12 cards,
    // then it could have a full deck that needs removed.
    if (getRank( c.last() ) == KStandardCardDeck::Ace &&
        getSuit( c.first() ) == getSuit( p->top() ) &&
        p->count() > 12)
    {
            Spider *b = dynamic_cast<Spider*>(scene());
            if (b) {
                b->checkPileDeck(p);
            }
    }
}

//-------------------------------------------------------------------------//

Spider::Spider()
    : DealerScene(), m_leg(0), m_redeal(0)
{
    const qreal dist_x = 1.12;
    const qreal smallNeg = -1e-6;

    m_suits = Settings::spiderSuitCount();

    createDeck();

    // Dealing the cards out into 5 piles so the user can see how many
    // sets of 10 cards are left to be dealt out
    for( int column = 0; column < 5; column++ ) {
        redeals[column] = new PatPile(column + 1, QString( "redeals%1" ).arg( column ));
        redeals[column]->setPileRole(PatPile::Stock);
        redeals[column]->setPilePos(smallNeg - dist_x / 3 * ( 4 - column ), smallNeg);
        redeals[column]->setZValue(12 * ( 5-column ));
        redeals[column]->setGraphicVisible( false );
        redeals[column]->setSpread(0, 0);
        connect(redeals[column], SIGNAL(clicked(KCard*)), SLOT(newCards()));
        addPile(redeals[column]);
    }

    // The 10 playing piles
    for( int column = 0; column < 10; column++ ) {
        stack[column] = new SpiderPile(column + 6, QString( "stack%1" ).arg( column ));
        stack[column]->setPileRole(PatPile::Tableau);
        stack[column]->setPilePos(dist_x * column, 0);
        stack[column]->setZValue(20);
        stack[column]->setAutoTurnTop(true);
        stack[column]->setReservedSpace( QSizeF( 1.0, 3.5 ) );
        addPile(stack[column]);
    }

    // The 8 'legs' so named by me because spiders have 8 legs - why
    // else the name Spider?
    for( int column = 0; column < 8; column++ ) {
        legs[column] = new PatPile(column + 16, QString( "legs%1" ).arg( column ));
        legs[column]->setPileRole(PatPile::Foundation);
        legs[column]->setFoundation(true);
        legs[column]->setPilePos(dist_x / 3 * column, smallNeg);
        legs[column]->setZValue(column+1);
        legs[column]->setGraphicVisible( false );
        legs[column]->setSpread(0, 0);
        legs[column]->setZValue(14 * column);
        addPile(legs[column]);
    }

    // Moving an A-K run to a leg is not really an autoDrop - the
    // user should have no choice.
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
        clearHighlightedItems();

        int cardWidth = deck()->cardWidth();
        createDeck();
        deck()->setCardWidth( cardWidth );

        Settings::setSpiderSuitCount( m_suits );

        if ( m_suits == 1 )
            options->setCurrentItem( 0 );
        else if ( m_suits == 2 )
            options->setCurrentItem( 1 );
        else
            options->setCurrentItem( 2 );
    }
}


void Spider::createDeck()
{
    // Delete the existing CardDeck.
    setupDeck( 0 );

    // These look a bit weird, but are needed to keep the game numbering
    // from breaking. The original logic always created groupings of 4
    // suits, while the new logic is more flexible. We maintain the card
    // ordering by always passing a list of 4 suits even if we really only
    // have one or two.
    QList<KStandardCardDeck::Suit> suits;
    if ( m_suits == 1 )
        suits << KStandardCardDeck::Spades << KStandardCardDeck::Spades << KStandardCardDeck::Spades << KStandardCardDeck::Spades;
    else if ( m_suits == 2 )
        suits << KStandardCardDeck::Hearts << KStandardCardDeck::Spades << KStandardCardDeck::Hearts << KStandardCardDeck::Spades;
    else
        suits << KStandardCardDeck::Clubs << KStandardCardDeck::Diamonds << KStandardCardDeck::Hearts << KStandardCardDeck::Spades;

    setupDeck( new KStandardCardDeck( 2, suits ) );
}


void Spider::cardStopped(KCard * t)
{
    for( int column = 0; column < 10; column++ ) {
        KCard *t = stack[column]->top();
        if (t && !t->isFaceUp())
           t->animate( t->pos(), t->zValue(), 1.0, 0.0, true, true, DURATION_FLIP );
    }
    t->disconnect(this, SLOT( cardStopped( KCard* ) ) );
}

//-------------------------------------------------------------------------//

bool Spider::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    // assuming the cardlist is a valid unit, since I allowed
    // it to be removed - can drop any card on empty pile or
    // on any suit card of one higher rank
    return pile->pileRole() == PatPile::Tableau
           && ( oldCards.isEmpty()
                || getRank( oldCards.last() ) == getRank( newCards.first() ) + 1 );
}

bool Spider::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau
           && isSameSuitDescending(cards);
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
    deck()->returnAllCards();
    deck()->shuffle( gameNumber() );
    deal();
    emit newCardsPossible(true);
}

//-------------------------------------------------------------------------//

QList<KCard*> Spider::getRun(KCard *c) const
{
    QList<KCard*> result;

    PatPile *p = dynamic_cast<PatPile*>( c->source() );
    if (!p || p->isEmpty())
        return result;

    result.append(c);

    KStandardCardDeck::Suit s = getSuit( c );
    int v = getRank( c );

    int index = p->indexOf(c);
    c = p->at(--index);
    while (index >= 0 && c->realFace()
           && getSuit( c ) == s && getRank( c ) == ++v)
    {
        result.prepend(c);
        c = p->at(--index);
    }

    return result;
}

bool Spider::checkPileDeck(PatPile *check, bool checkForDemo)
{
    if ( checkForDemo && demoActive() )
        return false;

    if (check->isEmpty())
        return false;

    if ( getRank( check->top() ) == KStandardCardDeck::Ace) {
        // just using the CardList to see if this goes to King
        QList<KCard*> run = getRun(check->top());
        if ( getRank( run.first() ) == KStandardCardDeck::King) {
            PatPile *leg = legs[m_leg];
            leg->setVisible(true);
            relayoutPiles( DURATION_RELAYOUT );

            qreal z = 1;
            foreach ( KCard *c, run ) {
                leg->add( c );
                c->moveTo( leg->pos(), leg->zValue() + z, DURATION_AUTODROP * (0.7 + z / 10) );
                ++z;
            }
            connect(run.last(), SIGNAL(animationStopped(KCard*)), SLOT(cardStopped(KCard*)));
            m_leg++;

            check->layoutCards();

            return true;
        }
    }
    return false;
}

void Spider::checkAllForRuns()
{
    disconnect(deck(), SIGNAL(cardAnimationDone()), this, SLOT(checkAllForRuns()));

    for (int i = 0; i < 10; ++i)
        checkPileDeck( stack[i], false );
}

//-------------------------------------------------------------------------//


QPointF Spider::randomPos()
{
    QRectF rect = sceneRect();
    qreal x = rect.left() + qreal(KRandom::random()) / RAND_MAX * (rect.width() - deck()->cardWidth());
    qreal y = rect.top() + qreal(KRandom::random()) / RAND_MAX * (rect.height() - deck()->cardHeight());
    return QPointF( x, y );
}


void Spider::deal()
{
    clearHighlightedItems();

    // make the redeal piles visible
    for (int i = 0; i < 5; i++ )
        redeals[i]->setVisible(true);

    // make the leg piles invisible
    for (int i = 0; i < 8; i++ )
        legs[i]->setVisible(false);

    relayoutPiles();

    m_leg = 0;
    m_redeal = 0;

    int column = 0;
    // deal face down cards (5 to first 4 piles, 4 to last 6)
    for (int i = 0; i < 44; i++ ) {
        addCardForDeal( stack[column], deck()->takeCard(), false, randomPos() );
        column = (column + 1) % 10;
    }
    // deal face up cards, one to each pile
    for (int i = 0; i < 10; i++ ) {
        addCardForDeal( stack[column], deck()->takeCard(), true, randomPos());
        column = (column + 1) % 10;
    }
    // deal the remaining cards into 5 'redeal' piles
    for (int column = 0; column < 5; column++ )
        for (int i = 0; i < 10; i++ )
            addCardForDeal( redeals[column], deck()->takeCard(), false, randomPos());

    startDealAnimation();
}

KCard *Spider::newCards()
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

    if (deck()->hasAnimatedCards())
        for (int i = 0; i < 10; ++i)
            if (stack[i]->top())
                return stack[i]->top();

    clearHighlightedItems();

    redeals[m_redeal]->setVisible(false);
    relayoutPiles();

    for (int column = 0; column < 10; ++column)
        stack[column]->animatedAdd(redeals[m_redeal]->top(), true);

    // I may put an Ace on a K->2 pile so it could need cleared.
    connect(deck(), SIGNAL(cardAnimationDone()), this, SLOT(checkAllForRuns()));

    ++m_redeal;

    onGameStateAlteredByUser();
    if (m_redeal > 4)
        emit newCardsPossible(false);

    return stack[0]->top();
}

void Spider::mapOldId(int id)
{
   switch (id) {
   case DealerInfo::SpiderOneSuitId :
       setSuits(1);
       break;
   case DealerInfo::SpiderTwoSuitId :
       setSuits(2);
       break;
   case DealerInfo::SpiderFourSuitId :
       setSuits(4);
       break;
   }
}

int Spider::oldId() const
{
    switch (m_suits) {
    case 1 :
        return DealerInfo::SpiderOneSuitId;
    case 2 :
        return DealerInfo::SpiderTwoSuitId;
    case 4 :
    default :
        return DealerInfo::SpiderFourSuitId;
    }
}



static class SpideDealerInfo : public DealerInfo
{
public:
    SpideDealerInfo()
      : DealerInfo(I18N_NOOP("Spider"), SpiderGeneralId)
    {
        addId(SpiderOneSuitId);
        addId(SpiderTwoSuitId);
        addId(SpiderFourSuitId);
    }

    virtual DealerScene *createGame() const
    {
        return new Spider();
    }

    virtual const char * name( int id ) const
    {
        switch ( id )
        {
        case DealerInfo::SpiderOneSuitId :
            return I18N_NOOP("Spider (1 Suit)");
        case DealerInfo::SpiderTwoSuitId :
            return I18N_NOOP("Spider (2 Suit)");
        case DealerInfo::SpiderFourSuitId :
            return I18N_NOOP("Spider (4 Suit)");
        default :
            return m_name;
        }
    }
} spideDealerInfo;


#include "spider.moc"
