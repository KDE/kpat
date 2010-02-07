/*
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

#ifndef KCARDTHEME_H
#define KCARDTHEME_H

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>

class KCardThemePrivate;

class KCardTheme
{
public:
    static QList<KCardTheme> findAll();

    KCardTheme();
    explicit KCardTheme( const QString & dirName );
    KCardTheme( const KCardTheme & other );
    ~KCardTheme();

    KCardTheme & operator=( const KCardTheme & other );

    bool isValid() const;
    QString dirName() const;
    QString displayName() const;
    QString desktopFilePath() const;
    QString graphicsFilePath() const;
    QDateTime lastModified() const;

private:
    QSharedDataPointer<KCardThemePrivate> d;
};

#endif
