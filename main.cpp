/*
     patience -- main program
      Copyright (C) 1995  Paul Olav Tvete <paul@troll.no>

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

 */

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include <QDomDocument>

#include <stdio.h>
#include <limits.h>

#include "version.h"
#include "pwidget.h"
#include "dealer.h"
#include "patsolve/patsolve.h"
#include "cardmaps.h"

static const char description[] = I18N_NOOP("KDE Patience Game");

static DealerScene *getDealer( int wanted_game )
{
    DealerInfo *di = 0;
    for (QList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin();
         it != DealerInfoList::self()->games().end(); ++it)
    {
        if ( (*it)->gameindex == wanted_game ) {
            di = *it;
            DealerScene *f = di->createGame();
            if ( !f || !f->solver() ) {
                kError() << "There is no solver for" << di->name;
                return 0;
            }

            fprintf( stdout, "Testing %s\n", di->name );
            return f;
        }
    }
    return 0;
}

int main( int argc, char **argv )
{
    KAboutData aboutData( "kpat", 0, ki18n("KPatience"),
                          KPAT_VERSION, ki18n(description), KAboutData::License_GPL,
                          ki18n("(c) 1995, Paul Olav Tvete\n"
                          "(c) 2000 Stephan Kulow"));
    aboutData.addAuthor(ki18n("Paul Olav Tvete"),KLocalizedString(),"paul@troll.no");
    aboutData.addAuthor(ki18n("Mario Weilguni"),KLocalizedString(),"mweilguni@kde.org");
    aboutData.addAuthor(ki18n("Matthias Ettrich"),KLocalizedString(),"ettrich@kde.org");
    aboutData.addAuthor(ki18n("Rodolfo Borges"),ki18n("Some Game Types"),"barrett@9hells.org");
    aboutData.addAuthor(ki18n("Peter H. Ruegg"),KLocalizedString(),"kpat@incense.org");
    aboutData.addAuthor(ki18n("Michael Koch"), ki18n("Bug fixes"), "koch@kde.org");
    aboutData.addAuthor(ki18n("Marcus Meissner"), ki18n("Shuffle algorithm for game numbers"),
                        "mm@caldera.de");
    aboutData.addAuthor(ki18n("Dr. Tom"), ki18n("Patience Solver"), "http://members.tripod.com/professor_tom/");
    aboutData.addAuthor(ki18n("Stephan Kulow"), ki18n("Rewrite and current maintainer"),
                        "coolo@kde.org");
    aboutData.addAuthor(ki18n("Erik Sigra"), ki18n("Improved Klondike"), "sigra@home.se");
    aboutData.addAuthor(ki18n("Josh Metzler"), ki18n("Spider Implementation"), "joshdeb@metzlers.org");
    aboutData.addAuthor(ki18n("Maren Pakura"), ki18n("Documentation"), "maren@kde.org");
    aboutData.addAuthor(ki18n("Inge Wallin"), ki18n("Bug fixes"), "inge@lysator.liu.se");
    aboutData.addAuthor(ki18n("Simon Hürlimann"), ki18n("Menu and Toolbar work"), "simon.huerlimann@huerlisi.ch");

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("solvegame <file>", ki18n( "Try to find a solution to the given savegame" ) );
    options.add("solve <num>", ki18n("Dealer to solve (debug)" ));
    options.add("start <num>", ki18n("Game range start (default 0:INT_MAX)" ));
    options.add("end <num>", ki18n("Game range end (default start:start if start given)" ));
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

        cardMap c;
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
        cardMap c;
        DealerScene *f = getDealer( wanted_game );
        if ( !f )
            return 1;

        QTime mytime;
        for ( int i = start_index; i <= end_index; i++ )
        {
            mytime.start();
            f->setGameNumber( i );
            f->restart();
            f->solver()->translate_layout();
            int ret = f->solver()->patsolve();
            if ( ret == Solver::WIN )
                fprintf( stdout, "%d won (%d ms)\n", i, mytime.elapsed() );
            else if ( ret == Solver::NOSOL )
                fprintf( stdout, "%d lost (%d ms)\n", i, mytime.elapsed()  );
            else
                fprintf( stdout, "%d unknown (%d ms)\n", i, mytime.elapsed() );
        }
        return 0;
    }

    if (application.isSessionRestored())
        RESTORE(pWidget)
    else {
        pWidget *w = new pWidget;
        if (args->count())
            w->openGame(args->url(0));
        else
            QTimer::singleShot(100, w, SLOT(slotNewGameType()));
    }
    args->clear();
    return application.exec();
}
