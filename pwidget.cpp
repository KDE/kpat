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
#include <qobjcoll.h>
#include <kmessagebox.h>

#include <kapp.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>
#include "pwidget.h"
#include "version.h"
#include <kstdaction.h>
#include <kaction.h>
#include <dealer.h>
#include <kdebug.h>
#include "cardmaps.h"
#include <kcarddialog.h>
#include <qinputdialog.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kcrash.h>

static pWidget *current_pwidget = 0;

void saveGame(int) {
    current_pwidget->saveGame();
}

pWidget::pWidget( const char* _name )
    : KMainWindow(0, _name), dill(0)
{
    current_pwidget = this;
    KCrash::setEmergencySaveFunction(::saveGame);
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
    (void)new KAction(i18n("&Choose game..."), 0, this, SLOT(chooseGame()),
                      actionCollection(), "choose_game");
    (void)new KAction(i18n("&Restart game"), QString::fromLatin1("reload"), 0,
                      this, SLOT(restart()),
                      actionCollection(), "restart_game");

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
        kdDebug() << "add " << list[index] << endl;
        if (max_type < index)
            max_type = index;
    }
    games->setItems(list);

    KGlobal::dirs()->addResourceType("wallpaper", KStandardDirs::kde_default("data") + "ksnake/backgrounds/");
    wallpapers = new KSelectAction(i18n("&Change Background"), 0, this,
                              SLOT(changeWallpaper()),
                              actionCollection(), "wallpaper");
    list.clear();
    wallpaperlist = KGlobal::dirs()->findAllResources("wallpaper", "*.jpg", false, true, list);
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
        *it = (*it).left((*it).length() - 4);
    wallpapers->setItems(list);
    wallpapers->setCurrentItem(list.findIndex("Totally-New-Product-1"));

    changeWallpaper();

    (void)new cardMap(midcolor);

    backs = new KAction(i18n("&Switch Cards..."), 0, this,
                        SLOT(changeBackside()),
                        actionCollection(), "backside");

    animation = new KToggleAction(i18n( "&Animation on startup" ),
                                  0, this, SLOT(animationChanged()),
                                  actionCollection(), "animation");

    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );

    bool animate = config->readBoolEntry( "Animation", true);
    animation->setChecked( animate );

    uint game = config->readNumEntry("DefaultGame", 0);
    if (game > max_type)
        game = max_type;
    games->setCurrentItem(game);

    createGUI(QString::null, false);

    newGameType();
}

pWidget::~pWidget()
{
    delete dill;
}

void pWidget::undoMove() {
    if( dill )
        dill->undo();
}

void pWidget::undoPossible(bool poss)
{
    undo->setEnabled(poss);
}

void pWidget::changeBackside() {
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);

    QString deck = config->readEntry("Back");
    QString dummy = config->readEntry("Cards");
    if (KCardDialog::getCardDeck(deck, dummy, this, KCardDialog::Both) == QDialog::Accepted)
    {
        setBackSide(deck, dummy);
    }
}

void pWidget::changeWallpaper()
{
    background = QPixmap(locate("wallpaper", wallpaperlist[wallpapers->currentItem()]));
    QImage bg = background.convertToImage().convertDepth(8, 0);
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

        QString deck = config->readEntry("Back");
        QString dummy = config->readEntry("Cards");
        setBackSide(deck, dummy);

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

void pWidget::newGame() {
    dill->setGameNumber(kapp->random());
    restart();
}

void pWidget::restart() {
    dill->resetSize(QSize(dill->visibleWidth(), dill->visibleHeight()));
    dill->startNew();
}

void pWidget::newGameType()
{
    delete dill;
    dill = 0;

    uint id = games->currentItem();
    for (QValueList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin(); it != DealerInfoList::self()->games().end(); ++it) {
        if ((*it)->gameindex == id) {
            dill = (*it)->createGame(this);
            connect(dill, SIGNAL(saveGame()), SLOT(saveGame()));
            dill->setGameId(id);
            dill->setupActions();
            dill->setBackgroundPixmap(background, midcolor);
            dill->startNew();
            break;
        }
    }

    connect(dill, SIGNAL(undoPossible(bool)), SLOT(undoPossible(bool)));
    connect(dill, SIGNAL(gameWon(bool)), SLOT(gameWon(bool)));

    // it's a bit tricky - we have to do this here as the
    // base class constructor runs before the derived class's
    dill->takeState();

    if (!dill) {
        kdError() << "unimplemented game type " << id << endl;
        dill = DealerInfoList::self()->games().first()->createGame(this);
    }
    QString name = games->currentText();
    QString newname;
    for (uint i = 0; i < name.length(); i++)
        if (name.at(i) != QChar('&'))
            newname += name.at(i);

    setCaption( newname );

    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    config->writeEntry("DefaultGame", id);

    QSize minsize(700, 400);
    kdDebug() << "size1 " << dill->canvas()->size().width() << endl;
    minsize = minsize.expandedTo(dill->sizeHint());
    dill->resetSize(minsize);
    dill->resize(minsize);
    dill->setMinimumSize(minsize);

    setCentralWidget(dill);
    dill->show();

    kdDebug() << "size2 " << dill->width() << " " << width() << " " << dill->sizeHint().width() << endl;
}

void pWidget::setBackSide(const QString &deck, const QString &cards)
{
    kdDebug() << "setBackSide\n";

    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    QPixmap pm(deck);
    if(!pm.isNull()) {
        cardMap::self()->setBackSide(pm);
        config->writeEntry("Back", deck);
        cardMap::self()->setCardDir(cards);
        config->writeEntry("Cards", cards);
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
    bool ok = false;
    QString text = QInputDialog::getText( i18n("Game Number"),
                                          i18n( "Enter a game number "
                                                "(1 and 32000 are the same as in the FreeCell FAQ)" ),
                                          QString::number(dill->gameNumber()), &ok, this );
    if ( ok && !text.isEmpty() ) {
        long number = text.toLong(&ok);
        if (ok) {
            dill->setGameNumber(number);
            restart();
        }
    }
}

void pWidget::gameWon(bool withhelp)
{
    QString congrats;
    if (withhelp)
        congrats = i18n("Congratulation! We've won!");
    else
        congrats = i18n("Congratulation! You've won!");
    KMessageBox::information(this, congrats, i18n("Congratulation!"));
    newGame();
}


void pWidget::openGame(const KURL &url)
{
    QString tmpFile;
    if( KIO::NetAccess::download( url, tmpFile ) )
    {
        QFile of(tmpFile);
        of.open(IO_ReadOnly);
        QDataStream is(&of);
        int current_file_format;
        is >> current_file_format; // file format
        is.setVersion(current_file_format);
        int id;
        is >> id; // dealer number
        if (id != games->currentItem()) {
            games->setCurrentItem(id);
            newGameType();
            if (!dill) {
                KMessageBox::error(this, i18n("The saved game is of unknown type!"));
                games->setCurrentItem(0);
                newGameType();
            }
        }
        dill->openGame(is);
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
    QDataStream *stream = file.dataStream();
    dill->saveGame(*stream);
    file.close();
    KIO::NetAccess::upload(file.name(), url);
    recent->addURL(url);
    recent->saveEntries(KGlobal::config());
}

#include "pwidget.moc"

