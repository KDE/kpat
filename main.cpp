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


#include "version.h"
#include "pwidget.h"
#include "freecell.h"
#include "cardmaps.h"

static const char description[] = I18N_NOOP("KDE Patience Game");

static KCmdLineOptions options[] =
{
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
    aboutData.addAuthor("Shlomi Fish", I18N_NOOP("Freecell Solver"), "shlomif@vipe.technion.ac.il");
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
#if 0
    cardMap c;
    Freecell *f = new Freecell();

    for ( int i = 0; i < 1000; i++ )
    {
        f->setGameNumber( i );
        f->restart();
        f->findSolution();
    }
    return 0;
#endif

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
