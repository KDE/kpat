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

#ifndef PATPILE_H
#define PATPILE_H

#include "libkcardgame/kcardpile.h"


class PatPile : public KCardPile
{
public:
    enum PileRole
    {
        NoRole,
        Stock,
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

    explicit PatPile( int index, const QString & objectName = QString() );

    int index() const;

    void setPileRole( PileRole role );
    PileRole pileRole() const;

    void setFoundation( bool foundation );
    bool isFoundation() const;

    virtual void add(KCard * card, int index = -1);

protected:
    virtual QPixmap normalPixmap( QSize size );
    virtual QPixmap highlightedPixmap( QSize size );

private:
    int m_index;
    PileRole m_role;
    bool m_foundation;
};

#endif
