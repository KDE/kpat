/*
 *  Portions Copyright (C) 2009 by Davide Bettio <davide.bettio@kdemail.net>
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.org>
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

#include "common.h"

#include "KCardTheme"

#include <KImageCache>

#include <QSize>


namespace
{
    const QString cacheNameTemplate( "libkcardgame-themes/%1" );
    const QString timeStampKey( "libkcardgame_timestamp" );
}


KImageCache * createCache( const KCardTheme & theme )
{
    KImageCache * cache = 0;

    if ( !theme.isValid() )
        return cache;

    QString cacheName = QString( cacheNameTemplate ).arg( theme.dirName() );
    cache = new KImageCache( cacheName, 3 * 1024 * 1024 );
    cache->setEvictionPolicy( KSharedDataCache::EvictLeastRecentlyUsed );

    // Enabling the pixmap cache has caused issues: we were getting back
    // different pixmaps than we had inserted. We keep a partial cache of the
    // pixmaps in KAbstractCardDeck already, so the builtin pixmap caching
    // doesn't really add that much benefit anyway.
    cache->setPixmapCaching( false );

    QDateTime cacheTimeStamp;
    if ( !cacheFind( cache, timeStampKey, &cacheTimeStamp )
         || cacheTimeStamp < theme.lastModified() )
    {
        cache->clear();
        cacheInsert( cache, timeStampKey, theme.lastModified() );
    }

    return cache;
}

