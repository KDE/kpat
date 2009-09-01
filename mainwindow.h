/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 1997 Mario Weilguni <mweilguni@sime.com>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@gmail.com>
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

class cardMap;
class DealerInfo;
class DealerScene;
class DemoBubbles;
class PatienceView;

class KAction;
class KRecentFilesAction;
class KSelectAction;
class KToggleAction;
#include <KUrl>
#include <KXmlGuiWindow>

class QLabel;
class QStackedWidget;


class MainWindow: public KXmlGuiWindow {
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

public slots:
    bool openGame(const KUrl &url, bool addToRecentFiles = true);
    void slotShowGameSelectionScreen();
    void slotGameSelected(int id);

protected slots:
    void newGame();
    void startRandom();
    void openGame();
    void restart();
    void chooseGame();
    void saveGame();
    void showStats();

    void undoMove();
    void redoMove();
    void toggleDemoAction(bool active);

    void enableAutoDrop(bool enable);
    void enableSolver(bool enable);
    void enableRememberState(bool enable);
    void slotPickRandom();
    void slotSelectDeck();

    void helpGame();

    void slotGameSolverReset();
    void slotGameSolverStart();
    void slotGameSolverWon();
    void slotGameSolverLost();
    void slotGameSolverUnknown();
    void slotGameLost();
    void slotUpdateMoves(int moves);

protected:
    virtual void closeEvent(QCloseEvent * e);
    virtual void saveNewToolbarConfig();

private slots:
    void slotSnapshot();
    void slotSnapshot2();

private:
    void newGameType(int id);
    void setGameCaption();
    void startNew(int gameNumber);
    void updateActions();
    void updateGameActionList();

    // Members
    KAction        *undo;
    KAction        *redo;
    KAction        *demoaction;
    KAction        *hintaction;
    KAction        *drawaction;
    KAction        *dealaction;
    KAction        *redealaction;
    KAction        *dropaction;
    KAction        *gamehelpaction;
    KToggleAction  *autodropaction;
    KToggleAction  *solveraction;
    KToggleAction  *rememberstateaction;
    KRecentFilesAction  *recent;

    QMap<int, const DealerInfo*>  m_dealer_map;
    QMap<int, const DealerInfo*>::const_iterator  m_dealer_it;
    int m_freeCellId;

    QStackedWidget *m_stack;
    PatienceView   *m_view;
    DealerScene    *m_dealer;
    DemoBubbles    *m_bubbles;

    QLabel         *solverStatus;
    QLabel         *moveStatus;
};

#endif
