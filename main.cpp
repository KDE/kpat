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
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KMenuBar>
#include <KStandardDirs>
#include <KUrl>

#include <QtCore/QFile>
#include <QtCore/QTime>
#include <QtGui/QResizeEvent>
#include <QtXml/QDomDocument>

#include <climits>
#include <time.h>

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
                kError() << "There is no solver for" << di->nameForId( wanted_game );;
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
    KAboutData aboutData( "kpat",
                          0,
                          ki18n("KPatience"),
                          KPAT_VERSION,
                          ki18n("KDE Patience Game"),
                          KAboutData::License_GPL_V2,
                          ki18n("© 1995 Paul Olav Tvete\n© 2000 Stephan Kulow"),
                          KLocalizedString(),
                          "http://games.kde.org/kpat" );

    aboutData.addAuthor( ki18n("Paul Olav Tvete"),
                         ki18n("Author of original Qt version"),
                         "paul@troll.no" );
    aboutData.addAuthor( ki18n("Mario Weilguni"),
                         ki18n("Initial KDE port"),
                         "mweilguni@kde.org" );
    aboutData.addAuthor( ki18n("Matthias Ettrich"),
                         KLocalizedString(),
                         "ettrich@kde.org" );
    aboutData.addAuthor( ki18n("Rodolfo Borges"),
                         ki18n("New game types"),
                         "barrett@9hells.org" );
    aboutData.addAuthor( ki18n("Peter H. Ruegg"),
                         KLocalizedString(),
                         "kpat@incense.org" );
    aboutData.addAuthor( ki18n("Michael Koch"),
                         ki18n("Bug fixes"),
                         "koch@kde.org" );
    aboutData.addAuthor( ki18n("Marcus Meissner"),
                         ki18n("Shuffle algorithm for game numbers"),
                         "mm@caldera.de" );
    aboutData.addAuthor( ki18n("Tom Holroyd"),
                         ki18n("Initial patience solver"),
                         "tomh@kurage.nimh.nih.gov" );
    aboutData.addAuthor( ki18n("Stephan Kulow"),
                         ki18n("Rewrite and current maintainer"),
                         "coolo@kde.org" );
    aboutData.addAuthor( ki18n("Erik Sigra"),
                         ki18n("Klondike improvements"),
                         "sigra@home.se" );
    aboutData.addAuthor( ki18n("Josh Metzler"),
                         ki18n("Spider implementation"),
                         "joshdeb@metzlers.org" );
    aboutData.addAuthor( ki18n("Maren Pakura"),
                         ki18n("Documentation"),
                         "maren@kde.org" );
    aboutData.addAuthor( ki18n("Inge Wallin"),
                         ki18n("Bug fixes"),
                         "inge@lysator.liu.se" );
    aboutData.addAuthor( ki18n("Simon Hürlimann"),
                         ki18n("Menu and toolbar work"),
                         "simon.huerlimann@huerlisi.ch" );
    aboutData.addAuthor( ki18n("Parker Coates"),
                         ki18n("Cleanup and polish"),
                         "coates@kde.org" );

    // Create a KLocale earlier than normal so that we can use i18n to translate
    // the names of the game types in the help text.
    KLocale *tmpLocale = new KLocale("kpat");
    QMap<QString, int> indexMap;
    QStringList gameList;
    foreach ( const DealerInfo *di, DealerInfoList::self()->games() )
    {
        KLocalizedString localizedKey = ki18n( di->untranslatedBaseName().constData() );
        const QString translatedKey = lowerAlphaNum( localizedKey.toString( tmpLocale ) );
        gameList << translatedKey;
        indexMap.insert( translatedKey, di->baseId() );
        indexMap.insert( di->baseIdString(), di->baseId() );
    }
    gameList.sort();
    const QString listSeparator = ki18nc( "List separator", ", " ).toString( tmpLocale );
    delete tmpLocale;

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("solvegame <file>", ki18n( "Try to find a solution to the given savegame" ) );
    options.add("solve <num>", ki18n("Dealer to solve (debug)" ));
    options.add("start <num>", ki18n("Game range start (default 0:INT_MAX)" ));
    options.add("end <num>", ki18n("Game range end (default start:start if start given)" ));
    options.add("gametype <game>", ki18n("Skip the selection screen and load a particular game type. Valid values are: %1").subs(gameList.join(listSeparator)));
    options.add("testdir <directory>", ki18n( "Directory with test cases" ) );
    options.add("generate", ki18n( "Generate random test cases" ) );
    options.add("+file", ki18n("File to load"));
    KCmdLineArgs::addCmdLineOptions (options);
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    KApplication application;
    KGlobal::locale()->insertCatalog( QLatin1String( "libkdegames" ));

    QString savegame = args->getOption( "solvegame" );
    if ( !savegame.isEmpty() )
    {
        QFile of(savegame);
        of.open(QIODevice::ReadOnly);
        QDomDocument doc;
        doc.setContent(&of);

        DealerScene *f = getDealer( doc.documentElement().attribute("id").toInt() );

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

    QString testdir = args->getOption("testdir");
    if ( !testdir.isEmpty() ) {
       qsrand(time(0));
       if ( args->isSet("generate") ) {
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
                   QFile file(QString("%1/%2-%3-1").arg(testdir).arg(dealer).arg(i));
                   file.open( QFile::WriteOnly );
                   f->saveLegacyFile( &file );
                }
                else if ( ret == Solver::NoSolutionExists ) {
                   fprintf( stdout, "%d: %d lost (%d ms)\n", dealer, i, mytime.elapsed()  );
                   count--;
                   QFile file(QString("%1/%2-%3-0").arg(testdir).arg(dealer).arg(i));
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
    if ( args->isSet( "solve" ) )
        wanted_game = args->getOption("solve").toInt( &ok );
    if ( ok )
    {
        ok = false;
        int end_index = -1;
        if ( args->isSet( "end" ) )
            end_index = args->getOption("end").toInt( &ok );
        if ( !ok )
            end_index = -1;
        ok = false;
        int start_index = -1;
        if ( args->isSet( "start" ) )
            start_index = args->getOption("start").toInt( &ok );
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

    QString gametype = args->getOption("gametype").toLower();
    QFile savedState( KStandardDirs::locateLocal("appdata", saved_state_file) );

    MainWindow *w = new MainWindow;
    if (args->count())
    {
        if ( !w->loadGame( args->url( 0 ), true ) )
            w->slotShowGameSelectionScreen();
    }
    else if (indexMap.contains(gametype))
    {
        w->slotGameSelected(indexMap.value(gametype));
    }
    else if (savedState.exists())
    {
        if ( !w->loadGame( savedState.fileName(), false ) )
            w->slotShowGameSelectionScreen();
    }
    else
    {
        w->slotShowGameSelectionScreen();
    }
    w->show();

    args->clear();
    return application.exec();
}
