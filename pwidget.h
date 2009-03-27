/* -*- C++ -*-
 *
 * patience -- main program
 *   Copyright (C) 1995  Paul Olav Tvete
 *
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
 *
 *
 * Heavily modified by Mario Weilguni <mweilguni@sime.com>
 *
 */

#ifndef PWIDGET_H
#define PWIDGET_H

#include <kxmlguiwindow.h>
#include <krecentfilesaction.h>

class DemoBubbles;
class PatienceView;
class DealerScene;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;
class KAction;
class QShowEvent;
class QStackedWidget;
class cardMap;
class DealerInfo;

class pWidget: public KXmlGuiWindow {
    Q_OBJECT

public:
    pWidget();
    ~pWidget();

public slots:
    void undoMove();
    void redoMove();
    void newGameType(int id);
    void startRandom();
    void restart();
    void slotShowGameSelectionScreen();

    void openGame();
    bool openGame(const KUrl &url, bool addToRecentFiles = true);
    void saveGame();

    void newGame();
    void chooseGame();
    void gameLost();
    void slotGameInfo(const QString &);
    void slotUpdateMoves();
    void helpGame();
    void enableAutoDrop();
    void enableSolver();
    void enableRememberState();
    void toggleDemoAction(bool active);
    void showStats();

    void slotGameSolverStart();
    void slotGameSolverWon();
    void slotGameSolverLost();
    void slotGameSolverUnknown();
    void slotPickRandom();
    void slotSelectDeck();

    void slotSnapshot();

    void slotGameSelected(int id);

protected:
    virtual void closeEvent(QCloseEvent * e);
    virtual void showEvent(QShowEvent *e);
    virtual void saveNewToolbarConfig();

private slots:
    void slotSnapshot2();

private:
    void setGameCaption();
    bool allowedToStartNewGame();
    void startNew(long gameNumber);
    void updateActions();

private:
    // Members
    KAction        *undo;
    KAction        *redo;
    KAction        *demoaction;
    KAction        *hintaction;
    KAction        *redealaction;
    KAction        *dropaction;
    KAction        *gamehelpaction;
    KToggleAction  *autodropaction;
    KToggleAction  *solveraction;
    KToggleAction  *rememberstateaction;
    KRecentFilesAction  *recent;

    cardMap        *m_cards; // possibly move to PatienceView
    QMap<int, const DealerInfo*>  m_dealer_map;
    QMap<int, const DealerInfo*>::const_iterator  m_dealer_it;
    int m_freeCellId;

    QStackedWidget *m_stack;
    PatienceView   *dill;
    DealerScene    *m_dealer;
    DemoBubbles    *m_bubbles;
};

#endif
