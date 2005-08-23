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

#include <stdio.h>

#include <qregexp.h>
#include <qtimer.h>
#include <qimage.h>

#include <kapplication.h>
#include <klocale.h>
#include <kaction.h>
#include <kdebug.h>
#include <kcarddialog.h>
#include <kinputdialog.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kaccelmanager.h>
#include <kmenubar.h>

#include "pwidget.h"
#include "version.h"
#include "dealer.h"
#include "cardmaps.h"
#include "speeds.h"
#include "gamestatsimpl.h"


static pWidget *current_pwidget = 0;


void saveGame(int) {
    current_pwidget->saveGame();
}

pWidget::pWidget()
    : KMainWindow(0, "pwidget"), dill(0)
{
    current_pwidget = this;
    // KCrash::setEmergencySaveFunction(::saveGame);
    KStdAction::quit(kapp, SLOT(quit()), actionCollection(), "game_exit");

    undo = KStdAction::undo(this, SLOT(undoMove()),
                     actionCollection(), "undo_move");
    undo->setEnabled(false);
    (void)KStdAction::openNew(this, SLOT(newGame()),
                              actionCollection(), "new_game");
    (void)KStdAction::open(this, SLOT(openGame()),
                           actionCollection(), "open");
    recent = KStdAction::openRecent(this, SLOT(openGame(const KURL&)),
                                    actionCollection(), "open_recent");
    recent->loadEntries(KGlobal::config());
    (void)KStdAction::saveAs(this, SLOT(saveGame()),
                           actionCollection(), "save");
    (void)new KAction(i18n("&Choose Game..."), 0, this, SLOT(chooseGame()),
                      actionCollection(), "choose_game");
    (void)new KAction(i18n("Restart &Game"), QString::fromLatin1("reload"), 0,
                      this, SLOT(restart()),
                      actionCollection(), "restart_game");
    (void)KStdAction::help(this, SLOT(helpGame()), actionCollection(), "help_game");

    games = new KSelectAction(i18n("&Game Type"), 0, this,
                              SLOT(newGameType()),
                              actionCollection(), "game_type");
    QStringList list;
    QValueList<DealerInfo*>::ConstIterator it;
    uint max_type = 0;

    for (it = DealerInfoList::self()->games().begin();
         it != DealerInfoList::self()->games().end(); ++it)
    {
        // while we develop, it may happen that some lower
        // indices do not exist
        uint index = (*it)->gameindex;
        for (uint i = 0; i <= index; i++)
            if (list.count() <= i)
                list.append("unknown");
        list[index] = i18n((*it)->name);
        if (max_type < index)
            max_type = index;
    }
    games->setItems(list);

    KGlobal::dirs()->addResourceType("wallpaper", KStandardDirs::kde_default("data") + "kpat/backgrounds/");
    KGlobal::dirs()->addResourceType("wallpaper", KStandardDirs::kde_default("data") + "ksnake/backgrounds/");
    wallpapers = new KSelectAction(i18n("&Change Background"), 0, this,
                              SLOT(changeWallpaper()),
                              actionCollection(), "wallpaper");
    list.clear();
    wallpaperlist.clear();
    QStringList wallpaperlist2 = KGlobal::dirs()->findAllResources("wallpaper", QString::null,
                                                                   false, true, list);
    QStringList list2;
    for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
	QString file = *it;
	int rindex = file.findRev('.');
	if (rindex != -1) {
	  QString ext = file.mid(rindex + 1).lower();
	  if (ext == "jpeg" || ext == "png" || ext == "jpg") {
	     list2.append(file.left(rindex));
             wallpaperlist.append( file );
          }
	}
    }

    wallpapers->setItems(list2);
    wallpapers->setCurrentItem(list2.findIndex("No-Ones-Laughing-3"));

    changeWallpaper();

    (void)new cardMap(midcolor);

    backs = new KAction(i18n("&Switch Cards..."), 0, this,
                        SLOT(changeBackside()),
                        actionCollection(), "backside");
	 stats = new KAction(i18n("&Statistics"), 0, this, SLOT(showStats()),
			 actionCollection(),"game_stats");

    animation = new KToggleAction(i18n( "&Animation on Startup" ),
                                  0, this, SLOT(animationChanged()),
                                  actionCollection(), "animation");
    dropaction = new KToggleAction(i18n("&Enable Autodrop"),
                                   0, this, SLOT(enableAutoDrop()),
                                   actionCollection(), "enable_autodrop");
    dropaction->setCheckedState(i18n("Disable Autodrop"));

    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );

    QString bgpath = config->readPathEntry("Background");
    kdDebug(11111) << "bgpath '" << bgpath << "'" << endl;
    if (bgpath.isEmpty())
        bgpath = locate("wallpaper", "No-Ones-Laughing-3.jpg");
    background = QPixmap(bgpath);

    bool animate = config->readBoolEntry( "Animation", true);
    animation->setChecked( animate );

    bool autodrop = config->readBoolEntry("Autodrop", true);
    dropaction->setChecked(autodrop);

    uint game = config->readNumEntry("DefaultGame", 0);
    if (game > max_type)
        game = max_type;
    games->setCurrentItem(game);

    statusBar()->insertItem( "", 1, 0, true );

    createGUI(QString::null, false);
    KAcceleratorManager::manage(menuBar());

    newGameType();
    adjustSize();
    setAutoSaveSettings();
}

pWidget::~pWidget()
{
    delete dill;
}

void pWidget::undoMove() {
    if( dill )
        dill->undo();
}

void pWidget::helpGame()
{
    if (!dill)
        return;
    kapp->invokeHelp(dill->anchorName());
}

void pWidget::undoPossible(bool poss)
{
    undo->setEnabled(poss);
}

void pWidget::changeBackside() {
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);

    QString deck = config->readEntry("Back", KCardDialog::getDefaultDeck());
    QString cards = config->readEntry("Cards", KCardDialog::getDefaultCardDir());
    if (KCardDialog::getCardDeck(deck, cards, this, KCardDialog::Both) == QDialog::Accepted)
    {
        QString imgname = KCardDialog::getCardPath(cards, 11);

        QImage image;
        image.load(imgname);
        if( image.isNull()) {
            kdDebug(11111) << "cannot load card pixmap \"" << imgname << "\" in " << cards << "\n";
            return;
        }

        bool change = false;
        if (image.width() != cardMap::CARDX() || image.height() != cardMap::CARDY())
        {
            change = true;
            if (KMessageBox::warningContinueCancel(this, i18n("The cards you have chosen have a different "
                                                              "size than the ones you are currently using. "
                                                              "This requires the current game to be restarted.")) == KMessageBox::Cancel)
                return;
        }
        setBackSide(deck, cards);
        if (change) {

            newGameType();
        }
    }

}

void pWidget::changeWallpaper()
{
    QString bgpath=locate("wallpaper", wallpaperlist[wallpapers->currentItem()]);
    if (bgpath.isEmpty())
        return;
    background = QPixmap(bgpath);
    if (background.isNull()) {
        KMessageBox::sorry(this, i18n("<qt>Couldn't load wallpaper<br/>%1</qt>").arg(bgpath));
        return;
    }

    QImage bg = background.convertToImage().convertDepth(8, 0);
    if (bg.isNull() || !bg.numColors())
        return;
    long r = 0;
    long g = 0;
    long b = 0;
    for (int i = 0; i < bg.numColors(); ++i)
    {
        QRgb rgb = bg.color(i);
        r += qRed(rgb);
        g += qGreen(rgb);
        b += qBlue(rgb);
    }
    r /= bg.numColors();
    b /= bg.numColors();
    g /= bg.numColors();
    midcolor = QColor(r, b, g);

    if (dill) {
        KConfig *config = kapp->config();
        KConfigGroupSaver kcs(config, settings_group);

        QString deck = config->readEntry("Back", KCardDialog::getDefaultDeck());
        QString dummy = config->readEntry("Cards", KCardDialog::getDefaultCardDir());
        setBackSide(deck, dummy);

	config->writePathEntry("Background", bgpath);
        dill->setBackgroundPixmap(background, midcolor);
        dill->canvas()->setAllChanged();
        dill->canvas()->update();
    }
}

void pWidget::animationChanged() {
    bool anim = animation->isChecked();
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );
    config->writeEntry( "Animation", anim);
}

void pWidget::enableAutoDrop()
{
    bool drop = dropaction->isChecked();
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );
    config->writeEntry( "Autodrop", drop);
    dill->setAutoDropEnabled(drop);
}

void pWidget::newGame()
{
    // Check if the user is already running a game, and if she is,
    // then ask if she wants to abort it.
    if (!dill->isGameWon() && !dill->isGameLost()
	&& KMessageBox::warningContinueCancel(0,
                                     i18n("You are already running an unfinished game.  "
                                          "If you abort the old game to start a new one, "
                                          "the old game will be registered as a loss in "
                                          "the statistics file.\n"
                                          "What do you want to do?"),
                                     i18n("Abort Current Game?"),
                                     i18n("Abort Old Game"),
                                     "careaboutstats" )  == KMessageBox::Cancel)
        return;

    dill->setGameNumber(kapp->random());
    setGameCaption();
    restart();
}


void pWidget::restart()
{
    statusBar()->clear();
    dill->startNew();
}

void pWidget::setGameCaption()
{
    QString name = games->currentText();
    QString newname;
    QString gamenum;
    gamenum.setNum( dill->gameNumber() );
    for (uint i = 0; i < name.length(); i++)
        if (name.at(i) != QChar('&'))
            newname += name.at(i);

    setCaption( newname + " - " + gamenum );
}

void pWidget::newGameType()
{
    delete dill;
    dill = 0;
    slotUpdateMoves();

    uint id = games->currentItem();
    for (QValueList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin(); it != DealerInfoList::self()->games().end(); ++it) {
        if ((*it)->gameindex == id) {
            dill = (*it)->createGame(this);
            QString name = (*it)->name;
            name = name.replace(QRegExp("[&']"), "");
            name = name.replace(QRegExp("[ ]"), "_").lower();
            dill->setAnchorName("game_" + name);
            connect(dill, SIGNAL(saveGame()), SLOT(saveGame()));
            connect(dill, SIGNAL(gameInfo(const QString&)),
                    SLOT(slotGameInfo(const QString &)));
            connect(dill, SIGNAL(updateMoves()),
                    SLOT(slotUpdateMoves()));
            dill->setGameId(id);
            dill->setupActions();
            dill->setBackgroundPixmap(background, midcolor);
            dill->startNew();
            break;
        }
    }

    if (!dill) {
        kdError() << "unimplemented game type " << id << endl;
        dill = DealerInfoList::self()->games().first()->createGame(this);
    }

    connect(dill, SIGNAL(undoPossible(bool)), SLOT(undoPossible(bool)));
    connect(dill, SIGNAL(gameWon(bool)), SLOT(gameWon(bool)));
    connect(dill, SIGNAL(gameLost()), SLOT(gameLost()));

    dill->setAutoDropEnabled(dropaction->isChecked());

    // it's a bit tricky - we have to do this here as the
    // base class constructor runs before the derived class's
    dill->takeState();

    setGameCaption();

    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    config->writeEntry("DefaultGame", id);

    QSize min(700,400);
    min = min.expandedTo(dill->minimumCardSize());
    dill->setMinimumSize(min);
    dill->resize(min);
    updateGeometry();
    setCentralWidget(dill);
    dill->show();
}

void pWidget::showEvent(QShowEvent *e)
{
    if (dill)
        dill->setMinimumSize(QSize(0,0));
    KMainWindow::showEvent(e);
}

void pWidget::slotGameInfo(const QString &text)
{
    statusBar()->message(text, 3000);
}

void pWidget::slotUpdateMoves()
{
    int moves = 0;
    if ( dill ) moves = dill->getMoves();
    statusBar()->changeItem( i18n("1 move", "%n moves", moves), 1 );
}

void pWidget::setBackSide(const QString &deck, const QString &cards)
{
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    QPixmap pm(deck);
    if(!pm.isNull()) {
        cardMap::self()->setBackSide(pm, false);
        config->writeEntry("Back", deck);
        bool ret = cardMap::self()->setCardDir(cards);
        if (!ret) {
            config->writeEntry("Back", "");

        }
        config->writeEntry("Cards", cards);
        cardMap::self()->setBackSide(pm, true);
    } else
        KMessageBox::sorry(this,
                           i18n("Could not load background image!"));

    if (dill) {
        dill->canvas()->setAllChanged();
        dill->canvas()->update();
    }
}

void pWidget::chooseGame()
{
    bool ok;
    long number = KInputDialog::getText(i18n("Game Number"), i18n("Enter a game number (FreeCell deals are the same as in the FreeCell FAQ):"), QString::number(dill->gameNumber()), 0, this).toLong(&ok);
    if (ok) {
        dill->setGameNumber(number);
        setGameCaption();
        restart();
    }
}

void pWidget::gameWon(bool withhelp)
{
    QString congrats;
    if (withhelp)
        congrats = i18n("Congratulations! We have won!");
    else
        congrats = i18n("Congratulations! You have won!");
#if TEST_SOLVER == 0
    KMessageBox::information(this, congrats, i18n("Congratulations!"));
#endif
    QTimer::singleShot(0, this, SLOT(newGame()));
#if TEST_SOLVER == 1
    dill->demo();
#endif
}

void pWidget::gameLost()
{
    QString   dontAskAgainName = "gameLostDontAskAgain";

    // The following code is taken out of kmessagebox.cpp in kdeui.
    // Is there a better way?
    KConfig  *config = 0;
    QString grpNotifMsgs = QString::fromLatin1("Notification Messages");

    config = KGlobal::config();
    KConfigGroupSaver saver(config,
			    QString::fromLatin1("Notification Messages"));
    QString dontAsk = config->readEntry(dontAskAgainName).lower();

    // If we are ordered never to ask again and to continue the game,
    // then do so.
    if (dontAsk == "no")
	return;
    // If it says yes, we ask anyway. Just starting a new game would
    // be incredibly annoying.
    if (dontAsk == "yes")
	dontAskAgainName = QString::null;

    if (KMessageBox::questionYesNo(this, i18n("You could not win this game, "
                                              "but there is always a second try.\nStart a new game?"),
                                   i18n("Could Not Win!"),
                                   i18n("New Game"),
				   KStdGuiItem::cont(),
				   dontAskAgainName) == KMessageBox::Yes) {

        QTimer::singleShot(0, this, SLOT(newGame()));
    }
}

void pWidget::openGame(const KURL &url)
{
    QString tmpFile;
    if( KIO::NetAccess::download( url, tmpFile, this ) )
    {
        QFile of(tmpFile);
        of.open(IO_ReadOnly);
        QDomDocument doc;
        QString error;
        if (!doc.setContent(&of, &error))
        {
            KMessageBox::sorry(this, error);
            return;
        }
        uint id = doc.documentElement().attribute("id").toUInt();

        if (id != (Q_UINT32)games->currentItem()) {
            games->setCurrentItem(id);
            newGameType();
            if (!dill) {
                KMessageBox::error(this, i18n("The saved game is of unknown type!"));
                games->setCurrentItem(0);
                newGameType();
            }
        }
        dill->openGame(doc);
        setGameCaption();
        KIO::NetAccess::removeTempFile( tmpFile );
        recent->addURL(url);
        recent->saveEntries(KGlobal::config());
    }
}

void pWidget::openGame()
{
    KURL url = KFileDialog::getOpenURL();
    openGame(url);
}

void pWidget::saveGame()
{
    KURL url = KFileDialog::getSaveURL();
    KTempFile file;
    QDomDocument doc("kpat");
    dill->saveGame(doc);
    QTextStream *stream = file.textStream();
    *stream << doc.toString();
    file.close();
    KIO::NetAccess::upload(file.name(), url, this);
    recent->addURL(url);
    recent->saveEntries(KGlobal::config());
}

void pWidget::showStats()
{
	GameStatsImpl* dlg = new GameStatsImpl(this,"statistics dialog");
	if (dill)
		dlg->showGameType(dill->gameId());
	dlg->exec();
}

#include "pwidget.moc"

