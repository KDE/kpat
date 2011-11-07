/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
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

#ifndef HINT_H
#define HINT_H

#include <QList>

class KCard;
class PatPile;


class MoveHint
{
public:
    MoveHint()
      : m_card( 0 ),
        m_to( 0 ),
        m_priority( 0 )
    {
    }

    MoveHint( KCard * card, PatPile * to, int prio )
      : m_card( card ),
        m_to( to ),
        m_priority( prio )
    {
        Q_ASSERT( card );
        Q_ASSERT( to );
    }

    KCard * card() const
    {
        return m_card;
    }

    PatPile * pile() const
    {
        return m_to;
    }

    int priority() const
    {
        return m_priority;
    }

    bool isValid() const
    {
        return m_card && m_to;
    }

    bool operator<( const MoveHint & other ) const
    {
        return m_priority < other.m_priority;
    }

private:
    KCard * m_card;
    PatPile * m_to;
    int m_priority;
};

#endif
