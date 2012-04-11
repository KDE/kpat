/*
 *  Copyright 2007-2010  Parker Coates <coates@kde.org>
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

#include <KGameRenderer>

class QSize;
class QString;
#include <QtCore/Qt>
class QColor;
class QPixmap;


class Renderer : public KGameRenderer
{
public:
    static Renderer * self();
    static void deleteSelf();

    qreal aspectRatioOfElement( const QString & elementId );
    QColor colorOfElement( const QString & elementId );

private:
    Renderer();

    static Renderer * s_instance;

    QHash<QString,QColor> m_colors;
    QByteArray m_cachedTheme;

    friend class RendererPrivate;
};


#endif
