/*
     patience -- main program
      Copyright (C) 1995  Paul Olav Tvete

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
#include <kurl.h>
#include <qtimer.h>
#include <stdio.h>
#include <kdebug.h>
#include <limits.h>

#include "version.h"
#include "pwidget.h"
#include "dealer.h"
#include "patsolve/patsolve.h"
#include "cardmaps.h"

static const char description[] = I18N_NOOP("KDE Patience Game");

static KCmdLineOptions options[] =
{
    { "solve <num>",    I18N_NOOP("Dealer to solve (debug)" ), 0 },
    { "start <num>",    I18N_NOOP("Game range start (default 0:INT_MAX)" ), 0 },
    { "end <num>",      I18N_NOOP("Game range end (default start:start if start given)" ), 0 },
    { "+file",          I18N_NOOP("File to load"), 0 },
    KCmdLineLastOption
};

int main( int argc, char **argv )
{
    KAboutData aboutData( "kpat", I18N_NOOP("KPatience"),
                          KPAT_VERSION, description, KAboutData::License_GPL,
                          "(c) 1995, Paul Olav Tvete\n"
                          "(c) 2000 Stephan Kulow");
    aboutData.addAuthor("Paul Olav Tvete");
    aboutData.addAuthor("Mario Weilguni",0,"mweilguni@kde.org");
    aboutData.addAuthor("Matthias Ettrich",0,"ettrich@kde.org");
    aboutData.addAuthor("Rodolfo Borges",I18N_NOOP("Some Game Types"),"barrett@9hells.org");
    aboutData.addAuthor("Peter H. Ruegg",0,"kpat@incense.org");
    aboutData.addAuthor("Michael Koch", I18N_NOOP("Bug fixes"), "koch@kde.org");
    aboutData.addAuthor("Marcus Meissner", I18N_NOOP("Shuffle algorithm for game numbers"),
                        "mm@caldera.de");
    aboutData.addAuthor("Dr. Tom", I18N_NOOP("Patience Solver"), "http://members.tripod.com/professor_tom/");
    aboutData.addAuthor("Stephan Kulow", I18N_NOOP("Rewrite and current maintainer"),
                        "coolo@kde.org");
    aboutData.addAuthor("Erik Sigra", I18N_NOOP("Improved Klondike"), "sigra@home.se");
    aboutData.addAuthor("Josh Metzler", I18N_NOOP("Spider Implementation"), "joshdeb@metzlers.org");
    aboutData.addAuthor("Maren Pakura", I18N_NOOP("Documentation"), "maren@kde.org");
    aboutData.addAuthor("Inge Wallin", I18N_NOOP("Bug fixes"), "inge@lysator.liu.se");

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions (options);
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    KApplication application;
    KGlobal::locale()->insertCatalog("libkdegames");

    bool ok = false;
    int wanted_game = -1;
    if ( args->isSet( "solve" ) )
        wanted_game = args->getOption("solve").toInt( &ok );
    if ( ok )
    {
        DealerInfo *di = 0;
        for (QList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin();
             it != DealerInfoList::self()->games().end(); ++it)
        {
            if ( (*it)->gameindex == wanted_game ) {
                di = *it;
                break;
            }
        }

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
        if ( !di ) {
            kError() << "There is no game with index " << wanted_game << endl;
            return -1;
        }

        cardMap c;
        DealerScene *f = di->createGame();
        if ( !f->solver() ) {
            kError() << "There is no solver for " << di->name << endl;
            return -1;
        }

        fprintf( stdout, "Testing %s\n", di->name );

        for ( int i = start_index; i <= end_index; i++ )
        {
            f->setGameNumber( i );
            f->restart();
            int ret = f->solver()->patsolve();
            if ( ret == Solver::WIN )
                fprintf( stderr, "%d won\n", i );
            else if ( ret == Solver::NOSOL )
                fprintf( stdout, "%d lost\n", i );
            else
                fprintf( stdout, "%d unknown\n", i );
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
            QTimer::singleShot(0, w, SLOT(slotNewGameType()));
    }
    return application.exec();
}
