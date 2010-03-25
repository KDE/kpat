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

#include "render.h"

#include "kgametheme.h"

#include <KDebug>
#include <KGlobal>
#include <KPixmapCache>
#include <KStandardDirs>

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>


class RenderPrivate
{
public:
    RenderPrivate()
      : m_svgRenderer(),
        m_pixmapCache("kpat-cache")
    {};

    QSvgRenderer m_svgRenderer;
    KPixmapCache m_pixmapCache;
    QHash<QString, QColor> m_colors;
    QString m_fileToLoad;
};

namespace Render
{
    K_GLOBAL_STATIC( RenderPrivate, rp )
}

bool Render::setTheme( const QString & fileName )
{
    bool result = false;

    KGameTheme newTheme;
    if ( newTheme.load( fileName ) )
    {
        const QDateTime cacheTimeStamp = QDateTime::fromTime_t( rp->m_pixmapCache.timestamp() );
        const QDateTime desktopFileTimeStamp = QFileInfo( newTheme.path() ).lastModified();
        const QDateTime svgFileTimeStamp = QFileInfo( newTheme.graphics() ).lastModified();

        QPixmap nullPixmap;
        const bool isNotInCache = !rp->m_pixmapCache.find( newTheme.path(), nullPixmap );
        kDebug( isNotInCache ) << "Theme is not already in cache.";

        const bool desktopFileIsNewer = desktopFileTimeStamp > cacheTimeStamp;
        if ( desktopFileIsNewer )
        {
            kDebug() << "Desktop file is newer than cache.";
            kDebug() << "Cache timestamp is" << cacheTimeStamp.toString( Qt::ISODate );
            kDebug() << "Desktop file timestamp is" << desktopFileTimeStamp.toString( Qt::ISODate );
        }

        const bool svgFileIsNewer = svgFileTimeStamp > cacheTimeStamp;
        if ( svgFileIsNewer )
        {
            kDebug() << "SVG file is newer than cache.";
            kDebug() << "Cache timestamp is" << cacheTimeStamp.toString( Qt::ISODate );
            kDebug() << "SVG file timestamp is" << svgFileTimeStamp.toString( Qt::ISODate );
        }

        // Discard the cache if the loaded theme doesn't match the one already
        // in the cache, or if either of the theme files have been updated
        // since the cache was created.
        const bool discardCache = isNotInCache || svgFileIsNewer || desktopFileIsNewer;
        if ( discardCache )
        {
            kDebug() << "Discarding cache.";
            rp->m_pixmapCache.discard();
            rp->m_pixmapCache.setTimestamp( QDateTime::currentDateTime().toTime_t() );

            // We store a null pixmap in the cache with the new theme's
            // file path as the key. This allows us to keep track of the
            // theme that is stored in the disk cache between runs.
            rp->m_pixmapCache.insert( newTheme.path(), nullPixmap );
        }

        if ( QFile( newTheme.graphics() ).exists() )
        {
            rp->m_fileToLoad = newTheme.graphics();
            result = true;
        }
    }
    return result;
}


QPixmap Render::renderElement( const QString & elementId, QSize size )
{
    QPixmap result;

    const QString key = elementId + QString::number( size.width() )
                        + 'x' + QString::number( size.height() );

    if ( !rp->m_pixmapCache.find( key, result ) )
    {
        kDebug() << "Rendering \"" << elementId << "\" at " << size << " pixels.";

        if ( !rp->m_svgRenderer.isValid() )
            loadTheme();

        result = QPixmap( size );
        result.fill( Qt::transparent );

        QPainter p( &result );
        rp->m_svgRenderer.render( &p, elementId );
        p.end();

        rp->m_pixmapCache.insert( key, result );
    }

    return result;
}


QSize Render::sizeOfElement( const QString & elementId )
{
    const QString key = elementId + "_default";
    QSize size;
    QPixmap pix;

    if ( rp->m_pixmapCache.find( key, pix ) )
    {
        size = pix.size();
    }
    else
    {
        if ( !rp->m_svgRenderer.isValid() )
            loadTheme();

        size = rp->m_svgRenderer.boundsOnElement( elementId ).size().toSize();
        rp->m_pixmapCache.insert( key, QPixmap( size ) );
    }

    return size;
}


qreal Render::aspectRatioOfElement( const QString & elementId )
{
    const QSizeF size( sizeOfElement( elementId ) );
    return size.width() / size.height();
}


QColor Render::colorOfElement( const QString & elementId )
{
    if ( rp->m_colors.contains( elementId ) )
        return rp->m_colors.value( elementId );

    QPixmap pix = renderElement( elementId, QSize( 3, 3 ) );
    QColor color = pix.toImage().pixel( 1, 1 );
    rp->m_colors.insert( elementId, color );
    return color;
}


QPixmap Render::renderGamePreview( int id, QSize size )
{
    QPixmap result;

    const QString key = QString("preview")
                        + QString::number( id )
                        + '_' + QString::number( size.width() )
                        + 'x' + QString::number( size.height() );

    if ( !rp->m_pixmapCache.find( key, result ) )
    {
        kDebug() << "Rendering preview number" << id << "at " << size << " pixels.";

        result = QPixmap( KStandardDirs::locate( "data", QString( "kpat/previews/%1.png" ).arg( id ) ) );
        result = result.scaled( size, Qt::KeepAspectRatio );
        rp->m_pixmapCache.insert( key, result );
    }

    return result;
}

void Render::loadTheme()
{
    rp->m_svgRenderer.load( rp->m_fileToLoad );
}
