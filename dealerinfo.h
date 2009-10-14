/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@gmail.com>
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

#ifndef DEALERINFO_H
#define DEALERINFO_H

class DealerInfoList;
class DealerScene;
class Solver;

#include <QtCore/QList>


class DealerInfo
{
public:
    DealerInfo( const char * name, int id );
    virtual ~DealerInfo();

    const char * name() const;
    void addId( int id );
    const QList<int> ids() const;

    virtual DealerScene * createGame() const = 0;

private:
    const char * m_name;
    QList<int> m_ids;
};


class DealerInfoList
{
private:
    friend class DealerInfoListPrivate;
    explicit DealerInfoList();
    virtual ~DealerInfoList();

public:
    static DealerInfoList * self();
    void add( DealerInfo * di );
    const QList<DealerInfo*> games() const;

private:
    QList<DealerInfo*> m_list;
};

#endif
