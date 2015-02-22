/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
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

#ifndef DEALER_H
#define DEALER_H

class CardDiff;
class DealerInfo;
#include "gamestate.h"
class MessageBox;
class MoveHint;
#include "patpile.h"
class Solver;
class SolverThread;
#include "speeds.h"
#include "view.h"
#include "patsolve/patsolve.h"

#include "KCardDeck"
#include "KCardScene"

class QAction;
#include <QtCore/QMap>
#include <QtCore/QStack>
#include <QtCore/QTimer>
class QDomDocument;


class DealerScene : public KCardScene
{
    Q_OBJECT

public:
    enum { None = 0, Hint = 1, Demo = 2, Draw = 4, Deal = 8, Redeal = 16 } Actions;

    DealerScene( const DealerInfo * di );
    ~DealerScene();

    virtual void initialize() = 0;

    virtual void relayoutScene();
    void updateWonItem();

    void addPatPile( PatPile * pile );
    void removePatPile( PatPile * pile );
    QList<PatPile*> patPiles() const;

    void setAutoDropEnabled( bool enabled );
    bool autoDropEnabled() const;

    int gameNumber() const;

    int gameId() const;

    void setActions( int actions );
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

    void setSolverEnabled( bool enabled );
    Solver * solver() const;
    void startSolver();

    virtual bool isGameLost() const;
    virtual bool isGameWon() const;

    bool allowedToStartNewGame();
    int moveCount() const;

    void saveFile( QIODevice * io );
    bool loadFile( QIODevice * io );
    void saveLegacyFile( QIODevice * io );
    bool loadLegacyFile( QIODevice * io );
    
    virtual void mapOldId(int id);
    virtual int oldId() const;
    void recordGameStatistics();

    QImage createDump() const;

signals:
    void undoPossible(bool poss);
    void redoPossible(bool poss);
    void newCardsPossible(bool poss);
    void gameInProgress(bool inProgress);

    void demoActive(bool active);
    void hintActive(bool active);
    void dropActive(bool active);
    void updateMoves(int moves);

    void solverStateChanged(QString text);

    void cardsPickedUp();
    void cardsPutDown();

public slots:
    void startNew( int dealNumber = -1 );

    void undo();
    void redo();

    void stop();

    void drawDealRowOrRedeal();
    virtual bool tryAutomaticMove( KCard * card );

protected:
    virtual bool allowedToAdd(const KCardPile * pile, const QList<KCard*> & cards) const;
    virtual bool allowedToRemove(const KCardPile * pile, const KCard * card) const;

    virtual bool checkAdd( const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards ) const;
    virtual bool checkRemove( const PatPile * pile, const QList<KCard*> & cards ) const;
    virtual bool checkPrefering( const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards ) const;

    virtual void cardsMoved( const QList<KCard*> & cards, KCardPile * oldPile, KCardPile * newPile );

    virtual void mouseDoubleClickEvent( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * mouseEvent );

    virtual void restart( const QList<KCard*> & cards ) = 0;

    void setSolver( Solver * solver );

    virtual QList<MoveHint> getHints();

    // reimplement these to store and load game-specific information in the state structure
    virtual QString getGameState() const;
    virtual void setGameState( const QString & state );

    // reimplement these to store and load game-specific options in the saved game file
    virtual QString getGameOptions() const;
    virtual void setGameOptions( const QString & options );

    void addCardForDeal( KCardPile * pile, KCard * card, bool faceUp, QPointF startPos );
    void startDealAnimation();

    void setNeededFutureMoves( int moves );
    int neededFutureMoves() const;

    void setDeckContents( int copies = 1,
                          const QList<KCardDeck::Suit> & suits = KCardDeck::standardSuits() );

    void multiStepMove( const QList<KCard*> & cards,
                        KCardPile * pile,
                        const QList<KCardPile*> & freePiles,
                        const QList<KCardPile*> & freeCells,
                        int duration  );

    QList<MoveHint> getSolverHints();

protected slots:
    void takeState();
    virtual void animationDone();
    virtual bool newCards();
    virtual bool drop();

private slots:
    void stopAndRestartSolver();
    void slotSolverEnded();
    void slotSolverFinished( int result );

    void demo();

    void showWonMessage();

private:
    void undoOrRedo( bool undo );

    void resetInternals();

    MoveHint chooseHint();

    void won();

    int speedUpTime( int delay ) const;

    void multiStepSubMove( QList<KCard*> cards,
                           KCardPile * pile,
                           QList<KCardPile*> freePiles,
                           const QList<KCardPile*> & freeCells );
    void continueMultiStepMove();

    const DealerInfo * const m_di;

    Solver * m_solver;
    SolverThread * m_solverThread;
    QList<MOVE> m_winningMoves;

    KCard * m_peekedCard;
    MessageBox * m_wonItem;

    QTimer m_solverUpdateTimer;
    QTimer m_demoTimer;
    QTimer m_dropTimer;

    int m_dealNumber;
    int m_loadedMoveCount;
    int m_neededFutureMoves;
    int m_supportedActions;

    bool m_autoDropEnabled;
    bool m_solverEnabled;

    bool m_dealStarted;
    bool m_dealWasEverWinnable;
    bool m_dealHasBeenWon;
    bool m_dealWasJustSaved;
    bool m_statisticsRecorded;
    bool m_playerReceivedHelp;

    // We need a flag to avoid telling someone the game is lost
    // just because the winning animation moved the cards away
    bool m_toldAboutWonGame;
    bool m_toldAboutLostGame;

    QSet<KCard*> m_cardsRemovedFromFoundations;
    qreal m_dropSpeedFactor;
    bool m_interruptAutoDrop;

    bool m_dealInProgress;
    bool m_hintInProgress;
    bool m_demoInProgress;
    bool m_dropInProgress;

    bool m_hintQueued;
    bool m_demoQueued;
    bool m_dropQueued;
    bool m_newCardsQueued;
    bool m_takeStateQueued;

    QStack<GameState*> m_undoStack;
    GameState * m_currentState;
    QStack<GameState*> m_redoStack;
    QHash<KCard*,CardState> m_lastKnownCardStates;

    QList<QPair<KCard*,KCardPile*> > m_multiStepMoves;
    int m_multiStepDuration;

    QList<PatPile*> m_patPiles;

    QMap<KCard*,QPointF> m_initDealPositions;
};

#endif
