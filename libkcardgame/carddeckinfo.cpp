/*
    This file is part of the KDE games library
    Copyright 2008 Andreas Pakulat <apaku@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "carddeckinfo.h"
#include "carddeckinfo_p.h"

#include <QFileInfo>
#include <QDir>

#include <klocale.h>
#include <kstandarddirs.h>
#include <krandom.h>
#include <kdebug.h>
#include <kconfiggroup.h>
#include <kglobal.h>

// KConfig entries
#define CONF_LOCKING QString::fromLatin1("Locking")
#define CONF_ALLOW_FIXED_CARDS QString::fromLatin1("AllowFixed")
#define CONF_CARD QString::fromLatin1("Cardname")
#define CONF_DECK QString::fromLatin1("Deckname")

/**
 * Local static information.
 */
class KCardThemeInfoStatic
{
public:
    KCardThemeInfoStatic()
    {
        KGlobal::dirs()->addResourceType( "cards", "data", "carddecks/" );
        KGlobal::locale()->insertCatalog( "libkdegames" );
        readBacks();
        readFronts();
    }
    ~KCardThemeInfoStatic()
    {
    }
    
    void readFronts()
    {
        // Empty data
        pngFrontInfo.clear();
        svgFrontInfo.clear();

        QStringList svg;
        // Add SVG card sets
        svg = KGlobal::dirs()->findAllResources( "cards", "svg*/index.desktop", KStandardDirs::NoDuplicates );
        const QStringList list = svg + KGlobal::dirs()->findAllResources( "cards", "card*/index.desktop", KStandardDirs::NoDuplicates );

        if ( list.isEmpty() ) return;

        for ( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it )
        {
            KConfig cfg( *it, KConfig::SimpleConfig );
            KConfigGroup cfgcg( &cfg, "KDE Backdeck" );
            QString path = ( *it ).left(( *it ).lastIndexOf( '/' ) + 1 );
            Q_ASSERT( path[path.length() - 1] == '/' );
            QPixmap pixmap( path + cfgcg.readEntry( "Preview", "12c.png" ) );
            if ( pixmap.isNull() ) continue;

            QString idx  = cfgcg.readEntryUntranslated( "Name", i18n( "unnamed" ) );
            QString name = cfgcg.readEntry( "Name", i18n( "unnamed" ) );
            KCardThemeInfo info;
            info.name         = name;
            info.noi18Name    = idx;
            info.comment      = cfgcg.readEntry( "Comment", QString() );
            info.preview      = pixmap;
            info.path         = path;
            info.back         = cfgcg.readEntry( "Back", QString() );
            // if (!info.back.isNull()) kDebug() << "FOUND BACK " << info.back;
            info.isDefault    = cfgcg.readEntry( "Default", false );

            QString svg    = cfgcg.readEntry( "SVG", QString() );
            if ( !svg.isEmpty() )
            {
                QFileInfo svgInfo( QDir( path ), svg );
                info.svgfile = svgInfo.filePath();
                svgFrontInfo[idx] = info;
            }
            else
            {
                info.svgfile.clear();
                pngFrontInfo[idx] = info;
            }
        }

    }


    void readBacks()
    {
        // Empty data
        svgBackInfo.clear();
        pngBackInfo.clear();
    
        const QStringList list = KGlobal::dirs()->findAllResources( "cards", "decks/*.desktop", KStandardDirs::NoDuplicates );

        if ( list.isEmpty() ) return;
    
        for ( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it )
        {
            KConfig cfg( *it, KConfig::SimpleConfig );
            QString path = ( *it ).left(( *it ).lastIndexOf( '/' ) + 1 );
            Q_ASSERT( path[path.length() - 1] == '/' );
            QPixmap pixmap( getBackFileNameFromIndex( *it ) );
            if ( pixmap.isNull() ) continue;

            KConfigGroup cfgcg( &cfg, "KDE Cards" );
            QString idx  = cfgcg.readEntryUntranslated( "Name", i18n( "unnamed" ) );
            QString name = cfgcg.readEntry( "Name", i18n( "unnamed" ) );
            KCardThemeInfo info;
            info.name         = name;
            info.noi18Name    = idx;
            info.path         = getBackFileNameFromIndex( *it );
            info.comment      = cfgcg.readEntry( "Comment", QString() );
            info.preview      = pixmap;
            info.isDefault    = cfgcg.readEntry( "Default", false );
    
            QString svg    = cfgcg.readEntry( "SVG", QString() );
            if ( !svg.isEmpty() )
            {
                QFileInfo svgInfo( QDir( path ), svg );
                info.svgfile = svgInfo.filePath();
                svgBackInfo[idx] = info;
            }
            else
            {
                info.svgfile.clear();
                pngBackInfo[idx] = info;
            }
        }
    
    }

    QString getBackFileNameFromIndex( const QString& desktop )
    {
        QString entry = desktop.left( desktop.length() - strlen( ".desktop" ) );
        if ( KStandardDirs::exists( entry + QString::fromLatin1( ".png" ) ) )
            return entry + QString::fromLatin1( ".png" );

        // rather theoretical
        if ( KStandardDirs::exists( entry + QString::fromLatin1( ".xpm" ) ) )
            return entry + QString::fromLatin1( ".xpm" );
   
        Q_ASSERT( false );
        return QString();
    }


    /** The card front sides for PNG decks.
     */
    QMap<QString, KCardThemeInfo> pngFrontInfo;

    /** The card front sides for SVG decks.
     */
    QMap<QString, KCardThemeInfo> svgFrontInfo;

    /** The card back sides for PNG decks.
     */
    QMap<QString, KCardThemeInfo> pngBackInfo;

    /** The card back sides for SVG decks.
     */
    QMap<QString, KCardThemeInfo> svgBackInfo;

    /** The default front side name.
     */
    QString defaultFront;

    /** The default back side name.
     */
    QString defaultBack;
};

K_GLOBAL_STATIC( KCardThemeInfoStatic, deckinfoStatic )

QDebug operator<<(QDebug debug, const KCardThemeInfo &cn)
{
    return debug << "name: " << cn.name
                 << " noi18Name: " << cn.noi18Name
                 << " comment: " << cn.comment
                 << " path: " << cn.path
                 << " back: " << cn.back
                 << " preview: " << cn.preview
                 << " svgfile: " << cn.svgfile
                 << " isDefault: " << cn.isDefault;
}

namespace CardDeckInfo
{

// Retrieve default card set name
QString defaultFrontName( bool pAllowPNG )
{
    QString noDefault;
    // Count filtered cards
    QMap<QString, KCardThemeInfo> temp = deckinfoStatic->svgFrontInfo;
    if ( pAllowPNG )
    {
        temp.unite( deckinfoStatic->pngFrontInfo );
    }
    QMapIterator<QString, KCardThemeInfo> it( temp );
    while ( it.hasNext() )
    {
        it.next();
        KCardThemeInfo v = it.value();
        // Filter
        if ( v.isDefault ) return v.noi18Name;
        // Collect any deck if no default is stored
        noDefault = v.noi18Name;
    }
    if ( noDefault.isNull() ) kError() << "Could not find default card name";
    return noDefault;
}


// Retrieve default deck name
QString defaultBackName( bool pAllowPNG )
{
    QString noDefault;
    QMap<QString, KCardThemeInfo> temp = deckinfoStatic->svgBackInfo;
    if ( pAllowPNG )
    {
        temp.unite( deckinfoStatic->pngBackInfo );
    }

    QMapIterator<QString, KCardThemeInfo> it( temp );
    while ( it.hasNext() )
    {
        it.next();
        KCardThemeInfo v = it.value();
        // Filter
        if ( v.isDefault ) return v.noi18Name;
        // Collect any deck if no default is stored
        noDefault = v.noi18Name;
    }
    if ( noDefault.isNull() ) kError() << "Could not find default deck name";
    return noDefault;
}


// Retrieve a random card name
QString randomFrontName( bool pAllowPNG )
{
    // Collect matching items
    QStringList list = deckinfoStatic->svgFrontInfo.keys();
    if ( pAllowPNG )
    {
        list += deckinfoStatic->pngFrontInfo.keys();
    }

    // Draw random one
    int d = KRandom::random() % list.count();
    return list.at( d );
}


// Retrieve a random deck name
QString randomBackName( bool pAllowPNG )
{
    // Collect matching items
    QStringList list = deckinfoStatic->svgBackInfo.keys();
    if ( pAllowPNG )
    {
        list += deckinfoStatic->pngBackInfo.keys();
    }

    // Draw random one
    int d = KRandom::random() % list.count();
    return list.at( d );
}


// Retrieve the PNG filename for a back side from its index.desktop filename
QString getBackFileNameFromIndex( const QString &desktop )
{
    QString entry = desktop.left( desktop.length() - strlen( ".desktop" ) );
    if ( KStandardDirs::exists( entry + QString::fromLatin1( ".png" ) ) )
        return entry + QString::fromLatin1( ".png" );

    // rather theoretical
    if ( KStandardDirs::exists( entry + QString::fromLatin1( ".xpm" ) ) )
        return entry + QString::fromLatin1( ".xpm" );
    return QString();
}



// Retrieve the SVG file belonging to the given card back deck.
QString backSVGFilePath( const QString& name )
{
    if ( !deckinfoStatic->svgBackInfo.contains( name ) ) return QString();
    const KCardThemeInfo &v = deckinfoStatic->svgBackInfo[name];
    return v.svgfile;
}


// Retrieve the SVG file belonging to the given card fronts.
QString frontSVGFilePath( const QString& name )
{
    if ( !deckinfoStatic->svgFrontInfo.contains( name ) ) return QString();
    const KCardThemeInfo &v = deckinfoStatic->svgFrontInfo[name];
    return v.svgfile;
}


// Retrieve the PNG file belonging to the given card back deck.
QString backFilename( const QString& name )
{
    if ( !deckinfoStatic->pngBackInfo.contains( name ) ) return QString();
    const KCardThemeInfo &v = deckinfoStatic->pngBackInfo[name];
    return v.path;
}


// Retrieve the directory belonging to the given card fronts.
QString frontDir( const QString& name )
{
    if ( !deckinfoStatic->pngFrontInfo.contains( name ) ) return QString();
    const KCardThemeInfo &v = deckinfoStatic->pngFrontInfo[name];
    return v.path;
}


// Check whether a card set is SVG
bool isSVGFront( const QString& name )
{
    return deckinfoStatic->svgFrontInfo.contains( name );
}


// Check whether a card deck is SVG
bool isSVGBack( const QString& name )
{
    return deckinfoStatic->svgBackInfo.contains( name );
}

QStringList frontNames()
{
    return ( deckinfoStatic->svgFrontInfo.keys() + deckinfoStatic->pngFrontInfo.keys() );
}

QStringList backNames()
{
    return ( deckinfoStatic->svgBackInfo.keys() + deckinfoStatic->pngBackInfo.keys() );
}

KCardThemeInfo frontInfo( const QString& name )
{
    if ( deckinfoStatic->svgFrontInfo.contains( name ) )
        return deckinfoStatic->svgFrontInfo.value( name );
    if ( deckinfoStatic->pngFrontInfo.contains( name ) )
        return deckinfoStatic->pngFrontInfo.value( name );
    return KCardThemeInfo();
}

KCardThemeInfo backInfo( const QString& name )
{
    if ( deckinfoStatic->svgBackInfo.contains( name ) )
        return deckinfoStatic->svgBackInfo.value( name );
    if ( deckinfoStatic->pngBackInfo.contains( name ) )
        return deckinfoStatic->pngBackInfo.value( name );
    return KCardThemeInfo();
}

QString frontTheme( const KConfigGroup& group, const QString& defaultTheme )
{
    QString theme = group.readEntry( CONF_CARD, defaultTheme );
    if (!frontNames().contains(theme))
        return defaultTheme;
    return theme;
}

QString backTheme( const KConfigGroup& group, const QString& defaultTheme )
{
    QString theme = group.readEntry( CONF_DECK, defaultTheme );
    if (!backNames().contains(theme))
        return defaultTheme;
    return theme;
}

bool allowFixedSizeDecks( const KConfigGroup& group, bool lockDefault )
{
  return group.readEntry( CONF_ALLOW_FIXED_CARDS, lockDefault );
}

bool lockFrontToBackside(const KConfigGroup& group, bool lockDefault)
{
  return group.readEntry( CONF_LOCKING, lockDefault );
}

void writeFrontTheme( KConfigGroup& group, const QString& theme )
{
  group.writeEntry( CONF_CARD, theme );
}

void writeBackTheme( KConfigGroup& group, const QString& theme )
{
  group.writeEntry( CONF_DECK, theme );
}

void writeAllowFixedSizeDecks( KConfigGroup& group, bool allowFixedSize )
{
  group.writeEntry( CONF_ALLOW_FIXED_CARDS, allowFixedSize );
}

void writeLockFrontToBackside( KConfigGroup& group, bool lock )
{
  group.writeEntry( CONF_LOCKING, lock );
}

}

