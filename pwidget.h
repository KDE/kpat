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

//Added by qt3to4:
#include <QPixmap>
#include <QLabel>
#include <QShowEvent>

#include <kxmlguiwindow.h>
#include <krecentfilesaction.h>

class DemoBubbles;
class PatienceView;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;
class QAction;
class QLabel;
class cardMap;

class pWidget: public KXmlGuiWindow {
    Q_OBJECT

public:
    pWidget();
    ~pWidget();

public slots:
    void undoMove();
    void redoMove();
    void newGameType();
    void slotNewGameType();
    void restart();

    void openGame();
    void openGame(const KUrl &url);
    void saveGame();

    void newGame();
    void chooseGame();
    void undoPossible(bool poss);
    void redoPossible(bool poss);
    void gameLost();
    void slotGameInfo(const QString &);
    void slotUpdateMoves();
    void helpGame();
    void enableAutoDrop();
    void enableSolver();
    void showStats();

    void slotGameSolverStart();
    void slotGameSolverWon();
    void slotGameSolverLost();
    void slotGameSolverUnknown();
    void slotPickRandom();
    void slotSelectDeck();

    void slotSnapshot();

    void slotGameSelected( int i );

private slots:
    void slotSnapshot2();

private:
    void setGameCaption();
    virtual void showEvent(QShowEvent *e);

private:
    // Members

    PatienceView   *dill;

    KSelectAction  *games;
    QAction        *undo, *redo;
    KToggleAction  *dropaction;
    KToggleAction  *solveraction;
    QAction        *stats;

    QPixmap         background;
    KRecentFilesAction  *recent;
    cardMap        *m_cards; // possibly move to PatienceView
    QMap<int, int>  m_dealer_map;
    QMap<int, int>::const_iterator  m_dealer_it;

    DemoBubbles    *m_bubbles;
};

#endif
