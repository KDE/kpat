/*
 * Copyright (C) 2003 Josh Metzler <joshdeb@metzlers.org>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include <KLocalizedString>
#include <KRandom>
#include <KSelectAction>


class InvisiblePile : public PatPile
{
public:
    InvisiblePile( DealerScene * scene, int index, const QString & objectName = QString() )
      : PatPile( scene, index, objectName )
    {
    };

protected:
    virtual void paintGraphic( QPainter * painter, qreal highlightedness )
    {
        Q_UNUSED( painter );
        Q_UNUSED( highlightedness );
    };
};


Spider::Spider( const DealerInfo * di )
  : DealerScene( di )
{
}


void Spider::initialize()
{
    m_leg = 0;
    m_redeal = 0;

    const qreal dist_x = 1.12;
    const qreal smallNeg = -1e-6;

    m_suits = Settings::spiderSuitCount();

    m_stackFaceup = Settings::spiderStackFaceup();

    createDeck();

    // Dealing the cards out into 5 piles so the user can see how many
    // sets of 10 cards are left to be dealt out
    for( int column = 0; column < 5; ++column )
    {
        redeals[column] = new InvisiblePile( this, column + 1, QString( "redeals%1" ).arg( column ) );
        redeals[column]->setPileRole(PatPile::Stock);
        redeals[column]->setLayoutPos( dist_x * (9 - (4.0 - column) / 3), smallNeg );
        redeals[column]->setZValue(12 * ( 5-column ));
        redeals[column]->setSpread(0, 0);
        redeals[column]->setKeyboardSelectHint( KCardPile::NeverFocus );
        redeals[column]->setKeyboardDropHint( KCardPile::NeverFocus );
        connect( redeals[column], SIGNAL(clicked(KCard*)), SLOT(drawDealRowOrRedeal()) );
    }

    // The 10 playing piles
    for( int column = 0; column < 10; ++column )
    {
        stack[column] = new PatPile( this, column + 6, QString( "stack%1" ).arg( column ) );
        stack[column]->setPileRole(PatPile::Tableau);
        stack[column]->setLayoutPos(dist_x * column, 0);
        stack[column]->setZValue(20);
        stack[column]->setAutoTurnTop(true);
        stack[column]->setBottomPadding( 1.5 );
        stack[column]->setHeightPolicy( KCardPile::GrowDown );
        stack[column]->setKeyboardSelectHint( KCardPile::AutoFocusDeepestRemovable );
        stack[column]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    // The 8 'legs' so named by me because spiders have 8 legs - why
    // else the name Spider?
    for( int column = 0; column < 8; ++column )
    {
        legs[column] = new InvisiblePile( this, column + 16, QString( "legs%1" ).arg( column ) );
        legs[column]->setPileRole(PatPile::Foundation);
        legs[column]->setLayoutPos(dist_x / 3 * column, smallNeg);
        legs[column]->setZValue(column+1);
        legs[column]->setSpread(0, 0);
        legs[column]->setZValue(14 * column);
        legs[column]->setVisible( false );
        legs[column]->setKeyboardSelectHint( KCardPile::NeverFocus );
        legs[column]->setKeyboardDropHint( KCardPile::NeverFocus );
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
    connect(options, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Spider::gameTypeChanged);

    m_stackFaceupOption = new KSelectAction(i18n("S&tack Options"), this );
    m_stackFaceupOption->addAction( i18n("Face &Down (harder)") );
    m_stackFaceupOption->addAction( i18n("Face &Up (easier)") );
    m_stackFaceupOption->setCurrentItem( m_stackFaceup );

    connect(m_stackFaceupOption, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Spider::gameTypeChanged);
}


QList<QAction*> Spider::configActions() const
{
    return QList<QAction*>() << options << m_stackFaceupOption;
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

        if ( m_stackFaceup != m_stackFaceupOption->currentItem() ) {
            m_stackFaceup = m_stackFaceupOption->currentItem();
            Settings::setSpiderStackFaceup( m_stackFaceup );
        }

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

        m_stackFaceupOption->setCurrentItem( m_stackFaceup );
    }
}


void Spider::setSuits(int suits)
{
    if ( suits != m_suits )
    {
        m_suits = suits;

        stopDemo();
        clearHighlightedItems();
        setKeyboardModeActive( false );

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
    // These look a bit weird, but are needed to keep the game numbering
    // from breaking. The original logic always created groupings of 4
    // suits, while the new logic is more flexible. We maintain the card
    // ordering by always passing a list of 4 suits even if we really only
    // have one or two.
    QList<KCardDeck::Suit> suits;
    if ( m_suits == 1 )
        suits << KCardDeck::Spades << KCardDeck::Spades << KCardDeck::Spades << KCardDeck::Spades;
    else if ( m_suits == 2 )
        suits << KCardDeck::Hearts << KCardDeck::Spades << KCardDeck::Hearts << KCardDeck::Spades;
    else
        suits << KCardDeck::Clubs << KCardDeck::Diamonds << KCardDeck::Hearts << KCardDeck::Spades;

    setDeckContents( 2, suits );
}


bool Spider::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    // assuming the cardlist is a valid unit, since I allowed
    // it to be removed - can drop any card on empty pile or
    // on any suit card of one higher rank
    return pile->pileRole() == PatPile::Tableau
           && ( oldCards.isEmpty()
                || oldCards.last()->rank() == newCards.first()->rank() + 1 );
}


bool Spider::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau
           && isSameSuitDescending(cards);
}


QString Spider::getGameState() const
{
    return QString::number(m_leg*10 + m_redeal);
}


void Spider::setGameState( const QString & state )
{
    int n = state.toInt();
    int numLegs = n / 10;
    int numRedeals = n % 10;

    if ( numRedeals != m_redeal || numLegs != m_leg )
    {
        m_redeal = numRedeals;
        for ( int i = 0; i < 5; ++i )
            redeals[i]->setVisible( i >= m_redeal );

        m_leg = numLegs;
        for ( int i = 0; i < 8; ++i )
            legs[i]->setVisible( i < m_leg );

        recalculatePileLayouts();
        foreach ( KCardPile * p, piles() )
            updatePileLayout( p, 0 );

        emit newCardsPossible(m_redeal <= 4);
    }
}


QString Spider::getGameOptions() const
{
    return QString::number(m_suits);
}


void Spider::setGameOptions( const QString & options )
{
    setSuits(options.toInt());
}


void Spider::restart( const QList<KCard*> & cards )
{
    m_pilesWithRuns.clear();

    // make the redeal piles visible
    for (int i = 0; i < 5; ++i )
        redeals[i]->setVisible( true );

    // make the leg piles invisible
    for (int i = 0; i < 8; ++i )
        legs[i]->setVisible( false );

    recalculatePileLayouts();

    m_leg = 0;
    m_redeal = 0;

    QList<KCard*> cardList = cards;

    int column = 0;
    // deal face down cards (5 to first 4 piles, 4 to last 6)
    for ( int i = 0; i < 44; ++i )
    {
        addCardForDeal( stack[column], cardList.takeLast(), m_stackFaceup == 1, randomPos() );
        column = (column + 1) % 10;
    }
    // deal face up cards, one to each pile
    for ( int i = 0; i < 10; ++i )
    {
        addCardForDeal( stack[column], cardList.takeLast(), true, randomPos() );
        column = (column + 1) % 10;
    }
    // deal the remaining cards into 5 'redeal' piles
    for ( int column = 0; column < 5; ++column )
        for ( int i = 0; i < 10; ++i )
            addCardForDeal( redeals[column], cardList.takeLast(), false, randomPos() );

    startDealAnimation();

    emit newCardsPossible(true);
}


void Spider::cardsMoved( const QList<KCard*> & cards, KCardPile * oldPile, KCardPile * newPile )
{
    PatPile * p = dynamic_cast<PatPile*>( newPile );

    // The solver treats the removal of complete runs from the table as a
    // separate move, so we don't do it automatically when the demo is active.
    if ( !isDemoActive()
         && p
         && p->pileRole() == PatPile::Tableau
         && pileHasFullRun( p ) )
    {
        m_pilesWithRuns << p;
    }

    DealerScene::cardsMoved( cards, oldPile, newPile );
}


bool Spider::pileHasFullRun( KCardPile * pile )
{
    QList<KCard*> potentialRun = pile->topCards( 13 );
    return pile->count() >= 13
           && potentialRun.first()->isFaceUp()
           && isSameSuitDescending( potentialRun );
}


void Spider::moveFullRunToLeg( KCardPile * pile )
{
    QList<KCard*> run = pile->topCards( 13 );

    PatPile * leg = legs[m_leg];
    ++m_leg;
    leg->setVisible( true );

    recalculatePileLayouts();
    for ( int i = 0; i < 10; ++i )
        if ( stack[i] != pile )
            updatePileLayout( stack[i], DURATION_RELAYOUT );

    for ( int i = 0; i < run.size(); ++i )
    {
        KCard * c = run.at( i );
        leg->add( c );
        int duration = DURATION_AUTODROP * (0.7 + i / 10.0);
        c->animate( leg->pos(), leg->zValue() + i, 0, true, true, duration );
    }

    updatePileLayout( pile, DURATION_RELAYOUT );
}


QPointF Spider::randomPos()
{
    QRectF rect = sceneRect();
    qreal x = rect.left() + qreal(KRandom::random()) / RAND_MAX * (rect.width() - deck()->cardWidth());
    qreal y = rect.top() + qreal(KRandom::random()) / RAND_MAX * (rect.height() - deck()->cardHeight());
    return QPointF( x, y );
}


bool Spider::newCards()
{
    // The solver doesn't distinguish between dealing a new row of cards and
    // removing complete runs from the tableau. So it we're in demo mode and
    // newCards() is called, we should check to see if there are any complete
    // runs to move before dealing a new row.
    if ( isDemoActive() )
    {
        for ( int i = 0; i < 10; ++i )
        {
            if ( pileHasFullRun( stack[i] ) )
            {
                moveFullRunToLeg( stack[i] );
                return true;
            }
        }
    }

    if ( m_redeal > 4 )
        return false;

    redeals[m_redeal]->setVisible(false);
    recalculatePileLayouts();

    for ( int column = 0; column < 10; ++column )
    {
        KCard * c = redeals[m_redeal]->topCard();
        if ( !c )
            break;

        flipCardToPileAtSpeed( c, stack[column], DEAL_SPEED );
        c->setZValue( c->zValue() + 10 - column );
    }

    ++m_redeal;

    if (m_redeal > 4)
        emit newCardsPossible(false);

    return true;
}


void Spider::animationDone()
{
    if ( !m_pilesWithRuns.isEmpty() )
        moveFullRunToLeg( m_pilesWithRuns.takeFirst() );
    else
        DealerScene::animationDone();
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
        addSubtype( SpiderOneSuitId, I18N_NOOP( "Spider (1 Suit)" ) );
        addSubtype( SpiderTwoSuitId, I18N_NOOP( "Spider (2 Suit)" ) );
        addSubtype( SpiderFourSuitId, I18N_NOOP( "Spider (4 Suit)" ) );
    }

    virtual DealerScene *createGame() const
    {
        return new Spider( this );
    }
} spideDealerInfo;



