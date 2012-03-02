/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "pileutils.h"

#include "KCard"
#include "KCardDeck"


namespace
{
    inline int alternateColor( int color )
    {
        return color == KCardDeck::Red ? KCardDeck::Black : KCardDeck::Red;
    }
}


bool isSameSuitAscending( const QList<KCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = cards.first()->suit();
    int lastRank = cards.first()->rank();

    for( int i = 1; i < cards.size(); ++i )
    {
        ++lastRank;
        if ( cards[i]->suit() != suit || cards[i]->rank() != lastRank )
            return false;
    }
    return true;
}


bool isSameSuitDescending( const QList<KCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = cards.first()->suit();
    int lastRank = cards.first()->rank();

    for( int i = 1; i < cards.size(); ++i )
    {
        --lastRank;
        if ( cards[i]->suit() != suit || cards[i]->rank() != lastRank )
            return false;
    }
    return true;
}


bool isAlternateColorDescending( const QList<KCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int lastColor = cards.first()->color();
    int lastRank = cards.first()->rank();

    for( int i = 1; i < cards.size(); ++i )
    {
        lastColor = alternateColor( lastColor );
        --lastRank;

        if ( cards[i]->color() != lastColor || cards[i]->rank() != lastRank )
            return false;
    }
    return true;
}


bool checkAddSameSuitAscendingFromAce( const QList<KCard*> & oldCards, const QList<KCard*> & newCards )
{
    if ( !isSameSuitAscending( newCards ) )
        return false;

    if ( oldCards.isEmpty() )
        return newCards.first()->rank() == KCardDeck::Ace;
    else
        return newCards.first()->suit() == oldCards.last()->suit()
            && newCards.first()->rank() == oldCards.last()->rank() + 1;
}


bool checkAddAlternateColorDescending( const QList<KCard*> & oldCards, const QList<KCard*> & newCards )
{
    return isAlternateColorDescending( newCards )
           && ( oldCards.isEmpty()
                || ( newCards.first()->color() == alternateColor( oldCards.last()->color() )
                     && newCards.first()->rank() == oldCards.last()->rank() - 1 ) );
}


bool checkAddAlternateColorDescendingFromKing( const QList<KCard*> & oldCards, const QList<KCard*> & newCards )
{
    if ( !isAlternateColorDescending( newCards ) )
        return false;

    if ( oldCards.isEmpty() )
        return newCards.first()->rank() == KCardDeck::King;
    else
        return newCards.first()->color() == alternateColor( oldCards.last()->color() )
            && newCards.first()->rank() == oldCards.last()->rank() - 1;
}

