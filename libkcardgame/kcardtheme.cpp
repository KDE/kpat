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

#include "kcardtheme.h"

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KStandardDirs>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>


class KCardThemePrivate
{
public:
    bool isValid;
    QString displayName;
    QString directoryName;
    QString desktopFilePath;
    QString graphicsFilePath;
    QDateTime lastModified;
};


QList<KCardTheme> KCardTheme::findAll()
{
    QList<KCardTheme> result;
    QStringList indexFiles = KGlobal::dirs()->findAllResources( "data", "carddecks/*/index.desktop" );
    foreach ( const QString & indexFilePath, indexFiles )
    {
        QString directoryName = QFileInfo( indexFilePath ).dir().dirName();
        KCardTheme t( directoryName );
        if ( t.isValid() )
            result << t;
    }
    return result;
}



KCardTheme::KCardTheme( const QString & directoryName )
  : d( new KCardThemePrivate )
{
    d->isValid = false;
    d->directoryName = directoryName;

    QString indexFilePath = KGlobal::dirs()->findResource( "data", QString( "carddecks/%1/index.desktop" ).arg( directoryName ) );
    if ( indexFilePath.isEmpty() )
        return;

    d->desktopFilePath = indexFilePath;

    KConfig config( indexFilePath, KConfig::SimpleConfig );
    if ( !config.hasGroup( "KDE Backdeck" ) )
        return;

    KConfigGroup configGroup = config.group( "KDE Backdeck" );

    d->displayName = configGroup.readEntry("Name");
    if ( d->displayName.isEmpty() )
        return;

    QString svgName = configGroup.readEntry("SVG");
    if ( svgName.isEmpty() )
        return;

    QFileInfo indexFile( indexFilePath );
    QFileInfo svgFile( indexFile.dir(), svgName );
    d->graphicsFilePath = svgFile.absoluteFilePath();

    if ( !svgFile.exists() )
        return;

    d->lastModified = qMax( svgFile.lastModified(), indexFile.lastModified() );

    d->isValid = true;
}


KCardTheme::KCardTheme( const KCardTheme & theme )
  : d( new KCardThemePrivate )
{
    d->isValid = theme.d->isValid;
    d->displayName = theme.d->displayName;
    d->directoryName = theme.d->directoryName;
    d->desktopFilePath = theme.d->desktopFilePath;
    d->graphicsFilePath = theme.d->graphicsFilePath;
    d->lastModified = theme.d->lastModified;
}


bool KCardTheme::isValid() const
{
    return d->isValid;
}


QString KCardTheme::displayName() const
{
    return d->displayName;
}


QString KCardTheme::directoryName() const
{
    return d->directoryName;
}


QString KCardTheme::desktopFilePath() const
{
    return d->desktopFilePath;
}


QString KCardTheme::graphicsFilePath() const
{
    return d->graphicsFilePath;
}


QDateTime KCardTheme::lastModified() const
{
    return d->lastModified;
}


KCardTheme::KCardTheme & KCardTheme::operator=( const KCardTheme & theme )
{
    d->isValid = theme.d->isValid;
    d->displayName = theme.d->displayName;
    d->directoryName = theme.d->directoryName;
    d->desktopFilePath = theme.d->desktopFilePath;
    d->graphicsFilePath = theme.d->graphicsFilePath;
    d->lastModified = theme.d->lastModified;

    return *this;
}

