/*
 *  Copyright 2010 Parker Coates <parker.coates@gmail.com>
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

#include "card.h"
#include "pile.h"

#include <KDebug>


bool isSameSuitAscending( const QList<Card*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = cards.first()->suit();
    int rank = cards.first()->rank();

    foreach ( const Card * c, cards )
    {
        if ( c->suit() != suit || c->rank() != rank )
            return false;
        ++rank;
    }
    return true;
}


bool isSameSuitDescending( const QList<Card*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = cards.first()->suit();
    int rank = cards.first()->rank();

    foreach ( const Card * c, cards )
    {
        if ( c->suit() != suit || c->rank() != rank )
            return false;
        --rank;
    }
    return true;
}


bool isAlternateColorDescending( const QList<Card*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    bool isRed = !cards.first()->isRed();
    int rank = cards.first()->rank();

    foreach ( const Card * c, cards )
    {
        if ( c->isRed() == isRed || c->rank() != rank )
            return false;
        isRed = !isRed;
        --rank;
    }
    return true;
}


bool checkAddSameSuitAscendingFromAce( const Pile * pile, const QList<Card*> & cards )
{
    if ( !isSameSuitAscending( cards ) )
        return false;

    if ( pile->isEmpty() )
        return cards.first()->rank() == Card::Ace;
    else
        return cards.first()->suit() == pile->top()->suit()
            && cards.first()->rank() == pile->top()->rank() + 1;
}


bool checkAddAlternateColorDescending( const Pile * pile, const QList<Card*> & cards )
{
    return isAlternateColorDescending( cards )
           && ( pile->isEmpty()
                || ( cards.first()->isRed() != pile->top()->isRed()
                     && cards.first()->rank() == pile->top()->rank() - 1 ) );
}


bool checkAddAlternateColorDescendingFromKing( const Pile * pile, const QList<Card*> & cards )
{
    if ( !isAlternateColorDescending( cards ) )
        return false;

    if ( pile->isEmpty() )
        return cards.first()->rank() == Card::King;
    else
        return cards.first()->isRed() != pile->top()->isRed()
            && cards.first()->rank() == pile->top()->rank() - 1;
}

