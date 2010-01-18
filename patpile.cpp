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

#include "patpile.h"


PatPile::PatPile( int index, const QString & objectName )
  : Pile( index, objectName ),
    m_role( NoRole )
{

}


void PatPile::setPileRole( PileRole role )
{
    m_role = role;
}


PatPile::PileRole PatPile::pileRole() const
{
    return m_role;
}


void PatPile::setFoundation( bool foundation )
{
    m_foundation = foundation;
}


bool PatPile::isFoundation() const
{
    return m_foundation;
}



