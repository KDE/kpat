/*
 * Copyright (C) 2009 Parker Coates <parker.coates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef HIGHLIGHTABLEITEM_H
#define HIGHLIGHTABLEITEM_H


class HighlightableItem
{
public:
    HighlightableItem()
      : m_highlighted( false )
    {
    };
    virtual ~HighlightableItem()
    {
    };
    virtual void setHighlighted( bool highlighted )
    {
        m_highlighted = highlighted;
    }
    virtual bool isHighlighted() const
    {
        return m_highlighted;
    }
private:
    bool m_highlighted;
};

#endif
