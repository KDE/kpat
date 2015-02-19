/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 1997 Mario Weilguni <mweilguni@sime.com>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <coates@kde.org>
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

class DealerInfo;
class DealerScene;
class GameSelectionScene;
class NumberedDealDialog;
class PatienceView;
class SoundEngine;

class KCardDeck;

class QAction;
class KRecentFilesAction;
class KToggleAction;
class QUrl;
#include <KXmlGuiWindow>

class QLabel;


class MainWindow: public KXmlGuiWindow {
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

public slots:
    bool loadGame( const QUrl & url, bool addToRecentFiles = true );
    void slotShowGameSelectionScreen();
    void slotGameSelected(int id);

protected slots:
    void newGame();
    void startRandom();
    void loadGame();
    void restart();
    void newNumberedDeal();
    void startNumbered( int gameId, int dealNumber );
    void nextDeal();
    void previousDeal();
    void saveGame();
    void showStats();

    void undoMove();
    void redoMove();

    void toggleDrop();
    void toggleHints();
    void toggleDemo();
    void toggleDemoAction(bool active);
    void toggleMenubar();

    void setAutoDropEnabled( bool enabled );
    void enableSolver(bool enable);
    void enableSounds(bool enable);
    void enableRememberState(bool enable);
    void slotPickRandom();
    void configureAppearance();
    void appearanceChanged();

    void helpGame();

    void updateSolverDescription(const QString & text);
    void slotUpdateMoves(int moves);

protected:
    virtual void closeEvent(QCloseEvent * e);
    virtual void saveNewToolbarConfig();

private slots:
    void slotSnapshot();
    void slotSnapshot2();
    void generateThemePreview();

private:
    void setupActions();
    void setGameType( int id );
    void setGameCaption();
    void startNew(int gameNumber);
    void updateActions();
    void updateGameActionList();
    void updateSoundEngine();

    // Members
    QAction * m_leftAction;
    QAction * m_rightAction;
    QAction * m_upAction;
    QAction * m_downAction;
    QAction * m_cancelAction;
    QAction * m_pickUpSetDownAction;    

    KRecentFilesAction * m_recentFilesAction;
    QAction * m_saveAction;
    QAction * m_undoAction;
    QAction * m_redoAction;
    QAction * m_demoAction;
    QAction * m_hintAction;
    QAction * m_drawAction;
    QAction * m_dealAction;
    QAction * m_redealAction;
    QAction * m_dropAction;
    KToggleAction * m_autoDropEnabledAction;
    KToggleAction * m_solverEnabledAction;
    KToggleAction * m_rememberStateAction;
    KToggleAction * m_playSoundsAction;
    KToggleAction * m_showMenubarAction;
    QAction * m_gameHelpAction;

    QMap<int, const DealerInfo*>  m_dealer_map;
    QMap<int, const DealerInfo*>::const_iterator  m_dealer_it;

    PatienceView * m_view;
    DealerScene * m_dealer;
    GameSelectionScene * m_selector;
    KCardDeck * m_cardDeck;
    SoundEngine * m_soundEngine;

    NumberedDealDialog * m_dealDialog;

    QLabel * m_solverStatusLabel;
    QLabel * m_moveCountStatusLabel;
};

#endif
