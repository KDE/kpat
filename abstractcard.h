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

#ifndef ABSTRACTCARD_H
#define ABSTRACTCARD_H

#include <QtGui/QGraphicsItem>


class AbstractCard : public QGraphicsPixmapItem
{
public:
    AbstractCard( quint32 data, bool faceUp = true );

    bool isFaceUp() const;
    quint32 data() const;

protected:
    bool m_faceup;
    const quint32 m_data;
};

#endif // ABSTRACTCARD_H
