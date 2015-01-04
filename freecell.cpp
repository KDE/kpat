/*
 * Copyright (C) 1997 Rodolfo Borges <barrett@labma.ufrj.br>
 * Copyright (C) 1998-2009 Stephan Kulow <coolo@kde.org>
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

#include "freecell.h"

#include "dealerinfo.h"
#include "pileutils.h"
#include "speeds.h"
#include "patsolve/freecellsolver.h"

#include <QDebug>
#include <KLocalizedString>


const int CHUNKSIZE = 100;


Freecell::Freecell( const DealerInfo * di )
  : DealerScene( di )
{
}


void Freecell::initialize()
{
    setDeckContents();

    const qreal topRowDist = 1.08;
    const qreal bottomRowDist = 1.13;
    const qreal targetOffsetDist = ( 7 * bottomRowDist + 1 ) - ( 3 * topRowDist + 1 );

    for ( int i = 0; i < 4; ++i )
    {
        freecell[i] = new PatPile ( this, 1 + 8 + i, QString( "freecell%1" ).arg( i ) );
        freecell[i]->setPileRole(PatPile::Cell);
        freecell[i]->setLayoutPos(topRowDist * i, 0);
        freecell[i]->setKeyboardSelectHint( KCardPile::AutoFocusTop );
        freecell[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    for ( int i = 0; i < 8; ++i )
    {
        store[i] = new PatPile( this, 1 + i, QString( "store%1" ).arg( i ) );
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos( bottomRowDist * i, 1.3 );
        store[i]->setBottomPadding( 2.5 );
        store[i]->setHeightPolicy( KCardPile::GrowDown );
        store[i]->setKeyboardSelectHint( KCardPile::AutoFocusDeepestRemovable );
        store[i]->setKeyboardDropHint( KCardPile::AutoFocusTop );
    }

    for ( int i = 0; i < 4; ++i )
    {
        target[i] = new PatPile(this, 1 + 8 + 4 + i, QString( "target%1" ).arg( i ));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(targetOffsetDist + topRowDist * i, 0);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint( KCardPile::NeverFocus );
        target[i]->setKeyboardDropHint( KCardPile::ForceFocusTop );
    }

    setActions(DealerScene::Demo | DealerScene::Hint);
    setSolver( new FreecellSolver( this ) );
    setNeededFutureMoves( 4 ); // reserve some
}


void Freecell::restart( const QList<KCard*> & cards )
{
    QList<KCard*> cardList = cards;

    int column = 0;
    while ( !cardList.isEmpty() )
    {
        addCardForDeal( store[column], cardList.takeLast(), true, store[0]->pos() );
        column = (column + 1) % 8;
    }

    startDealAnimation();
}


void Freecell::cardsDroppedOnPile( const QList<KCard*> & cards, KCardPile * pile )
{
    if ( cards.size() <= 1 )
    {
        DealerScene::moveCardsToPile( cards, pile, DURATION_MOVE );
        return;
    }

    QList<KCardPile*> freeCells;
    for ( int i = 0; i < 4; ++i )
        if ( freecell[i]->isEmpty() )
            freeCells << freecell[i];

    QList<KCardPile*> freeStores;
    for ( int i = 0; i < 8; ++i )
        if ( store[i]->isEmpty() && store[i] != pile )
            freeStores << store[i];

    multiStepMove( cards, pile, freeStores, freeCells, DURATION_MOVE );
}


bool Freecell::tryAutomaticMove(KCard *c)
{
    // target move
    if (DealerScene::tryAutomaticMove(c))
        return true;

    if (c->isAnimated())
        return false;

    if (c == c->pile()->topCard() && c->isFaceUp())
    {
        for (int i = 0; i < 4; i++)
        {
            if (freecell[i]->isEmpty())
            {
                moveCardToPile( c, freecell[i], DURATION_MOVE );
                return true;
            }
        }
    }
    return false;
}

bool Freecell::canPutStore( const KCardPile * pile, const QList<KCard*> & cards ) const
{
    int freeCells = 0;
    for ( int i = 0; i < 4; ++i )
        if ( freecell[i]->isEmpty() )
            ++freeCells;

    int freeStores = 0;
    for ( int i = 0; i < 8; ++i )
        if ( store[i]->isEmpty() && store[i] != pile )
            ++freeStores;

    return cards.size() <= (freeCells + 1) << freeStores
           && ( pile->isEmpty()
                || ( pile->topCard()->rank() == cards.first()->rank() + 1
                     && pile->topCard()->color() != cards.first()->color() ) );

}

bool Freecell::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return canPutStore(pile, newCards);
    case PatPile::Cell:
        return oldCards.isEmpty() && newCards.size() == 1;
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    default:
        return false;
    }
}

bool Freecell::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    switch (pile->pileRole())
    {
    case PatPile::Tableau:
        return isAlternateColorDescending(cards);
    case PatPile::Cell:
        return cards.first() == pile->topCard();
    case PatPile::Foundation:
    default:
        return false;
    }
}

QList<MoveHint> Freecell::getHints()
{
    QList<MoveHint> hintList = getSolverHints();

    if ( isDemoActive() )
        return hintList;

    foreach (PatPile * store, patPiles())
    {
        if (store->isEmpty())
            continue;

        QList<KCard*> cards = store->cards();
        while (cards.count() && !cards.first()->isFaceUp())
            cards.erase(cards.begin());

        QList<KCard*>::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (allowedToRemove(store, (*iti)))
            {
                foreach (PatPile * dest, patPiles())
                {
                    int cardIndex = store->indexOf(*iti);
                    if (cardIndex == 0 && dest->isEmpty() && !dest->isFoundation())
                        continue;

                    if (!checkAdd(dest, dest->cards(), cards))
                        continue;

                    if ( dest->isFoundation() ) // taken care by solver
                        continue;

                    QList<KCard*> cardsBelow = cards.mid(0, cardIndex);
                    // if it could be here as well, then it's no use
                    if ((cardsBelow.isEmpty() && !dest->isEmpty()) || !checkAdd(store, cardsBelow, cards))
                    {
                        hintList << MoveHint( *iti, dest, 0 );
                    }
                    else if (checkPrefering( dest, dest->cards(), cards )
                             && !checkPrefering( store, cardsBelow, cards ))
                    { // if checkPrefers says so, we add it nonetheless
                        hintList << MoveHint( *iti, dest, 0 );
                    }
                }
            }
            cards.erase(iti);
            iti = cards.begin();
        }
    }
    return hintList;
}


static class FreecellDealerInfo : public DealerInfo
{
public:
    FreecellDealerInfo()
      : DealerInfo(I18N_NOOP("Freecell"), FreecellId)
    {}

    virtual DealerScene *createGame() const
    {
        return new Freecell( this );
    }
} freecellDealerInfo;



