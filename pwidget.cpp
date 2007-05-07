/*
   patience -- main program
     Copyright (C) 1995  Paul Olav Tvete

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation.

   This file is provided AS IS with no warranties of any kind.  The author
   shall have no liability with respect to the infringement of copyrights,
   trade secrets or any patents by this file or any part thereof.  In no
   event will the author be liable for any lost revenue or profits or
   other special, indirect and consequential damages.


   Heavily modified by Mario Weilguni <mweilguni@sime.com>
*/

#include "pwidget.h"

#include <stdio.h>

#include <QRegExp>
#include <QTimer>
#include <qimage.h>
//Added by qt3to4:
#include <QPixmap>
#include <QTextStream>
#include <QList>
#include <QShowEvent>
#include <QDomDocument>
#include <QSvgRenderer>

#include <kapplication.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kdebug.h>
#include <kcarddialog.h>
#include <krandom.h>
#include <kinputdialog.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <ktemporaryfile.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kacceleratormanager.h>
#include <kmenubar.h>
#include <ktoolinvocation.h>
#include <kglobal.h>
#include <kicon.h>
#include <kconfiggroup.h>

#include "pile.h"
#include "version.h"
#include "dealer.h"
#include "view.h"
#include "cardmaps.h"
#include "gamestatsimpl.h"


static pWidget *current_pwidget = 0;

pWidget::pWidget()
    : KXmlGuiWindow(0), dill(0)
{
    setObjectName( "pwidget" );
    current_pwidget = this;
    // KCrash::setEmergencySaveFunction(::saveGame);
    actionCollection()->addAction( KStandardAction::Quit, "game_exit",
                                   kapp, SLOT(quit()) );

    undo = actionCollection()->addAction( KStandardAction::Undo, "undo_move",
                                          this, SLOT(undoMove()) );
    undo->setEnabled(false);
    actionCollection()->addAction( KStandardAction::New, "new_game",
                                   this, SLOT(newGame()) );
    actionCollection()->addAction( KStandardAction::Open, "open",
                                   this, SLOT(openGame()) );
    recent = KStandardAction::openRecent(this, SLOT(openGame(const KUrl&)), this);
    actionCollection()->addAction("open_recent", recent);
    recent->loadEntries( KGlobal::config()->group( QString() )) ;
    actionCollection()->addAction(KStandardAction::SaveAs, "save",
                                  this, SLOT(saveGame()));
    QAction *a = actionCollection()->addAction("choose_game");
    a->setText(i18n("&Choose Game..."));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( chooseGame() ) );

    a = actionCollection()->addAction("restart_game");
    a->setText(i18n("Restart &Game"));
    a->setIcon(KIcon("view-refresh"));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( restart() ) );

    actionCollection()->addAction(KStandardAction::Help, "help_game",
                                  this, SLOT(helpGame()));
    games = new KSelectAction(i18n("&Game Type"), this);
    actionCollection()->addAction("game_type", games);
    connect( games, SIGNAL( triggered( int ) ), SLOT( slotNewGameType() ) );

    QStringList list;
    QList<DealerInfo*>::ConstIterator it;
    int max_type = 0;

    for (it = DealerInfoList::self()->games().begin();
         it != DealerInfoList::self()->games().end(); ++it)
    {
        // while we develop, it may happen that some lower
        // indices do not exist
        int index = (*it)->gameindex;
        for (int i = 0; i <= index; i++)
            if (list.count() <= i)
                list.append("unknown");
        list[index] = i18n((*it)->name);
        if (max_type < index)
            max_type = index;
    }
    games->setItems(list);

    KGlobal::dirs()->addResourceType("wallpaper", KStandardDirs::kde_default("data") + "kpat/backgrounds/");

    dill = new PatienceView( this );
    connect(dill, SIGNAL(saveGame()), SLOT(saveGame()));
    setCentralWidget(dill);

    a = actionCollection()->addAction("random_set");
    a->setText(i18n("Random Cards"));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( slotPickRandom() ) );
    a->setShortcuts( KShortcut( Qt::Key_F9 ) );

    m_cards = new cardMap();

    stats = actionCollection()->addAction("game_stats");
    stats->setText(i18n("Statistics"));
    connect( stats, SIGNAL( triggered( bool ) ), SLOT(showStats()) );

    dropaction = new KToggleAction(i18n("&Enable Autodrop"), this);
    actionCollection()->addAction("enable_autodrop", dropaction);
    connect( dropaction, SIGNAL( triggered( bool ) ), SLOT(enableAutoDrop()) );
    dropaction->setCheckedState(KGuiItem(i18n("Disable Autodrop")));

    KConfigGroup cg(KGlobal::config(), settings_group );

    bool autodrop = cg.readEntry("Autodrop", true);
    dropaction->setChecked(autodrop);

    int game = cg.readEntry("DefaultGame", 0);
    if (game > max_type)
        game = max_type;
    games->setCurrentItem(game);

    statusBar()->insertPermanentItem( "", 1, 0 );
    createGUI(); // QString::null/*, false*/);
    //KAcceleratorManager::manage(menuBar());

    // QTimer::singleShot( 0, this, SLOT( newGameType() ) );
    setAutoSaveSettings();
}

pWidget::~pWidget()
{
    delete dill;
    delete m_cards;
    delete Pile::pileRenderer();
}

void pWidget::undoMove() {
    if( dill && dill->dscene() )
        dill->dscene()->undo();
}

void pWidget::helpGame()
{
    if (!dill)
        return;
    KToolInvocation::invokeHelp(dill->anchorName());
}

void pWidget::undoPossible(bool poss)
{
    undo->setEnabled(poss);
}

void pWidget::enableAutoDrop()
{
    bool drop = dropaction->isChecked();
    KConfigGroup cg(KGlobal::config(), settings_group );
    cg.writeEntry( "Autodrop", drop);
    dill->setAutoDropEnabled(drop);
}

void pWidget::newGame()
{
    // Check if the user is already running a game, and if she is,
    // then ask if she wants to abort it.
    if (!dill->dscene()->isGameWon() && !dill->dscene()->isGameLost()
	&& KMessageBox::warningContinueCancel(0,
                                     i18n("You are already running an unfinished game.  "
                                          "If you abort the old game to start a new one, "
                                          "the old game will be registered as a loss in "
                                          "the statistics file.\n"
                                          "What do you want to do?"),
                                     i18n("Abort Current Game?"),
                                     KGuiItem(i18n("Abort Current Game")),
                                     KStandardGuiItem::cancel(),
                                     "careaboutstats" )  == KMessageBox::Cancel)
        return;

    dill->setGameNumber(KRandom::random());
    setGameCaption();
    restart();
}


void pWidget::restart()
{
    statusBar()->clearMessage();
    dill->startNew();
}

void pWidget::slotPickRandom()
{
    QStringList list = KGlobal::dirs()->findAllResources("data", "carddecks/*/index.desktop");
    int hit = KRandom::random() % list.size();
    //kDebug() << list << " " << hit << " " << list.size() << " " << list[hit] << endl;
    QString theme = list[ hit ];
    theme = theme.left( theme.length() - strlen( "/index.desktop" ) );
    theme = theme.mid( theme.lastIndexOf( '/' ) + 1);

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );
    cs.writeEntry( "Theme", theme );

    cardMap::self()->updateTheme(cs);
    cardMap::self()->triggerRescale();
}

void pWidget::setGameCaption()
{
    QString name = games->currentText();
    QString newname;
    QString gamenum;
    gamenum.setNum( dill->gameNumber() );
    for (int i = 0; i < name.length(); i++)
        if (name.at(i) != QChar('&'))
            newname += name.at(i);

    setCaption( newname + " - " + gamenum );
}

void pWidget::slotNewGameType()
{
    newGameType();
    QTimer::singleShot(0 , this, SLOT( restart() ) );
}

void pWidget::newGameType()
{
    slotUpdateMoves();
    kDebug() << gettime() << " pWidget::newGameType\n";

    int id = games->currentItem();
    if ( id < 0 )
        id = 0;
    kDebug() << "newGameType " << id << endl;
    for (QList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin();
	it != DealerInfoList::self()->games().end(); ++it) {
        if ((*it)->gameindex == id) {
            dill->setScene( (*it)->createGame() );
            QString name = (*it)->name;
            name = name.replace(QRegExp("[&']"), "");
            name = name.replace(QRegExp("[ ]"), "_").toLower();
            dill->setAnchorName("game_" + name);
            dill->dscene()->setGameId(id);
            break;
        }
    }

    if (!dill->dscene()) {
        kError() << "unimplemented game type " << id << endl;
        dill->setScene( DealerInfoList::self()->games().first()->createGame() );
    }


    kDebug() << "dill " << dill << " " << dill->dscene() << endl;
    connect(dill->dscene(), SIGNAL(undoPossible(bool)), SLOT(undoPossible(bool)));
    connect(dill->dscene(), SIGNAL(gameLost()), SLOT(gameLost()));
    connect(dill->dscene(), SIGNAL(gameInfo(const QString&)),
            SLOT(slotGameInfo(const QString &)));
    connect(dill->dscene(), SIGNAL(updateMoves()), SLOT(slotUpdateMoves()));
    connect(dill->dscene(), SIGNAL(gameSolverWon()), SLOT(slotGameSolverWon()));
    connect(dill->dscene(), SIGNAL(gameSolverLost()), SLOT(slotGameSolverLost()));
    connect(dill->dscene(), SIGNAL(gameSolverUnknown()), SLOT(slotGameSolverUnknown()));

    dill->setAutoDropEnabled(dropaction->isChecked());

    // it's a bit tricky - we have to do this here as the
    // base class constructor runs before the derived class's
    // dill->takeState();

    setGameCaption();

    KConfigGroup cg(KGlobal::config(), settings_group);
    cg.writeEntry("DefaultGame", id);

    QTimer::singleShot( 0, this, SLOT( show() ) );
}

void pWidget::showEvent(QShowEvent *e)
{
    kDebug() << "showEvent "<< endl;
    dill->setWasShown( true );
    KXmlGuiWindow::showEvent(e);
}

void pWidget::slotGameInfo(const QString &text)
{
    statusBar()->showMessage(text, 3000);
}

void pWidget::slotUpdateMoves()
{
    int moves = 0;
    if ( dill && dill->dscene() ) moves = dill->dscene()->getMoves();
    statusBar()->changeItem( i18np("1 move", "%1 moves", moves), 1 );
}

void pWidget::chooseGame()
{
    bool ok;
    long number = KInputDialog::getText(i18n("Game Number"),
                                        i18n("Enter a game number (FreeCell deals are the same as in the FreeCell FAQ):"),
                                        QString::number(dill->gameNumber()), 0, this).toLong(&ok);
    if (ok) {
        dill->setGameNumber(number);
        setGameCaption();
        restart();
    }
}

void pWidget::gameLost()
{
    statusBar()->showMessage( i18n( "This game is lost." ) );
}

void pWidget::openGame(const KUrl &url)
{
    QString tmpFile;
    if( KIO::NetAccess::download( url, tmpFile, this ) )
    {
        QFile of(tmpFile);
        of.open(QIODevice::ReadOnly);
        QDomDocument doc;
        QString error;
        if (!doc.setContent(&of, &error))
        {
            KMessageBox::sorry(this, error);
            return;
        }
        int id = doc.documentElement().attribute("id").toInt();

        games->setCurrentItem(id);
        newGameType();
        if (!dill->dscene()) {
           KMessageBox::error(this, i18n("The saved game is of unknown type!"));
           games->setCurrentItem(0);
           newGameType();
        }
        show();
        dill->dscene()->openGame(doc);
        setGameCaption();
        KIO::NetAccess::removeTempFile( tmpFile );
        recent->addUrl(url);
        recent->saveEntries(KGlobal::config()->group( QString() ));
    }
}

void pWidget::openGame()
{
    KUrl url = KFileDialog::getOpenUrl();
    openGame(url);
}

void pWidget::saveGame()
{
    KUrl url = KFileDialog::getSaveUrl();
    KTemporaryFile file;
    file.open();
    QDomDocument doc("kpat");
    dill->dscene()->saveGame(doc);
    QTextStream stream (&file);
    stream << doc.toString();
    stream.flush();
    KIO::NetAccess::upload(file.fileName(), url, this);
    recent->addUrl(url);
    recent->saveEntries( KGlobal::config()->group( QString() ) );
}

void pWidget::showStats()
{
    GameStatsImpl dlg(this);
    if (dill)
        dlg.showGameType(dill->dscene()->gameId());
    dlg.exec();
}

void pWidget::slotGameSolverWon()
{
    statusBar()->showMessage(i18n( "I just won the game! Good luck to you." ), 3000);
}

void pWidget::slotGameSolverLost()
{
    statusBar()->showMessage(i18n( "Nope, this game cannot be won anymore." ), 3000);
}

void pWidget::slotGameSolverUnknown()
{
    statusBar()->clearMessage();
}

#include "pwidget.moc"

