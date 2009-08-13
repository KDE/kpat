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
    enum { None = 0, Hint = 1, Demo = 2, Draw = 4, Deal = 8, Redeal = 16 } Actions;

    DealerScene();
    ~DealerScene();

    void setSceneSize( const QSize &s );
    QRectF contentArea() const;

    void addPile(Pile *p);
    void removePile(Pile *p);

    void setLayoutMargin( qreal margin );
    qreal layoutMargin() const;

    void setLayoutSpacing( qreal spacing );
    qreal layoutSpacing() const;

    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    bool isMoving(Card *c) const;
    bool cardsAreMoving() const { return !movingCards.empty(); }
    void setWaiting(bool w);
    int waiting() const { return _waiting; }

    // use this for autodrop times
    int speedUpTime( int delay ) const;

    void createDump( QPaintDevice * );

    void setAutoDropEnabled(bool a);
    bool autoDrop() const { return _autodrop; }

    void setGameNumber(int gmn);
    int gameNumber() const;

    void setGameId(int id);
    int gameId() const;

    void setActions(int actions);
    int actions() const;

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
    void recordGameStatistics();

    virtual void restart() = 0;

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

public slots:
    void relayoutScene(const QSize& s = QSize());
    void relayoutPiles();

    void startNew(int gameNumber = -1);
    void hint();
    void showWonMessage();

    void undo();
    void redo();

protected:
    virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );

    virtual bool cardClicked(Card *);
    virtual bool pileClicked(Pile *);
    virtual bool cardDblClicked(Card *);

    virtual void drawBackground ( QPainter * painter, const QRectF & rect );
    virtual void drawForeground ( QPainter * painter, const QRectF & rect );

    State *getState();
    void setState(State *);
    void eraseRedo();

    void mark(Card *c);
    void unmarkAll();

    Pile *findTarget(Card *c);
    Pile *targetPile();

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
    void considerGameStarted();

    PileList piles;
    QList<MoveHint*> hints;

protected slots:
    virtual void demo();
    void waitForDemo(Card *);
    void waitForWonAnim(Card *c);
    void toggleDemo();


    void slotSolverEnded();
    void slotSolverFinished();
    void slotAutoDrop();

    void takeState();
    virtual Card *newCards();
    virtual bool startAutoDrop();

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

private slots:
    void waitForAutoDrop(Card *);
    void stopAndRestartSolver();
};

#endif
