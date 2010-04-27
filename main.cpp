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
#include "KStandardCardDeck"

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


static const char description[] = I18N_NOOP("KDE Patience Game");

static DealerScene *getDealer( int wanted_game )
{
    foreach ( DealerInfo * di, DealerInfoList::self()->games() )
    {
        if ( di->ids().contains( wanted_game ) )
        {
            DealerScene * d = di->createGame();
            Q_ASSERT( d );
            d->setDeck( new KStandardCardDeck( KCardTheme(), d ) );
            d->initialize();

            if ( !d->solver() )
            {
                kError() << "There is no solver for" << di->name();
                return 0;
            }

            fprintf( stdout, "Testing %s\n", di->name() );
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

// A function that forces all pending resize events to happen immediately.
// Used to prevent expensive resize events after the scene has been created.
void sendAllPendingResizeEvents( QWidget * widget )
{
    bool foundOne;
    do
    {
        foundOne = false;
        QList<QWidget *> allChildWidgets = widget->findChildren<QWidget *>();
        allChildWidgets.prepend( widget );
        foreach( QWidget* w, allChildWidgets )
        {
            if ( w->testAttribute(Qt::WA_PendingResizeEvent) )
            {
                QResizeEvent e(w->size(), QSize());
                QApplication::sendEvent(w, &e);
                w->setAttribute(Qt::WA_PendingResizeEvent, false);
                // hack: make QTabWidget think it's visible; no layout otherwise
                w->setAttribute(Qt::WA_WState_Visible, true);
                foundOne = true;
            }
        }
        // Process LayoutRequest events, in particular
        qApp->sendPostedEvents();

        if ( !foundOne )
        {
            // Reset visible flag, to avoid crashes in qt
            foreach( QWidget* w, allChildWidgets )
                w->setAttribute(Qt::WA_WState_Visible, false);
        }
    } while ( foundOne );
}

int main( int argc, char **argv )
{
    KAboutData aboutData( "kpat", 0, ki18n("KPatience"),
                          KPAT_VERSION, ki18n(description), KAboutData::License_GPL_V2,
                          ki18n("© 1995 Paul Olav Tvete\n© 2000 Stephan Kulow"),
                          KLocalizedString(), "http://games.kde.org/kpat" );

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
                         "parker.coates@kdemail.net" );

    // Create a KLocale earlier than normal so that we can use i18n to translate
    // the names of the game types in the help text.
    KLocale *tmpLocale = new KLocale("kpat");
    QMap<QString, int> indexMap;
    QStringList gameList;
    foreach ( const DealerInfo *di, DealerInfoList::self()->games() )
    {
        const QString translatedKey = lowerAlphaNum( ki18n( di->name() ).toString(tmpLocale) );
        gameList << translatedKey;
        indexMap.insert( translatedKey, di->ids().first() );
        indexMap.insert( lowerAlphaNum( QString( di->name() ) ), di->ids().first() );
    }
    gameList.sort();
    delete tmpLocale;

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("solvegame <file>", ki18n( "Try to find a solution to the given savegame" ) );
    options.add("solve <num>", ki18n("Dealer to solve (debug)" ));
    options.add("start <num>", ki18n("Game range start (default 0:INT_MAX)" ));
    options.add("end <num>", ki18n("Game range end (default start:start if start given)" ));
    options.add("gametype <game>", ki18n("Skip the selection screen and load a particular game type. Valid values are: %1").subs(gameList.join(i18nc("List separator", ", "))));
    options.add("+file", ki18n("File to load"));
    KCmdLineArgs::addCmdLineOptions (options);
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    KApplication application;
    KGlobal::locale()->insertCatalog("libkdegames");

    QString savegame = args->getOption( "solvegame" );
    if ( !savegame.isEmpty() )
    {
        QFile of(savegame);
        of.open(QIODevice::ReadOnly);
        QDomDocument doc;
        doc.setContent(&of);

        DealerScene *f = getDealer( doc.documentElement().attribute("id").toInt() );

        f->openGame( doc );
        f->solver()->translate_layout();
        int ret = f->solver()->patsolve();
        if ( ret == Solver::WIN )
            fprintf( stdout, "won\n");
        else if ( ret == Solver::NOSOL )
            fprintf( stdout, "lost\n" );
        else
            fprintf( stdout, "unknown\n");

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
            f->startNew( i );
            f->solver()->translate_layout();
            int ret = f->solver()->patsolve();
            if ( ret == Solver::WIN )
                fprintf( stdout, "%d won (%d ms)\n", i, mytime.elapsed() );
            else if ( ret == Solver::NOSOL )
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
    sendAllPendingResizeEvents(w->menuBar());
    if (args->count())
    {
        if (!w->openGame(args->url(0)))
            w->slotShowGameSelectionScreen();
    }
    else if (indexMap.contains(gametype))
    {
        w->slotGameSelected(indexMap.value(gametype));
    }
    else if (savedState.exists())
    {
        if (!w->openGame(savedState.fileName(), false))
            w->slotShowGameSelectionScreen();
        savedState.remove();
    }
    else
    {
        w->slotShowGameSelectionScreen();
    }
    w->show();

    args->clear();
    return application.exec();
}
