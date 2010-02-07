/*
 *  Copyright (C) 2007 David Faure <faure@kde.org>
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

#ifndef LIBKCARDGAME_EXPORT_H
#define LIBKCARDGAME_EXPORT_H

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

#ifndef LIBKCARDGAME_EXPORT
    #if defined(MAKE_LIBKCARDGAME_LIB)
        /* We are building this library */ 
        #define LIBKCARDGAME_EXPORT KDE_EXPORT
    #else
        /* We are using this library */ 
        #define LIBKCARDGAME_EXPORT KDE_IMPORT
    #endif
#endif

#ifndef LIBKCARDGAME_EXPORT_DEPRECATED
    #define LIBKCARDGAME_EXPORT_DEPRECATED KDE_DEPRECATED LIBKCARDGAME_EXPORT
#endif

#endif
