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

#ifndef PILEUTILS_H
#define PILEUTILS_H

class Pile;
class Card;

#include <QList>


enum PileTypes
{
    Stock = 1,
    Waste,
    Tableau,
    TableauType1 = Tableau,
    TableauType2,
    TableauType3,
    TableauType4,
    Foundation,
    FoundationType1 = Foundation,
    FoundationType2,
    FoundationType3,
    FoundationType4,
    Cell
};

bool isSameSuitAscending( const QList<Card*> & cards );
bool isSameSuitDescending( const QList<Card*> & cards );
bool isAlternateColorDescending( const QList<Card*> & cards );

bool checkAddSameSuitAscendingFromAce( const Pile * pile, const QList<Card*> & cards );
bool checkAddAlternateColorDescending( const Pile * pile, const QList<Card*> & cards );
bool checkAddAlternateColorDescendingFromKing( const Pile * pile, const QList<Card*> & cards );

#endif