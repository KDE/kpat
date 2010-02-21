/*
 * Copyright (C) 2001-2009 Stephan Kulow <coolo@kde.org>
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

#ifndef SHUFFLE_H
#define SHUFFLE_H

#include "libkcardgame_export.h"

#include <QtCore/QList>


template <class T>
QList<T> shuffled( const QList<T> & list, int seed )
{
    Q_ASSERT( seed >= 0 );

    QList<T> result = list;
    for ( int i = result.size(); i > 0; --i )
    {
        // We use the same pseudorandom number generation algorithm as Windows
        // Freecell, so that game numbers are the same between the two applications.
        // For more inforation, see 
        // http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q28150
        seed = 214013 * seed + 2531011;
        int rand = ( seed >> 16 ) & 0x7fff;

        int z = rand % i;

        T temp = result[ z ];
        result[ z ] = result[ i - 1 ];
        result[ i - 1 ] = temp;
    }

    return result;
}

#endif
