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
#ifndef _DEALER_H_
#define _DEALER_H_

#include "hint.h"
#include "pile.h"
#include <krandomsequence.h>
#include <QString>
#include <QGraphicsScene>

class Card;
class QPixmap;
class MoveHint;
class QDomDocument;
class DealerInfo;
class Pile;
class DealerScene;

typedef QList<Pile*> PileList;

class DealerInfoList {
public:
    static DealerInfoList *self();
    void add(DealerInfo *);

    const QList<DealerInfo*> games() const { return list; }
private:
    QList<DealerInfo*> list;
    static DealerInfoList *_self;
};

class DealerInfo
{
public:
    DealerInfo(const char *_name, int _index)
        : name(_name),
	gameindex(_index)
	{
	    DealerInfoList::self()->add(this);
	}
	virtual ~DealerInfo(){}
	const char *name;
	int gameindex;
	virtual DealerScene *createGame() = 0;
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

    enum UserTypes { CardTypeId = 1, PileTypeId = 2 };

    DealerScene();
    ~DealerScene();
    void unmarkAll();
    void mark(Card *c);

protected:
    virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void countLoss();
    void countGame();

public:
    Pile *findTarget(Card *c);
    virtual bool cardClicked(Card *);
    virtual void pileClicked(Pile *);
    virtual bool cardDblClicked(Card *);

    bool isMoving(Card *c) const;

    void saveGame(QDomDocument &doc);
    void openGame(QDomDocument &doc);

    virtual void getHints();
    void newHint(MoveHint *mh);
    void clearHints();
    // it's not const because it changes the random seed
    virtual MoveHint *chooseHint();

    void setAutoDropEnabled(bool a);
    bool autoDrop() const { return _autodrop; }

    void setGameNumber(long gmn);
    long gameNumber() const;

    int waiting() const { return _waiting; }
    void setWaiting(bool w);

    void setBackgroundPixmap(const QPixmap &background, const QColor &midcolor);

    void addPile(Pile *p);
    void removePile(Pile *p);

    virtual bool isGameLost() const;
    virtual bool isGameWon() const;

    void setGameId(int id) { _id = id; }
    int gameId() const { return _id; }

    void startNew();

    virtual void restart() = 0;
    virtual void stopDemo();

    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    // reimplement this to add game-specific information in the state structure
    virtual QString getGameState() const { return QString(); }
    // reimplement this to use the game-specific information from the state structure
    virtual void setGameState( const QString & ) {}

    QPointF maxPilePos() const;

    void setSceneSize( const QSize &s );

    void won();
    bool demoActive() const;
    int getMoves() const { return undoList.count(); }

    enum { None = 0, Hint = 1, Demo = 2, Redeal = 4 } Actions;

    void setActions(int actions) { myActions = actions; }
    int actions() const { return myActions; }

public slots:
    virtual bool startAutoDrop();
    void hint();
    void rescale(bool onlypiles);

    State *getState();
    void setState(State *);
    void relayoutPiles();
    void slotShowGame(bool);
    void takeState();
    void undo();

protected slots:
    virtual void demo();
    void waitForDemo(Card *);
    void waitForWonAnim(Card *c);
    void toggleDemo(bool);

signals:
    void undoPossible(bool poss);
    void hintPossible(bool poss);
    void demoPossible(bool poss);
    void redealPossible(bool poss);

    void enableRedeal(bool enable);
    void gameInfo(const QString &info);
    void gameWon(bool withhelp);
    void demoActive(bool en);
    void updateMoves();
    void gameLost();

private slots:
    void waitForAutoDrop(Card *);

protected:
    PileList piles;
    QList<MoveHint*> hints;
    KRandomSequence randseq;

    virtual Card *demoNewCards();
    virtual void newDemoMove(Card *m);

private:
    QList<QGraphicsItem *> marked;
    QList<State*> undoList;

    bool moved;
    CardList movingCards;

    QPointF moving_start;
    bool _autodrop;
    int _waiting;
    long gamenumber;

    QTimer *demotimer;
    bool demo_active;
    bool stop_demo_next;
    qreal m_autoDropFactor;

    bool _won;
    quint32 _id;
    bool _gameRecorded;
    QGraphicsItem *wonItem;
    bool gothelp;
    bool toldAboutLostGame;
    int myActions;
};

#endif
