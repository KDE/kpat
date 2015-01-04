/*
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

#include "clock.h"

#include "dealerinfo.h"
#include "patsolve/clocksolver.h"

#include <KLocalizedString>


Clock::Clock( const DealerInfo * di )
  : DealerScene( di )
{
}

void Clock::initialize()
{
    setSceneAlignment( AlignHCenter | AlignVCenter );

    setDeckContents();

    const qreal dist_x = 1.11;
    const qreal ys[12] = {   0./96,  15./96,  52./96, 158./96, 264./96, 301./96, 316./96, 301./96, 264./96, 158./96,  52./96,  15./96};
    const qreal xs[12] = { 200./72, 280./72, 360./72, 400./72, 360./72, 280./72, 200./72, 120./72, 40./72, 0./72, 40./72, 120./72};

    for ( int i = 0; i < 12; ++i )
    {
        target[i] = new PatPile( this, i + 1, QString("target%1").arg(i) );
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(4 * dist_x + 0.4 + xs[i], 0.2 + ys[i]);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint( KCardPile::NeverFocus );
        target[i]->setKeyboardDropHint( KCardPile::ForceFocusTop );
    }

    for ( int i = 0; i < 8; ++i )
    {
        store[i] = new PatPile( this, 14 + i, QString("store%1").arg(i) );
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(dist_x*(i%4), 2.5 * (i/4));
        store[i]->setBottomPadding( 1.3 );
        store[i]->setKeyboardSelectHint( KCardPile::AutoFocusTop );
        store[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new ClockSolver( this ) );
}

void Clock::restart( const QList<KCard*> & cards )
{
    static const KCardDeck::Suit suits[12] = { KCardDeck::Diamonds, KCardDeck::Spades, KCardDeck::Hearts, KCardDeck::Clubs,
                                               KCardDeck::Diamonds, KCardDeck::Spades, KCardDeck::Hearts, KCardDeck::Clubs,
                                               KCardDeck::Diamonds, KCardDeck::Spades, KCardDeck::Hearts, KCardDeck::Clubs };
    static const KCardDeck::Rank ranks[12] = { KCardDeck::Nine, KCardDeck::Ten, KCardDeck::Jack, KCardDeck::Queen,
                                               KCardDeck::King, KCardDeck::Two, KCardDeck::Three, KCardDeck::Four,
                                               KCardDeck::Five, KCardDeck::Six, KCardDeck::Seven, KCardDeck::Eight };

    const QPointF center = ( target[0]->pos() + target[6]->pos() ) / 2;

    int j = 0;
    QList<KCard*> cardList = cards;
    while ( !cardList.isEmpty() )
    {
        KCard * c = cardList.takeLast();
        for ( int i = 0; i < 12; ++i )
        {
            if ( c->rank() == ranks[i] && c->suit() == suits[i] )
            {
                QPointF initPos = (2 * center + target[(i + 2) % 12]->pos()) / 3;
                addCardForDeal( target[i], c, true, initPos );
                c = 0;
                break;
            }
        }
        if ( c )
        {
            addCardForDeal( store[j], c, true, store[ j < 4 ? 0 : 4 ]->pos() );
            j = (j + 1) % 8;
        }
    }

    startDealAnimation();
}


bool Clock::drop()
{
    return false;
}


bool Clock::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    if ( pile->pileRole() == PatPile::Tableau )
    {
        return oldCards.isEmpty()
               || newCards.first()->rank() == oldCards.last()->rank() - 1;
    }
    else
    {
        return oldCards.last()->suit() == newCards.first()->suit()
               && ( newCards.first()->rank() == oldCards.last()->rank() + 1
                    || ( oldCards.last()->rank() == KCardDeck::King
                         && newCards.first()->rank() == KCardDeck::Ace ) );
    }
}


bool Clock::checkRemove(const PatPile* pile, const QList<KCard*> & cards) const
{
    return pile->pileRole() == PatPile::Tableau
           && cards.first() == pile->topCard();
}


static class ClockDealerInfo : public DealerInfo
{
public:
    ClockDealerInfo()
      : DealerInfo(I18N_NOOP("Grandfather's Clock"), GrandfathersClockId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Clock( this );
    }
} clockDealerInfo;



