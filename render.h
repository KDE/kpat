/*
 *  Copyright 2007-2009  Parker Coates <parker.coates@gmail.com>
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

#ifndef RENDER_H
#define RENDER_H

class QSize;
class QString;
#include <QtCore/Qt>
class QColor;
class QPixmap;


namespace Render
{
    QPixmap renderElement( const QString & elementId, QSize size );
    QSize sizeOfElement( const QString & elementId );
    qreal aspectRatioOfElement( const QString & elementId );
    QColor colorOfElement( const QString & elementId );
    QPixmap renderGamePreview( int id, QSize size );

    bool loadTheme();
}


#endif
