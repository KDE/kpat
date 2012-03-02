/*
 *  Portions Copyright (C) 2009 by Davide Bettio <davide.bettio@kdemail.net>
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#ifndef COMMON_P_H
#define COMMON_P_H

#include <KSharedDataCache>

#include <QDataStream>
class QString;


template<class T>
bool cacheFind( KSharedDataCache * cache, const QString & key, T * result )
{
    QByteArray buffer;
    if ( cache->find( key, &buffer ) )
    {
        QDataStream stream( &buffer, QIODevice::ReadOnly );
        stream >> *result;
        return true;
    }
    return false;
}

template<class T>
bool cacheInsert( KSharedDataCache * cache, const QString & key, const T & value )
{
    QByteArray buffer;
    QDataStream stream( &buffer, QIODevice::WriteOnly );
    stream << value;
    return cache->insert( key, buffer );
}

#endif
