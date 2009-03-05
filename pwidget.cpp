/*
   patience -- main program
   Copyright (C) 1995  Paul Olav Tvete <paul@troll.no>

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
#include "pile.h"
#include "version.h"
#include "dealer.h"
#include "view.h"
#include "cardmaps.h"
#include "gamestatsimpl.h"
#include "demo.h"
#include "render.h"
#include <carddeckinfo.h>

#include <cstdio>

#include <QRegExp>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QTextStream>
#include <QList>
#include <QShowEvent>
#include <QDomDocument>
#include <QSvgRenderer>
#include <QDesktopWidget>
#include <QStackedWidget>

#include <kapplication.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kstandardgameaction.h>
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


static pWidget *current_pwidget = 0;

pWidget::pWidget()
  : KXmlGuiWindow(0),
    dill(0),
    m_bubbles(0)
{
    setObjectName( "pwidget" );
    current_pwidget = this;
    // KCrash::setEmergencySaveFunction(::saveGame);

    KAction *a;
    // Game
    a = KStandardGameAction::gameNew(this, SLOT(newGame()), actionCollection());
    a->setEnabled( false );
    a = KStandardGameAction::restart(this, SLOT(restart()), actionCollection());
    a->setEnabled( false );
    KStandardGameAction::load(this, SLOT(openGame()), actionCollection());
    recent = KStandardGameAction::loadRecent(this, SLOT(openGame(const KUrl&)), actionCollection());
    recent->loadEntries( KGlobal::config()->group( QString() ));
    a = KStandardGameAction::save(this, SLOT(saveGame()), actionCollection());
    a->setEnabled( false );
    KStandardGameAction::quit(this, SLOT(close()), actionCollection());

    // Move
    undo = KStandardGameAction::undo(this, SLOT(undoMove()), actionCollection());
    undo->setEnabled(false);

    // Move
    redo = KStandardGameAction::redo(this, SLOT(redoMove()), actionCollection());
    redo->setEnabled(false);

    a = actionCollection()->addAction("choose_game");
    a->setText(i18n("&Choose Game..."));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( chooseGame() ) );
    a->setEnabled( false );

    actionCollection()->addAction(KStandardAction::Help, "help_game",
                                  this, SLOT(helpGame()));

    a = actionCollection()->addAction("change_game_type");
    a->setText(i18n("Change Game Type..."));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( slotShowGameSelectionScreen() ) );
    a->setEnabled( false );

    a = actionCollection()->addAction("random_set");
    a->setText(i18n("Random Cards"));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( slotPickRandom() ) );
    a->setShortcuts( KShortcut( Qt::Key_F9 ) );
    a->setEnabled( false );

    a = actionCollection()->addAction("snapshot");
    connect( a, SIGNAL( triggered( bool ) ), SLOT( slotSnapshot() ) );
    if (!qgetenv("KDE_DEBUG").isEmpty()) // developer shortcut
       a->setShortcuts( KShortcut( Qt::Key_F8 ) );

    a = actionCollection()->addAction("select_deck");
    a->setText(i18n("Select Deck..."));
    connect( a, SIGNAL( triggered( bool ) ), SLOT( slotSelectDeck() ) );
    a->setShortcuts( KShortcut( Qt::Key_F10 ) );
    a->setEnabled( false );

    a = actionCollection()->addAction("game_stats");
    a->setText(i18n("Statistics"));
    connect( a, SIGNAL( triggered( bool ) ), SLOT(showStats()) );

    KConfigGroup cg(KGlobal::config(), settings_group );

    dropaction = new KToggleAction(i18n("&Enable Autodrop"), this);
    actionCollection()->addAction("enable_autodrop", dropaction);
    connect( dropaction, SIGNAL( triggered( bool ) ), SLOT(enableAutoDrop()) );
    dropaction->setEnabled( false );
    dropaction->setChecked( cg.readEntry("Autodrop", true) );

    solveraction = new KToggleAction(i18n("E&nable Solver"), this);
    actionCollection()->addAction("enable_solver", solveraction);
    connect( solveraction, SIGNAL( triggered( bool ) ), SLOT( enableSolver() ) );
    solveraction->setEnabled( false );
    solveraction->setChecked( cg.readEntry("Solver", true) );

    rememberstateaction = new KToggleAction(i18n("&Remember State on Exit"), this);
    actionCollection()->addAction("remember_state", rememberstateaction);
    connect( rememberstateaction, SIGNAL( triggered( bool ) ), SLOT( enableRememberState() ) );
    rememberstateaction->setChecked( cg.readEntry("RememberStateOnExit", false) );

    statusBar()->insertPermanentItem( "", 1, 0 );
    setupGUI(qApp->desktop()->availableGeometry().size()*0.7);
    setAutoSaveSettings();

    foreach( const DealerInfo * di, DealerInfoList::self()->games() )
    {
        foreach( int id, di->ids )
            m_dealer_map.insert( id, di );
        if ( QString(di->name).toLower() == "freecell" )
            m_freeCellId = di->ids.first();
    }
    m_dealer_it = m_dealer_map.constEnd();

    m_cards = new cardMap();

    m_stack = new QStackedWidget;
    setCentralWidget( m_stack );

    Render::loadTheme( KStandardDirs::locate( "data", "kpat/theme.svg" ) );
}

pWidget::~pWidget()
{
    delete dill;
    delete m_cards;
}

void pWidget::undoMove() {
    if( dill && dill->dscene() )
        dill->dscene()->undo();
}

void pWidget::redoMove() {
    if( dill && dill->dscene() )
        dill->dscene()->redo();
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

void pWidget::redoPossible(bool poss)
{
    redo->setEnabled(poss);
}

void pWidget::enableAutoDrop()
{
    bool drop = dropaction->isChecked();
    KConfigGroup cg(KGlobal::config(), settings_group );
    cg.writeEntry( "Autodrop", drop);
    if ( dill )
        dill->setAutoDropEnabled(drop);
}

void pWidget::enableSolver()
{
    bool solver = solveraction->isChecked();
    KConfigGroup cg(KGlobal::config(), settings_group );
    cg.writeEntry( "Solver", solver );
    if ( dill )
        dill->setSolverEnabled(solver);
}

void pWidget::enableRememberState()
{
    KConfigGroup cg(KGlobal::config(), settings_group );
    cg.writeEntry( "RememberStateOnExit", rememberstateaction->isChecked() );
}

void pWidget::newGame()
{
    if (allowedToStartNewGame())
        startRandom();
}

void pWidget::restart()
{
    startNew(-1);
}

void pWidget::startRandom()
{
    startNew(KRandom::random());
}

void pWidget::startNew(long gameNumber)
{
    statusBar()->clearMessage();
    dill->startNew(gameNumber);
    setGameCaption();
}

void pWidget::slotPickRandom()
{
    QString theme = CardDeckInfo::randomFrontName();
    kDebug(11111) << "theme" << theme;

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );
    CardDeckInfo::writeFrontTheme( cs, theme );

    cardMap::self()->updateTheme(cs);
    cardMap::self()->triggerRescale();
}

void pWidget::slotSelectDeck()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group);
    KCardWidget* cardwidget = new KCardWidget();
    cardwidget->readSettings(cs);
    QString deckName, oldDeckName;
    QString cardName, oldCardName;
    cardName = oldCardName = cardwidget->backName();
    deckName = oldDeckName = cardwidget->frontName();

    KCardDialog dlg(cardwidget);
    if (dlg.exec() == QDialog::Accepted)
    {
        // Always store the settings, as other things than only the deck may
        // have changed
        cardwidget->saveSettings(cs);
        cs.sync();

        deckName = cardwidget->backName();
        cardName = cardwidget->frontName();

        if (deckName != oldDeckName || cardName != oldCardName) {
            cardMap::self()->updateTheme(cs);
            cardMap::self()->triggerRescale();
        }
    }
}

void pWidget::setGameCaption()
{
    QString caption;
    if ( dill && dill->dscene() && m_stack->currentWidget() != m_bubbles )
    {
        const DealerInfo * di = m_dealer_map.value( dill->dscene()->gameId() );
        QString name = di->name;
        QString gamenum = QString::number( dill->dscene()->gameNumber() );
        caption = name + " - " + gamenum;
    }
    setCaption( caption );
}

bool pWidget::allowedToStartNewGame()
{
    // Check if the user is already running a game, and if she is,
    // then ask if she wants to about it.
    return !dill
           || !dill->dscene()
           || !dill->dscene()->hasBeenCounted()
           || dill->dscene()->isGameWon()
           || dill->dscene()->isGameLost()
           || m_stack->currentWidget() != dill
           || KMessageBox::warningContinueCancel(0,
                                                 i18n("You are already running an unfinished game. "
                                                      "If you abort the old game to start a new one, "
                                                      "the old game will be registered as a loss in "
                                                      "the statistics file.\n"
                                                      "What do you want to do?"),
                                                 i18n("Abort Current Game?"),
                                                 KGuiItem(i18n("Abort Current Game")),
                                                 KStandardGuiItem::cancel(),
                                                 "careaboutstats"
                                                ) == KMessageBox::Continue;
}

void pWidget::slotGameSelected(int id)
{
    if ( m_dealer_map.contains(id) )
    {
        newGameType(id);
        QTimer::singleShot(0 , this, SLOT( startRandom() ) );
    }
}

void pWidget::newGameType(int id)
{
    slotUpdateMoves();
    kDebug(11111) << gettime() << "pWidget::newGameType\n";

    if ( !dill )
    {
        dill = new PatienceView(this, m_stack);
        m_stack->addWidget(dill);
    }

    if ( m_dealer_map.contains(id) )
    {
        const DealerInfo * di = m_dealer_map.value(id);
        dill->setScene( di->createGame() );
        QString name = di->name;
        name = name.remove(QRegExp("[&']"));
        name = name.replace(QRegExp("[ ]"), "_").toLower();
        dill->setAnchorName("game_" + name);
        dill->dscene()->setGameId( id );
    }
    else
    {
        kError() << "unimplemented game type" << id;
        dill->setScene( DealerInfoList::self()->games().first()->createGame() );
    }

    actionCollection()->action( "random_set" )->setEnabled( true );
    actionCollection()->action( "choose_game" )->setEnabled( true );
    actionCollection()->action( "enable_autodrop" )->setEnabled( true );
    actionCollection()->action( "enable_solver" )->setEnabled( true );
    actionCollection()->action( "game_new" )->setEnabled( true );
    actionCollection()->action( "game_restart" )->setEnabled( true );
    actionCollection()->action( "game_save" )->setEnabled( true );
    actionCollection()->action( "select_deck" )->setEnabled( true );
    actionCollection()->action( "change_game_type" )->setEnabled( true );

    enableAutoDrop();
    enableSolver();

    kDebug(11111) << "dill" << dill << " " << dill->dscene();
    connect(dill->dscene(), SIGNAL(undoPossible(bool)), SLOT(undoPossible(bool)));
    connect(dill->dscene(), SIGNAL(redoPossible(bool)), SLOT(redoPossible(bool)));
    connect(dill->dscene(), SIGNAL(gameLost()), SLOT(gameLost()));
    connect(dill->dscene(), SIGNAL(gameInfo(const QString&)),
            SLOT(slotGameInfo(const QString &)));
    connect(dill->dscene(), SIGNAL(updateMoves()), SLOT(slotUpdateMoves()));
    connect(dill->dscene(), SIGNAL(gameSolverStart()), SLOT(slotGameSolverStart()));
    connect(dill->dscene(), SIGNAL(gameSolverWon()), SLOT(slotGameSolverWon()));
    connect(dill->dscene(), SIGNAL(gameSolverLost()), SLOT(slotGameSolverLost()));
    connect(dill->dscene(), SIGNAL(gameSolverUnknown()), SLOT(slotGameSolverUnknown()));

    dill->setAutoDropEnabled(dropaction->isChecked());

    m_stack->setCurrentWidget(dill);

    // it's a bit tricky - we have to do this here as the
    // base class constructor runs before the derived class's
    // dill->takeState();

    QTimer::singleShot( 0, this, SLOT( show() ) );
}

void pWidget::slotShowGameSelectionScreen()
{
    if (allowedToStartNewGame())
    {
        if (!m_bubbles)
        {
            m_bubbles = new DemoBubbles(m_stack);
            m_stack->addWidget(m_bubbles);
            connect( m_bubbles, SIGNAL( gameSelected( int ) ), SLOT( slotGameSelected( int ) ) );
        }

        actionCollection()->action( "random_set" )->setEnabled( false );
        actionCollection()->action( "choose_game" )->setEnabled( false );
        actionCollection()->action( "enable_autodrop" )->setEnabled( false );
        actionCollection()->action( "enable_solver" )->setEnabled( false );
        actionCollection()->action( "game_new" )->setEnabled( false );
        actionCollection()->action( "game_restart" )->setEnabled( false );
        actionCollection()->action( "game_save" )->setEnabled( false );
        actionCollection()->action( "select_deck" )->setEnabled( false );
        actionCollection()->action( "change_game_type" )->setEnabled( false );

        m_stack->setCurrentWidget(m_bubbles);

        setGameCaption();
    }
}

void pWidget::closeEvent(QCloseEvent *e)
{
    QFile savedState(KStandardDirs::locateLocal("appdata", "savedstate.kpat"));
    if (savedState.exists())
        savedState.remove();

    if (rememberstateaction->isChecked()
        && dill && dill->dscene()
        && m_stack->currentWidget() == dill
       )
    {
        QFile temp(dill->dscene()->save_it());
        temp.copy(savedState.fileName());
        temp.remove();
    }

    KXmlGuiWindow::closeEvent(e);
}

void pWidget::showEvent(QShowEvent *e)
{
    kDebug(11111) << "showEvent ";
    if ( dill )
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
    QString text = (dill && dill->dscene() && dill->dscene()->gameId() == m_freeCellId)
                   ? i18n("Enter a game number (FreeCell deals are the same as in the FreeCell FAQ):")
                   : i18n("Enter a game number:");
    bool ok;
    long number = KInputDialog::getText(i18n("Game Number"),
                                        text,
                                        QString::number(dill->dscene()->gameNumber()),
                                        0,
                                        this).toLong(&ok);

    if (ok && allowedToStartNewGame())
        startNew(number);
}

void pWidget::gameLost()
{
    statusBar()->showMessage( i18n( "This game is lost." ) );
}

void pWidget::openGame(const KUrl &url, bool addToRecentFiles)
{
    if (!url.isEmpty() && allowedToStartNewGame())
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
            if (!m_dealer_map.contains(id)) {
               KMessageBox::error(this, i18n("The saved game is of unknown type!"));
               id = m_dealer_map.keys().first();
            }

            if (doc.documentElement().hasChildNodes())
            {
                newGameType(id);
                // for old spider and klondike
                dill->dscene()->mapOldId(id);
                dill->dscene()->openGame(doc);
                setGameCaption();
            }
            else
            {
                slotGameSelected(id);
            }

            show();
            KIO::NetAccess::removeTempFile( tmpFile );

            if ( addToRecentFiles )
            {
                recent->addUrl(url);
                recent->saveEntries(KGlobal::config()->group( QString() ));
            }
        }
    }
}

void pWidget::openGame()
{
    KUrl url = KFileDialog::getOpenUrl();
    openGame(url);
}

void pWidget::saveGame()
{
    if (!dill)
       return;
    KUrl url = KFileDialog::getSaveUrl();
    KIO::NetAccess::upload(dill->dscene()->save_it(), url, this);
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
    statusBar()->showMessage(i18n( "This game can still be won! Good luck to you." ));
}

void pWidget::slotGameSolverStart()
{
    statusBar()->showMessage(i18n( "Calculating..." ) );
}

void pWidget::slotGameSolverLost()
{
    statusBar()->showMessage(i18n( "Nope, this game cannot be won anymore." ));
}

void pWidget::slotGameSolverUnknown()
{
    statusBar()->showMessage( i18n( "Timeout while playing - unknown if it can be won" ) );
}

void pWidget::slotSnapshot()
{
    if ( m_dealer_it == m_dealer_map.constEnd() )
    {
        // first call
        m_dealer_it = m_dealer_map.constBegin();
    }

    newGameType(m_dealer_it.key());
    startRandom();
    m_dealer_it++;
    QTimer::singleShot( 200, this, SLOT( slotSnapshot2() ) );
}

void pWidget::slotSnapshot2()
{
    if ( dill->dscene()->waiting() ) {
            QTimer::singleShot( 100, this, SLOT( slotSnapshot2() ) );
            return;
    }
    QImage img = QImage( dill->size(), QImage::Format_ARGB32 );
    img.fill( qRgba( 0, 0, 255, 0 ) );
    dill->dscene()->createDump( &img );
    img = img.scaled( 320, 320, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    img.save( QString( "out_%1.png" ).arg( dill->dscene()->gameId() ) );
    if ( m_dealer_it != m_dealer_map.constEnd() )
        QTimer::singleShot( 200, this, SLOT( slotSnapshot() ) );
}

#include "pwidget.moc"
