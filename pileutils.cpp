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

#include "libkcardgame/kstandardcard.h"

#include <KDebug>


bool isSameSuitAscending( const QList<KStandardCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = cards.first()->suit();
    int rank = cards.first()->rank();

    foreach ( const KStandardCard * c, cards )
    {
        if ( c->suit() != suit || c->rank() != rank )
            return false;
        ++rank;
    }
    return true;
}


bool isSameSuitDescending( const QList<KStandardCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    int suit = cards.first()->suit();
    int rank = cards.first()->rank();

    foreach ( const KStandardCard * c, cards )
    {
        if ( c->suit() != suit || c->rank() != rank )
            return false;
        --rank;
    }
    return true;
}


bool isAlternateColorDescending( const QList<KStandardCard*> & cards )
{
    if ( cards.size() <= 1 )
        return true;

    bool isRed = !cards.first()->isRed();
    int rank = cards.first()->rank();

    foreach ( const KStandardCard * c, cards )
    {
        if ( c->isRed() == isRed || c->rank() != rank )
            return false;
        isRed = !isRed;
        --rank;
    }
    return true;
}


bool checkAddSameSuitAscendingFromAce( const QList<KStandardCard*> & oldCards, const QList<KStandardCard*> & newCards )
{
    if ( !isSameSuitAscending( newCards ) )
        return false;

    if ( oldCards.isEmpty() )
        return newCards.first()->rank() == KStandardCard::Ace;
    else
        return newCards.first()->suit() == oldCards.last()->suit()
            && newCards.first()->rank() == oldCards.last()->rank() + 1;
}


bool checkAddAlternateColorDescending( const QList<KStandardCard*> & oldCards, const QList<KStandardCard*> & newCards )
{
    return isAlternateColorDescending( newCards )
           && ( oldCards.isEmpty()
                || ( newCards.first()->isRed() != oldCards.last()->isRed()
                     && newCards.first()->rank() == oldCards.last()->rank() - 1 ) );
}


bool checkAddAlternateColorDescendingFromKing( const QList<KStandardCard*> & oldCards, const QList<KStandardCard*> & newCards )
{
    if ( !isAlternateColorDescending( newCards ) )
        return false;

    if ( oldCards.isEmpty() )
        return newCards.first()->rank() == KStandardCard::King;
    else
        return newCards.first()->isRed() != oldCards.last()->isRed()
            && newCards.first()->rank() == oldCards.last()->rank() - 1;
}

