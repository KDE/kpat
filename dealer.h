/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#ifndef DEALER_H
#define DEALER_H

#include "card.h"
#include "cardscene.h"
class CardState;
class DealerScene;
class MoveHint;
#include "pile.h"
class Solver;
#include "view.h"

#include <KRandomSequence>

#include <QtCore/QSet>
#include <QtGui/QAction>
#include <QtGui/QGraphicsScene>
class QDomDocument;


typedef QList<Pile*> PileList;
typedef QList<CardState> CardStateList;

struct State
{
    CardStateList cards;
    QString gameData;
};

class DealerScene : public CardScene
{
    Q_OBJECT

public:
    enum { None = 0, Hint = 1, Demo = 2, Draw = 4, Deal = 8, Redeal = 16 } Actions;

    DealerScene();
    ~DealerScene();

    virtual void relayoutScene();

    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    // use this for autodrop times
    int speedUpTime( int delay ) const;

    void createDump( QPaintDevice * );

    void setAutoDropEnabled(bool a);
    bool autoDropEnabled() const { return _autodrop; }

    void setGameNumber(int gmn);
    int gameNumber() const;

    void setGameId(int id);
    int gameId() const;

    void setActions(int actions);
    int actions() const;

    virtual QList<QAction*> configActions() const;

    void setSolverEnabled(bool a);
    void setSolver( Solver *s);
    Solver *solver() const;
    void startSolver() const;
    void unlockSolver() const;
    void finishSolver() const;

    void setNeededFutureMoves(int);
    int neededFutureMoves() const;

    bool demoActive() const;
    virtual bool isGameLost() const;
    virtual bool isGameWon() const;
    bool isInitialDeal() const;
    bool allowedToStartNewGame();
    int getMoves() const;

    QString save_it();
    void saveGame(QDomDocument &doc);
    void openGame(QDomDocument &doc);
    virtual void mapOldId(int id);
    virtual int oldId() const;
    void recordGameStatistics();

    virtual void restart() = 0;

signals:
    void undoPossible(bool poss);
    void redoPossible(bool poss);
    void hintPossible(bool poss);
    void demoPossible(bool poss);
    void newCardsPossible(bool poss);

    void demoActive(bool en);
    void updateMoves(int moves);

    void solverStateChanged(QString text);

public slots:
    void startNew(int gameNumber = -1);
    void hint();

    void undo();
    void redo();

protected:
    virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );

    virtual bool cardDoubleClicked( Card * card );

    virtual void onGameStateAlteredByUser();

    State *getState();
    void setState(State *);
    void eraseRedo();

    Pile *findTarget(Card *c);

    QList<MoveHint*> hints() const;
    virtual void getHints();
    void getSolverHints();
    void newHint(MoveHint *mh);
    void clearHints();
    // it's not const because it changes the random seed
    virtual MoveHint *chooseHint();

    virtual void stopDemo();

    void won();
    void updateWonItem();

    // reimplement these to store and load game-specific information in the state structure
    virtual QString getGameState() { return QString(); }
    virtual void setGameState( const QString & ) {}

    // reimplement these to store and load game-specific options in the saved game file
    virtual QString getGameOptions() const { return QString(); }
    virtual void setGameOptions( const QString & ) {}

    virtual void newDemoMove(Card *m);

    void addCardForDeal( Pile * pile, Card * card, bool faceUp, QPointF startPos );
    void startDealAnimation();

protected slots:
    virtual void demo();
    void waitForDemo(Card *);
    void toggleDemo();

    void slotSolverEnded();
    void slotSolverFinished();
    void startManualDrop();

    void showWonMessage();

    void takeState();
    virtual Card *newCards();
    virtual bool drop();

private:
    bool _autodrop;
    bool _usesolver;
    int gamenumber;

    class DealerScenePrivate;
    DealerScenePrivate *d;

    QMap<Card*,QPointF> m_initDealPositions;

private slots:
    void waitForAutoDrop();
    void stopAndRestartSolver();
};

#endif
