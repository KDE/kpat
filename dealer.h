/*
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
*/

#ifndef DEALER_H
#define DEALER_H

#include "card.h"
class DealerInfo;
class DealerScene;
class MoveHint;
#include "pile.h"
class Solver;

#include <KRandomSequence>

#include <QGraphicsScene>
class QDomDocument;


typedef QList<Pile*> PileList;

class DealerInfoList {
public:
    static DealerInfoList *self();
    void add(DealerInfo *);

    const QList<DealerInfo*> games() const { return list; }
private:
    QList<DealerInfo*> list;
    static DealerInfoList *_self;
    static void cleanupDealerInfoList();
};

class DealerInfo
{
public:
    DealerInfo(const char *_name, int _index)
        : name(_name)
	{
	    DealerInfoList::self()->add(this);
            addOldId(_index);
	}
    virtual ~DealerInfo(){}
    const char *name;
    QList<int> ids;
    virtual DealerScene *createGame() const = 0;
    void addOldId(int id) { ids.push_back(id); }
    bool hasId(int id) { return ids.contains(id); }
};

class CardState;

typedef QList<CardState> CardStateList;

struct State
{
    CardStateList cards;
    QString gameData;
};

class DealerScene : public QGraphicsScene
{
    Q_OBJECT

public:
    DealerScene();
    ~DealerScene();

    void unmarkAll();
    void mark(Card *c);
    QString save_it();
    Pile * targetPile();

protected:
    virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void drawBackground ( QPainter * painter, const QRectF & rect );

public:
    Pile *findTarget(Card *c);
    virtual bool cardClicked(Card *);
    virtual bool pileClicked(Pile *);
    virtual bool cardDblClicked(Card *);

    bool isMoving(Card *c) const;

    void saveGame(QDomDocument &doc);
    void openGame(QDomDocument &doc);
    virtual void mapOldId(int id);

    virtual void getHints();
    void getSolverHints();
    void newHint(MoveHint *mh);
    void clearHints();
    // it's not const because it changes the random seed
    virtual MoveHint *chooseHint();

    void setSolverEnabled(bool a);
    void setAutoDropEnabled(bool a);
    bool autoDrop() const { return _autodrop; }

    // use this for autodrop times
    int speedUpTime( int delay ) const;

    void setGameNumber(int gmn);
    int gameNumber() const;

    int waiting() const { return _waiting; }
    void setWaiting(bool w);

    void addPile(Pile *p);
    void removePile(Pile *p);

    virtual bool isGameLost() const;
    virtual bool isGameWon() const;

    void setGameId(int id);
    int gameId() const;

    virtual void restart() = 0;
    virtual void stopDemo();

    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    QPointF maxPilePos() const;
    qreal offsetX() const;
    qreal offsetY() const;

    void setSceneSize( const QSize &s );

    void won();
    bool demoActive() const;
    int getMoves() const;

    enum { None = 0, Hint = 1, Demo = 2, Draw = 4, Deal = 8, Redeal = 16 } Actions;

    void setActions(int actions);
    int actions() const;

    void setSolver( Solver *s);
    Solver *solver() const;

    void startSolver() const;
    void unlockSolver() const;
    void finishSolver() const;

    void setNeededFutureMoves(int);
    int neededFutureMoves() const;

    bool isInitialDeal() const;
    void recordGameStatistics();

    bool cardsAreMoving() const { return !movingCards.empty(); }

    void createDump( QPaintDevice * );

    void updateWonItem();

    bool allowedToStartNewGame();

public slots:
    virtual bool startAutoDrop();
    virtual Card *newCards();
    void hint();
    void rescale(bool onlypiles);

    State *getState();
    void setState(State *);
    void relayoutPiles();
    void showWonMessage();
    void takeState();
    void eraseRedo();
    void undo();
    void redo();
    void slotSolverEnded();
    void slotSolverFinished();
    void slotAutoDrop();

    void startNew(int gameNumber = -1);

protected slots:
    virtual void demo();
    void waitForDemo(Card *);
    void waitForWonAnim(Card *c);
    void toggleDemo();

signals:
    void undoPossible(bool poss);
    void redoPossible(bool poss);
    void hintPossible(bool poss);
    void demoPossible(bool poss);
    void newCardsPossible(bool poss);

    void gameWon(bool withhelp);
    void demoActive(bool en);
    void updateMoves(int moves);
    void gameLost();

    void gameSolverReset();
    void gameSolverStart();
    void gameSolverWon();
    void gameSolverLost();
    void gameSolverUnknown();

private slots:
    void waitForAutoDrop(Card *);
    void stopAndRestartSolver();

protected:
    // reimplement these to store and load game-specific information in the state structure
    virtual QString getGameState() { return QString(); }
    virtual void setGameState( const QString & ) {}

    // reimplement these to store and load game-specific options in the saved game file
    virtual QString getGameOptions() const { return QString(); }
    virtual void setGameOptions( const QString & ) {}

    virtual void newDemoMove(Card *m);
    void considerGameStarted();

    PileList piles;
    QList<MoveHint*> hints;
    KRandomSequence randseq;

private:
    QList<QGraphicsItem *> marked;

    bool moved;
    CardList movingCards;

    QPointF moving_start;
    bool _autodrop;
    bool _usesolver;
    int _waiting;
    int gamenumber;

    class DealerScenePrivate;
    DealerScenePrivate *d;
};

#endif
