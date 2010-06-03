/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <parker.coates@kdemail.net>
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

class DealerScene;
class GameState;
class MoveHint;
#include "patpile.h"
#include "speeds.h"
#include "view.h"
class Solver;

#include "KStandardCardDeck"
#include "KCardScene"

class QAction;
#include <QtCore/QMap>
class QDomDocument;


class DealerScene : public KCardScene
{
    Q_OBJECT

public:
    enum { None = 0, Hint = 1, Demo = 2, Draw = 4, Deal = 8, Redeal = 16 } Actions;

    DealerScene();
    ~DealerScene();

    virtual void initialize() = 0;

    virtual void relayoutScene();
    void updateWonItem( bool force = false );

    // use this for autodrop times
    int speedUpTime( int delay ) const;

    QImage createDump() const;

    virtual void addPile( KCardPile * pile );
    virtual void removePile( KCardPile * pile );
    QList<PatPile*> patPiles() const;

    virtual void moveCardsToPile( QList<KCard*> cards, KCardPile * pile, int duration );

    void setAutoDropEnabled(bool a);
    bool autoDropEnabled() const { return _autodrop; }

    int gameNumber() const;

    void setGameId(int id);
    int gameId() const;

    void setActions(int actions);
    int actions() const;

    virtual QList<QAction*> configActions() const;

    void startHint();
    void stopHint();
    bool isHintActive() const;
 
    void startDrop();
    void stopDrop();
    bool isDropActive() const;

    void startDemo();
    void stopDemo();
    bool isDemoActive() const;

    void setSolverEnabled(bool a);
    void setSolver( Solver *s);
    Solver *solver() const;
    void startSolver() const;

    void setNeededFutureMoves(int);
    int neededFutureMoves() const;

    virtual bool isGameLost() const;
    virtual bool isGameWon() const;
    bool allowedToStartNewGame();
    int moveCount() const;

    QString save_it();
    void saveGame(QDomDocument &doc);
    void openGame(QDomDocument &doc);
    virtual void mapOldId(int id);
    virtual int oldId() const;
    void recordGameStatistics();

signals:
    void undoPossible(bool poss);
    void redoPossible(bool poss);
    void hintPossible(bool poss);
    void demoPossible(bool poss);
    void dropPossible(bool poss);
    void newCardsPossible(bool poss);

    void demoActive(bool active);
    void hintActive(bool active);
    void dropActive(bool active);
    void updateMoves(int moves);

    void solverStateChanged(QString text);

public slots:
    void startNew(int gameNumber = -1);

    void undo();
    void redo();

    void stop();

    void drawDealRowOrRedeal();

protected:
    virtual bool allowedToAdd(const KCardPile * pile, const QList<KCard*> & cards) const;
    virtual bool allowedToRemove(const KCardPile * pile, const KCard * card) const;

    virtual bool checkAdd( const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards ) const;
    virtual bool checkRemove( const PatPile * pile, const QList<KCard*> & cards ) const;
    virtual bool checkPrefering( const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards ) const;

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );

    virtual void restart() = 0;

    void undoOrRedo( bool undo );
    GameState * getState();
    void setState( GameState * state );

    PatPile *findTarget(KCard *c);

    virtual QList<MoveHint> getHints();
    QList<MoveHint> getSolverHints();
    // it's not const because it changes the random seed
    virtual MoveHint chooseHint();

    void won();

    // reimplement these to store and load game-specific information in the state structure
    virtual QString getGameState() { return QString(); }
    virtual void setGameState( const QString & ) {}

    // reimplement these to store and load game-specific options in the saved game file
    virtual QString getGameOptions() const { return QString(); }
    virtual void setGameOptions( const QString & ) {}

    void addCardForDeal( KCardPile * pile, KCard * card, bool faceUp, QPointF startPos );
    void startDealAnimation();

protected slots:
    virtual void animationDone();

    virtual void demo();

    void slotSolverEnded();
    void slotSolverFinished( int result );

    void showWonMessage();

    void takeState();
    virtual KCard *newCards();
    virtual bool drop();
    virtual bool tryAutomaticMove( KCard * card );

private:
    void resetInternals();

    bool _autodrop;
    bool _usesolver;
    int gamenumber;

    class DealerScenePrivate;
    DealerScenePrivate *d;

    QMap<KCard*,QPointF> m_initDealPositions;

private slots:
    void stopAndRestartSolver();
};

#endif
