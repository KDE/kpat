/*
 *  Copyright 2007-2010  Parker Coates <parker.coates@kdemail.net>
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

#include "renderer.h"

#include "settings.h"

#include <KGlobal>


class RendererPrivate
{
public:
    Renderer instance;
};

K_GLOBAL_STATIC( RendererPrivate, rp )


Renderer * Renderer::self()
{
    return &rp->instance;
}


qreal Renderer::aspectRatioOfElement( const QString & elementId )
{
    const QSizeF size = boundsOnSprite( elementId ).size();
    return size.width() / size.height();
}


QColor Renderer::colorOfElement( const QString & elementId )
{
    if ( m_cachedTheme != theme() )
    {
        m_colors.clear();
        m_cachedTheme = theme();
    }

    QColor color;
    QHash<QString,QColor>::const_iterator it = m_colors.constFind( elementId );
    if ( it != m_colors.end() )
    {
        color = it.value();
    }
    else
    {
        QPixmap pix = spritePixmap( elementId, QSize( 3, 3 ) );
        color = pix.toImage().pixel( 1, 1 );
        m_colors.insert( elementId, color );
    }
    return color;
}


Renderer::Renderer()
  : KGameRenderer( Settings::defaultThemeValue() )
{
    setTheme( Settings::theme() );
}

