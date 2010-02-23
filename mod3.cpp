/*
 * Copyright (C) 1997 Rodolfo Borges <barrett@labma.ufrj.br>
 * Copyright (C) 1998-2009 Stephan Kulow <coolo@kde.org>
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

#include "mod3.h"

#include "dealerinfo.h"
#include "patsolve/mod3solver.h"

#include "libkcardgame/shuffle.h"

#include <KDebug>
#include <KLocale>


void Mod3::initialize()
{
    // Piles are placed very close together. Set layoutSpacing to 0 to prevent
    // interference between them.
    setLayoutSpacing(0.0);

    const qreal dist_x = 1.114;
    const qreal dist_y = 1.31;
    const qreal bottomRowY = 3 * dist_y + 0.2;
    const qreal rightColumX = 8 * dist_x + 0.8;

    // This patience uses 2 deck of cards.
    static_cast<KStandardCardDeck*>( deck() )->setDeckContents( 2 );

    talon = new PatPile(0, "talon");
    talon->setPileRole(PatPile::Stock);
    talon->setPilePos(rightColumX, bottomRowY);
    talon->setSpread(0, 0);
    connect(talon, SIGNAL(clicked(KCard*)), SLOT(newCards()));
    addPile(talon);

    aces = new PatPile(50, "aces");
    aces->setPileRole(PatPile::FoundationType1);
    aces->setFoundation(true);
    aces->setPilePos(rightColumX, 0.5);
    aces->setReservedSpace( QSizeF( 1.0, 2.0 ));
    addPile(aces);

    for ( int r = 0; r < 4; r++ ) {
        for ( int c = 0; c < 8; c++ ) {
            int pileIndex = r * 10 + c  + 1;
            QString objectName = QString( "stack%1_%2" ).arg( r ).arg( c );
            stack[r][c] = new PatPile( pileIndex, objectName );

            // The first 3 rows are the playing field, the fourth is the store.
            if ( r < 3 ) {
                stack[r][c]->setFoundation( true );
                stack[r][c]->setPilePos( dist_x * c, dist_y * r );
                // Very tight spread makes it easy to quickly tell number of
                // cards in each pile and we don't care about the cards beneath.
                stack[r][c]->setSpread( 0, 0.08 );
                stack[r][c]->setReservedSpace( QSizeF( 1.0, 1.23 ) );
            } else {
                stack[r][c]->setPilePos( dist_x * c, bottomRowY );
                stack[r][c]->setReservedSpace( QSizeF( 1.0, 1.8 ) );
            }
            stack[r][c]->setPileRole( r == 0 ? PatPile::FoundationType2
                                      : r == 1 ? PatPile::FoundationType3
                                      : r == 2 ? PatPile::FoundationType4
                                      : PatPile::Tableau );
            addPile(stack[r][c]);
        }
    }

    setActions(DealerScene::Hint | DealerScene::Demo  | DealerScene::Deal);
    setSolver( new Mod3Solver( this ) );
}

bool mod3CheckAdd(int baseRank, const QList<KCard*> & oldCards, const QList<KCard*> & newCards)
{
    if (oldCards.isEmpty())
        return getRank( newCards.first() ) == baseRank;
    else
        return getRank( oldCards.first() ) == baseRank
               && getSuit( newCards.first() ) == getSuit( oldCards.last() )
               && getRank( newCards.first() ) == getRank( oldCards.last() ) + 3;
}

bool Mod3::checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const
{
    switch (pile->pileRole())
    {
    case PatPile::FoundationType1:
        return newCards.size() == 1 && getRank( newCards.first() ) == KStandardCardDeck::Ace;
    case PatPile::FoundationType2:
        return mod3CheckAdd(KStandardCardDeck::Two, oldCards, newCards);
    case PatPile::FoundationType3:
        return mod3CheckAdd(KStandardCardDeck::Three, oldCards, newCards);
    case PatPile::FoundationType4:
        return mod3CheckAdd(KStandardCardDeck::Four, oldCards, newCards);
    case PatPile::Tableau:
        return oldCards.isEmpty();
    case PatPile::Stock:
    default:
        return false;
    }
}

bool Mod3::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    switch (pile->pileRole())
    {
    case PatPile::FoundationType2:
    case PatPile::FoundationType3:
    case PatPile::FoundationType4:
    case PatPile::Tableau:
        return cards.first() == pile->top();
    case PatPile::FoundationType1:
    case PatPile::Stock:
    default:
        return false;
    }
}

void Mod3::moveCardsToPile( QList<KCard*> cards, KCardPile * pile, int duration )
{
    PatPile * p = dynamic_cast<PatPile*>( cards.first()->source() );

    KCardScene::moveCardsToPile( cards, pile, duration );

    if ( p
         && p->pileRole() == PatPile::Tableau
         && p->isEmpty()
         && !talon->isEmpty() )
    {
        flipCardToPile( talon->top(), p, duration );
    }
}



void Mod3::restart()
{
    foreach( KCardPile * p, piles() )
        p->clear();
    deal();
    emit newCardsPossible(true);
}


void Mod3::deal()
{
    clearHighlightedItems();

    QList<KCard*> cards = shuffled( deck()->cards(), gameNumber() );

    while ( !cards.isEmpty() )
    {
        KCard * c = cards.takeFirst();
        c->setPos( talon->pos() );
        c->setFaceUp( false );
        talon->add( c );
    }

    for (int r = 0; r < 4; r++)
    {
        for ( int c = 0; c < 8; ++c )
        {
            KCard * card = talon->top();
            card->setFaceUp( true );
            moveCardToPileAtSpeed( card, stack[r][c], DEAL_SPEED );

            // Fudge the z values to keep cards from popping through one another.
            card->setZValue( card->zValue() + (4-r)*(4-r) + (8-c)*(8-c) );
        }
    }
}

KCard *Mod3::newCards()
{
    if (talon->isEmpty())
        return 0;

    if (deck()->hasAnimatedCards())
        for (int i = 0; i < 8; ++i)
            if (stack[3][i]->top())
                return stack[3][i]->top();

    clearHighlightedItems();

    for ( int c = 0; c < 8; ++c )
    {
        if ( talon->isEmpty() )
            break;

        flipCardToPileAtSpeed( talon->top(), stack[3][c], DEAL_SPEED * 2 );
    }

    onGameStateAlteredByUser();
    if (talon->isEmpty())
        emit newCardsPossible(false);

    return stack[3][0]->top();
}

void Mod3::setGameState(const QString &)
{
    emit newCardsPossible(!talon->isEmpty());
}



static class Mod3DealerInfo : public DealerInfo
{
public:
    Mod3DealerInfo()
      : DealerInfo(I18N_NOOP("Mod3"), Mod3Id)
    {}

    virtual DealerScene *createGame() const
    {
        return new Mod3();
    }
} mod3DealerInfo;


#include "mod3.moc"
