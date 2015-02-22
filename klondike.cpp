/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#include "klondike.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "settings.h"
#include "speeds.h"
#include "patsolve/klondikesolver.h"

#include <KLocalizedString>
#include <KSelectAction>


KlondikePile::KlondikePile( DealerScene * scene, int index, const QString & objectName )
  : PatPile( scene, index, objectName ),
    m_cardsToShow( 1 )
{
}


void KlondikePile::setCardsToShow( int draw )
{
    m_cardsToShow = draw;
}


int KlondikePile::cardsToShow() const
{
    return m_cardsToShow;
}


QList<QPointF> KlondikePile::cardPositions() const
{
    QList<QPointF> positions;
    QPointF currentPos( 0, 0 );
    for ( int i = 0; i < count(); ++i )
    {
        positions << currentPos;
        if ( i >= count() - m_cardsToShow )
            currentPos += spread();
    }
    return positions;
}


Klondike::Klondike( const DealerInfo * di )
  : DealerScene( di )
{
}


void Klondike::initialize()
{
    // The units of the follwoing constants are pixels
    const qreal hspacing = 1.0 / 6 + 0.02; // horizontal spacing between card piles
    const qreal vspacing = 1.0 / 4; // vertical spacing between card piles

    setDeckContents();

    easyRules = Settings::klondikeIsDrawOne();

    talon = new PatPile( this, 0, "talon" );
    talon->setPileRole(PatPile::Stock);
    talon->setLayoutPos(0, 0);
    // Give the talon a low Z value to keep it out of the way during there
    // deal animation.
    talon->setZValue( -52 );
    talon->setKeyboardSelectHint( KCardPile::NeverFocus );
    talon->setKeyboardDropHint( KCardPile::NeverFocus );
    connect( talon, &KCardPile::clicked, this, &DealerScene::drawDealRowOrRedeal );

    pile = new KlondikePile( this, 13, "pile" );
    pile->setCardsToShow( easyRules ? 1 : 3 );
    pile->setPileRole( PatPile::Waste );
    pile->setRightPadding( 1.1 );
    pile->setLayoutPos( 1 + hspacing, 0 );
    pile->setSpread( 0.33, 0 );
    pile->setKeyboardSelectHint( KCardPile::ForceFocusTop );
    pile->setKeyboardDropHint( KCardPile::NeverFocus );

    for( int i = 0; i < 7; ++i )
    {
        play[i] = new PatPile( this, i + 5, QString( "play%1" ).arg( i ));
        play[i]->setPileRole(PatPile::Tableau);
        play[i]->setLayoutPos((1.0 + hspacing) * i, 1.0 + vspacing);
        play[i]->setAutoTurnTop(true);
        play[i]->setBottomPadding( play[i]->spread().y() * 7 );
        play[i]->setHeightPolicy( KCardPile::GrowDown );
        play[i]->setKeyboardSelectHint( KCardPile::AutoFocusDeepestFaceUp );
        play[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    for( int i = 0; i < 4; ++i )
    {
        target[i] = new PatPile( this, i + 1, QString( "target%1" ).arg( i ) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos((3 + i) * (1.0 + hspacing), 0);
        target[i]->setKeyboardSelectHint( KCardPile::ForceFocusTop );
        target[i]->setKeyboardDropHint( KCardPile::ForceFocusTop );
    }

    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Draw);
    setSolver( new KlondikeSolver( this, pile->cardsToShow() ) );
    redealt = false;

    options = new KSelectAction(i18n("Klondike &Options"), this );
    options->addAction( i18n("Draw 1" ));
    options->addAction( i18n("Draw 3" ));
    options->setCurrentItem( easyRules ? 0 : 1 );
    connect(options, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Klondike::gameTypeChanged);
}

bool Klondike::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return checkAddAlternateColorDescendingFromKing(oldCards, newCards);
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    case PatPile::Waste:
    case PatPile::Stock:
    default:
        return false;
    }
}

bool Klondike::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return isAlternateColorDescending(cards);
    case PatPile::Foundation:
        return easyRules && cards.first() == pile->topCard();
    case PatPile::Waste:
        return cards.first() == pile->topCard();
    case PatPile::Stock:
    default:
        return false;
    }
}


void Klondike::cardsMoved( const QList<KCard*> & cards, KCardPile * oldPile, KCardPile * newPile )
{
    DealerScene::cardsMoved( cards, oldPile, newPile );

    emit newCardsPossible( !talon->isEmpty() || pile->count() > 1 );
}


QList<QAction*> Klondike::configActions() const
{
    return QList<QAction*>() << options;
}


bool Klondike::newCards()
{
    if ( talon->isEmpty() && pile->count() <= 1 )
        return false;

    if ( talon->isEmpty() )
    {
        // Move the cards from the pile back to the deck
        flipCardsToPile( pile->cards(), talon, DURATION_MOVE );

        redealt = true;
    }
    else
    {
        QList<KCard*> cards = talon->topCards( pile->cardsToShow() );
        flipCardsToPile( cards, pile, DURATION_MOVE );
        setKeyboardFocus( pile->topCard() );
    }

    if ( talon->isEmpty() && pile->count() <= 1 )
       emit newCardsPossible( false );

    // we need to look that many steps in the future to see if we can lose
    setNeededFutureMoves( talon->count() + pile->count() );

    return true;
}


void Klondike::restart( const QList<KCard*> & cards )
{
    redealt = false;

    QList<KCard*> cardList = cards;

    for( int round = 0; round < 7; ++round )
        for ( int i = round; i < 7; ++i )
            addCardForDeal( play[i], cardList.takeLast(), (i == round), talon->pos());

    while ( !cardList.isEmpty() )
    {
        KCard * c = cardList.takeFirst();
        c->setPos( talon->pos() );
        c->setFaceUp( false );
        talon->add( c );
    }

    startDealAnimation();
}

void Klondike::gameTypeChanged()
{
    stopDemo();

    if ( allowedToStartNewGame() )
    {
        setEasy( options->currentItem() == 0 );
        startNew( gameNumber() );
    }
    else
    {
        // If we're not allowed, reset the option to
        // the current number of suits.
        options->setCurrentItem( easyRules ? 0 : 1 );
    }
}


void Klondike::setGameState( const QString & state )
{
    Q_UNUSED( state );

    emit newCardsPossible( !talon->isEmpty() || pile->count() > 1 );
}


QString Klondike::getGameOptions() const
{
    return QString::number( pile->cardsToShow() );
}

void Klondike::setGameOptions( const QString & options )
{
    setEasy( options.toInt() == 1 );
}

void Klondike::setEasy( bool _EasyRules )
{
    if ( _EasyRules != easyRules )
    {
        easyRules = _EasyRules;
        options->setCurrentItem( easyRules ? 0 : 1 );

        int drawNumber = easyRules ? 1 : 3;
        pile->setCardsToShow( drawNumber );

        KCardPile::KeyboardFocusHint hint = easyRules
                                          ? KCardPile::ForceFocusTop
                                          : KCardPile::NeverFocus;
        for( int i = 0; i < 4; ++i )
            target[i]->setKeyboardSelectHint( hint );

        setSolver( new KlondikeSolver( this, drawNumber ) );

        Settings::setKlondikeIsDrawOne( easyRules );
    }
}


bool Klondike::drop()
{
    bool pileempty = pile->isEmpty();
    if (!DealerScene::drop())
        return false;
    if (pile->isEmpty() && !pileempty)
        newCards();
    return true;
}

void Klondike::mapOldId(int id)
{
    switch ( id )
    {
    case DealerInfo::KlondikeDrawOneId:
        setEasy( true );
        break;
    case DealerInfo::KlondikeDrawThreeId:
        setEasy( false );
        break;
    case DealerInfo::KlondikeGeneralId:
    default:
        // Do nothing.
        break;
    }
}


int Klondike::oldId() const
{
    if ( easyRules )
        return DealerInfo::KlondikeDrawOneId;
    else
        return DealerInfo::KlondikeDrawThreeId;
}




static class KlondikeDealerInfo : public DealerInfo
{
public:
    KlondikeDealerInfo()
      : DealerInfo(I18N_NOOP("Klondike"), DealerInfo::KlondikeGeneralId)
    {
        addSubtype( KlondikeDrawOneId, I18N_NOOP( "Klondike (Draw 1)" ) );
        addSubtype( KlondikeDrawThreeId, I18N_NOOP( "Klondike (Draw 3)" ) );
    }

    virtual DealerScene *createGame() const
    {
        return new Klondike( this );
    }
} klondikeDealerInfo;



