/*
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include "libkcardgame/kstandardcarddeck.h"

#include <KDebug>


bool isSameSuitAscending( const QList<KCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = getSuit( cards.first() );
    int rank = getRank( cards.first() );

    foreach ( const KCard * c, cards )
    {
        if ( getSuit( c ) != suit || getRank( c ) != rank )
            return false;
        ++rank;
    }
    return true;
}


bool isSameSuitDescending( const QList<KCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = getSuit( cards.first() );
    int rank = getRank( cards.first() );

    foreach ( const KCard * c, cards )
    {
        if ( getSuit( c ) != suit || getRank( c ) != rank )
            return false;
        --rank;
    }
    return true;
}


bool isAlternateColorDescending( const QList<KCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    bool isRed = !getIsRed( cards.first() );
    int rank = getRank( cards.first() );

    foreach ( const KCard * c, cards )
    {
        if ( getIsRed( c ) == isRed || getRank( c ) != rank )
            return false;
        isRed = !isRed;
        --rank;
    }
    return true;
}


bool checkAddSameSuitAscendingFromAce( const QList<KCard*> & oldCards, const QList<KCard*> & newCards )
{
    if ( !isSameSuitAscending( newCards ) )
        return false;

    if ( oldCards.isEmpty() )
        return getRank( newCards.first() ) == KStandardCardDeck::Ace;
    else
        return getSuit( newCards.first() ) == getSuit( oldCards.last() )
            && getRank( newCards.first() ) == getRank( oldCards.last() ) + 1;
}


bool checkAddAlternateColorDescending( const QList<KCard*> & oldCards, const QList<KCard*> & newCards )
{
    return isAlternateColorDescending( newCards )
           && ( oldCards.isEmpty()
                || ( getIsRed( newCards.first() ) != getIsRed( oldCards.last() )
                     && getRank( newCards.first() ) == getRank( oldCards.last() ) - 1 ) );
}


bool checkAddAlternateColorDescendingFromKing( const QList<KCard*> & oldCards, const QList<KCard*> & newCards )
{
    if ( !isAlternateColorDescending( newCards ) )
        return false;

    if ( oldCards.isEmpty() )
        return getRank( newCards.first() ) == KStandardCardDeck::King;
    else
        return getIsRed( newCards.first() ) != getIsRed( oldCards.last() )
            && getRank( newCards.first() ) == getRank( oldCards.last() ) - 1;
}

