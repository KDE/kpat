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

#include "render.h"

#include <KDE/KDebug>
#include <KDE/KGlobal>
#include <KDE/KPixmapCache>
#include <KDE/KStandardDirs>
#include <KDE/KSvgRenderer>

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtGui/QPainter>


class RenderPrivate
{
public:
    RenderPrivate()
      : m_svgRenderer(),
        m_pixmapCache("kpat-cache"),
        m_hasBeenLoaded( false )
    {};

    KSvgRenderer m_svgRenderer;
    KPixmapCache m_pixmapCache;
    bool m_hasBeenLoaded;
};

namespace Render
{
    K_GLOBAL_STATIC( RenderPrivate, rp )
}


bool Render::loadTheme( const QString & fileName )
{
    bool result = false;

    const QDateTime cacheTimeStamp = QDateTime::fromTime_t( rp->m_pixmapCache.timestamp() );
    const QDateTime svgFileTimeStamp = QFileInfo( fileName ).lastModified();


    QPixmap nullPixmap;


    const bool svgFileIsNewer = svgFileTimeStamp > cacheTimeStamp;
    if ( svgFileIsNewer )
    {
        kDebug(11111) << "SVG file is newer than cache.";
        kDebug(11111) << "Cache timestamp is" << cacheTimeStamp.toString( Qt::ISODate );
        kDebug(11111) << "SVG file timestamp is" << svgFileTimeStamp.toString( Qt::ISODate );
    }

    // Only bother actually loading the SVG if no SVG has been loaded
    // yet or if the cache must be discarded.
    if ( !rp->m_hasBeenLoaded || svgFileIsNewer )
    {
        if ( svgFileIsNewer )
        {
            kDebug(11111) << "Discarding cache.";
            rp->m_pixmapCache.discard();
            rp->m_pixmapCache.setTimestamp( QDateTime::currentDateTime().toTime_t() );
        }

        result = rp->m_hasBeenLoaded = rp->m_svgRenderer.load( fileName );
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
        kDebug(11111) << "Rendering \"" << elementId << "\" at " << size << " pixels.";

        result = QPixmap( size );
        result.fill( Qt::transparent );

        QPainter p( &result );
        rp->m_svgRenderer.render( &p, elementId );
        p.end();

        rp->m_pixmapCache.insert( key, result );
    }

    return result;
}


QRectF Render::boundsOnElement( const QString & elementId )
{
    return rp->m_svgRenderer.boundsOnElement( elementId );
}


qreal Render::aspectRatioOfElement( const QString & elementId )
{
    const QRectF rect = rp->m_svgRenderer.boundsOnElement( elementId );
    return rect.width() / rect.height();
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
        kDebug(11111) << "Rendering preview number" << id << "at " << size << " pixels.";

        result = QPixmap( KStandardDirs::locate( "data", QString( "kpat/demos/demo_%1.png" ).arg( id ) ) );
        result = result.scaled( size, Qt::KeepAspectRatio );
        rp->m_pixmapCache.insert( key, result );
    }

    return result;
}
