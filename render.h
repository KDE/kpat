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

#ifndef KILLBOTS_RENDER_H
#define KILLBOTS_RENDER_H

class QSize;
class QString;
class QPixmap;
class QRectF;
#include <Qt>

namespace Render
{
    bool loadTheme( const QString & fileName );
    QPixmap renderElement( const QString & elementId, QSize size );
    QRectF boundsOnElement( const QString & elementId );
    qreal aspectRatioOfElement( const QString & elementId );
}


#endif
