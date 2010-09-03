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

#include <KRandomSequence>

#include <QtCore/QList>


template <class T>
QList<T> shuffled( const QList<T> & list, unsigned long seed = 0 )
{
    KRandomSequence randSeq( seed );

    QList<T> result = list;
    for ( int i = result.size(); i > 1; --i )
    {
        int j = int( randSeq.getLong( (unsigned long)i ) );
        result.swap( i - 1, j );
    }

    return result;
}


#endif
