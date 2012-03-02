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

#ifndef PILEUTILS_H
#define PILEUTILS_H
class KCard;

#include <QList>


bool isSameSuitAscending( const QList<KCard*> & cards );
bool isSameSuitDescending( const QList<KCard*> & cards );
bool isAlternateColorDescending( const QList<KCard*> & cards );

bool checkAddSameSuitAscendingFromAce( const QList<KCard*> & oldCards, const QList<KCard*> & newCards );
bool checkAddAlternateColorDescending( const QList<KCard*> & oldCards, const QList<KCard*> & newCards );
bool checkAddAlternateColorDescendingFromKing( const QList<KCard*> & oldCards, const QList<KCard*> & newCards );

#endif
