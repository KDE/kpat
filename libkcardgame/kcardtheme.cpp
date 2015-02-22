/*
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

#include "kcardtheme.h"

#include <KConfig>
#include <KConfigGroup>


#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QSharedData>
#include <QStandardPaths>


class KCardThemePrivate : public QSharedData
{
public:
    KCardThemePrivate( bool isValid,
                       const QString & dirName,
                       const QString & displayName,
                       const QString & desktopFilePath,
                       const QString & graphicsFilePath,
                       const QSet<QString> & supportedFeatures,
                       const QDateTime & lastModified )
      : isValid( isValid ),
        dirName( dirName ),
        displayName( displayName ),
        desktopFilePath( desktopFilePath ),
        graphicsFilePath( graphicsFilePath ),
        supportedFeatures( supportedFeatures ),
        lastModified( lastModified )
    {
    };

    const bool isValid;
    const QString dirName;
    const QString displayName;
    const QString desktopFilePath;
    const QString graphicsFilePath;
    const QSet<QString> supportedFeatures;
    const QDateTime lastModified;
};


QList<KCardTheme> KCardTheme::findAll()
{
    QList<KCardTheme> result;
    QStringList indexFiles = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "carddecks", QStandardPaths::LocateDirectory );
    Q_FOREACH (const QString &index, indexFiles) {
        const QStringList entries = QDir(index).entryList(QDir::Dirs);
        Q_FOREACH (const QString &d, entries) {
            QString indexFilePath = index + '/' + d + "/index.desktop";
            if (QFile::exists(indexFilePath)) {
                QString directoryName = QFileInfo( indexFilePath ).dir().dirName();
                KCardTheme t( directoryName );
                if ( t.isValid() )
                    result << t;

            }
        }
    }
    return result;
}


QList<KCardTheme> KCardTheme::findAllWithFeatures( const QSet<QString> & neededFeatures )
{
    QList<KCardTheme> result;
    QStringList indexFiles = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "carddecks", QStandardPaths::LocateDirectory );
    Q_FOREACH (const QString &index, indexFiles) {
        const QStringList entries = QDir(index).entryList(QDir::Dirs);
        Q_FOREACH (const QString &d, entries) {
            QString indexFilePath = index + '/' + d + "/index.desktop";
            if (QFile::exists(indexFilePath)) {
                QString directoryName = QFileInfo( indexFilePath ).dir().dirName();
                KCardTheme t( directoryName );
                if ( t.isValid() && t.supportedFeatures().contains( neededFeatures ) )
                    result << t;

            }
        }
    }
    return result;
}


KCardTheme::KCardTheme()
  : d( 0 )
{
}


KCardTheme::KCardTheme( const QString & dirName )
{
    bool isValid = false;
    QString displayName;
    QString desktopFilePath;
    QString graphicsFilePath;
    QStringList supportedFeatures;
    QDateTime lastModified;

    QString indexFilePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString( "carddecks/%1/index.desktop" ).arg( dirName ) );
    if ( !indexFilePath.isEmpty() )
    {
        desktopFilePath = indexFilePath;

        KConfig config( indexFilePath, KConfig::SimpleConfig );
        if ( config.hasGroup( "KDE Backdeck" ) )
        {
            KConfigGroup configGroup = config.group( "KDE Backdeck" );

            displayName = configGroup.readEntry( "Name" );

            supportedFeatures = configGroup.readEntry( "Features", QStringList() << "AngloAmerican" << "Backs1" );

            QString svgName = configGroup.readEntry( "SVG" );
            if ( !svgName.isEmpty() )
            {
                QFileInfo indexFile( indexFilePath );
                QFileInfo svgFile( indexFile.dir(), svgName );
                graphicsFilePath = svgFile.absoluteFilePath();

                if ( svgFile.exists() )
                {
                    lastModified = qMax( svgFile.lastModified(), indexFile.lastModified() );
                    isValid = true;
                }
            }
        }
    }

    d = new KCardThemePrivate( isValid,
                               dirName,
                               displayName,
                               desktopFilePath,
                               graphicsFilePath,
                               supportedFeatures.toSet(),
                               lastModified );
}


KCardTheme::KCardTheme( const KCardTheme & other )
  : d( other.d )
{
}


KCardTheme::~KCardTheme()
{
}


KCardTheme & KCardTheme::operator=( const KCardTheme & other )
{
    d = other.d;
    return *this;
}


bool KCardTheme::isValid() const
{
    return d && d->isValid;
}


QString KCardTheme::dirName() const
{
    return d ? d->dirName : QString();
}


QString KCardTheme::displayName() const
{
    return d ? d->displayName : QString();
}


QString KCardTheme::desktopFilePath() const
{
    return d ? d->desktopFilePath : QString();
}


QString KCardTheme::graphicsFilePath() const
{
    return d ? d->graphicsFilePath : QString();
}


QDateTime KCardTheme::lastModified() const
{
    return d ? d->lastModified : QDateTime();
}


QSet<QString> KCardTheme::supportedFeatures() const
{
    return d ? d->supportedFeatures : QSet<QString>();
}


bool KCardTheme::operator==( KCardTheme theme ) const
{
    return dirName() == theme.dirName();
}


bool KCardTheme::operator!=( KCardTheme theme ) const
{
    return !operator==( theme );
}

