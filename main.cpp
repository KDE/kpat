/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "dealer.h"
#include "dealerinfo.h"
#include "mainwindow.h"
#include "version.h"
#include "patsolve/patsolve.h"

#include "KCardTheme"
#include "KCardDeck"

#include <KAboutData>
#include <KCrash>

#include <QDebug>
#include <KLocalizedString>
#include <KDBusService>

#include <QFile>
#include <QTime>
#include <QResizeEvent>
#include <QDomDocument>

#include <Kdelibs4ConfigMigrator>
#include <climits>
#include <ctime>
#include <QStandardPaths>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

static DealerScene *getDealer( int wanted_game )
{
    foreach ( DealerInfo * di, DealerInfoList::self()->games() )
    {
        if ( di->providesId( wanted_game ) )
        {
            DealerScene * d = di->createGame();
            Q_ASSERT( d );
            d->setDeck( new KCardDeck( KCardTheme(), d ) );
            d->initialize();

            if ( !d->solver() )
            {
                qCritical() << "There is no solver for" << di->nameForId( wanted_game );;
                return 0;
            }

            return d;
        }
    }
    return 0;
}

// A function to remove all nonalphanumeric characters from a string
// and convert all letters to lowercase.
QString lowerAlphaNum( const QString & string )
{
    QString result;
    for ( int i = 0; i < string.size(); ++i )
    {
        QChar c = string.at( i );
        if ( c.isLetterOrNumber() )
            result += c.toLower();
    }
    return result;
}

int main( int argc, char **argv )
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kpat");

    Kdelibs4ConfigMigrator migrate(QStringLiteral("kpat"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kpatrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("kpatui.rc"));
    migrate.migrate();

    KAboutData aboutData( QStringLiteral("kpat"),
                          i18n("KPatience"),
                          KPAT_VERSION,
                          i18n("KDE Patience Game"),
                          KAboutLicense::GPL_V2,
                          i18n("© 1995 Paul Olav Tvete\n© 2000 Stephan Kulow"),
                          QString(),
                          QStringLiteral("http://games.kde.org/kpat") );

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.addAuthor( i18n("Paul Olav Tvete"),
                         i18n("Author of original Qt version"),
                         QStringLiteral("paul@troll.no") );
    aboutData.addAuthor( i18n("Mario Weilguni"),
                         i18n("Initial KDE port"),
                         QStringLiteral("mweilguni@kde.org") );
    aboutData.addAuthor( i18n("Matthias Ettrich"),
                         QString(),
                         QStringLiteral("ettrich@kde.org") );
    aboutData.addAuthor( i18n("Rodolfo Borges"),
                         i18n("New game types"),
                         QStringLiteral("barrett@9hells.org") );
    aboutData.addAuthor( i18n("Peter H. Ruegg"),
                         QString(),
                         QStringLiteral("kpat@incense.org") );
    aboutData.addAuthor( i18n("Michael Koch"),
                         i18n("Bug fixes"),
                         QStringLiteral("koch@kde.org") );
    aboutData.addAuthor( i18n("Marcus Meissner"),
                         i18n("Shuffle algorithm for game numbers"),
                         QStringLiteral("mm@caldera.de") );
    aboutData.addAuthor( i18n("Tom Holroyd"),
                         i18n("Initial patience solver"),
                         QStringLiteral("tomh@kurage.nimh.nih.gov") );
    aboutData.addAuthor( i18n("Stephan Kulow"),
                         i18n("Rewrite and current maintainer"),
                         QStringLiteral("coolo@kde.org") );
    aboutData.addAuthor( i18n("Erik Sigra"),
                         i18n("Klondike improvements"),
                         QStringLiteral("sigra@home.se") );
    aboutData.addAuthor( i18n("Josh Metzler"),
                         i18n("Spider implementation"),
                         QStringLiteral("joshdeb@metzlers.org") );
    aboutData.addAuthor( i18n("Maren Pakura"),
                         i18n("Documentation"),
                         QStringLiteral("maren@kde.org") );
    aboutData.addAuthor( i18n("Inge Wallin"),
                         i18n("Bug fixes"),
                         QStringLiteral("inge@lysator.liu.se") );
    aboutData.addAuthor( i18n("Simon Hürlimann"),
                         i18n("Menu and toolbar work"),
                         QStringLiteral("simon.huerlimann@huerlisi.ch") );
    aboutData.addAuthor( i18n("Parker Coates"),
                         i18n("Cleanup and polish"),
                         QStringLiteral("coates@kde.org") );

    // Create a KLocale earlier than normal so that we can use i18n to translate
    // the names of the game types in the help text.
    QMap<QString, int> indexMap;
    QStringList gameList;
    foreach ( const DealerInfo *di, DealerInfoList::self()->games() )
    {
        KLocalizedString localizedKey = ki18n( di->untranslatedBaseName().constData() );
        //QT5 const QString translatedKey = lowerAlphaNum( localizedKey.toString( tmpLocale ) );
        //QT5 gameList << translatedKey;
        //QT5 indexMap.insert( translatedKey, di->baseId() );
        indexMap.insert( di->baseIdString(), di->baseId() );
    }
    gameList.sort();
    const QString listSeparator = i18nc( "List separator", ", " );

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    KCrash::initialize();
    parser.addVersionOption();
    parser.addHelpOption();

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("solvegame"), i18n( "Try to find a solution to the given savegame" ), QStringLiteral("file")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("solve"), i18n("Dealer to solve (debug)" ), QStringLiteral("num")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("start"), i18n("Game range start (default 0:INT_MAX)" ), QStringLiteral("num")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("end"), i18n("Game range end (default start:start if start given)" ), QStringLiteral("num")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("gametype"), i18n("Skip the selection screen and load a particular game type. Valid values are: %1",gameList.join(listSeparator)), QStringLiteral("game")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("testdir"), i18n( "Directory with test cases" ), QStringLiteral("directory")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("generate"), i18n( "Generate random test cases" )));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("File to load"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kpat")));

    QString savegame = parser.value( QStringLiteral("solvegame") );
    if ( !savegame.isEmpty() )
    {
        QFile of(savegame);
        of.open(QIODevice::ReadOnly);
        QDomDocument doc;
        doc.setContent(&of);

        DealerScene *f = getDealer( doc.documentElement().attribute(QStringLiteral("id")).toInt() );

        f->loadLegacyFile( &of );
        f->solver()->translate_layout();
        int ret = f->solver()->patsolve();
        if ( ret == Solver::SolutionExists )
            fprintf( stdout, "won\n");
        else if ( ret == Solver::NoSolutionExists )
            fprintf( stdout, "lost\n" );
        else
            fprintf( stdout, "unknown\n");

        return 0;
    }

    QString testdir = parser.value(QStringLiteral("testdir"));
    if ( !testdir.isEmpty() ) {
       qsrand(std::time(nullptr));
       if ( parser.isSet(QStringLiteral("generate")) ) {
          for (int dealer = 0; dealer < 20; dealer++) {
              DealerScene *f = getDealer( dealer );
              if (!f) continue;
              int count = 100;
              QTime mytime;
              while (count) {
                if (f->deck()) f->deck()->stopAnimations();
                int i = qrand() % INT_MAX;
                f->startNew( i );
                mytime.start();
                f->solver()->translate_layout();
                int ret = f->solver()->patsolve();
                if ( ret == Solver::SolutionExists ) {
                   fprintf( stdout, "%d: %d won (%d ms)\n", dealer, i, mytime.elapsed() );
                   count--;
                   QFile file(QStringLiteral("%1/%2-%3-1").arg(testdir).arg(dealer).arg(i));
                   file.open( QFile::WriteOnly );
                   f->saveLegacyFile( &file );
                }
                else if ( ret == Solver::NoSolutionExists ) {
                   fprintf( stdout, "%d: %d lost (%d ms)\n", dealer, i, mytime.elapsed()  );
                   count--;
                   QFile file(QStringLiteral("%1/%2-%3-0").arg(testdir).arg(dealer).arg(i));
                   file.open( QFile::WriteOnly );
                   f->saveLegacyFile( &file );
                } else {
                   fprintf( stdout, "%d: %d unknown (%d ms)\n", dealer, i, mytime.elapsed() );
                }
             }
          }
       }
       return 0;
    }

    bool ok = false;
    int wanted_game = -1;
    if ( parser.isSet( QStringLiteral("solve") ) )
        wanted_game = parser.value(QStringLiteral("solve")).toInt( &ok );
    if ( ok )
    {
        ok = false;
        int end_index = -1;
        if ( parser.isSet( QStringLiteral("end") ) )
            end_index = parser.value(QStringLiteral("end")).toInt( &ok );
        if ( !ok )
            end_index = -1;
        ok = false;
        int start_index = -1;
        if ( parser.isSet( QStringLiteral("start") ) )
            start_index = parser.value(QStringLiteral("start")).toInt( &ok );
        if ( !ok ) {
            start_index = 0;
            end_index = INT_MAX;
        } else {
            if ( end_index == -1 )
                end_index = start_index;
        }
        DealerScene *f = getDealer( wanted_game );
        if ( !f )
            return 1;

        QTime mytime;
        for ( int i = start_index; i <= end_index; i++ )
        {
            mytime.start();
            f->deck()->stopAnimations();
            f->startNew( i );
            f->solver()->translate_layout();
            int ret = f->solver()->patsolve();
            if ( ret == Solver::SolutionExists )
                fprintf( stdout, "%d won (%d ms)\n", i, mytime.elapsed() );
            else if ( ret == Solver::NoSolutionExists )
                fprintf( stdout, "%d lost (%d ms)\n", i, mytime.elapsed()  );
            else
                fprintf( stdout, "%d unknown (%d ms)\n", i, mytime.elapsed() );
        }
        fprintf( stdout, "all_moves %ld\n", all_moves );
        return 0;
    }

    QString gametype = parser.value(QStringLiteral("gametype")).toLower();
    QFile savedState( QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + saved_state_file);

    MainWindow *w = new MainWindow;
    if (parser.positionalArguments().count())
    {
        if ( !w->loadGame( QUrl::fromLocalFile(parser.positionalArguments().at( 0 )), true ) )
            w->slotShowGameSelectionScreen();
    }
    else if (indexMap.contains(gametype))
    {
        w->slotGameSelected(indexMap.value(gametype));
    }
    else if (savedState.exists())
    {
        if ( !w->loadGame( QUrl::fromLocalFile(savedState.fileName()), false ) )
            w->slotShowGameSelectionScreen();
    }
    else
    {
        w->slotShowGameSelectionScreen();
    }
    w->show();

    const KDBusService dbusService(KDBusService::Multiple);

    return app.exec();
}
