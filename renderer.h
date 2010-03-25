/*
 *  Copyright 2007-2009  Parker Coates <parker.coates@kdemail.net>
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

#ifndef RENDERER_H
#define RENDERER_H

#include <KGameTheme>

#include <KPixmapCache>

class QSize;
class QString;
#include <QtCore/Qt>
class QColor;
class QPixmap;
#include <QtSvg/QSvgRenderer>


class Renderer
{
public:
    static Renderer * self();

    bool setTheme( const QString & fileName );
    const KGameTheme & theme() const;

    QPixmap renderElement( const QString & elementId, QSize size );
    QSize sizeOfElement( const QString & elementId );
    qreal aspectRatioOfElement( const QString & elementId );
    QColor colorOfElement( const QString & elementId );
    QPixmap renderGamePreview( int id, QSize size );

private:
    Renderer();

    void ensureSvgIsLoaded();

    QSvgRenderer m_svgRenderer;
    KPixmapCache m_pixmapCache;
    QHash<QString, QColor> m_colors;
    KGameTheme m_theme;
    bool m_themeIsLoaded;

    friend class RendererPrivate;
};


#endif
