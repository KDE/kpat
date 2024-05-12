/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 1997 Mario Weilguni <mweilguni@sime.com>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <coates@kde.org>
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

#include "mainwindow.h"

// own
#include "dealer.h"
#include "dealerinfo.h"
#include "gameselectionscene.h"
#include "kpat_debug.h"
#include "numbereddealdialog.h"
#include "renderer.h"
#include "settings.h"
#include "soundengine.h"
#include "statisticsdialog.h"
#include "view.h"
// KCardGame
#include <KCardTheme>
#include <KCardThemeWidget>
// KDEGames
#include <KGameStandardAction>
#include <KGameThemeSelector>
// KF
#include <KActionCollection>
#include <KConfigDialog>
#include <KHelpClient>
#include <KIO/StoredTransferJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KStandardAction>
#include <KToggleAction>
#include <KXMLGUIFactory>
// Qt
#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QFileDialog>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QMenuBar>
#include <QPointer>
#include <QRandomGenerator>
#include <QScreen>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTemporaryFile>
#include <QTimer>
#include <QXmlStreamReader>

namespace
{
const QUrl dialogUrl(QStringLiteral("kfiledialog:///kpat"));
const QString saveFileMimeType(QStringLiteral("application/vnd.kde.kpatience.savedgame"));
const QString legacySaveFileMimeType(QStringLiteral("application/vnd.kde.kpatience.savedstate"));
}

MainWindow::MainWindow()
    : KXmlGuiWindow(nullptr)
    , m_view(nullptr)
    , m_dealer(nullptr)
    , m_selector(nullptr)
    , m_cardDeck(nullptr)
    , m_soundEngine(nullptr)
    , m_dealDialog(nullptr)
{
    setObjectName(QStringLiteral("MainWindow"));
    // KCrash::setEmergencySaveFunction(::saveGame);

    setupActions();

    const auto games = DealerInfoList::self()->games();
    for (const DealerInfo *di : games) {
        m_dealer_map.insert(di->baseId(), di);
        const auto subtypeIds = di->subtypeIds();
        for (int id : subtypeIds)
            m_dealer_map.insert(id, di);
    }
    m_dealer_it = m_dealer_map.constEnd();

    m_view = new PatienceView(this);
    setCentralWidget(m_view);

    const QRect screenSize = screen()->availableGeometry();
    QSize defaultSize = screenSize.size() * 0.7;
    setupGUI(defaultSize, Create | Save | ToolBar | StatusBar | Keys);

    m_solverStatusLabel = new QLabel(QString(), statusBar());
    m_solverStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBar()->addWidget(m_solverStatusLabel, 1);

    m_moveCountStatusLabel = new QLabel(QString(), statusBar());
    m_moveCountStatusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addWidget(m_moveCountStatusLabel, 0);

    m_showMenubarAction->setChecked(!menuBar()->isHidden());
}

MainWindow::~MainWindow()
{
    m_recentFilesAction->saveEntries(KSharedConfig::openConfig()->group(QString()));

    Settings::self()->save();

    delete m_dealer;
    delete m_view;
    Renderer::deleteSelf();
}

void MainWindow::setupActions()
{
    QAction *a;

    // Game Menu
    a = actionCollection()->addAction(QStringLiteral("new_game"));
    a->setText(i18nc("@action Start a new game of a different type", "New &Game…"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    KActionCollection::setDefaultShortcut(a, Qt::CTRL | Qt::SHIFT | Qt::Key_N);
    connect(a, &QAction::triggered, this, &MainWindow::slotShowGameSelectionScreen);

    a = actionCollection()->addAction(QStringLiteral("new_deal"));
    a->setText(i18nc("@action Start a new game of without changing the game type", "New &Deal"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    KActionCollection::setDefaultShortcut(a, Qt::CTRL | Qt::Key_N);
    connect(a, &QAction::triggered, this, &MainWindow::newGame);

    a = actionCollection()->addAction(QStringLiteral("new_numbered_deal"));
    a->setText(i18nc("@action Start a game by giving its particular number", "New &Numbered Deal…"));
    KActionCollection::setDefaultShortcut(a, Qt::CTRL | Qt::Key_D);
    connect(a, &QAction::triggered, this, &MainWindow::newNumberedDeal);

    a = KGameStandardAction::restart(this, &MainWindow::restart, actionCollection());
    a->setText(i18nc("@action Replay the current deal from the start", "Restart Deal"));

    // Note that this action is not shown in the menu or toolbar. It is
    // only provided for advanced users who can use it by shorcut or add it to
    // the toolbar if they wish.
    a = actionCollection()->addAction(QStringLiteral("next_deal"));
    a->setText(i18nc("@action Start the game with the number one greater than the current one", "Next Deal"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    KActionCollection::setDefaultShortcut(a, Qt::CTRL | Qt::Key_Plus);
    connect(a, &QAction::triggered, this, &MainWindow::nextDeal);

    // Note that this action is not shown in the menu or toolbar. It is
    // only provided for advanced users who can use it by shorcut or add it to
    // the toolbar if they wish.
    a = actionCollection()->addAction(QStringLiteral("previous_deal"));
    a->setText(i18nc("@action Start the game with the number one less than the current one", "Previous Deal"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    KActionCollection::setDefaultShortcut(a, Qt::CTRL | Qt::Key_Minus);
    connect(a, &QAction::triggered, this, &MainWindow::previousDeal);

    KGameStandardAction::load(this, qOverload<>(&MainWindow::loadGame), actionCollection());

    m_recentFilesAction = KGameStandardAction::loadRecent(this, qOverload<const QUrl &>(&MainWindow::loadGame), actionCollection());
    m_recentFilesAction->loadEntries(KSharedConfig::openConfig()->group(QString()));

    m_saveAction = KGameStandardAction::saveAs(this, &MainWindow::saveGame, actionCollection());
    KActionCollection::setDefaultShortcut(m_saveAction, Qt::CTRL | Qt::Key_S);

    a = actionCollection()->addAction(QStringLiteral("game_stats"));
    a->setText(i18nc("@action", "Statistics"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("games-highscores")));
    connect(a, &QAction::triggered, this, &MainWindow::showStats);

    KGameStandardAction::quit(this, &MainWindow::close, actionCollection());

    // Move Menu
    m_undoAction = KGameStandardAction::undo(this, &MainWindow::undoMove, actionCollection());

    m_redoAction = KGameStandardAction::redo(this, &MainWindow::redoMove, actionCollection());

    m_demoAction = KGameStandardAction::demo(this, &MainWindow::toggleDemo, actionCollection());

    // KGameStandardAction::hint is a regular action, but we want a toggle
    // action, so we must create a new action and copy all the standard
    // properties over one by one.
    m_hintAction = new KToggleAction(actionCollection());
    a = KGameStandardAction::hint(nullptr, nullptr, nullptr);
    m_hintAction->setText(a->text());
    m_hintAction->setIcon(a->icon());
    KActionCollection::setDefaultShortcut(m_hintAction, a->shortcut());
    m_hintAction->setToolTip(a->toolTip());
    m_hintAction->setWhatsThis(a->whatsThis());
    delete a;
    const QString actionName = KGameStandardAction::name(KGameStandardAction::Hint);
    actionCollection()->addAction(actionName, m_hintAction);
    connect(m_hintAction, &QAction::triggered, this, &MainWindow::toggleHints);

    m_drawAction = actionCollection()->addAction(QStringLiteral("move_draw"));
    m_drawAction->setText(i18nc("@action Take one or more cards from the deck, flip them, and place them in play", "Dra&w"));
    m_drawAction->setIcon(QIcon::fromTheme(QStringLiteral("kpat")));
    KActionCollection::setDefaultShortcut(m_drawAction, Qt::Key_Tab);

    m_dealAction = actionCollection()->addAction(QStringLiteral("move_deal"));
    m_dealAction->setText(i18nc("@action Deal a new row of cards from the deck", "Dea&l Row"));
    m_dealAction->setIcon(QIcon::fromTheme(QStringLiteral("kpat")));
    KActionCollection::setDefaultShortcut(m_dealAction, Qt::Key_Return);

    m_redealAction = actionCollection()->addAction(QStringLiteral("move_redeal"));
    m_redealAction->setText(i18nc("@action Collect the cards in play, shuffle them and redeal them", "&Redeal"));
    m_redealAction->setIcon(QIcon::fromTheme(QStringLiteral("roll")));
    KActionCollection::setDefaultShortcut(m_redealAction, Qt::Key_R);

    m_dropAction = new KToggleAction(actionCollection());
    m_dropAction->setText(i18nc("@action Automatically move cards to the foundation piles", "Dro&p"));
    m_dropAction->setIcon(QIcon::fromTheme(QStringLiteral("games-endturn")));
    KActionCollection::setDefaultShortcut(m_dropAction, Qt::Key_P);
    actionCollection()->addAction(QStringLiteral("move_drop"), m_dropAction);
    connect(m_dropAction, &QAction::triggered, this, &MainWindow::toggleDrop);

    // Settings Menu
    m_autoDropEnabledAction = new KToggleAction(i18nc("@option:check", "&Enable Autodrop"), this);
    actionCollection()->addAction(QStringLiteral("enable_autodrop"), m_autoDropEnabledAction);
    connect(m_autoDropEnabledAction, &KToggleAction::triggered, this, &MainWindow::setAutoDropEnabled);
    m_autoDropEnabledAction->setChecked(Settings::autoDropEnabled());

    m_solverEnabledAction = new KToggleAction(i18nc("@option:check", "E&nable Solver"), this);
    actionCollection()->addAction(QStringLiteral("enable_solver"), m_solverEnabledAction);
    connect(m_solverEnabledAction, &KToggleAction::triggered, this, &MainWindow::enableSolver);
    m_solverEnabledAction->setChecked(Settings::solverEnabled());

    m_playSoundsAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("speaker")), i18nc("@option:check", "Play Sounds"), this);
    actionCollection()->addAction(QStringLiteral("play_sounds"), m_playSoundsAction);
    connect(m_playSoundsAction, &KToggleAction::triggered, this, &MainWindow::enableSounds);
    m_playSoundsAction->setChecked(Settings::playSounds());

    m_rememberStateAction = new KToggleAction(i18nc("@option:check", "&Remember State on Exit"), this);
    actionCollection()->addAction(QStringLiteral("remember_state"), m_rememberStateAction);
    connect(m_rememberStateAction, &KToggleAction::triggered, this, &MainWindow::enableRememberState);
    m_rememberStateAction->setChecked(Settings::rememberStateOnExit());

    // Help Menu
    m_gameHelpAction = actionCollection()->addAction(QStringLiteral("help_game"));
    m_gameHelpAction->setIcon(QIcon::fromTheme(QStringLiteral("help-contents")));
    connect(m_gameHelpAction, &QAction::triggered, this, &MainWindow::helpGame);
    KActionCollection::setDefaultShortcut(m_gameHelpAction, Qt::CTRL | Qt::SHIFT | Qt::Key_F1);

    KStandardAction::preferences(this, &MainWindow::configureAppearance, actionCollection());

    // Hidden actions
    if (!qEnvironmentVariableIsEmpty("KDE_DEBUG")) // developer shortcut
    {
        a = actionCollection()->addAction(QStringLiteral("themePreview"));
        a->setText(i18nc("@action", "Generate a Theme Preview Image"));
        connect(a, &QAction::triggered, this, &MainWindow::generateThemePreview);
        KActionCollection::setDefaultShortcut(a, Qt::Key_F7);

        a = actionCollection()->addAction(QStringLiteral("snapshot"));
        a->setText(i18nc("@action", "Take Game Preview Snapshots"));
        connect(a, &QAction::triggered, this, &MainWindow::slotSnapshot);
        KActionCollection::setDefaultShortcut(a, Qt::Key_F8);

        a = actionCollection()->addAction(QStringLiteral("random_set"));
        a->setText(i18nc("@action", "Random Cards"));
        connect(a, &QAction::triggered, this, &MainWindow::slotPickRandom);
        KActionCollection::setDefaultShortcut(a, Qt::Key_F9);
    }

    // Keyboard navigation actions
    m_leftAction = actionCollection()->addAction(QStringLiteral("focus_left"));
    m_leftAction->setText(i18nc("@action", "Move Focus to Previous Pile"));
    KActionCollection::setDefaultShortcut(m_leftAction, Qt::Key_Left);

    m_rightAction = actionCollection()->addAction(QStringLiteral("focus_right"));
    m_rightAction->setText(i18nc("@action", "Move Focus to Next Pile"));
    KActionCollection::setDefaultShortcut(m_rightAction, Qt::Key_Right);

    m_upAction = actionCollection()->addAction(QStringLiteral("focus_up"));
    m_upAction->setText(i18nc("@action", "Move Focus to Card Below"));
    KActionCollection::setDefaultShortcut(m_upAction, Qt::Key_Up);

    m_downAction = actionCollection()->addAction(QStringLiteral("focus_down"));
    m_downAction->setText(i18nc("@action", "Move Focus to Card Above"));
    KActionCollection::setDefaultShortcut(m_downAction, Qt::Key_Down);

    m_cancelAction = actionCollection()->addAction(QStringLiteral("focus_cancel"));
    m_cancelAction->setText(i18nc("@action", "Cancel Focus"));
    KActionCollection::setDefaultShortcut(m_cancelAction, Qt::Key_Escape);

    m_pickUpSetDownAction = actionCollection()->addAction(QStringLiteral("focus_activate"));
    m_pickUpSetDownAction->setText(i18nc("@action", "Pick Up or Set Down Focus"));
    KActionCollection::setDefaultShortcut(m_pickUpSetDownAction, Qt::Key_Space);

    // showMenubar isn't a part of KGameStandardAction
    m_showMenubarAction = KStandardAction::showMenubar(this, &MainWindow::toggleMenubar, actionCollection());

    KStandardAction::fullScreen(this, &MainWindow::toggleFullscreen, this, actionCollection());
}

void MainWindow::undoMove()
{
    if (m_dealer)
        m_dealer->undo();
}

void MainWindow::redoMove()
{
    if (m_dealer)
        m_dealer->redo();
}

void MainWindow::helpGame()
{
    if (m_dealer && m_dealer_map.contains(m_dealer->gameId())) {
        const DealerInfo *di = m_dealer_map.value(m_dealer->gameId());
        QString anchor = di->untranslatedBaseName();
        anchor = anchor.toLower();
        anchor = anchor.remove(QLatin1Char('\'')).replace(QLatin1Char('&'), QLatin1String("and")).replace(QLatin1Char(' '), QLatin1Char('-'));
        KHelpClient::invokeHelp(QLatin1String("rules-specific.html#") + anchor);
    }
}

void MainWindow::setAutoDropEnabled(bool enabled)
{
    Settings::setAutoDropEnabled(enabled);
    if (m_dealer) {
        m_dealer->setAutoDropEnabled(enabled);
        if (enabled)
            m_dealer->startDrop();
        else
            m_dealer->stopDrop();
    }
    updateGameActionList();
}

void MainWindow::enableSolver(bool enable)
{
    Settings::setSolverEnabled(enable);
    m_solverStatusLabel->setText(QString());
    if (m_dealer) {
        m_dealer->setSolverEnabled(enable);
        if (enable)
            m_dealer->startSolver();
    }
}

void MainWindow::enableSounds(bool enable)
{
    Settings::setPlaySounds(enable);
    updateSoundEngine();
}

void MainWindow::enableRememberState(bool enable)
{
    Settings::setRememberStateOnExit(enable);
}

void MainWindow::newGame()
{
    if (m_dealer && m_dealer->allowedToStartNewGame())
        startRandom();
}

void MainWindow::restart()
{
    startNew(-1);
}

void MainWindow::startRandom()
{
    const int gameNumber = int(QRandomGenerator::global()->bounded(quint32(1), quint32(INT_MAX)));
    startNew(gameNumber);
}

void MainWindow::startNew(int gameNumber)
{
    m_dealer->startNew(gameNumber);
    setGameCaption();
}

void MainWindow::slotPickRandom()
{
    QList<KCardTheme> themes = KCardTheme::findAll();
    KCardTheme theme = themes.at(QRandomGenerator::global()->bounded(themes.size()));
    Settings::setCardTheme(theme.dirName());

    appearanceChanged();
}

void MainWindow::configureAppearance()
{
    const QString previewFormat = QStringLiteral("back;10_spade,jack_diamond,queen_club,king_heart;1_spade");
    const QSet<QString> features = QSet<QString>() << QStringLiteral("AngloAmerican") << QStringLiteral("Backs1");

    if (!KConfigDialog::showDialog(QStringLiteral("KPatAppearanceDialog"))) {
        KConfigDialog *dialog = new KConfigDialog(this, QStringLiteral("KPatAppearanceDialog"), Settings::self());

        dialog->addPage(new KCardThemeWidget(features, previewFormat, this),
                        i18nc("@title:tab", "Card Deck"),
                        QStringLiteral("games-config-theme"),
                        i18n("Select a card deck"));

        KGameThemeProvider *provider = Renderer::self()->themeProvider();
        auto themeSelector = new KGameThemeSelector(provider, KGameThemeSelector::EnableNewStuffDownload, this);
        themeSelector->setNewStuffConfigFileName(QStringLiteral("kpat.knsrc"));
        dialog->addPage(themeSelector,
                        i18nc("@title:tab", "Game Theme"),
                        QStringLiteral("games-config-theme"),
                        i18n("Select a theme for non-card game elements"));

        connect(provider, &KGameThemeProvider::currentThemeChanged, this, &MainWindow::appearanceChanged);
        connect(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::appearanceChanged);
        dialog->show();
    }
}

void MainWindow::appearanceChanged()
{
    if (m_cardDeck && Settings::cardTheme() != m_cardDeck->theme().dirName()) {
        KCardTheme theme(Settings::cardTheme());
        if (theme.isValid()) {
            m_cardDeck->setTheme(KCardTheme(theme));
            if (m_dealer)
                m_dealer->relayoutScene();
        }
    }
}

void MainWindow::setGameCaption()
{
    QString caption;
    if (m_dealer) {
        const DealerInfo *di = m_dealer_map.value(m_dealer->gameId());
        caption = QStringLiteral("%1 - %2").arg(di->baseName()).arg(m_dealer->gameNumber());
    }
    setCaption(caption);
}

void MainWindow::slotGameSelected(int id)
{
    if (m_dealer_map.contains(id)) {
        setGameType(id);
        QTimer::singleShot(0, this, &MainWindow::startRandom);
    }
}

void MainWindow::setGameType(int id)
{
    // Only bother calling creating a new DealerScene if we don't already have
    // the right DealerScene open.
    if (m_dealer && m_dealer_map.value(id) == m_dealer_map.value(m_dealer->gameId())) {
        m_dealer->recordGameStatistics();
        m_dealer->mapOldId(id);
        return;
    }

    // If we're replacing an existing DealerScene, record the stats of the
    // game already in progress.
    if (m_dealer) {
        m_dealer->recordGameStatistics();
        delete m_dealer;
        m_view->setScene(nullptr);
        m_dealer = nullptr;
    }

    if (!m_cardDeck) {
        KCardTheme theme = KCardTheme(Settings::cardTheme());
        if (!theme.isValid())
            theme = KCardTheme(Settings::defaultCardThemeValue());

        m_cardDeck = new KCardDeck(theme, this);
    }

    const DealerInfo *di = m_dealer_map.value(id, DealerInfoList::self()->games().first());
    m_dealer = di->createGame();
    m_dealer->setDeck(m_cardDeck);
    m_dealer->initialize();
    m_dealer->mapOldId(id);
    m_dealer->setSolverEnabled(m_solverEnabledAction->isChecked());
    m_dealer->setAutoDropEnabled(m_autoDropEnabledAction->isChecked());

    m_view->setScene(m_dealer);

    m_gameHelpAction->setText(
        i18nc("@action Is disabled and changes to \"Help &with Current Game\" when"
              " there is no current game.",
              "Help &with %1",
              di->baseName().replace(QLatin1Char('&'), QLatin1String("&&"))));

    connect(m_dealer, &DealerScene::solverStateChanged, this, &MainWindow::updateSolverDescription);
    connect(m_dealer, &DealerScene::updateMoves, this, &MainWindow::slotUpdateMoves);

    m_solverStatusLabel->setText(QString());
    m_solverStatusLabel->setVisible(true);
    m_moveCountStatusLabel->setText(QString());
    m_moveCountStatusLabel->setVisible(true);

    updateActions();
    updateSoundEngine();
}

void MainWindow::slotShowGameSelectionScreen()
{
    if (!m_dealer || m_dealer->allowedToStartNewGame()) {
        if (m_dealer) {
            m_dealer->recordGameStatistics();
            delete m_dealer;
            m_view->setScene(nullptr);
            m_dealer = nullptr;
        }

        if (!m_selector) {
            m_selector = new GameSelectionScene(this);
            connect(m_selector, &GameSelectionScene::gameSelected, this, &MainWindow::slotGameSelected);
        }
        m_view->setScene(m_selector);

        m_gameHelpAction->setText(i18nc("@action Shown when there is no game open. Is always disabled.", "Help &with Current Game"));

        updateActions();

        setGameCaption();

        m_solverStatusLabel->setVisible(false);
        m_moveCountStatusLabel->setVisible(false);
    }
}

void MainWindow::updateActions()
{
    // Enable/disable application actions that aren't appropriate on game
    // selection screen.
    actionCollection()->action(QStringLiteral("new_game"))->setEnabled(m_dealer);
    actionCollection()->action(QStringLiteral("new_deal"))->setEnabled(m_dealer);
    actionCollection()->action(QStringLiteral("game_restart"))->setEnabled(m_dealer);
    actionCollection()->action(QStringLiteral("game_save_as"))->setEnabled(m_dealer);
    m_gameHelpAction->setEnabled(m_dealer);

    // Initially disable game actions. They'll be reenabled through signals
    // if/when appropriate.
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    m_hintAction->setEnabled(false);
    m_demoAction->setEnabled(false);
    m_dropAction->setEnabled(false);
    m_drawAction->setEnabled(false);
    m_dealAction->setEnabled(false);
    m_redealAction->setEnabled(false);

    // If a dealer exists, connect the game actions to it.
    if (m_dealer) {
        connect(m_dealer, &DealerScene::undoPossible, m_undoAction, &QAction::setEnabled);
        connect(m_dealer, &DealerScene::redoPossible, m_redoAction, &QAction::setEnabled);

        connect(m_dealer, &DealerScene::hintActive, m_hintAction, &QAction::setChecked);
        connect(m_dealer, &DealerScene::gameInProgress, m_hintAction, &QAction::setEnabled);

        connect(m_dealer, &DealerScene::demoActive, this, &MainWindow::toggleDemoAction);
        connect(m_dealer, &DealerScene::gameInProgress, m_demoAction, &QAction::setEnabled);

        connect(m_dealer, &DealerScene::dropActive, m_dropAction, &QAction::setChecked);
        connect(m_dealer, &DealerScene::gameInProgress, m_dropAction, &QAction::setEnabled);

        connect(m_dealer, &DealerScene::gameInProgress, m_saveAction, &QAction::setEnabled);

        connect(m_leftAction, &QAction::triggered, m_dealer, &DealerScene::keyboardFocusLeft);
        connect(m_rightAction, &QAction::triggered, m_dealer, &DealerScene::keyboardFocusRight);
        connect(m_upAction, &QAction::triggered, m_dealer, &DealerScene::keyboardFocusUp);
        connect(m_downAction, &QAction::triggered, m_dealer, &DealerScene::keyboardFocusDown);
        connect(m_cancelAction, &QAction::triggered, m_dealer, &DealerScene::keyboardFocusCancel);
        connect(m_pickUpSetDownAction, &QAction::triggered, m_dealer, &DealerScene::keyboardFocusSelect);

        connect(m_dealer, &DealerScene::newDeal, this, &MainWindow::newGame);

        if (m_dealer->actions() & DealerScene::Draw) {
            connect(m_drawAction, &QAction::triggered, m_dealer, &DealerScene::drawDealRowOrRedeal);
            connect(m_dealer, &DealerScene::newCardsPossible, m_drawAction, &QAction::setEnabled);
        } else if (m_dealer->actions() & DealerScene::Deal) {
            connect(m_dealAction, &QAction::triggered, m_dealer, &DealerScene::drawDealRowOrRedeal);
            connect(m_dealer, &DealerScene::newCardsPossible, m_dealAction, &QAction::setEnabled);
        } else if (m_dealer->actions() & DealerScene::Redeal) {
            connect(m_redealAction, &QAction::triggered, m_dealer, &DealerScene::drawDealRowOrRedeal);
            connect(m_dealer, &DealerScene::newCardsPossible, m_redealAction, &QAction::setEnabled);
        }

        guiFactory()->unplugActionList(this, QStringLiteral("dealer_options"));
        guiFactory()->plugActionList(this, QStringLiteral("dealer_options"), m_dealer->configActions());
    }

    updateGameActionList();
}

void MainWindow::updateGameActionList()
{
    guiFactory()->unplugActionList(this, QStringLiteral("game_actions"));

    m_dropAction->setEnabled(m_dealer && !m_dealer->autoDropEnabled());

    if (m_dealer) {
        QList<QAction *> actionList;
        if (m_dealer->actions() & DealerScene::Hint)
            actionList.append(m_hintAction);
        if (m_dealer->actions() & DealerScene::Demo)
            actionList.append(m_demoAction);
        if (m_dealer->actions() & DealerScene::Draw)
            actionList.append(m_drawAction);
        if (m_dealer->actions() & DealerScene::Deal)
            actionList.append(m_dealAction);
        if (m_dealer->actions() & DealerScene::Redeal)
            actionList.append(m_redealAction);
        if (!m_dealer->autoDropEnabled())
            actionList.append(m_dropAction);
        guiFactory()->plugActionList(this, QStringLiteral("game_actions"), actionList);
    }
}

void MainWindow::updateSoundEngine()
{
    if (m_dealer) {
        if (Settings::playSounds()) {
            if (!m_soundEngine)
                m_soundEngine = new SoundEngine(this);

            connect(m_dealer, &DealerScene::cardsPickedUp, m_soundEngine, &SoundEngine::cardsPickedUp);
            connect(m_dealer, &DealerScene::cardsPutDown, m_soundEngine, &SoundEngine::cardsPutDown);
        } else if (m_soundEngine) {
            disconnect(m_dealer, nullptr, m_soundEngine, nullptr);
        }
    }
}

void MainWindow::toggleDrop()
{
    if (m_dealer) {
        if (!m_dealer->isDropActive())
            m_dealer->startDrop();
        else
            m_dealer->stopDrop();
    }
}

void MainWindow::toggleHints()
{
    if (m_dealer) {
        if (!m_dealer->isHintActive())
            m_dealer->startHint();
        else
            m_dealer->stop();
    }
}

void MainWindow::toggleDemo()
{
    if (m_dealer) {
        if (!m_dealer->isDemoActive())
            m_dealer->startDemo();
        else
            m_dealer->stop();
    }
}

void MainWindow::toggleDemoAction(bool active)
{
    m_demoAction->setChecked(active);
    m_demoAction->setIcon(QIcon::fromTheme(active ? QStringLiteral("media-playback-pause") : QStringLiteral("media-playback-start")));
}

void MainWindow::toggleMenubar()
{
    if (m_showMenubarAction->isChecked())
        menuBar()->show();
    else if (KMessageBox::warningContinueCancel(this,
                                                i18n("Are you sure you want to hide the menubar? The current shortcut to show it again is %1.",
                                                     m_showMenubarAction->shortcut().toString(QKeySequence::NativeText)),
                                                i18nc("@title:window", "Hide Menubar"),
                                                KStandardGuiItem::cont(),
                                                KStandardGuiItem::cancel(),
                                                QStringLiteral("MenubarWarning"))
             == KMessageBox::Continue)
        menuBar()->hide();
    else
        m_showMenubarAction->setChecked(true);
}

void MainWindow::toggleFullscreen(bool fullScreen)
{
    KToggleFullScreenAction::setFullScreen(this, fullScreen);
}

void MainWindow::saveNewToolbarConfig()
{
    KXmlGuiWindow::saveNewToolbarConfig();
    updateGameActionList();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QString stateDirName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString stateFileName = stateDirName + QLatin1String("/" saved_state_file);
    QDir stateFileDir(stateDirName);
    if (!stateFileDir.exists()) {
        // create the directory if it doesn't exist (bug#350160)
        stateFileDir.mkpath(QStringLiteral("."));
    }
    QFile stateFile(stateFileName);

    // Remove the existing state file, if any.
    stateFile.remove();

    if (m_dealer) {
        if (Settings::rememberStateOnExit() && !m_dealer->isGameWon()) {
            stateFile.open(QFile::WriteOnly | QFile::Truncate);
            m_dealer->saveFile(&stateFile);
        } else {
            // If there's a game in progress and we aren't going to save it
            // then record its statistics, since the DealerScene will be destroyed
            // shortly.
            m_dealer->recordGameStatistics();
        }
    }

    KXmlGuiWindow::closeEvent(e);
}

void MainWindow::newNumberedDeal()
{
    if (!m_dealDialog) {
        m_dealDialog = new NumberedDealDialog(this);
        connect(m_dealDialog, &NumberedDealDialog::dealChosen, this, &MainWindow::startNumbered);
    }

    if (m_dealer) {
        m_dealDialog->setGameType(m_dealer->oldId());
        m_dealDialog->setDealNumber(m_dealer->gameNumber());
    }

    m_dealDialog->show();
}

void MainWindow::startNumbered(int gameId, int dealNumber)
{
    if (!m_dealer || m_dealer->allowedToStartNewGame()) {
        setGameType(gameId);
        startNew(dealNumber);
    }
}

void MainWindow::nextDeal()
{
    if (!m_dealer)
        newNumberedDeal();
    else if (m_dealer->allowedToStartNewGame())
        startNew(m_dealer->gameNumber() == INT_MAX ? 1 : m_dealer->gameNumber() + 1);
}

void MainWindow::previousDeal()
{
    if (!m_dealer)
        newNumberedDeal();
    else if (m_dealer->allowedToStartNewGame())
        startNew(m_dealer->gameNumber() == 1 ? INT_MAX : m_dealer->gameNumber() - 1);
}

bool MainWindow::loadGame(const QUrl &url, bool addToRecentFiles)
{
    KIO::StoredTransferJob *job = KIO::storedGet(url);
    if (!job->exec()) {
        KMessageBox::error(this, i18n("Downloading file failed: %1", job->errorString()));
        return false;
    }

    QXmlStreamReader xml(job->data());
    if (!xml.readNextStartElement()) {
        KMessageBox::error(this, i18n("Error reading XML file: ") + xml.errorString());
        return false;
    }

    int gameId = -1;
    bool isLegacyFile;

    if (xml.name() == QLatin1String("dealer")) {
        isLegacyFile = true;
        bool ok;
        int id = xml.attributes().value(QStringLiteral("id")).toString().toInt(&ok);
        if (ok)
            gameId = id;
    } else if (xml.name() == QLatin1String("kpat-game")) {
        isLegacyFile = false;
        gameId = DealerInfoList::self()->gameIdForFile(xml);
    } else {
        KMessageBox::error(this, i18n("XML file is not a KPat save."));
        return false;
    }

    if (!m_dealer_map.contains(gameId)) {
        KMessageBox::error(this, i18n("Unrecognized game id."));
        return false;
    }

    // Only bother the user to ask for permission after we've determined the
    // save game file is at least somewhat valid.
    if (m_dealer && !m_dealer->allowedToStartNewGame())
        return false;

    setGameType(gameId);

    xml.clear();

    QBuffer buffer;
    buffer.setData(job->data());
    buffer.open(QBuffer::ReadOnly);
    bool success = isLegacyFile ? m_dealer->loadLegacyFile(&buffer) : m_dealer->loadFile(&buffer);

    if (!success) {
        KMessageBox::error(this, i18n("Errors encountered while parsing file."));
        slotShowGameSelectionScreen();
        return false;
    }

    setGameCaption();

    if (addToRecentFiles)
        m_recentFilesAction->addUrl(url);

    return true;
}

void MainWindow::loadGame()
{
    QPointer<QFileDialog> dialog = new QFileDialog(this);
    dialog->selectUrl(dialogUrl);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->setMimeTypeFilters(QStringList() << saveFileMimeType << legacySaveFileMimeType << QStringLiteral("application/octet-stream"));
    dialog->setWindowTitle(i18nc("@title:window", "Load Game"));

    if (dialog->exec() == QFileDialog::Accepted) {
        if (dialog) {
            QUrl url = dialog->selectedUrls().at(0);
            if (!url.isEmpty())
                loadGame(url);
        }
    }
    delete dialog;
}

void MainWindow::loadGame(const QUrl &url)
{
    loadGame(url, true);
}

void MainWindow::saveGame()
{
    if (!m_dealer)
        return;

    QPointer<QFileDialog> dialog = new QFileDialog(this);
    dialog->selectUrl(dialogUrl);
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->setMimeTypeFilters(QStringList() << saveFileMimeType << legacySaveFileMimeType);
    dialog->setOption(QFileDialog::DontConfirmOverwrite, false);
    dialog->setWindowTitle(i18nc("@title:window", "Save Game"));
    if (dialog->exec() != QFileDialog::Accepted)
        return;

    QUrl url;
    if (dialog) {
        url = dialog->selectedUrls().at(0);
        if (url.isEmpty())
            return;
    }

    QFile localFile;
    QTemporaryFile tempFile;
    if (url.isLocalFile()) {
        localFile.setFileName(url.toLocalFile());
        if (!localFile.open(QFile::WriteOnly)) {
            KMessageBox::error(this, i18n("Error opening file for writing. Saving failed."));
            return;
        }
    } else {
        if (!tempFile.open()) {
            KMessageBox::error(this, i18n("Unable to create temporary file. Saving failed."));
            return;
        }
    }
    QFile &file = url.isLocalFile() ? localFile : tempFile;
    if (dialog && dialog->selectedNameFilter() == legacySaveFileMimeType) {
        m_dealer->saveLegacyFile(&file);
    } else {
        m_dealer->saveFile(&file);
    }
    file.close();

    if (!url.isLocalFile()) {
        tempFile.open();
        KIO::StoredTransferJob *job = KIO::storedPut(&tempFile, url, -1);
        if (!job->exec()) {
            KMessageBox::error(this, i18n("Error uploading file. Saving failed: %1", job->errorString()));
            return;
        }
    }

    m_recentFilesAction->addUrl(url);
}

void MainWindow::showStats()
{
    QPointer<StatisticsDialog> dlg = new StatisticsDialog(this);
    if (m_dealer)
        dlg->showGameType(m_dealer->oldId());
    dlg->exec();
    delete dlg;
}

void MainWindow::updateSolverDescription(const QString &text)
{
    m_solverStatusLabel->setText(text);
}

void MainWindow::slotUpdateMoves(int moves)
{
    m_moveCountStatusLabel->setText(i18np("1 move", "%1 moves", moves));
}

void MainWindow::slotSnapshot()
{
    if (m_dealer_it == m_dealer_map.constEnd()) {
        // first call
        m_dealer_it = m_dealer_map.constBegin();
    }

    setGameType(m_dealer_it.key());
    m_dealer->setAutoDropEnabled(false);
    startRandom();

    QTimer::singleShot(1000, this, &MainWindow::slotSnapshot2);
}

void MainWindow::slotSnapshot2()
{
    m_dealer->createDump().save(QStringLiteral("%1.png").arg(m_dealer->gameId()));

    ++m_dealer_it;
    if (m_dealer_it != m_dealer_map.constEnd())
        QTimer::singleShot(0, this, &MainWindow::slotSnapshot);
}

void MainWindow::generateThemePreview()
{
    const QSize previewSize(240, 160);
    m_view->setMinimumSize(previewSize);
    m_view->setMaximumSize(previewSize);

    QImage img(m_view->contentsRect().size(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);

    const auto cards = m_cardDeck->cards();
    for (KCard *c : cards)
        c->completeAnimation();

    m_view->render(&p);

    slotShowGameSelectionScreen();

    QRect leftHalf(0, 0, m_view->contentsRect().width() / 2, m_view->contentsRect().height());
    m_view->render(&p, leftHalf, leftHalf);

    p.end();

    img.save(QStringLiteral("preview.png"));

    m_view->setMinimumSize(0, 0);
    m_view->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

#include "moc_mainwindow.cpp"
